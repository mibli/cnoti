#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#ifdef DBUS_WIN
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <dbus/dbus.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

static char g_errstr[64] = "";
static void (*g_callback)(char *, uint32_t, char *, char *, char *, int32_t) = NULL;
static DBusConnection *g_connection;

// Message Processing

static void process_iter(DBusMessageIter *iter) {
  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  char *appname;
  dbus_message_iter_get_basic(iter, &appname);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_UINT32);
  uint32_t id;
  dbus_message_iter_get_basic(iter, &id);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  char *icon;
  dbus_message_iter_get_basic(iter, &icon);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  char *summary;
  dbus_message_iter_get_basic(iter, &summary);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  char *body;
  dbus_message_iter_get_basic(iter, &body);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_INT32);
  int32_t timeout;
  dbus_message_iter_get_basic(iter, &timeout);

  g_callback(appname, id, icon, summary, body, timeout);
}

static void process_message(DBusMessage *message) {
  DBusMessageIter iter;
  const char *sender;

  sender = dbus_message_get_sender(message);

  if (strcmp(sender, "org.freedesktop.DBus") != 0) {
    dbus_message_iter_init(message, &iter);
    process_iter(&iter);
  }
}

static DBusHandlerResult monitor_filter_func(DBusConnection *connection, DBusMessage *message, void *) {
  process_message(message);

  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    exit(0);
  }

  /* Monitors must not allow libdbus to reply to messages, so we eat
   * the message. See bug 1719. */
  return DBUS_HANDLER_RESULT_HANDLED;
}

static dbus_bool_t become_monitor(DBusConnection *connection, int filters_count, const char *const *filters) {
  DBusError error = DBUS_ERROR_INIT;
  DBusMessage *m;
  DBusMessage *r;
  int i;
  dbus_uint32_t zero = 0;
  DBusMessageIter appender, array_appender;

  m = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING,
                                   "BecomeMonitor");

  if (m == NULL) {
    strcpy(g_errstr, "OOM while becoming a monitor\n");
    return 0;
  }

  dbus_message_iter_init_append(m, &appender);

  if (!dbus_message_iter_open_container(&appender, DBUS_TYPE_ARRAY, "s", &array_appender)) {
    strcpy(g_errstr, "OOM while opening string array\n");
    return 0;
  }

  for (i = 0; i < filters_count; i++) {
    if (!dbus_message_iter_append_basic(&array_appender, DBUS_TYPE_STRING, &filters[i])) {
      strcpy(g_errstr, "OOM while adding filter to array\n");
      return 0;
    }

    if (!dbus_message_iter_close_container(&appender, &array_appender) ||
        !dbus_message_iter_append_basic(&appender, DBUS_TYPE_UINT32, &zero)) {
      strcpy(g_errstr, "OOM while finishing arguments\n");
      return 0;
    }

    r = dbus_connection_send_with_reply_and_block(connection, m, -1, &error);

    if (r != NULL) {
      dbus_message_unref(r);
    } else if (dbus_error_has_name(&error, DBUS_ERROR_UNKNOWN_INTERFACE)) {
      snprintf(g_errstr, sizeof(g_errstr),
               "dbus-monitor: unable to enable new-style monitoring, "
               "your dbus-daemon is too old. Falling back to eavesdropping.\n");
      dbus_error_free(&error);
    } else {
      snprintf(g_errstr, sizeof(g_errstr),
               "dbus-monitor: unable to enable new-style monitoring: "
               "%s: \"%s\". Falling back to eavesdropping.\n",
               error.name, error.message);
      dbus_error_free(&error);
    }
  }

  dbus_message_unref(m);

  return (r != NULL);
}

// ----------------
// PUBLIC_INTERFACE
// ----------------

void noti_set_callback(void (*new_callback)(char *, uint32_t, char *, char *, char *, int32_t)) {
  g_callback = new_callback;
}

int noti_init() {
  DBusError error;
  DBusBusType type = DBUS_BUS_SESSION;

  dbus_error_init(&error);

  g_connection = dbus_bus_get(type, &error);
  if (g_connection == NULL) {
    snprintf(g_errstr, sizeof(g_errstr), "Failed to open connection to session bus: %s\n", error.message);
    dbus_error_free(&error);
    exit(1);
  }

  dbus_connection_set_route_peer_messages(g_connection, TRUE);

  DBusHandleMessageFunction filter_func = monitor_filter_func;
  if (!dbus_connection_add_filter(g_connection, filter_func, NULL, NULL)) {
    strcpy(g_errstr, "Couldn't add filter!\n");
    exit(1);
  }

  char const *filter = "type='method_call',interface='org.freedesktop.Notifications',member='Notify'";
  char const *filters[] = {filter};
  uint32_t const filters_count = sizeof(filters) / sizeof(char *);
  if (!become_monitor(g_connection, filters_count, (const char *const *)filters)) {
    if (dbus_error_is_set(&error)) {
      snprintf(g_errstr, sizeof(g_errstr), "Failed to become monitor: %s\n", error.message);
      dbus_error_free(&error);
      exit(1);
    }
  }
}

int noti_process_events() { return dbus_connection_read_write_dispatch(g_connection, -1); }

char const *noti_get_error_msg() { return g_errstr[0] == 0 ? NULL : g_errstr; }

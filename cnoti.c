#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <dbus/dbus.h>

#include "cnoti.h"

#ifndef not
#define not !
#endif
#ifndef and
#define and &&
#endif
#ifndef or
#define or ||
#endif

static bool g_failed = true;
static char g_errstr[128] = "Not started";
static callback_type *g_callback = NULL;
static DBusConnection *g_connection;

// Message Processing

static void error(char const *str, ...) {
  va_list va;
  g_failed = true;
  va_start(va, str);
  int end = vsnprintf(g_errstr, sizeof(g_errstr), str, va);
  va_end(va);
  assert(g_errstr[end] != '\n');
}

static void warning(char const *str, ...) {
  va_list va;
  va_start(va, str);
  int end = vsnprintf(g_errstr, sizeof(g_errstr), str, va);
  va_end(va);
  assert(g_errstr[end] != '\n');
}

static void process_message(DBusMessage *message) {
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
  char *appname;
  dbus_message_iter_get_basic(&iter, &appname);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32);
  uint32_t id;
  dbus_message_iter_get_basic(&iter, &id);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
  char *icon;
  dbus_message_iter_get_basic(&iter, &icon);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
  char *summary;
  dbus_message_iter_get_basic(&iter, &summary);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING);
  char *body;
  dbus_message_iter_get_basic(&iter, &body);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_next(&iter);

  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32);
  int32_t timeout;
  dbus_message_iter_get_basic(&iter, &timeout);

  g_callback(appname, id, icon, summary, body, timeout);
}

static DBusHandlerResult monitor_filter(DBusConnection *connection, DBusMessage *message, void *user_data) {
  const char *sender;
  sender = dbus_message_get_sender(message);

  if (strcmp(sender, "org.freedesktop.DBus") != 0) {
    process_message(message);
  }

  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    error("cnoti-monitor-filter: Disconnected");
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}

static dbus_bool_t start_monitor(DBusConnection *connection, char const *filter) {
  DBusError err = DBUS_ERROR_INIT;
  DBusMessage *new_call_message;
  DBusMessage *connection_message;

  new_call_message = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS,
                                                  DBUS_INTERFACE_MONITORING, "BecomeMonitor");

  if (new_call_message == NULL) {
    error("cnoti-monitor: Out of memory while registering a monitoring interface.");
    return FALSE;
  }

  DBusMessageIter message_it, filters_it;
  dbus_message_iter_init_append(new_call_message, &message_it);

  dbus_uint32_t zero = 0;
  if (not(dbus_message_iter_open_container(&message_it, DBUS_TYPE_ARRAY, "s", &filters_it) and
          dbus_message_iter_append_basic(&filters_it, DBUS_TYPE_STRING, &filter) and
          dbus_message_iter_close_container(&message_it, &filters_it) and
          dbus_message_iter_append_basic(&message_it, DBUS_TYPE_UINT32, &zero))) {
    error("cnoti-monitor: Out of memory while registering the filter array.");
    return FALSE;
  }

  connection_message = dbus_connection_send_with_reply_and_block(connection, new_call_message, -1, &err);

  if (connection_message != NULL) {
    dbus_message_unref(connection_message);
  } else if (dbus_error_has_name(&err, DBUS_ERROR_UNKNOWN_INTERFACE)) {
    warning("cnoti-monitor: Failed to enable new-style monitoring. Listening instead.");
    dbus_error_free(&err);
  } else {
    warning("cnoti-monitor: Failed to enable monitoring: %s: \"%s\". Listening instead.", err.name,
            err.message);
    dbus_error_free(&err);
  }

  dbus_message_unref(new_call_message);

  return (connection_message != NULL);
}

// ----------------
// PUBLIC_INTERFACE
// ----------------

bool cnoti_init(callback_type *new_callback) {
  DBusError err;
  dbus_error_init(&err);

  g_errstr[0] = 0;
  g_failed = false;

  if (new_callback != NULL) {
    g_callback = new_callback;
  }
  assert(g_callback != NULL);

  g_connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
  if (g_connection == NULL) {
    error("Failed to open connection to session bus: %s", err.message);
    dbus_error_free(&err);
    return false;
  }

  dbus_connection_set_route_peer_messages(g_connection, TRUE);

  if (not dbus_connection_add_filter(g_connection, monitor_filter, NULL, NULL)) {
    error("Couldn't add filter!");
    return false;
  }

  if (not start_monitor(g_connection,
                        "type='method_call',interface='org.freedesktop.Notifications',member='Notify'")) {
    if (dbus_error_is_set(&err)) {
      error("Failed to start monitor: %s", err.message);
      dbus_error_free(&err);
      return false;
    }
  }

  return true;
}

bool cnoti_process_events() {
  if (g_failed) {
    return false;
  }
  return dbus_connection_read_write_dispatch(g_connection, -1);
}

char const *cnoti_get_error_msg() { return g_errstr[0] == 0 ? NULL : g_errstr; }

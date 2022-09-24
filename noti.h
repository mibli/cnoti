#ifndef __NOTI__
#define __NOTI__

#include <stdint.h>

/** Set callback to be called when a notification is received */
void noti_set_callback(void (*new_callback)(char *, uint32_t, char *, char *, char *, int32_t));

/** Initialize dbus connection */
int noti_init();

/** Process dbus events. */
int noti_process_events();

/** Return error message or NULL if none. Don't free the string. */
char *noti_get_error_msg();

#endif//__NOTI__

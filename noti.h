#ifndef __NOTI__
#define __NOTI__

#include <stdbool.h>
#include <stdint.h>

typedef void(callback_type)(char *, uint32_t, char *, char *, char *, int32_t);

/** Initialize dbus connection and set the callback */
bool noti_init(callback_type *new_callback);

/** Process dbus events. */
bool noti_process_events();

/** Return error message or NULL if none. Don't free the string. */
char const *noti_get_error_msg();

#endif //__NOTI__

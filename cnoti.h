#ifndef __NOTI__
#define __NOTI__

#include <stdbool.h>
#include <stdint.h>

typedef void(cnoti_callback_type)(char const *, uint32_t, char const *, char const *, char const *, int32_t);

/** Initialize dbus connection and set the callback */
bool cnoti_init(cnoti_callback_type *new_callback);

/** Process dbus events. */
bool cnoti_process_events();

/** Return error message or NULL if none. Don't free the string. */
char const *cnoti_get_error_msg();

/** Do all of the above in one call, probably should be combined with pthread */
char const *cnoti(cnoti_callback_type *callback);

#endif //__NOTI__

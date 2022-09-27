# cNoti - Little DBus Notification Monitor

Because subscribing to notifications should be simple. This library wraps around DBus notification
connection, which You don't want to manage, and allows You to hook a callback to it using standard
types.

## Building

Simply run this command to build

    make

To install run this

    sudo make install

## Usage

    bool cnoti_init(cnoti_callback_type *callback)

Initializes DBus connection and saves your callback pointer to call when a notification is received.
False means it failed, check error message. If callback is NULL, it will not be set.

    bool cnoti_process_events()

Process events, must be called in a loop. If exits with false, You should check an error. Will not
continue until `cnoti_init` is called again

    char const *cnoti_get_error_msg()

Returns pointer to static message array, or NULL if no there's no error message. Don't free the
string.

    char const *cnoti(cnoti_callback_type *callback);

Does all of the above in one call, returns NULL if there was no error, and returns pointer to a
message if there was an error. Don't free the string.

## Noticat (notifications-cat)

This is a simple example of usage, which can be used as simple transport. It's recommended to use
`USE_CJSON` option, which allows to use noticat as notification transport to JSON (and further for
example to MQTT) eg.

    USE_CJSON=YES make noticat
    noticat -json | mosquitto_pub -t /notifications -l

## Example

First You need a function that will handle the output:

    void print_notification(char const *appname, uint32_t id, char const *icon, char const *summary, char const *body, int32_t timeout) {
      printf("\"%s\" %u \"%s\" \"%s\" \"%s\" %d\n", appname, id, icon, summary, body, timeout)
      fflush(stdout);
    }

Then You initialize the DBUS connection, run loop processing events and get error messages:

    cnoti_init(print_notification);
    while (cnoti_process_events()) {}
    char const *errstr = cnoti_get_error_msg();
    if (errstr) {
      printf("%s\n", errstr);
    }

Or simply call cnoti which does it all:

    char const *errstr = cnoti(print_notification);
    if (errstr) {
      printf("%s\n", errstr);
    }

Probably in a separate thread though.

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

    bool cnoti_init(callback_type *callback)

Initializes DBus connection and saves your callback pointer to call when a notification is received.
False means it failed, check error message. If callback is NULL, it will not be set.

    bool cnoti_process_events()

Process events, must be called in a loop. If exits with false, You should check an error. Will not
continue until `cnoti_init` is called again

    char const *cnoti_get_error_msg()

Returns pointer to static message array, or NULL if no there's no error message. Don't free the
string.

## Noticat (notifications-cat)

This is a simple example of usage, which can be used as simple transport. It's recommended to use
`USE_CJSON` option, which allows to use noticat as notification transport to JSON (and further for
example to MQTT) eg.

    USE_CJSON=YES make noticat
    noticat -json | mosquitto_pub -t /notifications -l

## Example

See noticat.c.

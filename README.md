# Noti - Little DBus Notification Monitor

Because subscribing to notifications should be simple. This library wraps around DBus notification
connection, which You don't want to manage, and allows You to hook a callback to it using standard
types.

## Building

Simply run this command to build

    make

To install run this

    sudo make install

## Usage

    int noti_init(callback_type *callback)

Initializes DBus connection and saves your callback pointer to call when a notification is received.

    int noti_process_events()

Process events, must be called in a loop. If exits with 0, You should check an error.

    char const *noti_get_error_msg()

Returns pointer to static message array, or NULL if no there's no error message. Don't free the
string.

## Example

See noticat.c.

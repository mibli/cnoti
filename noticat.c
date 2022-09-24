#include "noti.h"

#include <stdio.h>

void print_notification(char *appname, uint32_t id, char *icon, char *summary, char *body, int32_t timeout) {
  printf("%s, %u, %s, %s, %s, %d\n", appname, id, icon, summary, body, timeout);
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  noti_set_callback(print_notification);
  noti_init();
  while (noti_process_events()) {
  };
  char const *errstr = noti_get_error_msg();
  if (errstr) {
    fprintf(stderr, "%s", errstr);
    return 1;
  }
}

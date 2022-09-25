#include "cnoti.h"

#include <stdio.h>
#include <string.h>

#ifdef USE_CJSON
#include <cjson/cJSON.h>
#include <stdlib.h>

void print_json(char *appname, uint32_t id, char *icon, char *summary, char *body, int32_t timeout) {
  cJSON *object = cJSON_CreateObject();
  cJSON_AddStringToObject(object, "application", appname);
  cJSON_AddNumberToObject(object, "id", id);
  cJSON_AddStringToObject(object, "icon", icon);
  cJSON_AddStringToObject(object, "summary", summary);
  cJSON_AddStringToObject(object, "body", body);
  cJSON_AddNumberToObject(object, "timeout", timeout);
  char *json_str = cJSON_PrintUnformatted(object);
  printf("%s\n", json_str);
  free(json_str);
  cJSON_free(object);
}
#endif

void print_cnotification(char *appname, uint32_t id, char *icon, char *summary, char *body, int32_t timeout) {
  printf("\"%s\" %u \"%s\" \"%s\" \"%s\" %d\n", appname, id, icon, summary, body, timeout);
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  callback_type *callback = print_cnotification;

  if (argc > 1) {
    if (argc == 2 && strcmp(argv[1], "-json") == 0) {
#ifdef USE_CJSON
      callback = print_json;
#else
      printf("JSON option not available in this build, please build with USE_CJSON=YES to use it.\n");
      return 1;
#endif //CJSON
    } else {
      printf("Usage: %s [-json]\n", argv[0]);
      return 0;
    }
  }


  cnoti_init(callback);
  while (cnoti_process_events()) {
  };
  char const *errstr = cnoti_get_error_msg();
  if (errstr) {
    fprintf(stderr, "%s", errstr);
    return 1;
  }
}

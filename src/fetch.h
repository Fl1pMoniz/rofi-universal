#ifndef ROFI_UNIVERSAL_FETCH_H
#define ROFI_UNIVERSAL_FETCH_H

#include <glib.h>

typedef enum {
  U_FETCH_NONE,
  U_FETCH_CURL,
  U_FETCH_WGET,
} UFetcher;

UFetcher u_fetch_find(char **path);
char **u_fetch_build_argv(UFetcher fetcher, const char *path, const char *url,
                          int timeout_seconds);

#endif

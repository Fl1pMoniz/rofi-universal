#include "fetch.h"

UFetcher u_fetch_find(char **path) {
  if (path) {
    *path = NULL;
  }
  char *curl = g_find_program_in_path("curl");
  if (curl) {
    if (path) {
      *path = curl;
    } else {
      g_free(curl);
    }
    return U_FETCH_CURL;
  }
  char *wget = g_find_program_in_path("wget");
  if (wget) {
    if (path) {
      *path = wget;
    } else {
      g_free(wget);
    }
    return U_FETCH_WGET;
  }
  return U_FETCH_NONE;
}

char **u_fetch_build_argv(UFetcher fetcher, const char *path, const char *url,
                          int timeout_seconds) {
  if (fetcher == U_FETCH_NONE || !path || !url) {
    return NULL;
  }
  char timeout[16];
  g_snprintf(timeout, sizeof(timeout), "%d", timeout_seconds);
  if (fetcher == U_FETCH_CURL) {
    char **argv = g_new0(char *, 6);
    argv[0] = g_strdup(path);
    argv[1] = g_strdup("-fsSL");
    argv[2] = g_strdup("--max-time");
    argv[3] = g_strdup(timeout);
    argv[4] = g_strdup(url);
    return argv;
  }
  if (fetcher == U_FETCH_WGET) {
    char **argv = g_new0(char *, 8);
    argv[0] = g_strdup(path);
    argv[1] = g_strdup("-q");
    argv[2] = g_strdup("-T");
    argv[3] = g_strdup(timeout);
    argv[4] = g_strdup("-O");
    argv[5] = g_strdup("-");
    argv[6] = g_strdup(url);
    return argv;
  }
  return NULL;
}

#include "fetch.h"

#include <glib.h>
#include <glib/gstdio.h>

static void test_fetcher(void) {
  char *old_path = g_strdup(g_getenv("PATH"));
  char *dir = g_dir_make_tmp("rofi-universal-fetch-XXXXXX", NULL);
  g_assert_nonnull(dir);
  g_setenv("PATH", dir, TRUE);

  char *path = (char *)"unchanged";
  g_assert_cmpint(u_fetch_find(&path), ==, U_FETCH_NONE);
  g_assert_null(path);

  char *curl = g_build_filename(dir, "curl", NULL);
  char *wget = g_build_filename(dir, "wget", NULL);
  g_assert_true(g_file_set_contents(curl, "#!/bin/sh\n", -1, NULL));
  g_assert_cmpint(g_chmod(curl, 0755), ==, 0);
  g_assert_cmpint(u_fetch_find(&path), ==, U_FETCH_CURL);
  g_assert_cmpstr(path, ==, curl);
  g_free(path);

  g_assert_true(g_file_set_contents(wget, "#!/bin/sh\n", -1, NULL));
  g_assert_cmpint(g_chmod(wget, 0755), ==, 0);
  g_assert_cmpint(u_fetch_find(&path), ==, U_FETCH_CURL);
  g_free(path);
  g_assert_cmpint(g_remove(curl), ==, 0);
  g_assert_cmpint(u_fetch_find(&path), ==, U_FETCH_WGET);
  g_assert_cmpstr(path, ==, wget);
  g_free(path);

  g_remove(wget);
  g_rmdir(dir);
  g_free(curl);
  g_free(wget);
  g_free(dir);
  if (old_path) {
    g_setenv("PATH", old_path, TRUE);
    g_free(old_path);
  } else {
    g_unsetenv("PATH");
  }
}

static void test_argv(void) {
  char **argv = u_fetch_build_argv(U_FETCH_CURL, "/usr/bin/curl",
                                   "https://example.test/weather", 7);
  g_assert_nonnull(argv);
  g_assert_cmpstr(argv[0], ==, "/usr/bin/curl");
  g_assert_cmpstr(argv[1], ==, "-fsSL");
  g_assert_cmpstr(argv[2], ==, "--max-time");
  g_assert_cmpstr(argv[3], ==, "7");
  g_assert_cmpstr(argv[4], ==, "https://example.test/weather");
  g_assert_null(argv[5]);
  g_strfreev(argv);

  argv = u_fetch_build_argv(U_FETCH_WGET, "/usr/bin/wget",
                            "https://example.test/weather", 4);
  g_assert_nonnull(argv);
  g_assert_cmpstr(argv[0], ==, "/usr/bin/wget");
  g_assert_cmpstr(argv[1], ==, "-q");
  g_assert_cmpstr(argv[2], ==, "-T");
  g_assert_cmpstr(argv[3], ==, "4");
  g_assert_cmpstr(argv[4], ==, "-O");
  g_assert_cmpstr(argv[5], ==, "-");
  g_assert_cmpstr(argv[6], ==, "https://example.test/weather");
  g_assert_null(argv[7]);
  g_strfreev(argv);

  g_assert_null(u_fetch_build_argv(U_FETCH_NONE, "/usr/bin/curl",
                                   "https://example.test", 4));
  g_assert_null(u_fetch_build_argv(U_FETCH_CURL, NULL,
                                   "https://example.test", 4));
  g_assert_null(u_fetch_build_argv(U_FETCH_CURL, "/usr/bin/curl", NULL, 4));
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/fetch/fetcher", test_fetcher);
  g_test_add_func("/fetch/argv", test_argv);
  return g_test_run();
}

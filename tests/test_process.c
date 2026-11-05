#include "process.h"

#include <glib.h>
#include <string.h>

typedef struct {
  GMainLoop *loop;
  char *stdout_text;
  char *stderr_text;
  gboolean success;
  int exit_status;
} ProcessTest;

static void process_done(const char *stdout_text, const char *stderr_text,
                         int exit_status, gboolean success, gpointer data) {
  ProcessTest *test = data;
  g_free(test->stdout_text);
  g_free(test->stderr_text);
  test->stdout_text = g_strdup(stdout_text);
  test->stderr_text = g_strdup(stderr_text);
  test->success = success;
  test->exit_status = exit_status;
  g_main_loop_quit(test->loop);
}

static void process_test_clear(ProcessTest *test) {
  g_free(test->stdout_text);
  g_free(test->stderr_text);
  g_main_loop_unref(test->loop);
}

static gboolean cancel_process(gpointer data) {
  g_cancellable_cancel(G_CANCELLABLE(data));
  return G_SOURCE_REMOVE;
}

static void test_success(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf hello", NULL};
  u_process_run(argv, NULL, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_true(test.success);
  g_assert_cmpint(test.exit_status, ==, 0);
  g_assert_cmpstr(test.stdout_text, ==, "hello");
  g_assert_cmpstr(test.stderr_text, ==, "");
  process_test_clear(&test);
}

static void test_below_limit(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf hello", NULL};
  u_process_run_limited(argv, NULL, 100, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_true(test.success);
  g_assert_cmpstr(test.stdout_text, ==, "hello");
  process_test_clear(&test);
}

static void test_exact_limit(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf 12345", NULL};
  u_process_run_limited(argv, NULL, 5, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_true(test.success);
  g_assert_cmpstr(test.stdout_text, ==, "12345");
  process_test_clear(&test);
}

static void test_above_limit(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf 1234567890", NULL};
  u_process_run_limited(argv, NULL, 5, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_false(test.success);
  g_assert_cmpuint(strlen(test.stdout_text), <=, 5);
  g_assert_cmpuint(strlen(test.stdout_text), >, 0);
  process_test_clear(&test);
}

static void test_partial_reads(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "yes x | head -c 20000", NULL};
  u_process_run_limited(argv, NULL, 30000, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_true(test.success);
  g_assert_cmpuint(strlen(test.stdout_text), ==, 20000);
  process_test_clear(&test);
}

static void test_large_stderr(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf 1234567890 >&2", NULL};
  u_process_run_limited(argv, NULL, 5, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_false(test.success);
  g_assert_cmpstr(test.stdout_text, ==, "");
  g_assert_cmpuint(strlen(test.stderr_text), <=, 5);
  g_assert_cmpuint(strlen(test.stderr_text), >, 0);
  process_test_clear(&test);
}

static void test_timeout(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  gint64 start = g_get_monotonic_time();
  const char *argv[] = {"/bin/sh", "-c", "sleep 5; printf late", NULL};
  u_process_run_limited(argv, NULL, 1024, 100, process_done, &test);
  g_main_loop_run(test.loop);
  gint64 elapsed = g_get_monotonic_time() - start;
  g_assert_false(test.success);
  g_assert_cmpint(elapsed, <, 2000000);
  g_assert_cmpstr(test.stdout_text, ==, "");
  process_test_clear(&test);
}

static void test_continuous_output(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  gint64 start = g_get_monotonic_time();
  const char *argv[] = {"/bin/sh", "-c", "yes x", NULL};
  u_process_run_limited(argv, NULL, 64, 2000, process_done, &test);
  g_main_loop_run(test.loop);
  gint64 elapsed = g_get_monotonic_time() - start;
  g_assert_false(test.success);
  g_assert_cmpint(elapsed, <, 3000000);
  g_assert_cmpuint(strlen(test.stdout_text), <=, 64);
  process_test_clear(&test);
}

static void test_invalid_utf8(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf '\\377'", NULL};
  u_process_run_limited(argv, NULL, 100, 0, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_false(test.success);
  process_test_clear(&test);
}

static void test_cancelled(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  GCancellable *cancellable = g_cancellable_new();
  gint64 start = g_get_monotonic_time();
  const char *argv[] = {"/bin/sh", "-c", "sleep 5; printf late", NULL};
  u_process_run(argv, cancellable, process_done, &test);
  g_timeout_add(50, cancel_process, cancellable);
  g_main_loop_run(test.loop);
  gint64 elapsed = g_get_monotonic_time() - start;
  g_assert_false(test.success);
  g_assert_cmpint(elapsed, <, 3000000);
  process_test_clear(&test);
  g_object_unref(cancellable);
}

static void test_failed(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/bin/sh", "-c", "printf bad; exit 2", NULL};
  u_process_run(argv, NULL, process_done, &test);
  g_main_loop_run(test.loop);
  g_assert_false(test.success);
  g_assert_cmpint(test.exit_status, ==, 2);
  g_assert_cmpstr(test.stdout_text, ==, "bad");
  process_test_clear(&test);
}

static void test_spawn_failure(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/definitely/not/a/rofi-universal-test", NULL};
  u_process_run(argv, NULL, process_done, &test);
  g_assert_false(test.success);
  g_assert_cmpint(test.exit_status, ==, -1);
  process_test_clear(&test);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/process/success", test_success);
  g_test_add_func("/process/below-limit", test_below_limit);
  g_test_add_func("/process/exact-limit", test_exact_limit);
  g_test_add_func("/process/above-limit", test_above_limit);
  g_test_add_func("/process/partial-reads", test_partial_reads);
  g_test_add_func("/process/large-stderr", test_large_stderr);
  g_test_add_func("/process/timeout", test_timeout);
  g_test_add_func("/process/continuous-output", test_continuous_output);
  g_test_add_func("/process/invalid-utf8", test_invalid_utf8);
  g_test_add_func("/process/cancelled", test_cancelled);
  g_test_add_func("/process/failed", test_failed);
  g_test_add_func("/process/spawn-failure", test_spawn_failure);
  return g_test_run();
}

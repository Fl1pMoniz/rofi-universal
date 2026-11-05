#include "process.h"

#include <glib.h>
#include <string.h>

typedef struct {
  GMainLoop *loop;
  char *stdout_text;
  gboolean success;
  int exit_status;
} ProcessTest;

static void process_done(const char *stdout_text, const char *stderr_text,
                         int exit_status, gboolean success, gpointer data) {
  (void)stderr_text;
  ProcessTest *test = data;
  g_free(test->stdout_text);
  test->stdout_text = g_strdup(stdout_text);
  test->success = success;
  test->exit_status = exit_status;
  g_main_loop_quit(test->loop);
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
  g_free(test.stdout_text);
  g_main_loop_unref(test.loop);
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
  g_free(test.stdout_text);
  g_object_unref(cancellable);
  g_main_loop_unref(test.loop);
}

static void test_spawn_failure(void) {
  ProcessTest test = {0};
  test.loop = g_main_loop_new(NULL, FALSE);
  const char *argv[] = {"/definitely/not/a/rofi-universal-test", NULL};
  u_process_run(argv, NULL, process_done, &test);
  g_assert_false(test.success);
  g_assert_cmpint(test.exit_status, ==, -1);
  g_free(test.stdout_text);
  g_main_loop_unref(test.loop);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/process/success", test_success);
  g_test_add_func("/process/cancelled", test_cancelled);
  g_test_add_func("/process/spawn-failure", test_spawn_failure);
  return g_test_run();
}

#include "process.h"

#include <string.h>

typedef struct {
  GSubprocess *process;
  GCancellable *cancellable;
  gulong cancel_handler;
  UProcessDoneFunc done;
  gpointer user_data;
  gboolean cancelled;
} UProcessContext;

static void process_cancelled(GCancellable *cancellable, gpointer data) {
  (void)cancellable;
  UProcessContext *context = data;
  context->cancelled = TRUE;
  if (context->process) {
    g_subprocess_force_exit(context->process);
  }
}

static void process_finished(GObject *source, GAsyncResult *result,
                             gpointer data) {
  UProcessContext *context = data;
  GSubprocess *process = G_SUBPROCESS(source);
  char *stdout_text = NULL;
  char *stderr_text = NULL;
  GError *error = NULL;
  gboolean success = g_subprocess_communicate_utf8_finish(
      process, result, &stdout_text, &stderr_text, &error);
  if (error) {
    if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      context->cancelled = TRUE;
    }
    g_error_free(error);
  }
  if (context->cancelled) {
    success = FALSE;
  }
  int exit_status = -1;
  if (!context->cancelled && g_subprocess_get_if_exited(process)) {
    exit_status = g_subprocess_get_exit_status(process);
  } else if (!context->cancelled && g_subprocess_get_if_signaled(process)) {
    exit_status = 128 + g_subprocess_get_term_sig(process);
  }
  if (!context->cancelled && success && exit_status != 0) {
    success = FALSE;
  }
  if (context->cancel_handler && context->cancellable) {
    g_cancellable_disconnect(context->cancellable, context->cancel_handler);
    context->cancel_handler = 0;
  }
  if (context->done) {
    context->done(stdout_text, stderr_text, exit_status, success,
                  context->user_data);
  }
  g_free(stdout_text);
  g_free(stderr_text);
  if (context->cancellable) {
    g_object_unref(context->cancellable);
  }
  g_clear_object(&context->process);
  g_free(context);
}

void u_process_run(const char *const *argv, GCancellable *cancellable,
                   UProcessDoneFunc done, gpointer user_data) {
  if (!argv || !argv[0]) {
    if (done) {
      done(NULL, NULL, -1, FALSE, user_data);
    }
    return;
  }
  GError *error = NULL;
  GSubprocess *process = g_subprocess_newv(
      argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
      &error);
  if (!process) {
    if (error) {
      g_error_free(error);
    }
    if (done) {
      done(NULL, NULL, -1, FALSE, user_data);
    }
    return;
  }
  UProcessContext *context = g_new0(UProcessContext, 1);
  context->process = process;
  context->done = done;
  context->user_data = user_data;
  if (cancellable) {
    context->cancellable = g_object_ref(cancellable);
    context->cancel_handler =
        g_cancellable_connect(cancellable, G_CALLBACK(process_cancelled),
                              context, NULL);
  }
  g_subprocess_communicate_utf8_async(process, NULL, cancellable,
                                      process_finished, context);
}

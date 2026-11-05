#include "process.h"

#include <string.h>

#define READ_CHUNK 8192

typedef struct {
  GSubprocess *process;
  GCancellable *cancellable;
  GCancellable *read_cancellable;
  gulong cancel_handler;
  UProcessDoneFunc done;
  gpointer user_data;
  GString *stdout_text;
  GString *stderr_text;
  gsize max_output;
  GSource *timeout_source;
  gboolean truncated;
  gboolean cancelled;
  gboolean timed_out;
  gboolean read_error;
  gboolean wait_ok;
  gboolean stdout_done;
  gboolean stderr_done;
  gboolean wait_done;
  gboolean finished;
  int exit_status;
} UProcessContext;

typedef struct {
  UProcessContext *context;
  GInputStream *stream;
  gboolean is_stdout;
  guint8 *buffer;
} UProcessRead;

static void process_finish(UProcessContext *context);

static void process_cancelled(GCancellable *cancellable, gpointer data) {
  (void)cancellable;
  UProcessContext *context = data;
  context->cancelled = TRUE;
  if (context->read_cancellable) {
    g_cancellable_cancel(context->read_cancellable);
  }
  if (context->process) {
    g_subprocess_force_exit(context->process);
  }
}

static gboolean process_timed_out(gpointer data) {
  UProcessContext *context = data;
  context->timed_out = TRUE;
  if (context->read_cancellable) {
    g_cancellable_cancel(context->read_cancellable);
  }
  if (context->process) {
    g_subprocess_force_exit(context->process);
  }
  return G_SOURCE_REMOVE;
}

static void process_wait_done(GObject *source, GAsyncResult *result,
                              gpointer data) {
  UProcessContext *context = data;
  GSubprocess *process = G_SUBPROCESS(source);
  GError *error = NULL;
  if (!g_subprocess_wait_finish(process, result, &error)) {
    if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      context->cancelled = TRUE;
    }
    g_clear_error(&error);
  } else {
    context->wait_ok = TRUE;
    if (g_subprocess_get_if_exited(process)) {
      context->exit_status = g_subprocess_get_exit_status(process);
    } else if (g_subprocess_get_if_signaled(process)) {
      context->exit_status = 128 + g_subprocess_get_term_sig(process);
    } else {
      context->exit_status = -1;
    }
  }
  context->wait_done = TRUE;
  process_finish(context);
}

static void process_read_more(UProcessRead *read);

static void process_read_done(GObject *source, GAsyncResult *result,
                              gpointer data) {
  UProcessRead *read = data;
  UProcessContext *context = read->context;
  GError *error = NULL;
  gssize count =
      g_input_stream_read_finish(G_INPUT_STREAM(source), result, &error);
  if (count > 0) {
    GString *target =
        read->is_stdout ? context->stdout_text : context->stderr_text;
    gsize remaining =
        context->max_output > target->len ? context->max_output - target->len
                                          : 0;
    gsize copy = MIN((gsize)count, remaining);
    if (copy > 0) {
      g_string_append_len(target, (const char *)read->buffer, copy);
    }
    g_clear_error(&error);
    if ((gsize)count > remaining) {
      context->truncated = TRUE;
      if (context->read_cancellable) {
        g_cancellable_cancel(context->read_cancellable);
      }
      if (context->process) {
        g_subprocess_force_exit(context->process);
      }
      if (read->is_stdout) {
        context->stdout_done = TRUE;
      } else {
        context->stderr_done = TRUE;
      }
      g_free(read->buffer);
      g_free(read);
      process_finish(context);
      return;
    }
    process_read_more(read);
    return;
  }
  if (count < 0) {
    if (!(error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))) {
      context->read_error = TRUE;
    } else if (!context->cancelled && !context->timed_out &&
               !context->truncated) {
      context->read_error = TRUE;
    }
    if (context->read_cancellable) {
      g_cancellable_cancel(context->read_cancellable);
    }
    if (context->process) {
      g_subprocess_force_exit(context->process);
    }
    g_clear_error(&error);
  }
  if (read->is_stdout) {
    context->stdout_done = TRUE;
  } else {
    context->stderr_done = TRUE;
  }
  g_free(read->buffer);
  g_free(read);
  process_finish(context);
}

static void process_read_more(UProcessRead *read) {
  g_input_stream_read_async(read->stream, read->buffer, READ_CHUNK,
                            G_PRIORITY_DEFAULT,
                            read->context->read_cancellable, process_read_done,
                            read);
}

static void process_finish(UProcessContext *context) {
  if (context->finished || !context->stdout_done || !context->stderr_done ||
      !context->wait_done) {
    return;
  }
  context->finished = TRUE;
  if (context->timeout_source) {
    g_source_destroy(context->timeout_source);
    g_source_unref(context->timeout_source);
    context->timeout_source = NULL;
  }
  if (context->cancel_handler && context->cancellable) {
    g_cancellable_disconnect(context->cancellable, context->cancel_handler);
    context->cancel_handler = 0;
  }
  gboolean success = context->wait_ok && !context->cancelled &&
                     !context->timed_out && !context->truncated &&
                     !context->read_error && context->exit_status == 0;
  if ((context->stdout_text->len > 0 &&
       memchr(context->stdout_text->str, '\0', context->stdout_text->len)) ||
      (context->stderr_text->len > 0 &&
       memchr(context->stderr_text->str, '\0', context->stderr_text->len))) {
    success = FALSE;
  }
  char *stdout_text = g_string_free(context->stdout_text, FALSE);
  char *stderr_text = g_string_free(context->stderr_text, FALSE);
  if (stdout_text && !g_utf8_validate(stdout_text, -1, NULL)) {
    char *valid = g_utf8_make_valid(stdout_text, -1);
    g_free(stdout_text);
    stdout_text = valid;
    success = FALSE;
  }
  if (stderr_text && !g_utf8_validate(stderr_text, -1, NULL)) {
    char *valid = g_utf8_make_valid(stderr_text, -1);
    g_free(stderr_text);
    stderr_text = valid;
    success = FALSE;
  }
  if (context->done) {
    context->done(stdout_text, stderr_text, context->exit_status, success,
                  context->user_data);
  }
  g_free(stdout_text);
  g_free(stderr_text);
  if (context->cancellable) {
    g_object_unref(context->cancellable);
  }
  if (context->read_cancellable) {
    g_object_unref(context->read_cancellable);
  }
  g_clear_object(&context->process);
  g_free(context);
}

static void process_start_read(UProcessContext *context, gboolean is_stdout) {
  UProcessRead *read = g_new0(UProcessRead, 1);
  read->context = context;
  read->stream = is_stdout ? g_subprocess_get_stdout_pipe(context->process)
                           : g_subprocess_get_stderr_pipe(context->process);
  read->is_stdout = is_stdout;
  if (!read->stream) {
    if (is_stdout) {
      context->stdout_done = TRUE;
    } else {
      context->stderr_done = TRUE;
    }
    g_free(read);
    process_finish(context);
    return;
  }
  read->buffer = g_malloc(READ_CHUNK);
  process_read_more(read);
}

void u_process_run_limited(const char *const *argv, GCancellable *cancellable,
                           gsize max_output_bytes, guint timeout_ms,
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
  context->max_output =
      max_output_bytes > 0 ? max_output_bytes : U_PROCESS_DEFAULT_MAX_OUTPUT;
  context->exit_status = -1;
  context->stdout_text = g_string_new(NULL);
  context->stderr_text = g_string_new(NULL);
  context->read_cancellable = g_cancellable_new();
  if (cancellable) {
    context->cancellable = g_object_ref(cancellable);
    context->cancel_handler =
        g_cancellable_connect(cancellable, G_CALLBACK(process_cancelled),
                              context, NULL);
  }
  if (timeout_ms > 0) {
    GSource *timeout = g_timeout_source_new(timeout_ms);
    g_source_set_callback(timeout, process_timed_out, context, NULL);
    g_source_attach(timeout, g_main_context_get_thread_default());
    context->timeout_source = timeout;
  }
  process_start_read(context, TRUE);
  process_start_read(context, FALSE);
  g_subprocess_wait_async(process, cancellable, process_wait_done, context);
}

void u_process_run(const char *const *argv, GCancellable *cancellable,
                   UProcessDoneFunc done, gpointer user_data) {
  u_process_run_limited(argv, cancellable, U_PROCESS_DEFAULT_MAX_OUTPUT, 0,
                        done, user_data);
}

typedef struct {
  GMainLoop *loop;
  char *stdout_text;
  char *stderr_text;
  gboolean success;
  gboolean done;
  int exit_status;
} USyncCapture;

static void sync_capture_done(const char *stdout_text,
                              const char *stderr_text, int exit_status,
                              gboolean success, gpointer data) {
  USyncCapture *capture = data;
  capture->stdout_text = g_strdup(stdout_text);
  capture->stderr_text = g_strdup(stderr_text);
  capture->success = success;
  capture->exit_status = exit_status;
  capture->done = TRUE;
  g_main_loop_quit(capture->loop);
}

gboolean u_process_capture_sync_limited(const char *const *argv,
                                        gsize max_output_bytes,
                                        guint timeout_ms,
                                        char **stdout_text,
                                        int *exit_status) {
  if (stdout_text) {
    *stdout_text = NULL;
  }
  if (exit_status) {
    *exit_status = -1;
  }
  if (!argv || !argv[0]) {
    return FALSE;
  }
  GMainContext *context = g_main_context_new();
  g_main_context_push_thread_default(context);
  GMainLoop *loop = g_main_loop_new(context, FALSE);
  USyncCapture capture = {.loop = loop, .exit_status = -1};
  u_process_run_limited(argv, NULL, max_output_bytes, timeout_ms,
                        sync_capture_done, &capture);
  if (!capture.done) {
    g_main_loop_run(loop);
  }
  g_main_loop_unref(loop);
  g_main_context_pop_thread_default(context);
  g_main_context_unref(context);
  gboolean ok = capture.success;
  if (ok && capture.stdout_text &&
      memchr(capture.stdout_text, '\0', strlen(capture.stdout_text)) != NULL) {
    ok = FALSE;
  }
  if (ok) {
    if (stdout_text) {
      *stdout_text = g_steal_pointer(&capture.stdout_text);
    }
  }
  if (exit_status) {
    *exit_status = capture.exit_status;
  }
  g_free(capture.stdout_text);
  g_free(capture.stderr_text);
  return ok;
}

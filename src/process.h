#ifndef ROFI_UNIVERSAL_PROCESS_H
#define ROFI_UNIVERSAL_PROCESS_H

#include <gio/gio.h>

#define U_PROCESS_DEFAULT_MAX_OUTPUT (4u * 1024u * 1024u)

typedef void (*UProcessDoneFunc)(const char *stdout_text,
                                 const char *stderr_text, int exit_status,
                                 gboolean success, gpointer user_data);

void u_process_run(const char *const *argv, GCancellable *cancellable,
                   UProcessDoneFunc done, gpointer user_data);
void u_process_run_limited(const char *const *argv, GCancellable *cancellable,
                           gsize max_output_bytes, guint timeout_ms,
                           UProcessDoneFunc done, gpointer user_data);
gboolean u_process_capture_sync_limited(const char *const *argv,
                                        gsize max_output_bytes,
                                        char **stdout_text,
                                        int *exit_status);

#endif

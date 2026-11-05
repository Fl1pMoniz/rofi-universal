#ifndef ROFI_UNIVERSAL_PROCESS_H
#define ROFI_UNIVERSAL_PROCESS_H

#include <gio/gio.h>

typedef void (*UProcessDoneFunc)(const char *stdout_text,
                                 const char *stderr_text, int exit_status,
                                 gboolean success, gpointer user_data);

void u_process_run(const char *const *argv, GCancellable *cancellable,
                   UProcessDoneFunc done, gpointer user_data);

#endif

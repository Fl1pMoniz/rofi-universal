#include "universal.h"

#include <glib.h>
#include <stdio.h>

static const char *queries[] = {
    "firefox",          "steam games",       "25 * 18",
    "10 km to miles",   "define ephemeral",  "weather in Lisbon",
    "github issues rofi", "linux hyprland configuration",
    "python 3.14 features", "~/Downloads/report.pdf", NULL};

int main(void) {
  const int iterations = 200000;
  gint64 start = g_get_monotonic_time();
  for (int i = 0; i < iterations; i++) {
    UQueryAnalysis analysis;
    u_analyze_query(queries[i % 10], &analysis);
  }
  gint64 analysis_elapsed = g_get_monotonic_time() - start;

  start = g_get_monotonic_time();
  for (int i = 0; i < iterations; i++) {
    volatile int score = u_app_match_score(
        "visual studio code", "code", "editor;ide",
        "visual studio code code editor", "code settings");
    (void)score;
  }
  gint64 match_elapsed = g_get_monotonic_time() - start;

  start = g_get_monotonic_time();
  for (int i = 0; i < iterations; i++) {
    volatile int score =
        u_file_match_score("/home/user/Downloads/report final.pdf",
                           "report.pdf");
    (void)score;
  }
  gint64 file_elapsed = g_get_monotonic_time() - start;

  UResult *result =
      u_result_new(U_PROVIDER_FILES, "report.pdf",
                   "File · open · /home/user/Downloads", "text-x-generic", 100,
                   U_ACTION_OPEN_PATH, "/home/user/Downloads/report.pdf",
                   "/home/user/Downloads/report.pdf");
  start = g_get_monotonic_time();
  for (int i = 0; i < iterations; i++) {
    char *text = u_result_display_text(result);
    g_free(text);
  }
  gint64 display_elapsed = g_get_monotonic_time() - start;
  u_result_free(result);

  GPtrArray *owned = g_ptr_array_new_with_free_func(u_result_free);
  for (int i = 0; i < 20; i++) {
    g_ptr_array_add(
        owned,
        u_result_new(U_PROVIDER_WEB, "result", "Web · search", "web", i,
                     U_ACTION_OPEN_URI, i % 2 == 0 ? "https://a" : "https://b",
                     "https://a"));
  }
  const int dedup_iterations = 50000;
  start = g_get_monotonic_time();
  for (int i = 0; i < dedup_iterations; i++) {
    GPtrArray *work = g_ptr_array_new();
    for (guint j = 0; j < owned->len; j++) {
      g_ptr_array_add(work, g_ptr_array_index(owned, j));
    }
    u_results_dedup(work);
    g_ptr_array_free(work, TRUE);
  }
  gint64 dedup_elapsed = g_get_monotonic_time() - start;
  g_ptr_array_free(owned, TRUE);

  printf("query analysis: %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, analysis_elapsed / 1000.0,
         (double)analysis_elapsed * 1000.0 / iterations);
  printf("app matching:   %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, match_elapsed / 1000.0,
         (double)match_elapsed * 1000.0 / iterations);
  printf("file matching:  %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, file_elapsed / 1000.0,
         (double)file_elapsed * 1000.0 / iterations);
  printf("display format: %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, display_elapsed / 1000.0,
         (double)display_elapsed * 1000.0 / iterations);
  printf("dedup (20):     %d iterations in %.3f ms (%.1f ns/op)\n",
         dedup_iterations, dedup_elapsed / 1000.0,
         (double)dedup_elapsed * 1000.0 / dedup_iterations);
  return 0;
}

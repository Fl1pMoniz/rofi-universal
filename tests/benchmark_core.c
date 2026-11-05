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

  printf("query analysis: %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, analysis_elapsed / 1000.0,
         (double)analysis_elapsed * 1000.0 / iterations);
  printf("app matching:   %d iterations in %.3f ms (%.1f ns/op)\n",
         iterations, match_elapsed / 1000.0,
         (double)match_elapsed * 1000.0 / iterations);
  return 0;
}

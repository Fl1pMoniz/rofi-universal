#include "universal.h"

#include <glib.h>

typedef struct {
  const char *name;
  const char *query;
  int intent;
  int calc;
  int app_min;
  int web_min;
  int file_min;
  int github_min;
  int weather_min;
  int dictionary_min;
  int path_like;
  int url_like;
  int check_explicit;
  UProvider explicit_provider;
} QueryCase;

static const QueryCase cases[] = {
    {.name = "app-python3",
     .query = "python3",
     .intent = U_INTENT_APP,
     .calc = 0,
     .app_min = 500},
    {.name = "web-python-features",
     .query = "python 3.14 features",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 750},
    {.name = "web-firefox-123",
     .query = "firefox 123",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "web-firefox-extensions",
     .query = "firefox extensions",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 750},
    {.name = "calc-multiply",
     .query = "25 * 18",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "calc-percent",
     .query = "25% of 180",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "calc-hex",
     .query = "0xff",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "calc-binary",
     .query = "0b1010",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "calc-octal",
     .query = "0o755",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "not-calc-units",
     .query = "10 km to miles",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "web-currency-eur-usd",
     .query = "300 eur to USD",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "web-currency-usd-eur",
     .query = "100 USD to EUR",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "web-currency-gbp-eur",
     .query = "50 gbp in eur",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "web-currency-jpy-usd",
     .query = "1000 JPY to USD",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 600},
    {.name = "calc-parentheses",
     .query = "(25 + 15) * 2",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "calc-sqrt",
     .query = "sqrt(144)",
     .intent = U_INTENT_CALC,
     .calc = 1},
    {.name = "not-calc-monitors",
     .query = "2 monitors",
     .intent = U_INTENT_WEB,
     .calc = 0},
    {.name = "not-calc-hypr-monitors",
     .query = "hyprland 2 monitors",
     .intent = U_INTENT_WEB,
     .calc = 0},
    {.name = "not-calc-version",
     .query = "application version 2.1",
     .intent = U_INTENT_WEB,
     .calc = 0},
    {.name = "web-terminal-commands",
     .query = "terminal commands",
     .intent = U_INTENT_WEB,
     .calc = 0,
     .web_min = 750},
    {.name = "app-steam-games",
     .query = "steam games",
     .intent = U_INTENT_WEB,
     .app_min = 250,
     .web_min = 600},
    {.name = "github-query",
     .query = "github hyprwhispr",
     .intent = U_INTENT_GITHUB,
     .github_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_GITHUB},
    {.name = "github-issues",
     .query = "github issues rofi",
     .intent = U_INTENT_GITHUB,
     .github_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_GITHUB},
    {.name = "url-domain",
     .query = "github.com/rofi",
     .intent = U_INTENT_URL,
     .web_min = 1000,
     .path_like = 0,
     .url_like = 1},
    {.name = "file-path",
     .query = "~/Downloads/report.pdf",
     .intent = U_INTENT_FILE,
     .file_min = 600,
     .path_like = 1,
     .url_like = 0,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_FILES},
    {.name = "file-config",
     .query = "config.ini",
     .intent = U_INTENT_APP,
     .file_min = 600,
     .url_like = 0},
    {.name = "weather-default",
     .query = "weather",
     .intent = U_INTENT_WEATHER,
     .weather_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_WEATHER},
    {.name = "weather-explicit",
     .query = "weather in Tokyo",
     .intent = U_INTENT_WEATHER,
     .weather_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_WEATHER},
    {.name = "dictionary-define",
     .query = "define ephemeral",
     .intent = U_INTENT_DICTIONARY,
     .dictionary_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_DICTIONARY},
    {.name = "dictionary-meaning",
     .query = "ephemeral meaning",
     .intent = U_INTENT_DICTIONARY,
     .dictionary_min = 600,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_DICTIONARY},
    {.name = "youtube-bang",
     .query = "!yt linux desktop customization",
     .intent = U_INTENT_WEB,
     .web_min = 1000,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_WEB},
    {.name = "arch-bang",
     .query = "!arch package-name",
     .intent = U_INTENT_WEB,
     .web_min = 1000,
     .check_explicit = 1,
     .explicit_provider = U_PROVIDER_WEB},
    {.name = "not-url-pdf",
     .query = "report.pdf",
     .intent = -1,
     .file_min = 600,
     .url_like = 0},
    {.name = "not-url-script",
     .query = "script.sh",
     .intent = -1,
     .file_min = 600,
     .url_like = 0},
    {.name = "not-url-ai",
     .query = "logo.ai",
     .intent = -1,
     .file_min = 600,
     .url_like = 0},
    {.name = "not-url-template",
     .query = "config.h.in",
     .intent = -1,
     .file_min = 600,
     .url_like = 0},
    {.name = "relative-path",
     .query = "foo/bar",
     .intent = U_INTENT_FILE,
     .file_min = 600,
     .path_like = 1},
    {.name = "unicode-app",
     .query = "café",
     .intent = U_INTENT_APP},
    {.name = "empty",
     .query = "",
     .intent = U_INTENT_EMPTY,
     .calc = 0},
    {.name = "whitespace",
     .query = "   ",
     .intent = U_INTENT_EMPTY,
     .calc = 0},
};


static void test_query_case(gconstpointer data) {
  const QueryCase *test = data;
  UQueryAnalysis analysis;
  u_analyze_query(test->query, &analysis);
  g_test_message("query: %s", test->query);
  if (test->intent >= 0) {
    g_assert_cmpint(analysis.intent, ==, test->intent);
  }
  if (test->calc >= 0) {
    g_assert_cmpint(analysis.calc_score >= 600, ==, test->calc);
  }
  if (test->app_min) {
    g_assert_cmpint(analysis.app_score, >=, test->app_min);
  }
  if (test->web_min) {
    g_assert_cmpint(analysis.web_score, >=, test->web_min);
  }
  if (test->file_min) {
    g_assert_cmpint(analysis.file_score, >=, test->file_min);
  }
  if (test->github_min) {
    g_assert_cmpint(analysis.github_score, >=, test->github_min);
  }
  if (test->weather_min) {
    g_assert_cmpint(analysis.weather_score, >=, test->weather_min);
  }
  if (test->dictionary_min) {
    g_assert_cmpint(analysis.dictionary_score, >=, test->dictionary_min);
  }
  if (test->path_like >= 0) {
    g_assert_cmpint(analysis.path_like, ==, test->path_like);
  }
  if (test->url_like >= 0) {
    g_assert_cmpint(analysis.url_like, ==, test->url_like);
  }
  if (test->check_explicit) {
    g_assert_cmpint(analysis.explicit_provider, ==, test->explicit_provider);
  }
}

static void test_ranking_regressions(void) {
  int exact = u_app_match_score("steam", "steam", "game store",
                                "steam steam game store", "steam");
  int weak = u_app_match_score("bar", "bar", "game store",
                               "bar bar game store", "steam");
  g_assert_cmpint(exact, >, weak);
  g_assert_cmpint(u_file_match_score("/tmp/report.pdf", "report.pdf"), >,
                  u_file_match_score("/tmp/archive/report.pdf", "archive"));
  int terminal_app =
      u_app_match_score("terminal", "kitty", "terminal emulator",
                        "terminal terminal emulator kitty", "terminal commands");
  g_assert_cmpint(u_score_signals(terminal_app, 250, TRUE), <,
                  u_score_signals(100, 750, FALSE));
  int steam_app = u_app_match_score("steam", "steam", "game store",
                                    "steam steam game store", "steam games");
  g_assert_cmpint(u_score_signals(steam_app, 250, TRUE), >,
                  u_score_signals(100, 600, FALSE));
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  char paths[G_N_ELEMENTS(cases)][128];
  for (guint i = 0; i < G_N_ELEMENTS(cases); i++) {
    g_snprintf(paths[i], sizeof(paths[i]), "/query/regression/%s",
               cases[i].name);
    g_test_add_data_func(paths[i], &cases[i], test_query_case);
  }
  g_test_add_func("/query/ranking-regressions", test_ranking_regressions);
  return g_test_run();
}

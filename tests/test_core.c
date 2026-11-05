#include "universal.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <unistd.h>

static void test_calculable(void) {
  g_assert_true(u_query_looks_calculable("25 * 18"));
  g_assert_true(u_query_looks_calculable("500 EUR to USD"));
  g_assert_true(u_query_looks_calculable("10 km to miles"));
  g_assert_true(u_query_looks_calculable("sqrt(16)"));
  g_assert_true(u_query_looks_calculable("50%"));
  g_assert_true(u_query_looks_calculable("2^10"));
  g_assert_true(u_query_looks_calculable("1/2"));
  g_assert_true(u_query_looks_calculable("150 to hex"));
  g_assert_true(u_query_looks_calculable("10 in binary"));
  g_assert_false(u_query_looks_calculable("firefox"));
  g_assert_false(u_query_looks_calculable("hyprland configuration"));
  g_assert_false(u_query_looks_calculable("python3"));
  g_assert_false(u_query_looks_calculable("firefox 123"));
  g_assert_false(u_query_looks_calculable("2 monitors"));
  g_assert_false(u_query_looks_calculable("hyprland 2 monitor"));
  g_assert_false(u_query_looks_calculable("application version 2.1"));
}


static void test_analysis(void) {
  UQueryAnalysis analysis;
  u_analyze_query("python3", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);
  g_assert_cmpint(analysis.app_score, >=, 500);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_APP);

  u_analyze_query("25 * 18", &analysis);
  g_assert_cmpint(analysis.calc_score, >=, 600);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_CALC);

  u_analyze_query("firefox 123", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_WEB);

  u_analyze_query("2 monitors", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);

  u_analyze_query("hyprland 2 monitor", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);

  u_analyze_query("500 EUR to USD", &analysis);
  g_assert_cmpint(analysis.calc_score, >=, 600);

  u_analyze_query("10 km to miles", &analysis);
  g_assert_cmpint(analysis.calc_score, >=, 600);

  u_analyze_query("define ephemeral", &analysis);
  g_assert_cmpint(analysis.dictionary_score, >=, 600);
  g_assert_cmpint(analysis.explicit_provider, ==, U_PROVIDER_DICTIONARY);

  u_analyze_query("weather in Tokyo", &analysis);
  g_assert_cmpint(analysis.weather_score, >=, 600);
  g_assert_cmpint(analysis.explicit_provider, ==, U_PROVIDER_WEATHER);

  u_analyze_query("github repo", &analysis);
  g_assert_cmpint(analysis.github_score, >=, 600);
  g_assert_cmpint(analysis.explicit_provider, ==, U_PROVIDER_GITHUB);

  u_analyze_query("~/Downloads/report.pdf", &analysis);
  g_assert_true(analysis.path_like);
  g_assert_cmpint(analysis.file_score, >=, 600);

  u_analyze_query("~/Downloads/report file.pdf", &analysis);
  g_assert_true(analysis.path_like);

  g_assert_false(u_query_is_path("1/2"));
  g_assert_true(u_query_is_path("foo/bar"));

  u_analyze_query("https://example.com", &analysis);
  g_assert_true(analysis.url_like);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_URL);

  u_analyze_query("café", &analysis);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_APP);

  u_analyze_query("日本語", &analysis);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_APP);

  u_analyze_query("clip github", &analysis);
  g_assert_cmpint(analysis.clipboard_score, >=, 600);
  g_assert_cmpint(analysis.explicit_provider, ==, U_PROVIDER_CLIPBOARD);

  u_analyze_query("linux hyprland configuration", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);
  g_assert_cmpint(analysis.web_score, >=, 600);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_WEB);

  u_analyze_query("python", &analysis);
  g_assert_cmpint(analysis.app_score, >=, 500);
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_APPS), >=,
                  u_provider_relevance(&analysis, U_PROVIDER_WEB));
}

static void test_provider_relevance(void) {
  UQueryAnalysis analysis;
  u_analyze_query("github repo", &analysis);
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_GITHUB), >,
                  u_provider_relevance(&analysis, U_PROVIDER_WEB));

  u_analyze_query("25 * 18", &analysis);
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_CALC), >,
                  u_provider_relevance(&analysis, U_PROVIDER_APPS));
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_CALC), >,
                  u_provider_relevance(&analysis, U_PROVIDER_WEB));

  u_analyze_query("~/Downloads/report.pdf", &analysis);
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_FILES), >,
                  u_provider_relevance(&analysis, U_PROVIDER_WEB));

  g_assert_cmpint(u_score_signals(500, 100, TRUE), >,
                  u_score_signals(500, 100, FALSE));
}

static void test_intent(void) {
  gboolean calc = FALSE;
  gboolean path = FALSE;
  g_assert_cmpint(u_classify_intent("25 * 18", &calc, &path), ==, U_INTENT_CALC);
  g_assert_true(calc);
  g_assert_cmpint(u_classify_intent("500 EUR to USD", &calc, &path), ==,
                  U_INTENT_CALC);
  g_assert_cmpint(u_classify_intent("firefox", &calc, &path), ==, U_INTENT_APP);
  g_assert_cmpint(u_classify_intent("define ephemeral", &calc, &path), ==,
                  U_INTENT_DICTIONARY);
  g_assert_cmpint(u_classify_intent("weather in Tokyo", &calc, &path), ==,
                  U_INTENT_WEATHER);
  g_assert_cmpint(u_classify_intent("github hyprwhispr", &calc, &path), ==,
                  U_INTENT_GITHUB);
  g_assert_cmpint(u_classify_intent("~/Downloads/report.pdf", &calc, &path), ==,
                  U_INTENT_FILE);
  g_assert_true(path);
  g_assert_cmpint(u_classify_intent("https://example.com", &calc, &path), ==,
                  U_INTENT_URL);
  g_assert_cmpint(u_classify_intent("1/2", &calc, &path), ==, U_INTENT_CALC);
  g_assert_false(path);
  g_assert_cmpint(u_classify_intent("clip github", &calc, &path), ==,
                  U_INTENT_CLIPBOARD);
  g_assert_cmpint(u_classify_intent("weather Tokyo", &calc, &path), ==,
                  U_INTENT_WEATHER);
  g_assert_cmpint(u_classify_intent("!yt linux", &calc, &path), ==,
                  U_INTENT_WEB);
  g_assert_cmpint(u_classify_intent("", &calc, &path), ==, U_INTENT_EMPTY);
}

static void test_urls(void) {
  g_assert_true(u_query_is_url("https://example.com"));
  g_assert_true(u_query_is_url("www.example.com"));
  g_assert_false(u_query_is_url("example"));
  char *url = u_search_url("google", "a b");
  g_assert_cmpstr(url, ==, "https://www.google.com/search?q=a%20b");
  g_free(url);
  url = u_search_url("youtube", "linux tricks");
  g_assert_cmpstr(url, ==,
                  "https://www.youtube.com/results?search_query=linux%20tricks");
  g_free(url);
  url = u_search_url("https://example.org/?q=%s", "a b");
  g_assert_cmpstr(url, ==, "https://example.org/?q=a%20b");
  g_free(url);
  url = u_search_url("duckduckgo", "a&b?c");
  g_assert_cmpstr(url, ==, "https://duckduckgo.com/?q=a%26b%3Fc");
  g_free(url);
}

static void test_ranking(void) {
  GPtrArray *results = g_ptr_array_new_with_free_func(u_result_free);
  g_ptr_array_add(results,
                  u_result_new(U_PROVIDER_WEB, "web", "Web", "web", 100,
                               U_ACTION_OPEN_URI, "https://example.com", NULL));
  g_ptr_array_add(results,
                  u_result_new(U_PROVIDER_APPS, "firefox", "Application",
                               "firefox", 900, U_ACTION_APP, "firefox.desktop",
                               "firefox"));
  g_ptr_array_sort(results, u_result_compare);
  UResult *first = g_ptr_array_index(results, 0);
  g_assert_cmpstr(first->title, ==, "firefox");
  g_ptr_array_free(results, TRUE);
  g_assert_cmpint(u_intent_boost(U_INTENT_CALC, U_PROVIDER_CALC), >,
                  u_intent_boost(U_INTENT_CALC, U_PROVIDER_WEB));
  g_assert_cmpint(u_intent_boost(U_INTENT_APP, U_PROVIDER_APPS), >,
                  u_intent_boost(U_INTENT_APP, U_PROVIDER_WEB));
}

static void test_config(void) {
  UConfig config;
  u_config_init(&config);
  g_assert_cmpint(config.max_results, ==, 40);
  g_assert_cmpstr(config.web_engine, ==, "duckduckgo");
  g_assert_true(config.calculator);
  g_assert_false(config.clipboard_automatic);
  g_assert_cmpint(config.clipboard_cache_seconds, ==, 30);
  u_config_clear(&config);

  char *path = NULL;
  GError *error = NULL;
  int fd = g_file_open_tmp("rofi-universal-test-XXXXXX.ini", &path, &error);
  g_assert_no_error(error);
  g_assert_cmpint(fd, >=, 0);
  close(fd);
  const char *content =
      "[general]\nmax_results=12\nweb_engine=google\n"
      "[providers]\ncalculator=false\nclipboard=false\n"
      "[files]\nmax_depth=2\nroots=/tmp\n"
      "[clipboard]\nautomatic=true\ncache_seconds=5\nmax_results=3\n";
  g_assert_true(g_file_set_contents(path, content, -1, &error));
  g_assert_no_error(error);
  u_config_init(&config);
  g_assert_true(u_config_load(&config, path));
  g_assert_cmpint(config.max_results, ==, 12);
  g_assert_cmpstr(config.web_engine, ==, "google");
  g_assert_false(config.calculator);
  g_assert_false(config.clipboard);
  g_assert_true(config.web);
  g_assert_cmpint(config.file_max_depth, ==, 2);
  g_assert_cmpstr(config.file_roots, ==, "/tmp");
  g_assert_true(config.clipboard_automatic);
  g_assert_cmpint(config.clipboard_cache_seconds, ==, 5);
  g_assert_cmpint(config.clipboard_max_results, ==, 3);
  u_config_clear(&config);
  g_remove(path);
  g_free(path);

  u_config_init(&config);
  g_assert_false(u_config_load(&config, "/definitely/not/a/rofi-universal/config.ini"));
  g_assert_cmpint(config.max_results, ==, 40);
  u_config_clear(&config);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/core/calculable", test_calculable);
  g_test_add_func("/core/analysis", test_analysis);
  g_test_add_func("/core/provider-relevance", test_provider_relevance);
  g_test_add_func("/core/intent", test_intent);
  g_test_add_func("/core/urls", test_urls);
  g_test_add_func("/core/ranking", test_ranking);
  g_test_add_func("/core/config", test_config);
  return g_test_run();
}

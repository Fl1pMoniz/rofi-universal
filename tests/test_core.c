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
  g_assert_true(u_query_looks_calculable("25% of 180"));
  g_assert_true(u_query_looks_calculable("0xff to decimal"));
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
  g_assert_cmpint(analysis.web_score, >=, 750);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_WEB);

  u_analyze_query("python 3.14 features", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);
  g_assert_cmpint(analysis.web_score, >=, 750);
  g_assert_cmpint(analysis.intent, ==, U_INTENT_WEB);

  u_analyze_query("25% of 180", &analysis);
  g_assert_cmpint(analysis.calc_score, >=, 600);

  u_analyze_query("weather in Lisbon", &analysis);
  g_assert_cmpint(analysis.weather_score, >=, 600);

  u_analyze_query("weather", &analysis);
  g_assert_cmpint(analysis.weather_score, >=, 600);

  u_analyze_query("github hyprwhispr", &analysis);
  g_assert_cmpint(analysis.github_score, >=, 600);

  u_analyze_query("steam", &analysis);
  g_assert_cmpint(analysis.app_score, >=, 500);

  u_analyze_query("report.pdf", &analysis);
  g_assert_cmpint(analysis.file_score, >=, 600);

  u_analyze_query("python", &analysis);
  g_assert_cmpint(analysis.app_score, >=, 500);
  g_assert_cmpint(u_provider_relevance(&analysis, U_PROVIDER_APPS), >=,
                  u_provider_relevance(&analysis, U_PROVIDER_WEB));

  u_analyze_query("python 3.14 examples", &analysis);
  g_assert_cmpint(analysis.calc_score, <, 600);
  g_assert_cmpint(analysis.web_score, >=, 750);

  u_analyze_query("rofi alternatives", &analysis);
  g_assert_cmpint(analysis.web_score, >=, 750);
}

static void test_match_helpers(void) {
  g_assert_cmpint(u_app_match_score("firefox", "firefox", "web browser",
                                    "firefox firefox web browser", "firefox"),
                  ==, 1000);
  g_assert_cmpint(u_app_match_score(
                      "visual studio code", "code", "editor;ide",
                      "visual studio code code editor", "code"),
                  >=, 1000);
  g_assert_cmpint(u_app_match_score("steam", "steam", "gaming;store",
                                    "steam steam gaming store", "steam games"),
                  >=, 500);
  g_assert_cmpint(u_app_match_score("python", "python3", "language",
                                    "python python3 language",
                                    "python 3.14 features"),
                  >=, 500);
  g_assert_cmpint(u_app_match_score("007 first light", "steam", "game",
                                    "007 first light game steam",
                                    "steam games"),
                  >=, 400);
  g_assert_cmpint(u_app_match_score("007 first light", "steam", "game",
                                    "007 first light game steam",
                                    "steam games"),
                  <, 500);
  g_assert_cmpint(u_app_match_score("firefox", "firefox", "web browser",
                                    "firefox firefox web browser", "browser"),
                  >=, 500);
  g_assert_cmpint(u_app_match_score("kitty", "kitty", "terminal emulator",
                                    "kitty kitty terminal emulator",
                                    "terminal"),
                  >=, 500);
  g_assert_cmpint(u_app_match_score("foo", "foo", "bar",
                                    "foo foo bar games", "games"),
                  <, 550);
  g_assert_cmpint(u_app_match_score("firefox", "firefox", "web browser",
                                    "firefox firefox web browser",
                                    "libreoffice"),
                  ==, 0);
  g_assert_cmpint(u_file_match_score("/tmp/report.pdf", "report.pdf"), >=,
                  1000);
  g_assert_cmpint(u_file_match_score("/tmp/hyprland.conf", "hypr"), >=, 850);
  g_assert_cmpint(u_file_match_score("/tmp/foo.png", "*.png"), >=, 400);

  g_assert_true(u_query_has_glob("*.png"));
  g_assert_true(u_query_has_glob("report?.pdf"));
  g_assert_true(u_query_has_glob("[ab].txt"));
  g_assert_false(u_query_has_glob("report.pdf"));
  g_assert_false(u_query_has_glob("hyprland.conf"));
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
  url = u_search_url("arch", "hyprland configuration");
  g_assert_cmpstr(url, ==,
                  "https://wiki.archlinux.org/index.php?search=hyprland%20"
                  "configuration");
  g_free(url);
  url = u_search_url("man", "rofi");
  g_assert_cmpstr(url, ==, "https://man.archlinux.org/search?q=rofi");
  g_free(url);
  url = u_search_url("docs", "glib main loop");
  g_assert_cmpstr(url, ==, "https://devdocs.io/#q=glib%20main%20loop");
  g_free(url);
  url = u_search_url("pkg", "rofi");
  g_assert_cmpstr(url, ==, "https://archlinux.org/packages/?q=rofi");
  g_free(url);
  url = u_github_search_url("rofi plugins", "repositories");
  g_assert_cmpstr(url, ==,
                  "https://github.com/search?type=repositories&q=rofi%20"
                  "plugins");
  g_free(url);
  url = u_github_search_url("rofi", "users");
  g_assert_cmpstr(url, ==,
                  "https://github.com/search?type=users&q=rofi");
  g_free(url);
  url = u_github_repo_url("Fl1pMoniz/rofi-universal");
  g_assert_cmpstr(url, ==,
                  "https://github.com/Fl1pMoniz/rofi-universal");
  g_free(url);
  g_assert_null(u_github_repo_url("rofi plugins"));
  g_assert_null(u_github_repo_url("a/b/c"));

  UGitHubCategory category = U_GITHUB_REPOS;
  char *term = u_github_strip_category("issues rofi", &category);
  g_assert_cmpint(category, ==, U_GITHUB_ISSUES);
  g_assert_cmpstr(term, ==, "rofi");
  g_free(term);
  term = u_github_strip_category("users torvalds", &category);
  g_assert_cmpint(category, ==, U_GITHUB_USERS);
  g_assert_cmpstr(term, ==, "torvalds");
  g_free(term);
  term = u_github_strip_category("repos rofi plugins", &category);
  g_assert_cmpint(category, ==, U_GITHUB_REPOS);
  g_assert_cmpstr(term, ==, "rofi plugins");
  g_free(term);
  term = u_github_strip_category("rofi plugins", &category);
  g_assert_cmpint(category, ==, U_GITHUB_REPOS);
  g_assert_cmpstr(term, ==, "rofi plugins");
  g_free(term);
  term = u_github_strip_category("issues", &category);
  g_assert_cmpint(category, ==, U_GITHUB_REPOS);
  g_assert_cmpstr(term, ==, "issues");
  g_free(term);
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

static void test_dedup(void) {
  UResult *low =
      u_result_new(U_PROVIDER_WEATHER, "low", "d", "i", 100,
                   U_ACTION_OPEN_URI, "https://example/weather", "low");
  UResult *high =
      u_result_new(U_PROVIDER_WEATHER, "high", "d", "i", 300,
                   U_ACTION_OPEN_URI, "https://example/weather", "high");
  UResult *web = u_result_new(U_PROVIDER_WEB, "web", "d", "i", 200,
                              U_ACTION_OPEN_URI, "https://example/web", "web");
  UResult *hint_a = u_result_new(U_PROVIDER_CALC, "hint-a", "d", "i", 100,
                                 U_ACTION_NONE, NULL, NULL);
  UResult *hint_b = u_result_new(U_PROVIDER_CALC, "hint-b", "d", "i", 200,
                                 U_ACTION_NONE, NULL, NULL);
  GPtrArray *results = g_ptr_array_new();
  g_ptr_array_add(results, low);
  g_ptr_array_add(results, high);
  g_ptr_array_add(results, web);
  g_ptr_array_add(results, hint_a);
  g_ptr_array_add(results, hint_b);
  u_results_dedup(results);
  g_assert_cmpuint(results->len, ==, 4);
  gboolean saw_high = FALSE;
  for (guint i = 0; i < results->len; i++) {
    UResult *result = g_ptr_array_index(results, i);
    if (result->provider == U_PROVIDER_WEATHER &&
        g_strcmp0(result->payload, "https://example/weather") == 0) {
      g_assert_cmpstr(result->title, ==, "high");
      saw_high = TRUE;
    }
  }
  g_assert_true(saw_high);
  g_ptr_array_free(results, TRUE);
  u_result_free(low);
  u_result_free(high);
  u_result_free(web);
  u_result_free(hint_a);
  u_result_free(hint_b);
}

static void test_dictionary_query(void) {
  char *word =
      u_dictionary_word_from_query("define ephemeral", "define ephemeral");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  word = u_dictionary_word_from_query("definition of ephemeral",
                                      "definition of ephemeral");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  word = u_dictionary_word_from_query("ephemeral meaning", "ephemeral meaning");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  word = u_dictionary_word_from_query("what does ephemeral mean",
                                      "what does ephemeral mean");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  word = u_dictionary_word_from_query("meaning of ephemeral",
                                      "meaning of ephemeral");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  word = u_dictionary_word_from_query("ephemeral definition",
                                      "ephemeral definition");
  g_assert_cmpstr(word, ==, "ephemeral");
  g_free(word);
  g_assert_null(u_dictionary_word_from_query("define", "define"));
  g_assert_null(u_dictionary_word_from_query("meaning", "meaning"));
  g_assert_null(u_dictionary_word_from_query("what does mean", "what does mean"));
  g_assert_null(u_dictionary_word_from_query("define /tmp/file", "define /tmp/file"));
  g_assert_null(u_dictionary_word_from_query("define 123", "define 123"));
  g_assert_null(u_dictionary_word_from_query("ephemeral", "ephemeral"));

  UQueryAnalysis analysis;
  u_analyze_query("ephemeral meaning", &analysis);
  g_assert_cmpint(analysis.dictionary_score, >=, 600);
  g_assert_cmpint(analysis.explicit_provider, ==, U_PROVIDER_DICTIONARY);
  u_analyze_query("what does ephemeral mean", &analysis);
  g_assert_cmpint(analysis.dictionary_score, >=, 600);
  u_analyze_query("ephemeral", &analysis);
  g_assert_cmpint(analysis.dictionary_score, <, 600);
}

static void test_dedup_key(void) {
  UResult *one = u_result_new(U_PROVIDER_DICTIONARY, "one", "d", "i", 100,
                              U_ACTION_OPEN_URI, "https://example/1", "one");
  UResult *two = u_result_new(U_PROVIDER_DICTIONARY, "two", "d", "i", 200,
                              U_ACTION_OPEN_URI, "https://example/1", "two");
  UResult *low = u_result_new(U_PROVIDER_DICTIONARY, "low", "d", "i", 50,
                              U_ACTION_OPEN_URI, "https://example/2", "low");
  UResult *high = u_result_new(U_PROVIDER_DICTIONARY, "high", "d", "i", 150,
                               U_ACTION_OPEN_URI, "https://example/3", "high");
  u_result_set_dedup_key(one, "one");
  u_result_set_dedup_key(two, "two");
  u_result_set_dedup_key(low, "same");
  u_result_set_dedup_key(high, "same");
  GPtrArray *results = g_ptr_array_new();
  g_ptr_array_add(results, one);
  g_ptr_array_add(results, two);
  g_ptr_array_add(results, low);
  g_ptr_array_add(results, high);
  u_results_dedup(results);
  g_assert_cmpuint(results->len, ==, 3);
  gboolean saw_high = FALSE;
  for (guint i = 0; i < results->len; i++) {
    UResult *result = g_ptr_array_index(results, i);
    if (g_strcmp0(result->title, "high") == 0) {
      saw_high = TRUE;
    }
  }
  g_assert_true(saw_high);
  g_ptr_array_free(results, TRUE);
  u_result_free(one);
  u_result_free(two);
  u_result_free(low);
  u_result_free(high);
}

static void test_config(void) {
  UConfig config;
  u_config_init(&config);
  g_assert_cmpint(config.max_results, ==, 40);
  g_assert_cmpstr(config.web_engine, ==, "duckduckgo");
  g_assert_true(config.calculator);
  g_assert_false(config.clipboard_automatic);
  g_assert_cmpint(config.clipboard_cache_seconds, ==, 30);
  g_assert_cmpstr(config.weather_location, ==, "");
  g_assert_cmpint(config.weather_cache_seconds, ==, 600);
  g_assert_cmpint(config.weather_timeout_seconds, ==, 4);
  g_assert_cmpint(config.dictionary_cache_seconds, ==, 86400);
  g_assert_cmpint(config.dictionary_timeout_seconds, ==, 4);
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
      "[clipboard]\nautomatic=true\ncache_seconds=5\nmax_results=3\n"
      "[weather]\nlocation=Porto\ncache_seconds=120\ntimeout_seconds=2\n"
      "[dictionary]\ncache_seconds=3600\ntimeout_seconds=3\n";
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
  g_assert_cmpstr(config.weather_location, ==, "Porto");
  g_assert_cmpint(config.weather_cache_seconds, ==, 120);
  g_assert_cmpint(config.weather_timeout_seconds, ==, 2);
  g_assert_cmpint(config.dictionary_cache_seconds, ==, 3600);
  g_assert_cmpint(config.dictionary_timeout_seconds, ==, 3);
  u_config_clear(&config);

  content = "[weather]\ncache_seconds=1\ntimeout_seconds=99\n";
  g_assert_true(g_file_set_contents(path, content, -1, &error));
  g_assert_no_error(error);
  u_config_init(&config);
  g_assert_true(u_config_load(&config, path));
  g_assert_cmpstr(config.weather_location, ==, "");
  g_assert_cmpint(config.weather_cache_seconds, ==, 30);
  g_assert_cmpint(config.weather_timeout_seconds, ==, 15);
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
  g_test_add_func("/core/match-helpers", test_match_helpers);
  g_test_add_func("/core/intent", test_intent);
  g_test_add_func("/core/urls", test_urls);
  g_test_add_func("/core/ranking", test_ranking);
  g_test_add_func("/core/dedup", test_dedup);
  g_test_add_func("/core/dedup-key", test_dedup_key);
  g_test_add_func("/core/dictionary-query", test_dictionary_query);
  g_test_add_func("/core/config", test_config);
  return g_test_run();
}

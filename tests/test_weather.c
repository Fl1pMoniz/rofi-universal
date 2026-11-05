#include "weather.h"

#include <glib.h>
#include <glib/gstdio.h>

static void test_location(void) {
  char *city = u_weather_location_from_query("weather in Lisbon",
                                             "weather in lisbon", "");
  g_assert_cmpstr(city, ==, "Lisbon");
  g_free(city);
  city = u_weather_location_from_query("weather New York", "weather new york",
                                       "");
  g_assert_cmpstr(city, ==, "New York");
  g_free(city);
  city = u_weather_location_from_query("weather", "weather", "Porto");
  g_assert_cmpstr(city, ==, "Porto");
  g_free(city);
  city = u_weather_location_from_query("weather", "weather", "");
  g_assert_null(city);
}

static void test_urls(void) {
  char *url = u_weather_forecast_url("New York");
  g_assert_cmpstr(url, ==, "https://wttr.in/New%20York");
  g_free(url);
  url = u_weather_summary_url("Lisbon");
  g_assert_cmpstr(url, ==,
                  "https://wttr.in/Lisbon?format=%l:+%c+%t+(feels+%f),+"
                  "wind+%w,+humidity+%h");
  g_free(url);
}

static void test_parse(void) {
  char *summary = u_weather_parse_response(
      "  Lisbon: cloudy +16°C (feels +13°C), wind 12km/h, humidity 53%\n\t");
  g_assert_cmpstr(summary, ==,
                  "Lisbon: cloudy +16°C (feels +13°C), wind 12km/h, "
                  "humidity 53%");
  g_free(summary);
  summary = u_weather_parse_response("Lisbon: +16°C");
  g_assert_cmpstr(summary, ==, "Lisbon: +16°C");
  g_free(summary);

  g_assert_null(u_weather_parse_response(NULL));
  g_assert_null(u_weather_parse_response(""));
  g_assert_null(u_weather_parse_response("   \n\t"));
  g_assert_null(u_weather_parse_response("<html>oops</html>"));
  g_assert_null(u_weather_parse_response("Unknown location; please try ~Lisbon"));
  g_assert_null(u_weather_parse_response("error: unavailable"));
  g_assert_null(u_weather_parse_response("Lisbon:"));
  g_assert_null(u_weather_parse_response("Lisbon: cloudy"));
  g_assert_null(u_weather_parse_response("Lisbon: 16C"));
  g_assert_null(u_weather_parse_response("\xffLisbon: +16°C"));
}

static void test_cache(void) {
  UWeatherCache cache;
  u_weather_cache_init(&cache, 60, 2);
  gboolean fresh = TRUE;
  g_assert_null(u_weather_cache_get(&cache, "Lisbon", 1 * G_USEC_PER_SEC,
                                    &fresh));
  g_assert_false(fresh);

  u_weather_cache_put(&cache, "Lisbon", "Lisbon: +16°C",
                      1 * G_USEC_PER_SEC);
  g_assert_cmpstr(u_weather_cache_get(&cache, "Lisbon", 60 * G_USEC_PER_SEC,
                                      &fresh),
                  ==, "Lisbon: +16°C");
  g_assert_true(fresh);
  g_assert_cmpstr(u_weather_cache_get(&cache, "Lisbon", 62 * G_USEC_PER_SEC,
                                      &fresh),
                  ==, "Lisbon: +16°C");
  g_assert_false(fresh);

  u_weather_cache_put(&cache, "Porto", "Porto: +14°C",
                      2 * G_USEC_PER_SEC);
  u_weather_cache_put(&cache, "Braga", "Braga: +13°C",
                      3 * G_USEC_PER_SEC);
  g_assert_null(u_weather_cache_get(&cache, "Lisbon", 3 * G_USEC_PER_SEC,
                                    &fresh));
  g_assert_cmpstr(u_weather_cache_get(&cache, "Porto", 3 * G_USEC_PER_SEC,
                                      &fresh),
                  ==, "Porto: +14°C");
  g_assert_cmpstr(u_weather_cache_get(&cache, "Braga", 3 * G_USEC_PER_SEC,
                                      &fresh),
                  ==, "Braga: +13°C");
  u_weather_cache_clear(&cache);
}

static void test_backoff(void) {
  gint64 failed = 10 * G_USEC_PER_SEC;
  gint64 now = 20 * G_USEC_PER_SEC;
  g_assert_true(u_weather_retry_backoff_active(failed, now, 30));
  g_assert_false(
      u_weather_retry_backoff_active(failed, 41 * G_USEC_PER_SEC, 30));
  g_assert_false(u_weather_retry_backoff_active(0, now, 30));
  g_assert_false(u_weather_retry_backoff_active(failed, 5 * G_USEC_PER_SEC, 30));
}

static void test_fetcher(void) {
  char *old_path = g_strdup(g_getenv("PATH"));
  char *dir = g_dir_make_tmp("rofi-universal-weather-XXXXXX", NULL);
  g_assert_nonnull(dir);
  g_setenv("PATH", dir, TRUE);

  char *path = (char *)"unchanged";
  g_assert_cmpint(u_weather_find_fetcher(&path), ==, U_WEATHER_FETCH_NONE);
  g_assert_null(path);

  char *curl = g_build_filename(dir, "curl", NULL);
  char *wget = g_build_filename(dir, "wget", NULL);
  g_assert_true(g_file_set_contents(curl, "#!/bin/sh\n", -1, NULL));
  g_assert_cmpint(g_chmod(curl, 0755), ==, 0);
  g_assert_cmpint(u_weather_find_fetcher(&path), ==, U_WEATHER_FETCH_CURL);
  g_assert_cmpstr(path, ==, curl);
  g_free(path);

  g_assert_true(g_file_set_contents(wget, "#!/bin/sh\n", -1, NULL));
  g_assert_cmpint(g_chmod(wget, 0755), ==, 0);
  g_assert_cmpint(u_weather_find_fetcher(&path), ==, U_WEATHER_FETCH_CURL);
  g_free(path);
  g_assert_cmpint(g_remove(curl), ==, 0);
  g_assert_cmpint(u_weather_find_fetcher(&path), ==, U_WEATHER_FETCH_WGET);
  g_assert_cmpstr(path, ==, wget);
  g_free(path);

  g_remove(wget);
  g_rmdir(dir);
  g_free(curl);
  g_free(wget);
  g_free(dir);
  if (old_path) {
    g_setenv("PATH", old_path, TRUE);
    g_free(old_path);
  } else {
    g_unsetenv("PATH");
  }
}

static void test_argv(void) {
  char **argv = u_weather_build_argv(U_WEATHER_FETCH_CURL, "/usr/bin/curl",
                                     "https://example.test/weather", 7);
  g_assert_nonnull(argv);
  g_assert_cmpstr(argv[0], ==, "/usr/bin/curl");
  g_assert_cmpstr(argv[1], ==, "-fsSL");
  g_assert_cmpstr(argv[2], ==, "--max-time");
  g_assert_cmpstr(argv[3], ==, "7");
  g_assert_cmpstr(argv[4], ==, "https://example.test/weather");
  g_assert_null(argv[5]);
  g_strfreev(argv);

  argv = u_weather_build_argv(U_WEATHER_FETCH_WGET, "/usr/bin/wget",
                              "https://example.test/weather", 4);
  g_assert_nonnull(argv);
  g_assert_cmpstr(argv[0], ==, "/usr/bin/wget");
  g_assert_cmpstr(argv[1], ==, "-q");
  g_assert_cmpstr(argv[2], ==, "-T");
  g_assert_cmpstr(argv[3], ==, "4");
  g_assert_cmpstr(argv[4], ==, "-O");
  g_assert_cmpstr(argv[5], ==, "-");
  g_assert_cmpstr(argv[6], ==, "https://example.test/weather");
  g_assert_null(argv[7]);
  g_strfreev(argv);

  g_assert_null(u_weather_build_argv(U_WEATHER_FETCH_NONE, "/usr/bin/curl",
                                     "https://example.test", 4));
  g_assert_null(u_weather_build_argv(U_WEATHER_FETCH_CURL, NULL,
                                     "https://example.test", 4));
  g_assert_null(u_weather_build_argv(U_WEATHER_FETCH_CURL, "/usr/bin/curl",
                                     NULL, 4));
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/weather/location", test_location);
  g_test_add_func("/weather/urls", test_urls);
  g_test_add_func("/weather/parse", test_parse);
  g_test_add_func("/weather/cache", test_cache);
  g_test_add_func("/weather/backoff", test_backoff);
  g_test_add_func("/weather/fetcher", test_fetcher);
  g_test_add_func("/weather/argv", test_argv);
  return g_test_run();
}

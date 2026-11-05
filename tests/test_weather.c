#include "weather.h"

#include <glib.h>

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
      "Lisbon: cloudy +16°C (feels +13°C), wind 12km/h, humidity 53%\n");
  g_assert_cmpstr(summary, ==,
                  "Lisbon: cloudy +16°C (feels +13°C), wind 12km/h, "
                  "humidity 53%");
  g_free(summary);
  g_assert_null(u_weather_parse_response(""));
  g_assert_null(u_weather_parse_response("<html>oops</html>"));
  g_assert_null(u_weather_parse_response("Unknown location; please try ~Lisbon"));
  g_assert_null(u_weather_parse_response("error: unavailable"));
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/weather/location", test_location);
  g_test_add_func("/weather/urls", test_urls);
  g_test_add_func("/weather/parse", test_parse);
  return g_test_run();
}

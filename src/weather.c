#include "weather.h"

#include <string.h>

static char *trimmed_copy(const char *text) {
  char *copy = g_strdup(text ? text : "");
  return g_strstrip(copy);
}

char *u_weather_location_from_query(const char *query, const char *folded_query,
                                    const char *default_location) {
  const char *start = NULL;
  if (query && folded_query) {
    if (g_str_has_prefix(folded_query, "weather in ")) {
      start = query + 11;
    } else if (g_str_has_prefix(folded_query, "weather ")) {
      start = query + 8;
    }
  }
  char *city = start ? trimmed_copy(start) : NULL;
  if ((!city || !*city) && default_location && *default_location) {
    g_free(city);
    city = trimmed_copy(default_location);
  }
  if (city && !*city) {
    g_clear_pointer(&city, g_free);
  }
  return city;
}

char *u_weather_forecast_url(const char *city) {
  char *encoded = g_uri_escape_string(city ? city : "", NULL, TRUE);
  char *url = g_strdup_printf("https://wttr.in/%s", encoded);
  g_free(encoded);
  return url;
}

char *u_weather_summary_url(const char *city) {
  char *encoded = g_uri_escape_string(city ? city : "", NULL, TRUE);
  char *url = g_strdup_printf(
      "https://wttr.in/%s?format=%%l:+%%c+%%t+(feels+%%f),+wind+%%w,+"
      "humidity+%%h",
      encoded);
  g_free(encoded);
  return url;
}

char *u_weather_parse_response(const char *raw) {
  char *text = trimmed_copy(raw);
  if (!*text) {
    g_free(text);
    return NULL;
  }
  char *folded = g_utf8_casefold(text, -1);
  gboolean invalid = strstr(folded, "<html") != NULL ||
                     strstr(folded, "unknown location") != NULL ||
                     strstr(folded, "sorry") != NULL ||
                     strstr(folded, "service unavailable") != NULL ||
                     strstr(folded, "error") != NULL || strchr(text, ':') == NULL;
  g_free(folded);
  if (invalid) {
    g_free(text);
    return NULL;
  }
  return text;
}

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
  if (!*text || !g_utf8_validate(text, -1, NULL)) {
    g_free(text);
    return NULL;
  }
  gboolean has_digit = FALSE;
  for (const char *p = text; *p; p++) {
    if (g_ascii_isdigit(*p)) {
      has_digit = TRUE;
      break;
    }
  }
  char *folded = g_utf8_casefold(text, -1);
  gboolean invalid = strstr(folded, "<html") != NULL ||
                     strstr(folded, "unknown location") != NULL ||
                     strstr(folded, "sorry") != NULL ||
                     strstr(folded, "service unavailable") != NULL ||
                     strstr(folded, "error") != NULL || !has_digit ||
                     strchr(text, ':') == NULL || strstr(text, "°") == NULL;
  g_free(folded);
  if (invalid) {
    g_free(text);
    return NULL;
  }
  return text;
}

static void weather_entry_free(gpointer data) {
  UWeatherCacheEntry *entry = data;
  g_free(entry->summary);
  g_free(entry);
}

void u_weather_cache_init(UWeatherCache *cache, int ttl_seconds,
                          guint capacity) {
  cache->ttl_seconds = ttl_seconds > 0 ? ttl_seconds : 600;
  cache->capacity = capacity > 0 ? capacity : 64;
  cache->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                         weather_entry_free);
}

void u_weather_cache_clear(UWeatherCache *cache) {
  if (cache && cache->entries) {
    g_hash_table_destroy(cache->entries);
    cache->entries = NULL;
  }
}

const char *u_weather_cache_get(UWeatherCache *cache, const char *city,
                                gint64 now, gboolean *fresh) {
  if (fresh) {
    *fresh = FALSE;
  }
  if (!cache || !cache->entries || !city) {
    return NULL;
  }
  UWeatherCacheEntry *entry = g_hash_table_lookup(cache->entries, city);
  if (!entry) {
    return NULL;
  }
  if (fresh) {
    *fresh = now >= entry->fetched_at &&
             now - entry->fetched_at <
                 (gint64)cache->ttl_seconds * G_USEC_PER_SEC;
  }
  return entry->summary;
}

void u_weather_cache_put(UWeatherCache *cache, const char *city,
                         const char *summary, gint64 now) {
  if (!cache || !cache->entries || !city || !summary) {
    return;
  }
  if (g_hash_table_size(cache->entries) >= cache->capacity) {
    const char *oldest_key = NULL;
    gint64 oldest_time = G_MAXINT64;
    GHashTableIter iter;
    gpointer key;
    gpointer value;
    g_hash_table_iter_init(&iter, cache->entries);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
      UWeatherCacheEntry *entry = value;
      if (entry->fetched_at < oldest_time) {
        oldest_time = entry->fetched_at;
        oldest_key = key;
      }
    }
    if (oldest_key) {
      g_hash_table_remove(cache->entries, oldest_key);
    }
  }
  UWeatherCacheEntry *entry = g_new0(UWeatherCacheEntry, 1);
  entry->summary = g_strdup(summary);
  entry->fetched_at = now;
  g_hash_table_replace(cache->entries, g_strdup(city), entry);
}

gboolean u_weather_retry_backoff_active(gint64 failed_at, gint64 now,
                                        int retry_seconds) {
  if (failed_at <= 0 || retry_seconds <= 0 || now < failed_at) {
    return FALSE;
  }
  return now - failed_at < (gint64)retry_seconds * G_USEC_PER_SEC;
}

#ifndef ROFI_UNIVERSAL_WEATHER_H
#define ROFI_UNIVERSAL_WEATHER_H

#include <glib.h>

typedef struct {
  char *summary;
  gint64 fetched_at;
} UWeatherCacheEntry;

typedef struct {
  GHashTable *entries;
  int ttl_seconds;
  guint capacity;
} UWeatherCache;

char *u_weather_location_from_query(const char *query, const char *folded_query,
                                    const char *default_location);
char *u_weather_forecast_url(const char *city);
char *u_weather_summary_url(const char *city);
char *u_weather_parse_response(const char *raw);

void u_weather_cache_init(UWeatherCache *cache, int ttl_seconds,
                          guint capacity);
void u_weather_cache_clear(UWeatherCache *cache);
const char *u_weather_cache_get(UWeatherCache *cache, const char *city,
                                gint64 now, gboolean *fresh);
void u_weather_cache_put(UWeatherCache *cache, const char *city,
                         const char *summary, gint64 now);
gboolean u_weather_retry_backoff_active(gint64 failed_at, gint64 now,
                                        int retry_seconds);

#endif

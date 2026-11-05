#ifndef ROFI_UNIVERSAL_WEATHER_H
#define ROFI_UNIVERSAL_WEATHER_H

#include <glib.h>

char *u_weather_location_from_query(const char *query, const char *folded_query,
                                    const char *default_location);
char *u_weather_forecast_url(const char *city);
char *u_weather_summary_url(const char *city);
char *u_weather_parse_response(const char *raw);

#endif

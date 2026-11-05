#ifndef ROFI_UNIVERSAL_CURRENCY_H
#define ROFI_UNIVERSAL_CURRENCY_H

#include <glib.h>

typedef struct {
  const char *code;
  guint8 minor_units;
} UCurrencyInfo;

typedef struct {
  char base[4];
  char quote[4];
  double amount;
} UCurrencyQuery;

typedef struct {
  char *base;
  char *date;
  GHashTable *rates;
  gint64 fetched_at;
} UCurrencyRates;

typedef struct {
  GHashTable *entries;
  int ttl_seconds;
  guint capacity;
} UCurrencyCache;

gboolean u_currency_parse_query(const char *query, UCurrencyQuery *out);
const UCurrencyInfo *u_currency_lookup(const char *code);
int u_currency_minor_units(const char *code);
char *u_currency_rates_url(const char *base);
UCurrencyRates *u_currency_parse_rates(const char *raw,
                                       const char *expected_base);
gboolean u_currency_rate(const UCurrencyRates *rates, const char *quote,
                         double *rate);
char *u_currency_format_amount(double amount, int minor_units);
void u_currency_rates_free(gpointer data);

void u_currency_cache_init(UCurrencyCache *cache, int ttl_seconds,
                           guint capacity);
void u_currency_cache_clear(UCurrencyCache *cache);
UCurrencyRates *u_currency_cache_get(UCurrencyCache *cache, const char *base,
                                     gint64 now, gboolean *fresh);
void u_currency_cache_put(UCurrencyCache *cache, UCurrencyRates *rates,
                          gint64 now);

#endif

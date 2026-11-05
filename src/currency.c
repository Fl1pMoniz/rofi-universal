#include "currency.h"

#include <json-glib/json-glib.h>
#include <math.h>
#include <string.h>

static const UCurrencyInfo currencies[] = {
    {"AED", 2}, {"AFN", 2}, {"ALL", 2}, {"AMD", 2}, {"ANG", 2},
    {"AOA", 2}, {"ARS", 2}, {"AUD", 2}, {"AWG", 2}, {"AZN", 2},
    {"BAM", 2}, {"BBD", 2}, {"BDT", 2}, {"BGN", 2}, {"BHD", 3},
    {"BIF", 0}, {"BMD", 2}, {"BND", 2}, {"BOB", 2}, {"BRL", 2},
    {"BSD", 2}, {"BTN", 2}, {"BWP", 2}, {"BYN", 2}, {"BZD", 2},
    {"CAD", 2}, {"CDF", 2}, {"CHF", 2}, {"CLP", 0}, {"CNY", 2},
    {"COP", 2}, {"CRC", 2}, {"CUP", 2}, {"CVE", 2}, {"CZK", 2},
    {"DJF", 0}, {"DKK", 2}, {"DOP", 2}, {"DZD", 2}, {"EGP", 2},
    {"ERN", 2}, {"ETB", 2}, {"EUR", 2}, {"FJD", 2}, {"FKP", 2},
    {"GBP", 2}, {"GEL", 2}, {"GHS", 2}, {"GIP", 2}, {"GMD", 2},
    {"GNF", 0}, {"GTQ", 2}, {"GYD", 2}, {"HKD", 2}, {"HNL", 2},
    {"HTG", 2}, {"HUF", 2}, {"IDR", 2}, {"ILS", 2}, {"INR", 2},
    {"IQD", 3}, {"IRR", 2}, {"ISK", 0}, {"JMD", 2}, {"JOD", 3},
    {"JPY", 0}, {"KES", 2}, {"KGS", 2}, {"KHR", 2}, {"KMF", 0},
    {"KPW", 2}, {"KRW", 0}, {"KWD", 3}, {"KYD", 2}, {"KZT", 2},
    {"LAK", 2}, {"LBP", 2}, {"LKR", 2}, {"LRD", 2}, {"LSL", 2},
    {"LYD", 3}, {"MAD", 2}, {"MDL", 2}, {"MGA", 2}, {"MKD", 2},
    {"MMK", 2}, {"MNT", 2}, {"MOP", 2}, {"MRU", 2}, {"MUR", 2},
    {"MVR", 2}, {"MWK", 2}, {"MXN", 2}, {"MYR", 2}, {"MZN", 2},
    {"NAD", 2}, {"NGN", 2}, {"NIO", 2}, {"NOK", 2}, {"NPR", 2},
    {"NZD", 2}, {"OMR", 3}, {"PAB", 2}, {"PEN", 2}, {"PGK", 2},
    {"PHP", 2}, {"PKR", 2}, {"PLN", 2}, {"PYG", 0}, {"QAR", 2},
    {"RON", 2}, {"RSD", 2}, {"RUB", 2}, {"RWF", 0}, {"SAR", 2},
    {"SBD", 2}, {"SCR", 2}, {"SDG", 2}, {"SEK", 2}, {"SGD", 2},
    {"SHP", 2}, {"SLE", 2}, {"SOS", 2}, {"SRD", 2}, {"SSP", 2},
    {"STN", 2}, {"SVC", 2}, {"SYP", 2}, {"SZL", 2}, {"THB", 2},
    {"TJS", 2}, {"TMT", 2}, {"TND", 3}, {"TOP", 2}, {"TRY", 2},
    {"TTD", 2}, {"TWD", 2}, {"TZS", 2}, {"UAH", 2}, {"UGX", 0},
    {"USD", 2}, {"UYU", 2}, {"UZS", 2}, {"VES", 2}, {"VND", 0},
    {"VUV", 0}, {"WST", 2}, {"XAF", 0}, {"XCD", 2}, {"XOF", 0},
    {"XPF", 0}, {"YER", 2}, {"ZAR", 2}, {"ZMW", 2},
    {NULL, 0},
};

static gboolean code_shape_valid(const char *code) {
  return code && g_ascii_isalpha(code[0]) && g_ascii_isalpha(code[1]) &&
         g_ascii_isalpha(code[2]) && !g_ascii_isalnum(code[3]);
}

const UCurrencyInfo *u_currency_lookup(const char *code) {
  if (!code || !code_shape_valid(code)) {
    return NULL;
  }
  for (int i = 0; currencies[i].code; i++) {
    if (g_ascii_strcasecmp(currencies[i].code, code) == 0) {
      return &currencies[i];
    }
  }
  return NULL;
}

int u_currency_minor_units(const char *code) {
  const UCurrencyInfo *info = u_currency_lookup(code);
  return info ? info->minor_units : -1;
}

static const char *skip_spaces(const char *text) {
  while (text && g_ascii_isspace(*text)) {
    text++;
  }
  return text;
}

static gboolean read_code(const char **cursor, char out[4]) {
  const char *p = *cursor;
  if (!code_shape_valid(p)) {
    return FALSE;
  }
  for (int i = 0; i < 3; i++) {
    out[i] = g_ascii_toupper(p[i]);
  }
  out[3] = '\0';
  *cursor = p + 3;
  return TRUE;
}

static gboolean read_connector(const char **cursor) {
  const char *p = skip_spaces(*cursor);
  if (g_ascii_strncasecmp(p, "to", 2) == 0) {
    *cursor = p + 2;
    return TRUE;
  }
  if (g_ascii_strncasecmp(p, "in", 2) == 0) {
    *cursor = p + 2;
    return TRUE;
  }
  return FALSE;
}

gboolean u_currency_parse_query(const char *query, UCurrencyQuery *out) {
  if (!query || !out) {
    return FALSE;
  }
  const char *p = skip_spaces(query);
  char *end = NULL;
  double amount = g_ascii_strtod(p, &end);
  if (end == p || !isfinite(amount)) {
    return FALSE;
  }
  p = skip_spaces(end);
  char base[4];
  char quote[4];
  if (!read_code(&p, base)) {
    return FALSE;
  }
  if (!read_connector(&p)) {
    return FALSE;
  }
  p = skip_spaces(p);
  if (!read_code(&p, quote)) {
    return FALSE;
  }
  p = skip_spaces(p);
  if (*p) {
    return FALSE;
  }
  const UCurrencyInfo *base_info = u_currency_lookup(base);
  const UCurrencyInfo *quote_info = u_currency_lookup(quote);
  if (!base_info || !quote_info) {
    return FALSE;
  }
  memcpy(out->base, base, sizeof(base));
  memcpy(out->quote, quote, sizeof(quote));
  out->amount = amount;
  return TRUE;
}

char *u_currency_rates_url(const char *base) {
  char *encoded = g_uri_escape_string(base ? base : "", NULL, TRUE);
  char *url = g_strdup_printf("https://api.frankfurter.dev/v2/rates?base=%s",
                              encoded);
  g_free(encoded);
  return url;
}

static const char *object_string(JsonObject *object, const char *key) {
  if (!object || !json_object_has_member(object, key)) {
    return NULL;
  }
  JsonNode *node = json_object_get_member(object, key);
  if (!node || !JSON_NODE_HOLDS_VALUE(node) ||
      json_node_get_value_type(node) != G_TYPE_STRING) {
    return NULL;
  }
  const char *value = json_node_get_string(node);
  return value && *value ? value : NULL;
}

static gboolean object_number(JsonObject *object, const char *key,
                              double *out) {
  if (!object || !json_object_has_member(object, key)) {
    return FALSE;
  }
  JsonNode *node = json_object_get_member(object, key);
  if (!node || !JSON_NODE_HOLDS_VALUE(node) ||
      (json_node_get_value_type(node) != G_TYPE_DOUBLE &&
       json_node_get_value_type(node) != G_TYPE_INT64)) {
    return FALSE;
  }
  double value = json_node_get_double(node);
  if (!isfinite(value) || value <= 0.0) {
    return FALSE;
  }
  *out = value;
  return TRUE;
}

void u_currency_rates_free(gpointer data) {
  UCurrencyRates *rates = data;
  if (!rates) {
    return;
  }
  g_free(rates->base);
  g_free(rates->date);
  if (rates->rates) {
    g_hash_table_destroy(rates->rates);
  }
  g_free(rates);
}

UCurrencyRates *u_currency_parse_rates(const char *raw,
                                       const char *expected_base) {
  if (!raw || !*raw || !g_utf8_validate(raw, -1, NULL)) {
    return NULL;
  }
  JsonParser *parser = json_parser_new();
  GError *error = NULL;
  if (!json_parser_load_from_data(parser, raw, -1, &error)) {
    g_clear_error(&error);
    g_object_unref(parser);
    return NULL;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
    g_object_unref(parser);
    return NULL;
  }
  JsonArray *array = json_node_get_array(root);
  if (!array || json_array_get_length(array) == 0) {
    g_object_unref(parser);
    return NULL;
  }
  UCurrencyRates *result = g_new0(UCurrencyRates, 1);
  result->rates =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  gboolean valid = TRUE;
  for (guint i = 0; i < json_array_get_length(array) && valid; i++) {
    JsonNode *row_node = json_array_get_element(array, i);
    if (!row_node || !JSON_NODE_HOLDS_OBJECT(row_node)) {
      valid = FALSE;
      break;
    }
    JsonObject *row = json_node_get_object(row_node);
    const char *base = object_string(row, "base");
    const char *quote = object_string(row, "quote");
    const char *date = object_string(row, "date");
    double rate;
    if (!base || !quote || !date || !code_shape_valid(base) ||
        !code_shape_valid(quote) || !object_number(row, "rate", &rate) ||
        (expected_base && g_ascii_strcasecmp(base, expected_base) != 0)) {
      valid = FALSE;
      break;
    }
    if (!result->base) {
      result->base = g_ascii_strup(base, -1);
      result->date = g_strdup(date);
    } else if (g_ascii_strcasecmp(result->base, base) != 0 ||
               g_strcmp0(result->date, date) != 0) {
      valid = FALSE;
      break;
    }
    char *quote_upper = g_ascii_strup(quote, -1);
    if (g_hash_table_contains(result->rates, quote_upper)) {
      g_free(quote_upper);
      valid = FALSE;
      break;
    }
    double *stored = g_new(double, 1);
    *stored = rate;
    g_hash_table_insert(result->rates, quote_upper, stored);
  }
  g_object_unref(parser);
  if (!valid || !result->base || g_hash_table_size(result->rates) == 0) {
    u_currency_rates_free(result);
    return NULL;
  }
  return result;
}

gboolean u_currency_rate(const UCurrencyRates *rates, const char *quote,
                         double *rate) {
  if (!rates || !rates->rates || !quote) {
    return FALSE;
  }
  char upper[4];
  if (!code_shape_valid(quote)) {
    return FALSE;
  }
  for (int i = 0; i < 3; i++) {
    upper[i] = g_ascii_toupper(quote[i]);
  }
  upper[3] = '\0';
  double *stored = g_hash_table_lookup(rates->rates, upper);
  if (!stored) {
    return FALSE;
  }
  if (rate) {
    *rate = *stored;
  }
  return TRUE;
}

char *u_currency_format_amount(double amount, int minor_units) {
  if (!isfinite(amount) || minor_units < 0 || minor_units > 3) {
    return NULL;
  }
  if (amount == 0.0) {
    amount = 0.0;
  }
  char format[8];
  g_snprintf(format, sizeof(format), "%%.%df", minor_units);
  char buffer[512];
  g_ascii_formatd(buffer, sizeof(buffer), format, amount);
  return g_strdup(buffer);
}

static void currency_cache_entry_free(gpointer data) {
  u_currency_rates_free(data);
}

void u_currency_cache_init(UCurrencyCache *cache, int ttl_seconds,
                           guint capacity) {
  cache->ttl_seconds = ttl_seconds > 0 ? ttl_seconds : 21600;
  cache->capacity = capacity > 0 ? capacity : 8;
  cache->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                         currency_cache_entry_free);
}

void u_currency_cache_clear(UCurrencyCache *cache) {
  if (cache && cache->entries) {
    g_hash_table_destroy(cache->entries);
    cache->entries = NULL;
  }
}

UCurrencyRates *u_currency_cache_get(UCurrencyCache *cache, const char *base,
                                     gint64 now, gboolean *fresh) {
  if (fresh) {
    *fresh = FALSE;
  }
  if (!cache || !cache->entries || !base) {
    return NULL;
  }
  char *upper = g_ascii_strup(base, -1);
  UCurrencyRates *rates = g_hash_table_lookup(cache->entries, upper);
  g_free(upper);
  if (!rates) {
    return NULL;
  }
  if (fresh) {
    *fresh = now >= rates->fetched_at &&
             now - rates->fetched_at <
                 (gint64)cache->ttl_seconds * G_USEC_PER_SEC;
  }
  return rates;
}

void u_currency_cache_put(UCurrencyCache *cache, UCurrencyRates *rates,
                          gint64 now) {
  if (!cache || !cache->entries || !rates || !rates->base) {
    u_currency_rates_free(rates);
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
      UCurrencyRates *entry = value;
      if (entry->fetched_at < oldest_time) {
        oldest_time = entry->fetched_at;
        oldest_key = key;
      }
    }
    if (oldest_key) {
      g_hash_table_remove(cache->entries, oldest_key);
    }
  }
  rates->fetched_at = now;
  g_hash_table_replace(cache->entries, g_strdup(rates->base), rates);
}

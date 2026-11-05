#include "currency.h"

#include <glib.h>
#include <math.h>

static void test_currency_query(void) {
  UCurrencyQuery query;
  g_assert_true(u_currency_parse_query("300 eur to USD", &query));
  g_assert_cmpfloat_with_epsilon(query.amount, 300.0, 1e-12);
  g_assert_cmpstr(query.base, ==, "EUR");
  g_assert_cmpstr(query.quote, ==, "USD");

  g_assert_true(u_currency_parse_query("100 USD to EUR", &query));
  g_assert_cmpstr(query.base, ==, "USD");
  g_assert_cmpstr(query.quote, ==, "EUR");

  g_assert_true(u_currency_parse_query("50 gbp in eur", &query));
  g_assert_cmpstr(query.base, ==, "GBP");
  g_assert_cmpstr(query.quote, ==, "EUR");

  g_assert_true(u_currency_parse_query("1000 JPY to USD", &query));
  g_assert_cmpstr(query.base, ==, "JPY");
  g_assert_cmpstr(query.quote, ==, "USD");

  g_assert_true(u_currency_parse_query("25 usd in eur", &query));
  g_assert_true(u_currency_parse_query("500usd to eur", &query));
  g_assert_cmpstr(query.base, ==, "USD");
  g_assert_cmpstr(query.quote, ==, "EUR");

  g_assert_true(u_currency_parse_query("100.50 EUR to USD", &query));
  g_assert_cmpfloat_with_epsilon(query.amount, 100.5, 1e-12);
  g_assert_true(u_currency_parse_query("1e3 usd to eur", &query));
  g_assert_cmpfloat_with_epsilon(query.amount, 1000.0, 1e-12);
  g_assert_true(u_currency_parse_query("0 usd to eur", &query));
  g_assert_cmpfloat_with_epsilon(query.amount, 0.0, 1e-12);
  g_assert_true(u_currency_parse_query("-100 usd to eur", &query));
  g_assert_cmpfloat_with_epsilon(query.amount, -100.0, 1e-12);
  g_assert_true(u_currency_parse_query("300 USD to USD", &query));

  g_assert_false(u_currency_parse_query("300 XYZ to USD", &query));
  g_assert_false(u_currency_parse_query("300 USD to XYZ", &query));
  g_assert_false(u_currency_parse_query("300 USD to", &query));
  g_assert_false(u_currency_parse_query("300 USD to EUR extra", &query));
  g_assert_false(u_currency_parse_query("300 USD", &query));
  g_assert_false(u_currency_parse_query("100/4", &query));
  g_assert_false(u_currency_parse_query("USD to EUR", &query));
  g_assert_false(u_currency_parse_query("300 $ to EUR", &query));
  g_assert_false(u_currency_parse_query("", &query));
  g_assert_false(u_currency_parse_query(NULL, &query));
}

static void test_currency_metadata(void) {
  const UCurrencyInfo *usd = u_currency_lookup("usd");
  g_assert_nonnull(usd);
  g_assert_cmpstr(usd->code, ==, "USD");
  g_assert_cmpint(usd->minor_units, ==, 2);
  g_assert_cmpint(u_currency_minor_units("JPY"), ==, 0);
  g_assert_cmpint(u_currency_minor_units("BHD"), ==, 3);
  g_assert_cmpint(u_currency_minor_units("KWD"), ==, 3);
  g_assert_cmpint(u_currency_minor_units("XYZ"), ==, -1);
  g_assert_null(u_currency_lookup("US"));
  g_assert_null(u_currency_lookup("USDD"));
  g_assert_null(u_currency_lookup(""));

  char *url = u_currency_rates_url("EUR");
  g_assert_cmpstr(url, ==,
                  "https://api.frankfurter.dev/v2/rates?base=EUR");
  g_free(url);
}

static const char *valid_rates =
    "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"GBP\","
    "\"rate\":0.83519},"
    "{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
    "\"rate\":1.1251},"
    "{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"JPY\","
    "\"rate\":178.12}]";

static void test_currency_parsing(void) {
  UCurrencyRates *rates = u_currency_parse_rates(valid_rates, "EUR");
  g_assert_nonnull(rates);
  g_assert_cmpstr(rates->base, ==, "EUR");
  g_assert_cmpstr(rates->date, ==, "2026-11-05");
  double rate = 0;
  g_assert_true(u_currency_rate(rates, "USD", &rate));
  g_assert_cmpfloat_with_epsilon(rate, 1.1251, 1e-12);
  g_assert_true(u_currency_rate(rates, "jpy", &rate));
  g_assert_cmpfloat_with_epsilon(rate, 178.12, 1e-12);
  g_assert_false(u_currency_rate(rates, "CHF", &rate));
  g_assert_false(u_currency_rate(rates, "USDX", &rate));
  u_currency_rates_free(rates);

  rates = u_currency_parse_rates(valid_rates, "usd");
  g_assert_null(rates);
  rates = u_currency_parse_rates("[]", "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates("{}", "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates("not json", "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":0}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":-1}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":\"1.1\"}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\"}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":1.1},{\"date\":\"2026-11-05\",\"base\":\"GBP\","
      "\"quote\":\"EUR\",\"rate\":1.2}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"rate\":1.1}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"quote\":\"USD\",\"rate\":1.1}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":1e999}]",
      "EUR");
  g_assert_null(rates);
  rates = u_currency_parse_rates("[\"not an object\"]", "EUR");
  g_assert_null(rates);
  u_currency_rates_free(NULL);
}

static void test_currency_format(void) {
  char *formatted = u_currency_format_amount(351.42, 2);
  g_assert_cmpstr(formatted, ==, "351.42");
  g_free(formatted);
  formatted = u_currency_format_amount(351.4, 2);
  g_assert_cmpstr(formatted, ==, "351.40");
  g_free(formatted);
  formatted = u_currency_format_amount(337.0, 0);
  g_assert_cmpstr(formatted, ==, "337");
  g_free(formatted);
  formatted = u_currency_format_amount(1.2346, 3);
  g_assert_cmpstr(formatted, ==, "1.235");
  g_free(formatted);
  formatted = u_currency_format_amount(-1.0, 2);
  g_assert_cmpstr(formatted, ==, "-1.00");
  g_free(formatted);
  formatted = u_currency_format_amount(0.0, 2);
  g_assert_cmpstr(formatted, ==, "0.00");
  g_free(formatted);
  formatted = u_currency_format_amount(0.01, 2);
  g_assert_cmpstr(formatted, ==, "0.01");
  g_free(formatted);
  formatted = u_currency_format_amount(123456789.99, 2);
  g_assert_cmpstr(formatted, ==, "123456789.99");
  g_free(formatted);
  g_assert_null(u_currency_format_amount(NAN, 2));
  g_assert_null(u_currency_format_amount(1.0, -1));
  g_assert_null(u_currency_format_amount(1.0, 4));
}

static void test_currency_cache(void) {
  UCurrencyCache cache;
  u_currency_cache_init(&cache, 100, 2);
  gint64 now = 1000 * G_USEC_PER_SEC;
  UCurrencyRates *rates = u_currency_parse_rates(valid_rates, "EUR");
  g_assert_nonnull(rates);
  u_currency_cache_put(&cache, rates, now);

  gboolean fresh = FALSE;
  double rate = 0;
  UCurrencyRates *cached =
      u_currency_cache_get(&cache, "eur", now + 50 * G_USEC_PER_SEC, &fresh);
  g_assert_nonnull(cached);
  g_assert_true(fresh);
  cached = u_currency_cache_get(&cache, "EUR", now + 200 * G_USEC_PER_SEC,
                                &fresh);
  g_assert_nonnull(cached);
  g_assert_false(fresh);
  UCurrencyRates *refreshed = u_currency_parse_rates(
      "[{\"date\":\"2026-11-06\",\"base\":\"EUR\",\"quote\":\"USD\","
      "\"rate\":2.0}]",
      "EUR");
  g_assert_nonnull(refreshed);
  u_currency_cache_put(&cache, refreshed, now + 201 * G_USEC_PER_SEC);
  cached = u_currency_cache_get(&cache, "EUR", now + 202 * G_USEC_PER_SEC,
                                &fresh);
  g_assert_nonnull(cached);
  g_assert_true(fresh);
  g_assert_true(u_currency_rate(cached, "USD", &rate));
  g_assert_cmpfloat_with_epsilon(rate, 2.0, 1e-12);
  g_assert_null(u_currency_cache_get(&cache, "USD", now, &fresh));
  g_assert_false(fresh);

  UCurrencyRates *usd_rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"USD\",\"quote\":\"EUR\","
      "\"rate\":0.9}]",
      "USD");
  UCurrencyRates *gbp_rates = u_currency_parse_rates(
      "[{\"date\":\"2026-11-05\",\"base\":\"GBP\",\"quote\":\"EUR\","
      "\"rate\":1.2}]",
      "GBP");
  g_assert_nonnull(usd_rates);
  g_assert_nonnull(gbp_rates);
  u_currency_cache_put(&cache, usd_rates, now + 202 * G_USEC_PER_SEC);
  u_currency_cache_put(&cache, gbp_rates, now + 203 * G_USEC_PER_SEC);
  g_assert_null(u_currency_cache_get(&cache, "EUR", now + 204 * G_USEC_PER_SEC,
                                     NULL));
  g_assert_nonnull(
      u_currency_cache_get(&cache, "USD", now + 204 * G_USEC_PER_SEC, NULL));
  g_assert_nonnull(
      u_currency_cache_get(&cache, "GBP", now + 204 * G_USEC_PER_SEC, NULL));
  u_currency_cache_clear(&cache);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/currency/query", test_currency_query);
  g_test_add_func("/currency/metadata", test_currency_metadata);
  g_test_add_func("/currency/parsing", test_currency_parsing);
  g_test_add_func("/currency/format", test_currency_format);
  g_test_add_func("/currency/cache", test_currency_cache);
  return g_test_run();
}

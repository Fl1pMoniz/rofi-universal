#include "calculator.h"

#include <glib.h>
#include <math.h>

static void assert_eval(const char *expression, double expected) {
  double value = 0;
  UCalcStatus status = u_calc_evaluate(expression, &value);
  g_assert_cmpint(status, ==, U_CALC_OK);
  g_assert_cmpfloat_with_epsilon(value, expected, 1e-12);
}

static void assert_format(const char *expression, const char *expected) {
  double value = 0;
  g_assert_cmpint(u_calc_evaluate(expression, &value), ==, U_CALC_OK);
  char *formatted = u_calc_format(value);
  g_assert_nonnull(formatted);
  g_assert_cmpstr(formatted, ==, expected);
  g_free(formatted);
}

static void assert_fails(const char *expression, UCalcStatus expected) {
  double value = 0;
  g_assert_cmpint(u_calc_evaluate(expression, &value), ==, expected);
}

static void test_arithmetic(void) {
  assert_eval("25 * 18", 450);
  assert_eval("100 / 4", 25);
  assert_eval("25 + 15", 40);
  assert_eval("25 - 15", 10);
  assert_eval("10 % 3", 1);
  assert_eval("2^10", 1024);
  assert_eval("(25 + 15) * 2", 80);
  assert_eval("((2 + 3) * (4 + 5))", 45);
  assert_eval("2^3^2", 512);
  assert_eval("-2^2", -4);
  assert_eval("(-2)^2", 4);
  assert_eval("2^-2", 0.25);
  assert_eval("+5", 5);
  assert_eval("--5", 5);
  assert_eval("2.5 * 4", 10);
  assert_eval(".5 + .25", 0.75);
  assert_eval("1e3", 1000);
  assert_eval("1.5e-2", 0.015);
  assert_eval("1e+2", 100);
  assert_eval("25%", 0.25);
  assert_eval("25% of 180", 45);
  assert_eval("10% of (100 + 80)", 18);
  assert_eval("25% + 5", 5.25);
  assert_eval("0xff", 255);
  assert_eval("0b1010", 10);
  assert_eval("0o755", 493);
}

static void test_functions(void) {
  assert_eval("sqrt(144)", 12);
  assert_eval("abs(-5)", 5);
  assert_eval("sin(0)", 0);
  assert_eval("cos(0)", 1);
  assert_eval("tan(0)", 0);
  assert_eval("asin(0)", 0);
  assert_eval("acos(1)", 0);
  assert_eval("atan(0)", 0);
  assert_eval("log(1)", 0);
  assert_eval("log10(1000)", 3);
  assert_eval("ln(1)", 0);
  assert_eval("exp(0)", 1);
  assert_eval("floor(2.9)", 2);
  assert_eval("ceil(2.1)", 3);
  assert_eval("round(2.6)", 3);
  assert_eval("round(-1.5)", -2);
  double value = 0;
  g_assert_cmpint(u_calc_evaluate("pi", &value), ==, U_CALC_OK);
  g_assert_cmpfloat_with_epsilon(value, 3.14159265358979323846, 1e-12);
  g_assert_cmpint(u_calc_evaluate("e", &value), ==, U_CALC_OK);
  g_assert_cmpfloat_with_epsilon(value, 2.71828182845904523536, 1e-12);
  g_assert_cmpint(u_calc_evaluate("log(e)", &value), ==, U_CALC_OK);
  g_assert_cmpfloat_with_epsilon(value, 1.0, 1e-12);
  g_assert_cmpint(u_calc_evaluate("sin(pi/2)", &value), ==, U_CALC_OK);
  g_assert_cmpfloat_with_epsilon(value, 1.0, 1e-12);
}

static void test_formatting(void) {
  assert_format("450", "450");
  assert_format("25", "25");
  assert_format("1.0/3", "0.333333333333333");
  assert_format("0.1+0.2", "0.3");
  assert_format("2.5", "2.5");
  assert_format("-4", "-4");
  double value = 0;
  g_assert_cmpint(u_calc_evaluate("0", &value), ==, U_CALC_OK);
  char *formatted = u_calc_format(value);
  g_assert_cmpstr(formatted, ==, "0");
  g_free(formatted);
  g_assert_cmpint(u_calc_evaluate("1e308*10", &value), ==, U_CALC_NON_FINITE);
  g_assert_null(u_calc_format(INFINITY));
  g_assert_null(u_calc_format(NAN));
}

static void test_errors(void) {
  assert_fails("", U_CALC_EMPTY);
  assert_fails("   ", U_CALC_EMPTY);
  assert_fails("2+", U_CALC_INVALID);
  assert_fails("(2+3", U_CALC_INVALID);
  assert_fails("2+3)", U_CALC_INVALID);
  assert_fails("2 3", U_CALC_INVALID);
  assert_fails("2(3)", U_CALC_INVALID);
  assert_fails("foo", U_CALC_INVALID);
  assert_fails("sqrt 4", U_CALC_INVALID);
  assert_fails("sqrt()", U_CALC_INVALID);
  assert_fails("1/0", U_CALC_DIVIDE_BY_ZERO);
  assert_fails("1%0", U_CALC_DIVIDE_BY_ZERO);
  assert_fails("sqrt(-1)", U_CALC_DOMAIN);
  assert_fails("log(0)", U_CALC_DOMAIN);
  assert_fails("log10(-1)", U_CALC_DOMAIN);
  assert_fails("asin(2)", U_CALC_DOMAIN);
  assert_fails("acos(-2)", U_CALC_DOMAIN);
  assert_fails("1e308*10", U_CALC_NON_FINITE);
  assert_fails("0^-1", U_CALC_DOMAIN);
  assert_fails("0x", U_CALC_INVALID);
  assert_fails("0b2", U_CALC_INVALID);
  assert_fails("0o8", U_CALC_INVALID);
  assert_fails("0xFFFFFFFFFFFFFFFFF", U_CALC_INVALID);
  assert_fails("50% off", U_CALC_INVALID);
}

static void test_limits(void) {
  char *too_long = g_malloc0(514);
  memset(too_long, '1', 513);
  assert_fails(too_long, U_CALC_TOO_COMPLEX);
  g_free(too_long);

  GString *too_many_tokens = g_string_new(NULL);
  for (int i = 0; i < 200; i++) {
    g_string_append(too_many_tokens, "1+");
  }
  g_string_append_c(too_many_tokens, '1');
  assert_fails(too_many_tokens->str, U_CALC_TOO_COMPLEX);
  g_string_free(too_many_tokens, TRUE);

  GString *too_deep = g_string_new(NULL);
  for (int i = 0; i < 80; i++) {
    g_string_append_c(too_deep, '(');
  }
  g_string_append_c(too_deep, '1');
  for (int i = 0; i < 80; i++) {
    g_string_append_c(too_deep, ')');
  }
  assert_fails(too_deep->str, U_CALC_TOO_COMPLEX);
  g_string_free(too_deep, TRUE);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/calculator/arithmetic", test_arithmetic);
  g_test_add_func("/calculator/functions", test_functions);
  g_test_add_func("/calculator/formatting", test_formatting);
  g_test_add_func("/calculator/errors", test_errors);
  g_test_add_func("/calculator/limits", test_limits);
  return g_test_run();
}

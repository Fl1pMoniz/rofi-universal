#ifndef ROFI_UNIVERSAL_CALCULATOR_H
#define ROFI_UNIVERSAL_CALCULATOR_H

#include <glib.h>

typedef enum {
  U_CALC_OK = 0,
  U_CALC_EMPTY,
  U_CALC_INVALID,
  U_CALC_DIVIDE_BY_ZERO,
  U_CALC_DOMAIN,
  U_CALC_NON_FINITE,
  U_CALC_TOO_COMPLEX
} UCalcStatus;

UCalcStatus u_calc_evaluate(const char *expression, double *value);
char *u_calc_format(double value);

#endif

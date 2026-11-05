#include "calculator.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define CALC_MAX_EXPRESSION 512
#define CALC_MAX_TOKENS 256
#define CALC_MAX_DEPTH 64

typedef enum {
  TOK_END,
  TOK_NUMBER,
  TOK_PLUS,
  TOK_MINUS,
  TOK_STAR,
  TOK_SLASH,
  TOK_PERCENT,
  TOK_CARET,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_IDENT,
  TOK_INVALID
} TokenType;

typedef struct {
  TokenType type;
  double number;
  const char *start;
  size_t length;
} Token;

typedef struct {
  const char *cursor;
  Token tokens[CALC_MAX_TOKENS];
  int count;
  UCalcStatus status;
} Lexer;

typedef enum {
  FUNC_SQRT,
  FUNC_ABS,
  FUNC_SIN,
  FUNC_COS,
  FUNC_TAN,
  FUNC_ASIN,
  FUNC_ACOS,
  FUNC_ATAN,
  FUNC_LOG,
  FUNC_LOG10,
  FUNC_LN,
  FUNC_EXP,
  FUNC_FLOOR,
  FUNC_CEIL,
  FUNC_ROUND
} FunctionId;

typedef struct {
  Token *tokens;
  int count;
  int position;
  int depth;
  UCalcStatus status;
} Parser;

static gboolean ident_equals(const Token *token, const char *text) {
  size_t length = strlen(text);
  return token->type == TOK_IDENT && token->length == length &&
         g_ascii_strncasecmp(token->start, text, length) == 0;
}

static gboolean lex_number(Lexer *lexer, Token *token) {
  const char *start = lexer->cursor;
  if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X' ||
                          start[1] == 'b' || start[1] == 'B' ||
                          start[1] == 'o' || start[1] == 'O')) {
    guint base = 10;
    if (start[1] == 'x' || start[1] == 'X') {
      base = 16;
    } else if (start[1] == 'b' || start[1] == 'B') {
      base = 2;
    } else {
      base = 8;
    }
    const char *digits = start + 2;
    if (!*digits) {
      return FALSE;
    }
    guint64 value = 0;
    const char *p = digits;
    for (;; p++) {
      guint digit;
      if (*p >= '0' && *p <= '9') {
        digit = (guint)(*p - '0');
      } else if (*p >= 'a' && *p <= 'f') {
        digit = (guint)(*p - 'a' + 10);
      } else if (*p >= 'A' && *p <= 'F') {
        digit = (guint)(*p - 'A' + 10);
      } else {
        break;
      }
      if (digit >= base) {
        return FALSE;
      }
      if (value > (G_MAXUINT64 - digit) / base) {
        return FALSE;
      }
      value = value * base + digit;
    }
    if (p == digits) {
      return FALSE;
    }
    token->type = TOK_NUMBER;
    token->number = (double)value;
    lexer->cursor = p;
    return TRUE;
  }

  char *end = NULL;
  double value = g_ascii_strtod(start, &end);
  if (end == start || !isfinite(value)) {
    return FALSE;
  }
  token->type = TOK_NUMBER;
  token->number = value;
  lexer->cursor = end;
  return TRUE;
}

static gboolean lex_all(const char *expression, Lexer *lexer) {
  lexer->cursor = expression;
  lexer->count = 0;
  lexer->status = U_CALC_OK;
  while (*lexer->cursor) {
    if (g_ascii_isspace(*lexer->cursor)) {
      lexer->cursor++;
      continue;
    }
    if (lexer->count >= CALC_MAX_TOKENS - 1) {
      lexer->status = U_CALC_TOO_COMPLEX;
      return FALSE;
    }
    Token *token = &lexer->tokens[lexer->count];
    memset(token, 0, sizeof(*token));
    token->start = lexer->cursor;
    char c = *lexer->cursor;
    if (g_ascii_isdigit(c) || c == '.') {
      if (!lex_number(lexer, token)) {
        lexer->status = U_CALC_INVALID;
        return FALSE;
      }
      token->length = (size_t)(lexer->cursor - token->start);
    } else if (g_ascii_isalpha(c) || c == '_') {
      token->type = TOK_IDENT;
      const char *start = lexer->cursor;
      while (g_ascii_isalnum(*lexer->cursor) || *lexer->cursor == '_') {
        lexer->cursor++;
      }
      token->length = (size_t)(lexer->cursor - start);
    } else {
      lexer->cursor++;
      switch (c) {
      case '+':
        token->type = TOK_PLUS;
        break;
      case '-':
        token->type = TOK_MINUS;
        break;
      case '*':
        token->type = TOK_STAR;
        break;
      case '/':
        token->type = TOK_SLASH;
        break;
      case '%':
        token->type = TOK_PERCENT;
        break;
      case '^':
        token->type = TOK_CARET;
        break;
      case '(':
        token->type = TOK_LPAREN;
        break;
      case ')':
        token->type = TOK_RPAREN;
        break;
      default:
        token->type = TOK_INVALID;
        lexer->status = U_CALC_INVALID;
        return FALSE;
      }
      token->length = 1;
    }
    lexer->count++;
  }
  lexer->tokens[lexer->count].type = TOK_END;
  lexer->tokens[lexer->count].length = 0;
  return TRUE;
}

static gboolean parser_fail(Parser *parser, UCalcStatus status) {
  if (parser->status == U_CALC_OK) {
    parser->status = status;
  }
  return FALSE;
}

static gboolean parse_expression(Parser *parser, double *value);
static gboolean parse_unary(Parser *parser, double *value);

static gboolean function_from_token(const Token *token, FunctionId *id) {
  static const struct {
    const char *name;
    FunctionId id;
  } functions[] = {
      {"sqrt", FUNC_SQRT},   {"abs", FUNC_ABS},   {"sin", FUNC_SIN},
      {"cos", FUNC_COS},     {"tan", FUNC_TAN},   {"asin", FUNC_ASIN},
      {"acos", FUNC_ACOS},   {"atan", FUNC_ATAN}, {"log", FUNC_LOG},
      {"log10", FUNC_LOG10}, {"ln", FUNC_LN},     {"exp", FUNC_EXP},
      {"floor", FUNC_FLOOR}, {"ceil", FUNC_CEIL}, {"round", FUNC_ROUND},
      {NULL, 0},
  };
  for (int i = 0; functions[i].name; i++) {
    if (ident_equals(token, functions[i].name)) {
      *id = functions[i].id;
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean apply_function(Parser *parser, FunctionId id, double argument,
                               double *value) {
  double result;
  if ((id == FUNC_ASIN || id == FUNC_ACOS) &&
      (argument < -1.0 || argument > 1.0)) {
    return parser_fail(parser, U_CALC_DOMAIN);
  }
  if ((id == FUNC_SQRT && argument < 0.0) ||
      (id == FUNC_LOG && argument <= 0.0) ||
      (id == FUNC_LOG10 && argument <= 0.0) ||
      (id == FUNC_LN && argument <= 0.0)) {
    return parser_fail(parser, U_CALC_DOMAIN);
  }
  switch (id) {
  case FUNC_SQRT:
    result = sqrt(argument);
    break;
  case FUNC_ABS:
    result = fabs(argument);
    break;
  case FUNC_SIN:
    result = sin(argument);
    break;
  case FUNC_COS:
    result = cos(argument);
    break;
  case FUNC_TAN:
    result = tan(argument);
    break;
  case FUNC_ASIN:
    result = asin(argument);
    break;
  case FUNC_ACOS:
    result = acos(argument);
    break;
  case FUNC_ATAN:
    result = atan(argument);
    break;
  case FUNC_LOG:
  case FUNC_LN:
    result = log(argument);
    break;
  case FUNC_LOG10:
    result = log10(argument);
    break;
  case FUNC_EXP:
    result = exp(argument);
    break;
  case FUNC_FLOOR:
    result = floor(argument);
    break;
  case FUNC_CEIL:
    result = ceil(argument);
    break;
  case FUNC_ROUND:
    result = round(argument);
    break;
  default:
    return parser_fail(parser, U_CALC_INVALID);
  }
  if (isnan(result)) {
    return parser_fail(parser, U_CALC_DOMAIN);
  }
  if (!isfinite(result)) {
    return parser_fail(parser, U_CALC_NON_FINITE);
  }
  *value = result;
  return TRUE;
}

static gboolean parse_primary(Parser *parser, double *value) {
  Token *token = &parser->tokens[parser->position];
  if (token->type == TOK_NUMBER) {
    parser->position++;
    *value = token->number;
    return TRUE;
  }
  if (token->type == TOK_LPAREN) {
    if (++parser->depth > CALC_MAX_DEPTH) {
      return parser_fail(parser, U_CALC_TOO_COMPLEX);
    }
    parser->position++;
    if (!parse_expression(parser, value)) {
      return FALSE;
    }
    if (parser->tokens[parser->position].type != TOK_RPAREN) {
      return parser_fail(parser, U_CALC_INVALID);
    }
    parser->position++;
    parser->depth--;
    return TRUE;
  }
  if (token->type == TOK_IDENT) {
    if (ident_equals(token, "pi")) {
      parser->position++;
      *value = 3.14159265358979323846;
      return TRUE;
    }
    if (ident_equals(token, "e")) {
      parser->position++;
      *value = 2.71828182845904523536;
      return TRUE;
    }
    FunctionId function;
    if (!function_from_token(token, &function)) {
      return parser_fail(parser, U_CALC_INVALID);
    }
    parser->position++;
    if (parser->tokens[parser->position].type != TOK_LPAREN) {
      return parser_fail(parser, U_CALC_INVALID);
    }
    if (++parser->depth > CALC_MAX_DEPTH) {
      return parser_fail(parser, U_CALC_TOO_COMPLEX);
    }
    parser->position++;
    double argument;
    if (!parse_expression(parser, &argument)) {
      return FALSE;
    }
    if (parser->tokens[parser->position].type != TOK_RPAREN) {
      return parser_fail(parser, U_CALC_INVALID);
    }
    parser->position++;
    parser->depth--;
    return apply_function(parser, function, argument, value);
  }
  return parser_fail(parser, U_CALC_INVALID);
}

static gboolean parse_power(Parser *parser, double *value) {
  if (!parse_primary(parser, value)) {
    return FALSE;
  }
  if (parser->tokens[parser->position].type == TOK_CARET) {
    parser->position++;
    double exponent;
    if (!parse_unary(parser, &exponent)) {
      return FALSE;
    }
    if (*value == 0.0 && exponent < 0.0) {
      return parser_fail(parser, U_CALC_DOMAIN);
    }
    double result = pow(*value, exponent);
    if (isnan(result)) {
      return parser_fail(parser, U_CALC_DOMAIN);
    }
    if (!isfinite(result)) {
      return parser_fail(parser, U_CALC_NON_FINITE);
    }
    *value = result;
  }
  return TRUE;
}

static gboolean parse_unary(Parser *parser, double *value) {
  TokenType type = parser->tokens[parser->position].type;
  if (type == TOK_PLUS || type == TOK_MINUS) {
    parser->position++;
    if (!parse_unary(parser, value)) {
      return FALSE;
    }
    if (type == TOK_MINUS) {
      *value = -*value;
    }
    return TRUE;
  }
  return parse_power(parser, value);
}

static gboolean parse_term(Parser *parser, double *value) {
  if (!parse_unary(parser, value)) {
    return FALSE;
  }
  for (;;) {
    TokenType type = parser->tokens[parser->position].type;
    if (type != TOK_STAR && type != TOK_SLASH && type != TOK_PERCENT) {
      return TRUE;
    }
    parser->position++;
    if (type == TOK_PERCENT &&
        ident_equals(&parser->tokens[parser->position], "of")) {
      parser->position++;
      double rhs;
      if (!parse_unary(parser, &rhs)) {
        return FALSE;
      }
      double result = (*value / 100.0) * rhs;
      if (!isfinite(result)) {
        return parser_fail(parser, U_CALC_NON_FINITE);
      }
      *value = result;
      continue;
    }
    if (type == TOK_PERCENT) {
      TokenType next = parser->tokens[parser->position].type;
      if (next == TOK_END || next == TOK_RPAREN || next == TOK_PLUS ||
          next == TOK_MINUS || next == TOK_STAR || next == TOK_SLASH ||
          next == TOK_CARET) {
        *value = *value / 100.0;
        continue;
      }
    }
    double rhs;
    if (!parse_unary(parser, &rhs)) {
      return FALSE;
    }
    if (type == TOK_SLASH) {
      if (rhs == 0.0) {
        return parser_fail(parser, U_CALC_DIVIDE_BY_ZERO);
      }
      *value = *value / rhs;
    } else if (type == TOK_PERCENT) {
      if (rhs == 0.0) {
        return parser_fail(parser, U_CALC_DIVIDE_BY_ZERO);
      }
      *value = fmod(*value, rhs);
    } else {
      *value = *value * rhs;
    }
    if (!isfinite(*value)) {
      return parser_fail(parser, U_CALC_NON_FINITE);
    }
  }
}

static gboolean parse_addition(Parser *parser, double *value) {
  if (!parse_term(parser, value)) {
    return FALSE;
  }
  for (;;) {
    TokenType type = parser->tokens[parser->position].type;
    if (type != TOK_PLUS && type != TOK_MINUS) {
      return TRUE;
    }
    parser->position++;
    double rhs;
    if (!parse_term(parser, &rhs)) {
      return FALSE;
    }
    *value = type == TOK_PLUS ? *value + rhs : *value - rhs;
    if (!isfinite(*value)) {
      return parser_fail(parser, U_CALC_NON_FINITE);
    }
  }
}

static gboolean parse_expression(Parser *parser, double *value) {
  return parse_addition(parser, value);
}

UCalcStatus u_calc_evaluate(const char *expression, double *value) {
  if (value) {
    *value = 0.0;
  }
  if (!expression) {
    return U_CALC_EMPTY;
  }
  char *trimmed = g_strdup(expression);
  g_strstrip(trimmed);
  if (!*trimmed) {
    g_free(trimmed);
    return U_CALC_EMPTY;
  }
  if (strlen(trimmed) > CALC_MAX_EXPRESSION) {
    g_free(trimmed);
    return U_CALC_TOO_COMPLEX;
  }
  Lexer lexer;
  if (!lex_all(trimmed, &lexer)) {
    UCalcStatus status = lexer.status;
    g_free(trimmed);
    return status;
  }
  Parser parser = {
      .tokens = lexer.tokens,
      .count = lexer.count,
      .position = 0,
      .depth = 0,
      .status = U_CALC_OK,
  };
  double result = 0.0;
  gboolean ok = parse_expression(&parser, &result);
  if (ok && parser.tokens[parser.position].type != TOK_END) {
    parser_fail(&parser, U_CALC_INVALID);
    ok = FALSE;
  }
  if (!ok) {
    UCalcStatus status = parser.status;
    g_free(trimmed);
    return status == U_CALC_OK ? U_CALC_INVALID : status;
  }
  if (!isfinite(result)) {
    g_free(trimmed);
    return U_CALC_NON_FINITE;
  }
  if (value) {
    *value = result;
  }
  g_free(trimmed);
  return U_CALC_OK;
}

char *u_calc_format(double value) {
  if (!isfinite(value)) {
    return NULL;
  }
  if (value == 0.0) {
    return g_strdup("0");
  }
  char buffer[64];
  g_ascii_formatd(buffer, sizeof(buffer), "%.15g", value);
  return g_strdup(buffer);
}

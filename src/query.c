#include "universal.h"

#include <string.h>

static char *trimmed_copy(const char *text) {
  char *copy = g_strdup(text ? text : "");
  return g_strstrip(copy);
}

static gboolean matches(const char *pattern, const char *text) {
  return g_regex_match_simple(pattern, text, G_REGEX_CASELESS, 0);
}

int u_intent_boost(UIntent intent, UProvider provider) {
  UQueryAnalysis analysis;
  memset(&analysis, 0, sizeof(analysis));
  analysis.intent = intent;
  switch (intent) {
  case U_INTENT_EMPTY:
    analysis.app_score = 1000;
    break;
  case U_INTENT_APP:
    analysis.app_score = 1000;
    analysis.file_score = 250;
    analysis.web_score = 100;
    break;
  case U_INTENT_CALC:
    analysis.calc_score = 1000;
    analysis.web_score = 100;
    break;
  case U_INTENT_WEB:
    analysis.web_score = 1000;
    analysis.app_score = 100;
    break;
  case U_INTENT_FILE:
    analysis.file_score = 1000;
    analysis.web_score = 100;
    break;
  case U_INTENT_DICTIONARY:
    analysis.dictionary_score = 1000;
    analysis.web_score = 100;
    break;
  case U_INTENT_GITHUB:
    analysis.github_score = 1000;
    analysis.web_score = 100;
    break;
  case U_INTENT_WEATHER:
    analysis.weather_score = 1000;
    break;
  case U_INTENT_CLIPBOARD:
    analysis.clipboard_score = 1000;
    break;
  case U_INTENT_URL:
    analysis.web_score = 1000;
    break;
  }
  return u_provider_relevance(&analysis, provider);
}

gboolean u_query_is_url(const char *query) {
  if (!query) {
    return FALSE;
  }
  return g_regex_match_simple("^[A-Za-z][A-Za-z0-9+.-]*://", query, 0, 0) ||
         g_regex_match_simple("^www\\.", query, G_REGEX_CASELESS, 0);
}

gboolean u_query_is_path(const char *query) {
  if (!query || !*query) {
    return FALSE;
  }
  if (g_str_has_prefix(query, "/") || g_str_has_prefix(query, "~/") ||
      g_str_has_prefix(query, "./") || g_str_has_prefix(query, "../") ||
      g_strcmp0(query, "~") == 0) {
    return TRUE;
  }
  if (!strchr(query, '/')) {
    return FALSE;
  }
  return !g_regex_match_simple("^[0-9\\s.()+*/^%=-]+$", query, 0, 0);
}

static int calc_relevance(const char *text) {
  if (!text || !*text) {
    return 0;
  }
  if (text[0] == '=') {
    return 1000;
  }
  if (g_regex_match_simple("^[0-9]+(?:\\.[0-9]+)?$", text, 0, 0)) {
    return 250;
  }
  if (matches("\\b(sqrt|cbrt|sin|cos|tan|asin|acos|atan|log|ln|abs|round|"
              "floor|ceil|factorial)\\s*\\(", text)) {
    return 850;
  }
  if (matches("[0-9]\\s*[+*/^%]\\s*[0-9]", text) ||
      matches("[0-9]\\s*-\\s*[0-9]", text) ||
      matches("[0-9]\\s*(plus|minus|times|divided\\s+by|mod)\\s*[0-9]",
              text) ||
      matches("[0-9]\\s*%", text)) {
    return 900;
  }
  if (matches("[0-9].*\\b(to|in)\\b.*\\b(hex|hexadecimal|binary|bin|"
              "octal|oct|decimal|dec)\\b", text)) {
    return 800;
  }
  if (matches("\\b(pi|e)\\b\\s*[+*/^-]", text) ||
      matches("[+*/^-]\\s*\\b(pi|e)\\b", text) ||
      matches("\\b(now|today|tomorrow|yesterday)\\b\\s*[+-]", text)) {
    return 750;
  }
  gboolean currency =
      matches("\\b(usd|eur|gbp|jpy|cad|aud|chf|cny|inr|brl|rub|krw|sek|nok|"
              "dkk|pln|try|nzd|zar|mxn|sgd|hkd|btc|eth)\\b", text);
  gboolean connector =
      matches("(\\b(to|in|into)\\b|->|→)", text);
  gboolean unit_after_number =
      matches("[0-9]\\s*(km|kilometers?|kilometres?|mi|miles?|m|meters?|"
              "metres?|cm|centimeters?|centimetres?|mm|millimeters?|"
              "millimetres?|inch(?:es)?|ft|feet|kg|kilograms?|lbs?|pounds?|"
              "oz|ounces?|g|grams?|mg|bytes?|kb|mb|gb|tb|kib|mib|gib|tib|"
              "celsius|fahrenheit|kelvin|hours?|minutes?|seconds?|days?|"
              "weeks?|months?|years?|liters?|litres?|gallons?|mph|kmh|kph)\\b",
              text);
  if (currency && connector) {
    return 900;
  }
  if (unit_after_number && connector) {
    return 900;
  }
  if (currency) {
    return 450;
  }
  if (unit_after_number) {
    return 600;
  }
  return 0;
}

static int app_shape_score(const char *folded, int spaces) {
  if (!folded || !*folded) {
    return 0;
  }
  if (spaces == 0) {
    if (g_regex_match_simple("^[\\p{L}\\p{N}_.+-]+$", folded, 0, 0)) {
      return 700;
    }
    return 400;
  }
  if (spaces == 1) {
    return 250;
  }
  return 100;
}

static int count_spaces(const char *text) {
  int spaces = 0;
  for (const char *p = text; p && *p; p++) {
    if (*p == ' ') {
      spaces++;
    }
  }
  return spaces;
}

void u_analyze_query(const char *query, UQueryAnalysis *analysis) {
  memset(analysis, 0, sizeof(*analysis));
  analysis->intent = U_INTENT_EMPTY;
  analysis->explicit_provider = U_PROVIDER_SYSTEM;
  char *text = trimmed_copy(query);
  char *folded = g_utf8_casefold(text, -1);
  if (!*folded) {
    g_free(text);
    g_free(folded);
    return;
  }
  int spaces = count_spaces(folded);
  analysis->app_score = app_shape_score(folded, spaces);
  analysis->web_score = spaces > 0 ? 600 : 300;
  analysis->file_score = matches("\\.[A-Za-z0-9]{1,8}$", folded) ? 600
                          : (spaces == 0 ? 100 : 0);
  analysis->clipboard_score = strlen(folded) >= 3 ? 100 : 0;

  if (u_query_is_url(folded)) {
    analysis->url_like = TRUE;
    analysis->web_score = 1000;
    analysis->file_score = 0;
    analysis->intent = U_INTENT_URL;
    analysis->explicit_provider = U_PROVIDER_WEB;
    goto done;
  }
  if (folded[0] == '!') {
    analysis->web_score = 1100;
    analysis->intent = U_INTENT_WEB;
    analysis->explicit_provider = U_PROVIDER_WEB;
    goto done;
  }
  if (g_str_has_prefix(folded, "github ") || g_str_has_prefix(folded, "gh ")) {
    analysis->github_score = 1100;
    analysis->web_score = 150;
    analysis->intent = U_INTENT_GITHUB;
    analysis->explicit_provider = U_PROVIDER_GITHUB;
    goto done;
  }
  if (g_str_has_prefix(folded, "weather ") ||
      g_str_has_prefix(folded, "weather in ")) {
    analysis->weather_score = 1100;
    analysis->web_score = 150;
    analysis->intent = U_INTENT_WEATHER;
    analysis->explicit_provider = U_PROVIDER_WEATHER;
    goto done;
  }
  if (g_str_has_prefix(folded, "define ") ||
      g_str_has_prefix(folded, "definition ") ||
      g_str_has_prefix(folded, "dict ")) {
    analysis->dictionary_score = 1100;
    analysis->web_score = 150;
    analysis->intent = U_INTENT_DICTIONARY;
    analysis->explicit_provider = U_PROVIDER_DICTIONARY;
    goto done;
  }
  if (g_str_has_prefix(folded, "clipboard ") ||
      g_str_has_prefix(folded, "clip ")) {
    analysis->clipboard_score = 1100;
    analysis->web_score = 0;
    analysis->intent = U_INTENT_CLIPBOARD;
    analysis->explicit_provider = U_PROVIDER_CLIPBOARD;
    goto done;
  }
  if (u_query_is_path(text)) {
    analysis->path_like = TRUE;
    analysis->file_score = 1000;
    analysis->web_score = 100;
    analysis->intent = U_INTENT_FILE;
    analysis->explicit_provider = U_PROVIDER_FILES;
    goto done;
  }
  analysis->calc_score = calc_relevance(folded);
  if (analysis->calc_score >= 600) {
    analysis->web_score = MIN(analysis->web_score, 150);
    analysis->app_score = MIN(analysis->app_score, 150);
    analysis->intent = U_INTENT_CALC;
    goto done;
  }
  if (analysis->app_score >= 500) {
    analysis->intent = U_INTENT_APP;
  } else if (analysis->web_score >= 500) {
    analysis->intent = U_INTENT_WEB;
  } else {
    analysis->intent = U_INTENT_WEB;
  }
done:
  g_free(text);
  g_free(folded);
}

int u_provider_relevance(const UQueryAnalysis *analysis, UProvider provider) {
  if (!analysis) {
    return 0;
  }
  switch (provider) {
  case U_PROVIDER_APPS:
    return analysis->app_score;
  case U_PROVIDER_CALC:
    return analysis->calc_score;
  case U_PROVIDER_WEB:
    return analysis->web_score;
  case U_PROVIDER_FILES:
    return analysis->file_score;
  case U_PROVIDER_CLIPBOARD:
    return analysis->clipboard_score;
  case U_PROVIDER_DICTIONARY:
    return analysis->dictionary_score;
  case U_PROVIDER_GITHUB:
    return analysis->github_score;
  case U_PROVIDER_WEATHER:
    return analysis->weather_score;
  case U_PROVIDER_BOOKMARKS:
    return MAX(analysis->app_score / 2, analysis->web_score / 3);
  default:
    return 0;
  }
}

int u_score_signals(int match, int relevance, gboolean local) {
  return match + relevance + (local ? 40 : 0);
}

gboolean u_query_looks_calculable(const char *query) {
  UQueryAnalysis analysis;
  u_analyze_query(query, &analysis);
  return analysis.calc_score >= 600;
}

UIntent u_classify_intent(const char *query, gboolean *calc_like,
                          gboolean *path_like) {
  UQueryAnalysis analysis;
  u_analyze_query(query, &analysis);
  if (calc_like) {
    *calc_like = analysis.calc_score >= 600;
  }
  if (path_like) {
    *path_like = analysis.path_like;
  }
  return analysis.intent;
}

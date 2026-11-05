#ifndef ROFI_UNIVERSAL_H
#define ROFI_UNIVERSAL_H

#include <glib.h>

typedef enum {
  U_PROVIDER_APPS,
  U_PROVIDER_CALC,
  U_PROVIDER_WEB,
  U_PROVIDER_FILES,
  U_PROVIDER_CLIPBOARD,
  U_PROVIDER_DICTIONARY,
  U_PROVIDER_GITHUB,
  U_PROVIDER_WEATHER,
  U_PROVIDER_BOOKMARKS,
  U_PROVIDER_SYSTEM
} UProvider;

typedef enum {
  U_ACTION_NONE,
  U_ACTION_APP,
  U_ACTION_OPEN_URI,
  U_ACTION_OPEN_PATH,
  U_ACTION_COPY,
  U_ACTION_CLIPBOARD
} UAction;

typedef enum {
  U_GITHUB_REPOS,
  U_GITHUB_USERS,
  U_GITHUB_ISSUES
} UGitHubCategory;

typedef enum {
  U_INTENT_EMPTY,
  U_INTENT_APP,
  U_INTENT_CALC,
  U_INTENT_WEB,
  U_INTENT_FILE,
  U_INTENT_DICTIONARY,
  U_INTENT_GITHUB,
  U_INTENT_WEATHER,
  U_INTENT_CLIPBOARD,
  U_INTENT_URL
} UIntent;

typedef struct {
  char *title;
  char *description;
  char *icon;
  char *copy_text;
  char *payload;
  char *dedup_key;
  UAction action;
  UProvider provider;
  int score;
} UResult;

typedef struct {
  UIntent intent;
  UProvider explicit_provider;
  int calc_score;
  int app_score;
  int web_score;
  int file_score;
  int clipboard_score;
  int dictionary_score;
  int github_score;
  int weather_score;
  gboolean path_like;
  gboolean url_like;
} UQueryAnalysis;

typedef struct {
  gboolean applications;
  gboolean calculator;
  gboolean web;
  gboolean files;
  gboolean clipboard;
  gboolean clipboard_automatic;
  int clipboard_cache_seconds;
  char *weather_location;
  int weather_cache_seconds;
  int weather_timeout_seconds;
  gboolean dictionary;
  int dictionary_cache_seconds;
  int dictionary_timeout_seconds;
  gboolean github;
  gboolean weather;
  gboolean bookmarks;
  int max_results;
  int file_max_depth;
  int file_max_results;
  int clipboard_max_results;
  char *qalc_binary;
  char *web_engine;
  char *open_command;
  char *file_roots;
} UConfig;

UResult *u_result_new(UProvider provider, const char *title,
                      const char *description, const char *icon, int score,
                      UAction action, const char *payload,
                      const char *copy_text);
void u_result_free(gpointer data);
void u_result_set_dedup_key(UResult *result, const char *key);
void u_results_dedup(GPtrArray *results);
int u_result_compare(gconstpointer a, gconstpointer b);
const char *u_provider_name(UProvider provider);
const char *u_provider_icon(UProvider provider);
int u_intent_boost(UIntent intent, UProvider provider);

void u_analyze_query(const char *query, UQueryAnalysis *analysis);
UIntent u_classify_intent(const char *query, gboolean *calc_like,
                          gboolean *path_like);
int u_provider_relevance(const UQueryAnalysis *analysis, UProvider provider);
int u_score_signals(int match, int relevance, gboolean local);
int u_match_score_folded(const char *haystack, const char *title,
                         const char *query);
int u_app_match_score(const char *name, const char *executable,
                      const char *keywords, const char *terms,
                      const char *query);
int u_file_match_score(const char *path, const char *query);
gboolean u_query_has_glob(const char *query);
char *u_dictionary_word_from_query(const char *query,
                                   const char *folded_query);
gboolean u_query_is_url(const char *query);
gboolean u_query_is_path(const char *query);
gboolean u_query_looks_calculable(const char *query);

char *u_url_encode(const char *text);
char *u_search_url(const char *engine, const char *query);
const char *u_web_engine_template(const char *engine);
char *u_github_search_url(const char *query, const char *type);
char *u_github_repo_url(const char *term);
char *u_github_strip_category(const char *term, UGitHubCategory *category);

void u_config_init(UConfig *config);
gboolean u_config_load(UConfig *config, const char *path);
void u_config_clear(UConfig *config);
char *u_expand_path(const char *path);
char *u_default_config_path(void);

#endif

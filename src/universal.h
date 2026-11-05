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
  UAction action;
  UProvider provider;
  int score;
} UResult;

typedef struct {
  gboolean applications;
  gboolean calculator;
  gboolean web;
  gboolean files;
  gboolean clipboard;
  gboolean dictionary;
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
int u_result_compare(gconstpointer a, gconstpointer b);
const char *u_provider_name(UProvider provider);
const char *u_provider_icon(UProvider provider);
int u_intent_boost(UIntent intent, UProvider provider);

UIntent u_classify_intent(const char *query, gboolean *calc_like,
                          gboolean *path_like);
gboolean u_query_is_url(const char *query);
gboolean u_query_is_path(const char *query);
gboolean u_query_looks_calculable(const char *query);

char *u_url_encode(const char *text);
char *u_search_url(const char *engine, const char *query);
const char *u_web_engine_template(const char *engine);

void u_config_init(UConfig *config);
gboolean u_config_load(UConfig *config, const char *path);
void u_config_clear(UConfig *config);
char *u_expand_path(const char *path);
char *u_default_config_path(void);

#endif

#include "universal.h"

#include <string.h>

UResult *u_result_new(UProvider provider, const char *title,
                      const char *description, const char *icon, int score,
                      UAction action, const char *payload,
                      const char *copy_text) {
  UResult *result = g_new0(UResult, 1);
  result->provider = provider;
  result->title = g_strdup(title ? title : "");
  result->description = g_strdup(description ? description : "");
  result->icon = g_strdup(icon ? icon : u_provider_icon(provider));
  result->score = score;
  result->action = action;
  result->payload = g_strdup(payload);
  result->copy_text = g_strdup(copy_text);
  return result;
}

void u_result_free(gpointer data) {
  UResult *result = data;
  if (!result) {
    return;
  }
  g_free(result->title);
  g_free(result->description);
  g_free(result->icon);
  g_free(result->copy_text);
  g_free(result->payload);
  g_free(result);
}

int u_result_compare(gconstpointer a, gconstpointer b) {
  const UResult *left = *(UResult *const *)a;
  const UResult *right = *(UResult *const *)b;
  if (left->score != right->score) {
    return right->score - left->score;
  }
  if (left->provider != right->provider) {
    return (int)left->provider - (int)right->provider;
  }
  return g_utf8_collate(left->title, right->title);
}

const char *u_provider_name(UProvider provider) {
  switch (provider) {
  case U_PROVIDER_APPS:
    return "Application";
  case U_PROVIDER_CALC:
    return "Calculator";
  case U_PROVIDER_WEB:
    return "Web";
  case U_PROVIDER_FILES:
    return "File";
  case U_PROVIDER_CLIPBOARD:
    return "Clipboard";
  case U_PROVIDER_DICTIONARY:
    return "Dictionary";
  case U_PROVIDER_GITHUB:
    return "GitHub";
  case U_PROVIDER_WEATHER:
    return "Weather";
  case U_PROVIDER_BOOKMARKS:
    return "Bookmark";
  default:
    return "System";
  }
}

const char *u_provider_icon(UProvider provider) {
  switch (provider) {
  case U_PROVIDER_APPS:
    return "application-x-executable";
  case U_PROVIDER_CALC:
    return "accessories-calculator";
  case U_PROVIDER_WEB:
    return "applications-internet";
  case U_PROVIDER_FILES:
    return "text-x-generic";
  case U_PROVIDER_CLIPBOARD:
    return "edit-paste";
  case U_PROVIDER_DICTIONARY:
    return "accessories-dictionary";
  case U_PROVIDER_GITHUB:
    return "applications-internet";
  case U_PROVIDER_WEATHER:
    return "weather-clear";
  case U_PROVIDER_BOOKMARKS:
    return "user-bookmarks";
  default:
    return "dialog-information";
  }
}

char *u_url_encode(const char *text) {
  return g_uri_escape_string(text ? text : "", NULL, TRUE);
}

const char *u_web_engine_template(const char *engine) {
  if (!engine || !*engine) {
    return "https://duckduckgo.com/?q=%s";
  }
  if (strstr(engine, "%s") || strstr(engine, "{query}")) {
    return engine;
  }
  if (g_ascii_strcasecmp(engine, "google") == 0 ||
      g_ascii_strcasecmp(engine, "g") == 0) {
    return "https://www.google.com/search?q=%s";
  }
  if (g_ascii_strcasecmp(engine, "duckduckgo") == 0 ||
      g_ascii_strcasecmp(engine, "ddg") == 0) {
    return "https://duckduckgo.com/?q=%s";
  }
  if (g_ascii_strcasecmp(engine, "brave") == 0) {
    return "https://search.brave.com/search?q=%s";
  }
  if (g_ascii_strcasecmp(engine, "bing") == 0) {
    return "https://www.bing.com/search?q=%s";
  }
  if (g_ascii_strcasecmp(engine, "startpage") == 0) {
    return "https://www.startpage.com/sp/search?query=%s";
  }
  if (g_ascii_strcasecmp(engine, "wikipedia") == 0 ||
      g_ascii_strcasecmp(engine, "wiki") == 0 ||
      g_ascii_strcasecmp(engine, "w") == 0) {
    return "https://en.wikipedia.org/w/index.php?search=%s";
  }
  if (g_ascii_strcasecmp(engine, "youtube") == 0 ||
      g_ascii_strcasecmp(engine, "yt") == 0) {
    return "https://www.youtube.com/results?search_query=%s";
  }
  if (g_ascii_strcasecmp(engine, "github") == 0 ||
      g_ascii_strcasecmp(engine, "gh") == 0) {
    return "https://github.com/search?type=repositories&q=%s";
  }
  if (g_ascii_strcasecmp(engine, "aur") == 0) {
    return "https://aur.archlinux.org/packages?K=%s";
  }
  if (g_ascii_strcasecmp(engine, "maps") == 0 ||
      g_ascii_strcasecmp(engine, "openstreetmap") == 0) {
    return "https://www.openstreetmap.org/search?query=%s";
  }
  if (g_ascii_strcasecmp(engine, "dictionary") == 0 ||
      g_ascii_strcasecmp(engine, "define") == 0) {
    return "https://duckduckgo.com/?q=define+%s";
  }
  if (g_ascii_strcasecmp(engine, "wiktionary") == 0) {
    return "https://en.wiktionary.org/w/index.php?search=%s";
  }
  return "https://duckduckgo.com/?q=%s";
}

char *u_search_url(const char *engine, const char *query) {
  const char *template = u_web_engine_template(engine);
  char *encoded = u_url_encode(query);
  char *url;
  if (strstr(template, "{query}")) {
    char **parts = g_strsplit(template, "{query}", -1);
    url = g_strjoinv(encoded, parts);
    g_strfreev(parts);
  } else if (strstr(template, "%s")) {
    url = g_strdup_printf(template, encoded);
  } else {
    url = g_strdup(template);
  }
  g_free(encoded);
  return url;
}

static gboolean key_bool(GKeyFile *file, const char *group, const char *key,
                         gboolean fallback) {
  GError *error = NULL;
  gboolean value = g_key_file_get_boolean(file, group, key, &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

static int key_int(GKeyFile *file, const char *group, const char *key,
                   int fallback) {
  GError *error = NULL;
  int value = g_key_file_get_integer(file, group, key, &error);
  if (error) {
    g_error_free(error);
    return fallback;
  }
  return value;
}

static char *key_string(GKeyFile *file, const char *group, const char *key,
                        const char *fallback) {
  GError *error = NULL;
  char *value = g_key_file_get_string(file, group, key, &error);
  if (error) {
    g_error_free(error);
    return g_strdup(fallback);
  }
  return value;
}

void u_config_init(UConfig *config) {
  memset(config, 0, sizeof(*config));
  config->applications = TRUE;
  config->calculator = TRUE;
  config->web = TRUE;
  config->files = TRUE;
  config->clipboard = TRUE;
  config->clipboard_automatic = FALSE;
  config->clipboard_cache_seconds = 30;
  config->dictionary = TRUE;
  config->github = TRUE;
  config->weather = TRUE;
  config->bookmarks = TRUE;
  config->max_results = 40;
  config->file_max_depth = 4;
  config->file_max_results = 8;
  config->clipboard_max_results = 10;
  config->qalc_binary = g_strdup("qalc");
  config->web_engine = g_strdup("duckduckgo");
  config->open_command = g_strdup("xdg-open");
  config->file_roots = g_strdup("~");
}

gboolean u_config_load(UConfig *config, const char *path) {
  char *own_path = NULL;
  if (!path) {
    own_path = u_default_config_path();
    path = own_path;
  }
  GKeyFile *file = g_key_file_new();
  GError *error = NULL;
  gboolean loaded = g_key_file_load_from_file(file, path, G_KEY_FILE_NONE,
                                               &error);
  if (!loaded) {
    if (error) {
      g_error_free(error);
    }
    g_key_file_free(file);
    g_free(own_path);
    return FALSE;
  }
  config->applications = key_bool(file, "providers", "applications",
                                  config->applications);
  config->calculator =
      key_bool(file, "providers", "calculator", config->calculator);
  config->web = key_bool(file, "providers", "web", config->web);
  config->files = key_bool(file, "providers", "files", config->files);
  config->clipboard =
      key_bool(file, "providers", "clipboard", config->clipboard);
  config->clipboard_automatic =
      key_bool(file, "clipboard", "automatic", config->clipboard_automatic);
  config->clipboard_cache_seconds = key_int(
      file, "clipboard", "cache_seconds", config->clipboard_cache_seconds);
  config->clipboard_cache_seconds =
      CLAMP(config->clipboard_cache_seconds, 1, 3600);
  config->dictionary =
      key_bool(file, "providers", "dictionary", config->dictionary);
  config->github = key_bool(file, "providers", "github", config->github);
  config->weather = key_bool(file, "providers", "weather", config->weather);
  config->bookmarks =
      key_bool(file, "providers", "bookmarks", config->bookmarks);
  config->max_results =
      key_int(file, "general", "max_results", config->max_results);
  config->max_results = CLAMP(config->max_results, 5, 200);
  config->file_max_depth =
      key_int(file, "files", "max_depth", config->file_max_depth);
  config->file_max_depth = CLAMP(config->file_max_depth, 1, 12);
  config->file_max_results =
      key_int(file, "files", "max_results", config->file_max_results);
  config->file_max_results = CLAMP(config->file_max_results, 1, 50);
  config->clipboard_max_results = key_int(
      file, "clipboard", "max_results", config->clipboard_max_results);
  config->clipboard_max_results =
      CLAMP(config->clipboard_max_results, 1, 50);
  char *value = key_string(file, "general", "qalc_binary",
                           config->qalc_binary);
  g_free(config->qalc_binary);
  config->qalc_binary = value;
  value = key_string(file, "general", "web_engine", config->web_engine);
  g_free(config->web_engine);
  config->web_engine = value;
  value = key_string(file, "general", "open_command", config->open_command);
  g_free(config->open_command);
  config->open_command = value;
  value = key_string(file, "files", "roots", config->file_roots);
  g_free(config->file_roots);
  config->file_roots = value;
  g_key_file_free(file);
  g_free(own_path);
  return TRUE;
}

void u_config_clear(UConfig *config) {
  if (!config) {
    return;
  }
  g_free(config->qalc_binary);
  g_free(config->web_engine);
  g_free(config->open_command);
  g_free(config->file_roots);
  memset(config, 0, sizeof(*config));
}

char *u_expand_path(const char *path) {
  if (!path) {
    return NULL;
  }
  if (g_strcmp0(path, "~") == 0) {
    return g_strdup(g_get_home_dir());
  }
  if (g_str_has_prefix(path, "~/")) {
    return g_build_filename(g_get_home_dir(), path + 2, NULL);
  }
  return g_strdup(path);
}

char *u_default_config_path(void) {
  return g_build_filename(g_get_user_config_dir(), "rofi-universal", "config.ini",
                          NULL);
}

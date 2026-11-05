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
  g_free(result->dedup_key);
  g_free(result);
}

void u_result_set_dedup_key(UResult *result, const char *key) {
  if (!result) {
    return;
  }
  g_free(result->dedup_key);
  result->dedup_key = g_strdup(key);
}

void u_results_dedup(GPtrArray *results) {
  if (!results || results->len < 2) {
    return;
  }
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  GPtrArray *unique = g_ptr_array_new();
  for (guint i = 0; i < results->len; i++) {
    UResult *result = g_ptr_array_index(results, i);
    const char *identity = result->dedup_key && *result->dedup_key
                               ? result->dedup_key
                               : result->payload;
    if (!identity || !*identity) {
      g_ptr_array_add(unique, result);
      continue;
    }
    char *key = g_strdup_printf("%d:%s", (int)result->provider, identity);
    gpointer found = g_hash_table_lookup(seen, key);
    if (found) {
      guint index = GPOINTER_TO_UINT(found) - 1;
      UResult *existing = g_ptr_array_index(unique, index);
      if (result->score > existing->score) {
        unique->pdata[index] = result;
      }
      g_free(key);
    } else {
      g_ptr_array_add(unique, result);
      g_hash_table_insert(seen, key, GUINT_TO_POINTER(unique->len));
    }
  }
  g_ptr_array_set_size(results, 0);
  for (guint i = 0; i < unique->len; i++) {
    g_ptr_array_add(results, g_ptr_array_index(unique, i));
  }
  g_ptr_array_free(unique, TRUE);
  g_hash_table_destroy(seen);
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
  if (g_ascii_strcasecmp(engine, "arch") == 0 ||
      g_ascii_strcasecmp(engine, "archwiki") == 0) {
    return "https://wiki.archlinux.org/index.php?search=%s";
  }
  if (g_ascii_strcasecmp(engine, "man") == 0) {
    return "https://man.archlinux.org/search?q=%s";
  }
  if (g_ascii_strcasecmp(engine, "docs") == 0 ||
      g_ascii_strcasecmp(engine, "devdocs") == 0) {
    return "https://devdocs.io/#q=%s";
  }
  if (g_ascii_strcasecmp(engine, "pkg") == 0 ||
      g_ascii_strcasecmp(engine, "packages") == 0) {
    return "https://archlinux.org/packages/?q=%s";
  }
  return "https://duckduckgo.com/?q=%s";
}

char *u_github_search_url(const char *query, const char *type) {
  const char *kind = type && *type ? type : "repositories";
  char *encoded = u_url_encode(query ? query : "");
  char *url = g_strdup_printf("https://github.com/search?type=%s&q=%s",
                              kind, encoded);
  g_free(encoded);
  return url;
}

char *u_github_strip_category(const char *term, UGitHubCategory *category) {
  if (category) {
    *category = U_GITHUB_REPOS;
  }
  if (!term) {
    return NULL;
  }
  char *text = g_strdup(term);
  g_strstrip(text);
  char *folded = g_utf8_casefold(text, -1);
  if (g_str_has_prefix(folded, "issues ")) {
    if (category) {
      *category = U_GITHUB_ISSUES;
    }
    memmove(text, text + 7, strlen(text + 7) + 1);
  } else if (g_str_has_prefix(folded, "issue ")) {
    if (category) {
      *category = U_GITHUB_ISSUES;
    }
    memmove(text, text + 6, strlen(text + 6) + 1);
  } else if (g_str_has_prefix(folded, "users ")) {
    if (category) {
      *category = U_GITHUB_USERS;
    }
    memmove(text, text + 6, strlen(text + 6) + 1);
  } else if (g_str_has_prefix(folded, "user ")) {
    if (category) {
      *category = U_GITHUB_USERS;
    }
    memmove(text, text + 5, strlen(text + 5) + 1);
  } else if (g_str_has_prefix(folded, "repositories ")) {
    memmove(text, text + 13, strlen(text + 13) + 1);
  } else if (g_str_has_prefix(folded, "repos ")) {
    memmove(text, text + 6, strlen(text + 6) + 1);
  } else if (g_str_has_prefix(folded, "repo ")) {
    memmove(text, text + 5, strlen(text + 5) + 1);
  }
  g_free(folded);
  g_strstrip(text);
  if (!*text) {
    g_free(text);
    return NULL;
  }
  return text;
}

char *u_github_repo_url(const char *term) {
  if (!term || !*term ||
      !g_regex_match_simple("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$", term, 0,
                            0)) {
    return NULL;
  }
  return g_strdup_printf("https://github.com/%s", term);
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
  config->weather_location = g_strdup("");
  config->weather_cache_seconds = 600;
  config->weather_timeout_seconds = 4;
  config->dictionary = TRUE;
  config->dictionary_cache_seconds = 86400;
  config->dictionary_timeout_seconds = 4;
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
  config->weather_cache_seconds =
      key_int(file, "weather", "cache_seconds", config->weather_cache_seconds);
  config->weather_cache_seconds =
      CLAMP(config->weather_cache_seconds, 30, 86400);
  config->weather_timeout_seconds = key_int(
      file, "weather", "timeout_seconds", config->weather_timeout_seconds);
  config->weather_timeout_seconds =
      CLAMP(config->weather_timeout_seconds, 1, 15);
  config->dictionary =
      key_bool(file, "providers", "dictionary", config->dictionary);
  config->dictionary_cache_seconds = key_int(
      file, "dictionary", "cache_seconds", config->dictionary_cache_seconds);
  config->dictionary_cache_seconds =
      CLAMP(config->dictionary_cache_seconds, 60, 604800);
  config->dictionary_timeout_seconds = key_int(
      file, "dictionary", "timeout_seconds", config->dictionary_timeout_seconds);
  config->dictionary_timeout_seconds =
      CLAMP(config->dictionary_timeout_seconds, 1, 15);
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
  value = key_string(file, "weather", "location", config->weather_location);
  g_free(config->weather_location);
  config->weather_location = value;
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
  g_free(config->weather_location);
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

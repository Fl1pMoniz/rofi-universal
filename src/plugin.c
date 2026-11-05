#include "universal.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <rofi/mode-private.h>
#include <rofi/mode.h>
#include <rofi/rofi-icon-fetcher.h>
#include <stdlib.h>
#include <string.h>

G_MODULE_EXPORT Mode mode;

extern void rofi_view_reload(void);

typedef struct _RofiViewState RofiViewState;
extern RofiViewState *rofi_view_get_active(void);
extern const char *rofi_view_get_user_input(const RofiViewState *state);

typedef struct {
  char *id;
  char *name;
  char *description;
  char *icon;
  char *folded_terms;
  char *folded_name;
} UApp;

typedef struct {
  char *title;
  char *url;
  char *folded_terms;
} UBookmark;

typedef struct {
  UConfig config;
  GPtrArray *results;
  GPtrArray *apps;
  GPtrArray *bookmarks;
  char *query;
  char *calc_text;
  char *calc_query;
  guint calc_generation;
  guint reload_source;
  gboolean calc_like;
  gboolean path_like;
  UIntent intent;
  GPtrArray *jobs;
} UMode;

typedef struct {
  UMode *mode;
  guint generation;
} CalcJob;

static gboolean update_query(UMode *mode, const char *input);

static char *fold_string(const char *text) {
  return g_utf8_casefold(text ? text : "", -1);
}

static void append_term(GString *terms, const char *text) {
  if (text && *text) {
    g_string_append_c(terms, ' ');
    g_string_append(terms, text);
  }
}

static int match_score_folded(const char *haystack, const char *title,
                              const char *query) {
  if (!query || !*query) {
    return 0;
  }
  int score = 0;
  if (title && g_strcmp0(title, query) == 0) {
    score = 1000;
  } else if (title && g_str_has_prefix(title, query)) {
    score = 850;
  } else if (haystack && g_str_has_prefix(haystack, query)) {
    score = 750;
  } else if (title && strstr(title, query)) {
    score = 700;
  } else if (haystack && strstr(haystack, query)) {
    score = 550;
  }
  if (score == 0 && haystack) {
    char **tokens = g_strsplit_set(query, " \t", -1);
    int total = 0;
    int matched = 0;
    for (int i = 0; tokens[i]; i++) {
      if (!*tokens[i]) {
        continue;
      }
      total++;
      if (strstr(haystack, tokens[i])) {
        matched++;
      }
    }
    if (total > 0 && matched == total) {
      score = 300 + total * 30;
    }
    g_strfreev(tokens);
  }
  return score;
}

static int result_in_intent(const UMode *mode, UProvider provider) {
  return u_intent_boost(mode->intent, provider);
}

static void add_result(UMode *mode, UResult *result) {
  if (!result) {
    return;
  }
  result->score += result_in_intent(mode, result->provider);
  g_ptr_array_add(mode->results, result);
}

static void add_app_result(UMode *mode, const UApp *app, int score) {
  char *description;
  if (app->description && *app->description) {
    description = g_strdup_printf("Application · %s", app->description);
  } else {
    description = g_strdup("Application");
  }
  add_result(mode, u_result_new(U_PROVIDER_APPS, app->name, description,
                                app->icon, score, U_ACTION_APP, app->id,
                                app->name));
  g_free(description);
}

static void app_free(gpointer data) {
  UApp *app = data;
  g_free(app->id);
  g_free(app->name);
  g_free(app->description);
  g_free(app->icon);
  g_free(app->folded_terms);
  g_free(app->folded_name);
  g_free(app);
}

static int app_compare(gconstpointer a, gconstpointer b) {
  const UApp *left = *(UApp *const *)a;
  const UApp *right = *(UApp *const *)b;
  return g_utf8_collate(left->name, right->name);
}

static void load_apps(UMode *mode) {
  GList *all = g_app_info_get_all();
  for (GList *item = all; item; item = item->next) {
    GAppInfo *info = G_APP_INFO(item->data);
    if (!G_IS_DESKTOP_APP_INFO(info) || !g_app_info_should_show(info)) {
      g_object_unref(info);
      continue;
    }
    const char *id = g_app_info_get_id(info);
    const char *name = g_app_info_get_name(info);
    if (!id || !name || !*id || !*name) {
      g_object_unref(info);
      continue;
    }
    UApp *app = g_new0(UApp, 1);
    app->id = g_strdup(id);
    app->name = g_strdup(name);
    const char *description = g_app_info_get_description(info);
    app->description = g_strdup(description ? description : "");
    GIcon *gicon = g_app_info_get_icon(info);
    char *icon = gicon ? g_icon_to_string(gicon) : NULL;
    app->icon = g_strdup(icon && *icon ? icon : u_provider_icon(U_PROVIDER_APPS));
    g_free(icon);
    GDesktopAppInfo *desktop = G_DESKTOP_APP_INFO(info);
    char *generic = g_desktop_app_info_get_string(desktop, "GenericName");
    char *keywords = g_desktop_app_info_get_string(desktop, "Keywords");
    char *categories = g_desktop_app_info_get_string(desktop, "Categories");
    const char *executable = g_app_info_get_executable(info);
    const char *command = g_app_info_get_commandline(info);
    GString *terms = g_string_new(NULL);
    append_term(terms, name);
    append_term(terms, description);
    append_term(terms, generic);
    append_term(terms, keywords);
    append_term(terms, categories);
    append_term(terms, executable);
    append_term(terms, command);
    char *folded = fold_string(terms->str);
    app->folded_terms = folded;
    app->folded_name = fold_string(name);
    g_string_free(terms, TRUE);
    g_free(generic);
    g_free(keywords);
    g_free(categories);
    g_ptr_array_add(mode->apps, app);
    g_object_unref(info);
  }
  g_list_free(all);
  g_ptr_array_sort(mode->apps, app_compare);
}

static void bookmark_free(gpointer data) {
  UBookmark *bookmark = data;
  g_free(bookmark->title);
  g_free(bookmark->url);
  g_free(bookmark->folded_terms);
  g_free(bookmark);
}

static gboolean run_capture(const char *const *argv, char **output,
                            int *exit_status) {
  GError *error = NULL;
  int status = 0;
  *output = NULL;
  gboolean ok = g_spawn_sync(NULL, (char **)argv, NULL, G_SPAWN_SEARCH_PATH,
                             NULL, NULL, output, NULL, &status, &error);
  if (exit_status) {
    *exit_status = status;
  }
  if (!ok) {
    if (error) {
      g_error_free(error);
    }
    g_free(*output);
    *output = NULL;
    return FALSE;
  }
  return TRUE;
}

static void add_bookmark_line(UMode *mode, const char *title, const char *url) {
  if (!title || !url || !*title || !*url) {
    return;
  }
  UBookmark *bookmark = g_new0(UBookmark, 1);
  bookmark->title = g_strdup(title);
  bookmark->url = g_strdup(url);
  char *terms = g_strdup_printf("%s %s", title, url);
  bookmark->folded_terms = fold_string(terms);
  g_free(terms);
  g_ptr_array_add(mode->bookmarks, bookmark);
}

static void parse_bookmark_tsv(UMode *mode, char *output) {
  char **lines = g_strsplit(output ? output : "", "\n", -1);
  for (int i = 0; lines[i]; i++) {
    char *tab = strchr(lines[i], '\t');
    if (!tab) {
      continue;
    }
    *tab = '\0';
    add_bookmark_line(mode, lines[i], tab + 1);
  }
  g_strfreev(lines);
}

static void load_chromium_bookmarks(UMode *mode) {
  const char *jq = g_find_program_in_path("jq");
  if (!jq) {
    return;
  }
  const char *bases[] = {
      "google-chrome", "chromium", "BraveSoftware/Brave-Browser", "vivaldi",
      "microsoft-edge", NULL};
  for (int i = 0; bases[i]; i++) {
    char *base = g_build_filename(g_get_user_config_dir(), bases[i], NULL);
    GDir *dir = g_dir_open(base, 0, NULL);
    if (!dir) {
      g_free(base);
      continue;
    }
    const char *profile;
    while ((profile = g_dir_read_name(dir)) != NULL) {
      if (g_strcmp0(profile, "Default") != 0 &&
          !g_str_has_prefix(profile, "Profile ")) {
        continue;
      }
      char *path = g_build_filename(base, profile, "Bookmarks", NULL);
      if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        const char *argv[] = {
            jq, "-r",
            ".. | objects | select(.type==\"url\") | [.name,.url] | @tsv",
            path, NULL};
        char *output = NULL;
        if (run_capture(argv, &output, NULL)) {
          parse_bookmark_tsv(mode, output);
          g_free(output);
        }
      }
      g_free(path);
    }
    g_dir_close(dir);
    g_free(base);
  }
}

static void load_firefox_bookmarks_from(UMode *mode, const char *root) {
  const char *sqlite3 = g_find_program_in_path("sqlite3");
  if (!sqlite3) {
    return;
  }
  GDir *dir = g_dir_open(root, 0, NULL);
  if (!dir) {
    return;
  }
  const char *profile;
  while ((profile = g_dir_read_name(dir)) != NULL) {
    char *path = g_build_filename(root, profile, "places.sqlite", NULL);
    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
      g_free(path);
      continue;
    }
    const char *argv[] = {
        sqlite3, "-separator", "\t", path,
        "SELECT COALESCE(b.title,''), p.url FROM moz_bookmarks b "
        "JOIN moz_places p ON p.id=b.fk WHERE b.type=1 AND p.url IS NOT NULL",
        NULL};
    char *output = NULL;
    if (run_capture(argv, &output, NULL)) {
      parse_bookmark_tsv(mode, output);
      g_free(output);
    }
    g_free(path);
  }
  g_dir_close(dir);
}

static void load_bookmarks(UMode *mode) {
  if (!mode->config.bookmarks) {
    return;
  }
  load_chromium_bookmarks(mode);
  char *firefox = g_build_filename(g_get_home_dir(), ".mozilla", "firefox", NULL);
  load_firefox_bookmarks_from(mode, firefox);
  g_free(firefox);
  char *zen =
      g_build_filename(g_get_user_config_dir(), "zen", "Profiles", NULL);
  load_firefox_bookmarks_from(mode, zen);
  g_free(zen);
}

static void add_apps(UMode *mode, const char *folded_query) {
  if (!mode->config.applications) {
    return;
  }
  if (!*folded_query) {
    guint limit = MIN((guint)mode->config.max_results, mode->apps->len);
    for (guint i = 0; i < limit; i++) {
      add_app_result(mode, g_ptr_array_index(mode->apps, i), 100);
    }
    return;
  }
  for (guint i = 0; i < mode->apps->len; i++) {
    UApp *app = g_ptr_array_index(mode->apps, i);
    int score =
        match_score_folded(app->folded_terms, app->folded_name, folded_query);
    if (score > 0) {
      add_app_result(mode, app, score);
    }
  }
}

static void add_web(UMode *mode, const char *query, const char *folded_query) {
  if (!mode->config.web || mode->intent == U_INTENT_GITHUB) {
    return;
  }
  if (u_query_is_url(query)) {
    char *url = g_str_has_prefix(query, "www.") ? g_strdup_printf("https://%s", query)
                                                : g_strdup(query);
    char *title = g_strdup_printf("Open %s", url);
    add_result(mode, u_result_new(U_PROVIDER_WEB, title, "Web · open address",
                                  "applications-internet", 1000,
                                  U_ACTION_OPEN_URI, url, url));
    g_free(title);
    g_free(url);
    return;
  }

  const char *engine = mode->config.web_engine;
  char *text = g_strdup(query);
  char *folded = g_strdup(folded_query);
  if (folded[0] == '!') {
    char *space = strchr(text, ' ');
    char *fspace = strchr(folded, ' ');
    if (space && fspace) {
      *space = '\0';
      *fspace = '\0';
      const char *bang = text + 1;
      if (g_ascii_strcasecmp(bang, "g") == 0 ||
          g_ascii_strcasecmp(bang, "google") == 0) {
        engine = "google";
      } else if (g_ascii_strcasecmp(bang, "ddg") == 0 ||
                 g_ascii_strcasecmp(bang, "duckduckgo") == 0) {
        engine = "duckduckgo";
      } else if (g_ascii_strcasecmp(bang, "w") == 0 ||
                 g_ascii_strcasecmp(bang, "wiki") == 0 ||
                 g_ascii_strcasecmp(bang, "wikipedia") == 0) {
        engine = "wikipedia";
      } else if (g_ascii_strcasecmp(bang, "yt") == 0 ||
                 g_ascii_strcasecmp(bang, "youtube") == 0) {
        engine = "youtube";
      } else if (g_ascii_strcasecmp(bang, "gh") == 0 ||
                 g_ascii_strcasecmp(bang, "github") == 0) {
        engine = "github";
      } else if (g_ascii_strcasecmp(bang, "aur") == 0) {
        engine = "aur";
      } else if (g_ascii_strcasecmp(bang, "maps") == 0 ||
                 g_ascii_strcasecmp(bang, "osm") == 0) {
        engine = "openstreetmap";
      } else if (g_ascii_strcasecmp(bang, "define") == 0 ||
                 g_ascii_strcasecmp(bang, "dict") == 0) {
        engine = "dictionary";
      } else if (g_ascii_strcasecmp(bang, "wiktionary") == 0) {
        engine = "wiktionary";
      }
      memmove(text, space + 1, strlen(space + 1) + 1);
    }
  } else if (g_str_has_prefix(folded, "github ")) {
    engine = "github";
    memmove(text, text + 7, strlen(text + 7) + 1);
  } else if (g_str_has_prefix(folded, "gh ")) {
    engine = "github";
    memmove(text, text + 3, strlen(text + 3) + 1);
  }
  g_strstrip(text);
  if (*text) {
    char *url = u_search_url(engine, text);
    const char *template = u_web_engine_template(engine);
    const char *label = template;
    if (strstr(template, "google.com")) {
      label = "Google";
    } else if (strstr(template, "duckduckgo.com")) {
      label = "DuckDuckGo";
    } else if (strstr(template, "wikipedia.org")) {
      label = "Wikipedia";
    } else if (strstr(template, "youtube.com")) {
      label = "YouTube";
    } else if (strstr(template, "github.com")) {
      label = "GitHub";
    } else if (strstr(template, "aur.archlinux.org")) {
      label = "AUR";
    } else if (strstr(template, "openstreetmap.org")) {
      label = "OpenStreetMap";
    } else if (strstr(template, "wiktionary.org")) {
      label = "Wiktionary";
    } else {
      label = "Web";
    }
    char *title = g_strdup_printf("Search %s for %s", label, text);
    char *description = g_strdup_printf("Web · %s", label);
    add_result(mode, u_result_new(U_PROVIDER_WEB, title, description,
                                  "applications-internet", 100,
                                  U_ACTION_OPEN_URI, url, url));
    g_free(description);
    g_free(title);
    g_free(url);
  }
  g_free(folded);
  g_free(text);
}

static void add_dictionary(UMode *mode, const char *query,
                           const char *folded_query) {
  if (!mode->config.dictionary) {
    return;
  }
  const char *word = NULL;
  if (g_str_has_prefix(folded_query, "define ")) {
    word = query + 7;
  } else if (g_str_has_prefix(folded_query, "definition ")) {
    word = query + 11;
  } else if (g_str_has_prefix(folded_query, "dict ")) {
    word = query + 5;
  }
  if (word) {
    while (*word == ' ') {
      word++;
    }
  }
  if (!word || !*word) {
    return;
  }
  char *encoded = u_url_encode(word);
  char *title = g_strdup_printf("Define “%s”", word);
  char *dictionary_url = g_strdup_printf(
      "https://www.dictionary.com/browse/%s", encoded);
  char *wiktionary_url = g_strdup_printf(
      "https://en.wiktionary.org/w/index.php?search=%s", encoded);
  add_result(mode, u_result_new(U_PROVIDER_DICTIONARY, title,
                                "Dictionary · Dictionary.com",
                                "accessories-dictionary", 500,
                                U_ACTION_OPEN_URI, dictionary_url,
                                dictionary_url));
  char *wiktionary_title = g_strdup_printf("Define “%s” on Wiktionary", word);
  add_result(mode, u_result_new(U_PROVIDER_DICTIONARY, wiktionary_title,
                                "Dictionary · Wiktionary",
                                "accessories-dictionary", 450,
                                U_ACTION_OPEN_URI, wiktionary_url,
                                wiktionary_url));
  g_free(wiktionary_title);
  g_free(title);
  g_free(dictionary_url);
  g_free(wiktionary_url);
  g_free(encoded);
}

static void add_github(UMode *mode, const char *query,
                       const char *folded_query) {
  if (!mode->config.github || mode->intent != U_INTENT_GITHUB) {
    return;
  }
  const char *term = NULL;
  if (g_str_has_prefix(folded_query, "github ")) {
    term = query + 7;
  } else if (g_str_has_prefix(folded_query, "gh ")) {
    term = query + 3;
  }
  if (!term || !*term) {
    return;
  }
  char *url = u_search_url("github", term);
  char *title = g_strdup_printf("Search GitHub for %s", term);
  add_result(mode, u_result_new(U_PROVIDER_GITHUB, title,
                                "GitHub · repositories", "applications-internet",
                                500, U_ACTION_OPEN_URI, url, url));
  g_free(title);
  g_free(url);
}

static void add_weather(UMode *mode, const char *query,
                        const char *folded_query) {
  if (!mode->config.weather || mode->intent != U_INTENT_WEATHER) {
    return;
  }
  const char *city = NULL;
  if (g_str_has_prefix(folded_query, "weather in ")) {
    city = query + 11;
  } else if (g_str_has_prefix(folded_query, "weather ")) {
    city = query + 8;
  }
  if (city) {
    while (*city == ' ') {
      city++;
    }
  }
  char *encoded = city && *city ? u_url_encode(city) : g_strdup("");
  char *url = g_strdup_printf("https://wttr.in/%s?format=3", encoded);
  char *title = city && *city ? g_strdup_printf("Weather for %s", city)
                              : g_strdup("Current weather");
  add_result(mode, u_result_new(U_PROVIDER_WEATHER, title,
                                "Weather · wttr.in", "weather-clear", 500,
                                U_ACTION_OPEN_URI, url, url));
  g_free(title);
  g_free(url);
  g_free(encoded);
}

static void add_path_result(UMode *mode, const char *query) {
  char *expanded = u_expand_path(query);
  if (!expanded || !g_file_test(expanded, G_FILE_TEST_EXISTS)) {
    g_free(expanded);
    return;
  }
  char *base = g_path_get_basename(expanded);
  gboolean is_dir = g_file_test(expanded, G_FILE_TEST_IS_DIR);
  char *description = g_strdup_printf("File · %s", expanded);
  add_result(mode, u_result_new(U_PROVIDER_FILES, base, description,
                                is_dir ? "folder" : "text-x-generic", 1200,
                                U_ACTION_OPEN_PATH, expanded, expanded));
  g_free(description);
  g_free(base);
  g_free(expanded);
}

static void add_file_search(UMode *mode, const char *query,
                            const char *folded_query) {
  (void)folded_query;
  if (!mode->config.files || mode->path_like || strlen(query) < 3 ||
      (mode->intent != U_INTENT_FILE && mode->intent != U_INTENT_APP)) {
    return;
  }
  const char *fd = g_find_program_in_path("fd");
  if (!fd) {
    return;
  }
  const char *timeout = g_find_program_in_path("timeout");
  char *roots = g_strdup(mode->config.file_roots ? mode->config.file_roots : "~");
  char **root_list = g_strsplit_set(roots, ":;", -1);
  GHashTable *seen = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  char max_results[16];
  char max_depth[16];
  g_snprintf(max_results, sizeof(max_results), "%d",
             mode->config.file_max_results);
  g_snprintf(max_depth, sizeof(max_depth), "%d", mode->config.file_max_depth);
  for (int i = 0; root_list[i]; i++) {
    if (!*root_list[i]) {
      continue;
    }
    char *root = u_expand_path(root_list[i]);
    if (!g_file_test(root, G_FILE_TEST_IS_DIR)) {
      g_free(root);
      continue;
    }
    GPtrArray *args = g_ptr_array_new();
    if (timeout) {
      g_ptr_array_add(args, (gpointer)timeout);
      g_ptr_array_add(args, (gpointer)"0.20");
    }
    g_ptr_array_add(args, (gpointer)fd);
    g_ptr_array_add(args, (gpointer)"--color=never");
    g_ptr_array_add(args, (gpointer)"--hidden");
    g_ptr_array_add(args, (gpointer)"--fixed-strings");
    g_ptr_array_add(args, (gpointer)"--max-results");
    g_ptr_array_add(args, max_results);
    g_ptr_array_add(args, (gpointer)"--max-depth");
    g_ptr_array_add(args, max_depth);
    g_ptr_array_add(args, (gpointer)"--exclude");
    g_ptr_array_add(args, (gpointer)".git");
    g_ptr_array_add(args, (gpointer)"--exclude");
    g_ptr_array_add(args, (gpointer)"node_modules");
    g_ptr_array_add(args, (gpointer)"--exclude");
    g_ptr_array_add(args, (gpointer)".cache");
    g_ptr_array_add(args, (gpointer)"--");
    g_ptr_array_add(args, (gpointer)query);
    g_ptr_array_add(args, root);
    g_ptr_array_add(args, NULL);
    char *output = NULL;
    if (run_capture((const char *const *)args->pdata, &output, NULL)) {
      char **lines = g_strsplit(output ? output : "", "\n", -1);
      for (int j = 0; lines[j]; j++) {
        if (!*lines[j] || g_hash_table_contains(seen, lines[j])) {
          continue;
        }
        g_hash_table_add(seen, g_strdup(lines[j]));
        const char *path = lines[j];
        char *base = g_path_get_basename(path);
        char *parent = g_path_get_dirname(path);
        gboolean is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);
        char *description = g_strdup_printf("File · %s", parent);
        add_result(mode, u_result_new(U_PROVIDER_FILES, base, description,
                                      is_dir ? "folder" : "text-x-generic", 500,
                                      U_ACTION_OPEN_PATH, path, path));
        g_free(description);
        g_free(parent);
        g_free(base);
      }
      g_strfreev(lines);
    }
    g_free(output);
    g_ptr_array_free(args, TRUE);
    g_free(root);
  }
  g_hash_table_destroy(seen);
  g_strfreev(root_list);
  g_free(roots);
}

static void add_clipboard(UMode *mode, const char *query,
                          const char *folded_query) {
  if (!mode->config.clipboard || mode->intent != U_INTENT_CLIPBOARD) {
    return;
  }
  const char *term = query;
  if (g_str_has_prefix(folded_query, "clipboard ")) {
    term = query + 10;
  } else if (g_str_has_prefix(folded_query, "clip ")) {
    term = query + 5;
  }
  char *search = g_strdup(term);
  g_strstrip(search);
  char *search_folded = fold_string(search);
  g_free(search);
  if (!*search_folded) {
    g_free(search_folded);
    return;
  }
  const char *cliphist = g_find_program_in_path("cliphist");
  if (!cliphist) {
    g_free(search_folded);
    return;
  }
  const char *argv[] = {cliphist, "list", NULL};
  char *output = NULL;
  if (!run_capture(argv, &output, NULL)) {
    g_free(output);
    return;
  }
  char **lines = g_strsplit(output ? output : "", "\n", -1);
  int added = 0;
  for (int i = 0; lines[i] && added < mode->config.clipboard_max_results; i++) {
    char *tab = strchr(lines[i], '\t');
    if (!tab) {
      continue;
    }
    *tab = '\0';
    const char *preview = tab + 1;
    if (!*preview || !*lines[i]) {
      continue;
    }
    char *folded = fold_string(preview);
    int score = match_score_folded(folded, folded, search_folded);
    g_free(folded);
    if (score == 0) {
      continue;
    }
    char *title = g_strdup(preview);
    if (strlen(title) > 90) {
      title[90] = '\0';
    }
    add_result(mode, u_result_new(U_PROVIDER_CLIPBOARD, title,
                                  "Clipboard · cliphist", "edit-paste", 450,
                                  U_ACTION_CLIPBOARD, lines[i], preview));
    g_free(title);
    added++;
  }
  g_strfreev(lines);
  g_free(output);
  g_free(search_folded);
}

static void add_bookmarks(UMode *mode, const char *folded_query) {
  if (!mode->config.bookmarks || !*folded_query) {
    return;
  }
  for (guint i = 0; i < mode->bookmarks->len; i++) {
    UBookmark *bookmark = g_ptr_array_index(mode->bookmarks, i);
    int score = match_score_folded(bookmark->folded_terms, bookmark->folded_terms,
                                   folded_query);
    if (score > 0) {
      add_result(mode, u_result_new(U_PROVIDER_BOOKMARKS, bookmark->title,
                                    "Bookmark", "user-bookmarks", score,
                                    U_ACTION_OPEN_URI, bookmark->url,
                                    bookmark->url));
    }
  }
}

static void rebuild_results(UMode *mode) {
  g_ptr_array_set_size(mode->results, 0);
  char *query = g_strdup(mode->query ? mode->query : "");
  char *folded = fold_string(query);
  mode->intent = u_classify_intent(query, &mode->calc_like, &mode->path_like);
  if (mode->intent == U_INTENT_EMPTY) {
    add_apps(mode, folded);
    g_free(query);
    g_free(folded);
    return;
  }
  if (mode->calc_text && mode->calc_query &&
      g_strcmp0(mode->calc_query, query) == 0) {
    char *description = g_strdup_printf("Calculator · %s", query);
    add_result(mode, u_result_new(U_PROVIDER_CALC, mode->calc_text, description,
                                  "accessories-calculator", 1000, U_ACTION_COPY,
                                  mode->calc_text, mode->calc_text));
    g_free(description);
  } else if (mode->calc_like && mode->config.calculator &&
             !g_find_program_in_path(mode->config.qalc_binary)) {
    add_result(mode, u_result_new(U_PROVIDER_CALC, "Calculator unavailable",
                                  "Calculator · install qalc (Qalculate!)",
                                  "accessories-calculator", 300,
                                  U_ACTION_NONE, NULL, NULL));
  }
  add_apps(mode, folded);
  add_web(mode, query, folded);
  add_dictionary(mode, query, folded);
  add_github(mode, query, folded);
  add_weather(mode, query, folded);
  add_bookmarks(mode, folded);
  if (mode->path_like) {
    add_path_result(mode, query);
  }
  add_file_search(mode, query, folded);
  add_clipboard(mode, query, folded);
  g_ptr_array_sort(mode->results, u_result_compare);
  if (mode->results->len > (guint)mode->config.max_results) {
    g_ptr_array_set_size(mode->results, mode->config.max_results);
  }
  g_free(query);
  g_free(folded);
}

static gboolean reload_cb(gpointer data) {
  UMode *mode = data;
  mode->reload_source = 0;
  rofi_view_reload();
  return G_SOURCE_REMOVE;
}

static void schedule_reload(UMode *mode) {
  if (!mode->reload_source) {
    mode->reload_source = g_idle_add(reload_cb, mode);
  }
}

static void sync_from_view(UMode *mode) {
  if (!mode) {
    return;
  }
  RofiViewState *state = rofi_view_get_active();
  if (state && update_query(mode, rofi_view_get_user_input(state))) {
    schedule_reload(mode);
  }
}

static void calc_finished(GObject *source, GAsyncResult *result, gpointer data) {
  CalcJob *job = data;
  UMode *mode = job->mode;
  char *output = NULL;
  GError *error = NULL;
  gboolean ok =
      g_subprocess_communicate_utf8_finish(G_SUBPROCESS(source), result, &output,
                                           NULL, &error);
  if (ok && mode && mode->calc_generation == job->generation && output) {
    char *text = g_strstrip(g_strdup(output));
    if (*text && !g_str_has_prefix(text, "error") &&
        !strstr(text, "error:") && !strstr(text, "warning:")) {
      g_free(mode->calc_text);
      g_free(mode->calc_query);
      mode->calc_text = text;
      mode->calc_query = g_strdup(mode->query ? mode->query : "");
      rebuild_results(mode);
      schedule_reload(mode);
    } else {
      g_free(text);
    }
  }
  if (error) {
    g_error_free(error);
  }
  if (mode) {
    g_ptr_array_remove(mode->jobs, job);
  }
  g_free(output);
  g_free(job);
}

static void spawn_calc(UMode *mode, const char *query) {
  if (!mode->config.calculator || !mode->calc_like || !*query) {
    return;
  }
  if (!g_find_program_in_path(mode->config.qalc_binary)) {
    return;
  }
  const char *argv[] = {mode->config.qalc_binary, "-t", "+u8", "-s",
                        "update_exchange_rates 1days", query, NULL};
  GError *error = NULL;
  GSubprocess *process = g_subprocess_newv(
      argv, G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
      &error);
  if (!process) {
    if (error) {
      g_error_free(error);
    }
    return;
  }
  CalcJob *job = g_new0(CalcJob, 1);
  job->mode = mode;
  job->generation = ++mode->calc_generation;
  g_ptr_array_add(mode->jobs, job);
  g_subprocess_communicate_utf8_async(process, NULL, NULL, calc_finished, job);
  g_object_unref(process);
}

static gboolean copy_to_clipboard(const char *text) {
  if (!text) {
    return FALSE;
  }
  const char *program = g_find_program_in_path("wl-copy");
  const char *args[4] = {NULL, NULL, NULL, NULL};
  if (program) {
    args[0] = program;
  } else {
    program = g_find_program_in_path("xclip");
    if (program) {
      args[0] = program;
      args[1] = "-selection";
      args[2] = "clipboard";
    } else {
      program = g_find_program_in_path("xsel");
      if (!program) {
        return FALSE;
      }
      args[0] = program;
      args[1] = "--clipboard";
      args[2] = "--input";
    }
  }
  GError *error = NULL;
  GSubprocess *process = g_subprocess_newv(
      args, G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
                G_SUBPROCESS_FLAGS_STDERR_SILENCE,
      &error);
  if (!process) {
    if (error) {
      g_error_free(error);
    }
    return FALSE;
  }
  gboolean ok = g_subprocess_communicate_utf8(process, text, NULL, NULL, NULL,
                                              &error);
  if (error) {
    g_error_free(error);
  }
  g_object_unref(process);
  return ok;
}

static void open_uri(UMode *mode, const char *uri) {
  GError *error = NULL;
  if (!uri || !*uri) {
    return;
  }
  if (g_app_info_launch_default_for_uri(uri, NULL, &error)) {
    return;
  }
  if (error) {
    g_error_free(error);
    error = NULL;
  }
  int argc = 0;
  char **argv = NULL;
  const char *command =
      mode->config.open_command ? mode->config.open_command : "xdg-open";
  if (g_shell_parse_argv(command, &argc, &argv, &error)) {
    argv = g_realloc(argv, sizeof(char *) * (argc + 2));
    argv[argc] = g_strdup(uri);
    argv[argc + 1] = NULL;
    g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
                  &error);
    g_strfreev(argv);
  }
  if (error) {
    g_error_free(error);
  }
}

static void open_path(UMode *mode, const char *path) {
  if (!path) {
    return;
  }
  char *uri = g_filename_to_uri(path, NULL, NULL);
  open_uri(mode, uri);
  g_free(uri);
}

static void launch_app(const char *id) {
  if (!id) {
    return;
  }
  GDesktopAppInfo *info = g_desktop_app_info_new(id);
  if (!info) {
    return;
  }
  GError *error = NULL;
  g_app_info_launch(G_APP_INFO(info), NULL, NULL, &error);
  if (error) {
    g_error_free(error);
  }
  g_object_unref(info);
}

static void execute_clipboard(const char *id) {
  const char *cliphist = g_find_program_in_path("cliphist");
  if (!cliphist || !id) {
    return;
  }
  const char *argv[] = {cliphist, "decode", id, NULL};
  char *output = NULL;
  if (run_capture(argv, &output, NULL)) {
    copy_to_clipboard(output);
  }
  g_free(output);
}

static void execute_result(UMode *mode, UResult *result, gboolean copy_only) {
  if (!result) {
    return;
  }
  if (copy_only) {
    const char *text = result->copy_text ? result->copy_text
                       : result->payload ? result->payload
                                         : result->title;
    copy_to_clipboard(text);
    return;
  }
  switch (result->action) {
  case U_ACTION_APP:
    launch_app(result->payload);
    break;
  case U_ACTION_OPEN_URI:
    open_uri(mode, result->payload);
    break;
  case U_ACTION_OPEN_PATH:
    open_path(mode, result->payload);
    break;
  case U_ACTION_COPY:
    copy_to_clipboard(result->payload ? result->payload : result->title);
    break;
  case U_ACTION_CLIPBOARD:
    execute_clipboard(result->payload);
    break;
  case U_ACTION_NONE:
  default:
    break;
  }
}

static gboolean update_query(UMode *mode, const char *input) {
  if (!mode || g_strcmp0(mode->query, input ? input : "") == 0) {
    return FALSE;
  }
  g_free(mode->query);
  mode->query = g_strdup(input ? input : "");
  g_free(mode->calc_text);
  mode->calc_text = NULL;
  g_free(mode->calc_query);
  mode->calc_query = NULL;
  mode->calc_generation++;
  rebuild_results(mode);
  spawn_calc(mode, mode->query);
  return TRUE;
}

static int universal_init(Mode *sw) {
  if (mode_get_private_data(sw)) {
    return TRUE;
  }
  UMode *mode = g_new0(UMode, 1);
  u_config_init(&mode->config);
  u_config_load(&mode->config, g_getenv("ROFI_UNIVERSAL_CONFIG"));
  mode->results = g_ptr_array_new_with_free_func(u_result_free);
  mode->apps = g_ptr_array_new_with_free_func(app_free);
  mode->bookmarks = g_ptr_array_new_with_free_func(bookmark_free);
  mode->jobs = g_ptr_array_new();
  mode->query = g_strdup("");
  load_apps(mode);
  load_bookmarks(mode);
  rebuild_results(mode);
  mode_set_private_data(sw, mode);
  return TRUE;
}

static void universal_destroy(Mode *sw) {
  UMode *mode = mode_get_private_data(sw);
  if (!mode) {
    return;
  }
  if (mode->reload_source) {
    g_source_remove(mode->reload_source);
  }
  for (guint i = 0; i < mode->jobs->len; i++) {
    CalcJob *job = g_ptr_array_index(mode->jobs, i);
    job->mode = NULL;
  }
  g_ptr_array_free(mode->jobs, TRUE);
  g_ptr_array_free(mode->results, TRUE);
  g_ptr_array_free(mode->apps, TRUE);
  g_ptr_array_free(mode->bookmarks, TRUE);
  g_free(mode->query);
  g_free(mode->calc_text);
  g_free(mode->calc_query);
  u_config_clear(&mode->config);
  g_free(mode);
  mode_set_private_data(sw, NULL);
}

static unsigned int universal_get_num_entries(const Mode *sw) {
  UMode *mode = mode_get_private_data(sw);
  if (mode) {
    RofiViewState *state = rofi_view_get_active();
    if (state) {
      update_query(mode, rofi_view_get_user_input(state));
    }
  }
  return mode ? mode->results->len : 0;
}

static char *universal_get_display_value(const Mode *sw, unsigned int selected,
                                         int *state, GList **attrs,
                                         int get_entry) {
  (void)attrs;
  UMode *mode = mode_get_private_data(sw);
  sync_from_view(mode);
  if (!mode || selected >= mode->results->len) {
    return get_entry ? g_strdup("") : NULL;
  }
  if (!get_entry) {
    return NULL;
  }
  if (state) {
    *state = 0;
  }
  UResult *result = g_ptr_array_index(mode->results, selected);
  if (result->description && *result->description) {
    return g_strdup_printf("%s  ·  %s", result->title, result->description);
  }
  return g_strdup(result->title);
}

static cairo_surface_t *universal_get_icon(const Mode *sw, unsigned int selected,
                                           unsigned int height) {
  UMode *mode = mode_get_private_data(sw);
  sync_from_view(mode);
  if (!mode || selected >= mode->results->len) {
    return NULL;
  }
  UResult *result = g_ptr_array_index(mode->results, selected);
  const char *icon =
      result->icon && *result->icon ? result->icon : u_provider_icon(result->provider);
  uint32_t uid = rofi_icon_fetcher_query(icon, height);
  return rofi_icon_fetcher_get(uid);
}

static char *universal_get_completion(const Mode *sw, unsigned int selected) {
  UMode *mode = mode_get_private_data(sw);
  if (!mode || selected >= mode->results->len) {
    return NULL;
  }
  UResult *result = g_ptr_array_index(mode->results, selected);
  return g_strdup(result->title);
}

static int universal_token_match(const Mode *sw, rofi_int_matcher **tokens,
                                 unsigned int selected) {
  (void)sw;
  (void)tokens;
  (void)selected;
  return TRUE;
}

static char *universal_preprocess_input(Mode *sw, const char *input) {
  UMode *mode = mode_get_private_data(sw);
  if (mode && update_query(mode, input)) {
    schedule_reload(mode);
  }
  return g_strdup("");
}

static char *universal_get_message(const Mode *sw) {
  UMode *mode = mode_get_private_data(sw);
  sync_from_view(mode);
  if (!mode) {
    return NULL;
  }
  if (!mode->query || !*mode->query) {
    return g_markup_printf_escaped(
        "Type to search · <b>Enter</b> open · <b>Shift+Enter</b> copy");
  }
  return g_markup_printf_escaped(
      "%u results · <b>Enter</b> open · <b>Shift+Enter</b> copy",
      mode->results->len);
}

static ModeMode universal_result(Mode *sw, int menu_entry, char **input,
                                 unsigned int selected) {
  (void)input;
  UMode *mode = mode_get_private_data(sw);
  if (!mode || selected >= mode->results->len) {
    return MODE_EXIT;
  }
  if (menu_entry & MENU_OK) {
    execute_result(mode, g_ptr_array_index(mode->results, selected),
                   (menu_entry & MENU_CUSTOM_ACTION) != 0);
    return MODE_EXIT;
  }
  return RELOAD_DIALOG;
}

Mode mode = {
    .abi_version = ABI_VERSION,
    .name = "universal",
    .cfg_name_key = "display-universal",
    ._init = universal_init,
    ._get_num_entries = universal_get_num_entries,
    ._result = universal_result,
    ._destroy = universal_destroy,
    ._token_match = universal_token_match,
    ._get_display_value = universal_get_display_value,
    ._get_icon = universal_get_icon,
    ._get_completion = universal_get_completion,
    ._preprocess_input = universal_preprocess_input,
    ._get_message = universal_get_message,
    .private_data = NULL,
    .free = NULL,
#if ABI_VERSION >= 7u
    .type = MODE_TYPE_SWITCHER,
#endif
};

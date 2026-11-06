#include "calculator.h"
#include "currency.h"
#include "dictionary.h"
#include "fetch.h"
#include "process.h"
#include "universal.h"
#include "weather.h"

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <rofi/mode-private.h>
#include <rofi/mode.h>
#include <rofi/rofi-icon-fetcher.h>
#include <math.h>
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
  char *folded_executable;
  char *folded_keywords;
} UApp;

typedef struct {
  char *title;
  char *url;
  char *folded_terms;
} UBookmark;

typedef struct {
  char *id;
  char *preview;
  char *folded;
} UClipboardEntry;

typedef struct {
  UConfig config;
  GPtrArray *results;
  GPtrArray *local_results;
  GPtrArray *async_results;
  GPtrArray *apps;
  GPtrArray *bookmarks;
  GPtrArray *clipboard_entries;
  char *query;
  guint query_generation;
  GCancellable *life_cancellable;
  GCancellable *query_cancellable;
  guint reload_source;
  UQueryAnalysis analysis;
  GPtrArray *jobs;
  gint64 clipboard_loaded_at;
  gboolean clipboard_loading;
  UWeatherCache weather_cache;
  char *weather_pending_city;
  guint weather_pending_generation;
  char *weather_failed_city;
  gint64 weather_failed_at;
  UDictionaryCache dictionary_cache;
  char *dictionary_pending_word;
  guint dictionary_pending_generation;
  char *dictionary_failed_word;
  gint64 dictionary_failed_at;
  UCurrencyCache currency_cache;
  char *currency_pending_base;
  guint currency_pending_generation;
  char *currency_failed_base;
  gint64 currency_failed_at;
  const char *currency_notice;
} UMode;

typedef struct {
  UMode *mode;
  guint generation;
  char *base;
} CurrencyJob;

typedef struct {
  UMode *mode;
  guint generation;
} FileJob;

typedef struct {
  UMode *mode;
} ClipboardJob;

typedef struct {
  UMode *mode;
} BookmarkJob;

typedef struct {
  UMode *mode;
  guint generation;
  char *city;
} WeatherJob;

typedef struct {
  UMode *mode;
  guint generation;
  char *word;
} DictionaryJob;

static gboolean update_query(UMode *mode, const char *input);
static void rebuild_local_results(UMode *mode);
static void merge_results(UMode *mode);
static void schedule_reload(UMode *mode);

static char *fold_string(const char *text) {
  return g_utf8_casefold(text ? text : "", -1);
}

static void append_term(GString *terms, const char *text) {
  if (text && *text) {
    if (terms->len > 0) {
      g_string_append_c(terms, ' ');
    }
    g_string_append(terms, text);
  }
}

static gboolean provider_is_local(UProvider provider) {
  return provider == U_PROVIDER_APPS || provider == U_PROVIDER_CALC ||
         provider == U_PROVIDER_FILES || provider == U_PROVIDER_CLIPBOARD;
}

static void add_result_to(UMode *mode, GPtrArray *target, UResult *result) {
  if (!mode || !target || !result) {
    return;
  }
  result->score = u_score_signals(
      result->score, u_provider_relevance(&mode->analysis, result->provider),
      provider_is_local(result->provider));
  g_ptr_array_add(target, result);
}

static void add_result(UMode *mode, UResult *result) {
  add_result_to(mode, mode->local_results, result);
}

static void add_async_result(UMode *mode, UResult *result) {
  add_result_to(mode, mode->async_results, result);
}

static void add_app_result(UMode *mode, const UApp *app, int score) {
  char *description;
  if (app->description && *app->description) {
    description = g_strdup_printf("Application · %s", app->description);
  } else {
    description = g_strdup("Application · launch");
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
  g_free(app->folded_executable);
  g_free(app->folded_keywords);
  g_free(app);
}

static int app_compare(gconstpointer a, gconstpointer b) {
  const UApp *left = *(UApp *const *)a;
  const UApp *right = *(UApp *const *)b;
  return g_utf8_collate(left->name, right->name);
}

static void load_apps(UMode *mode) {
  GList *all = g_app_info_get_all();
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  for (GList *item = all; item; item = item->next) {
    GAppInfo *info = G_APP_INFO(item->data);
    if (!G_IS_DESKTOP_APP_INFO(info) || !g_app_info_should_show(info)) {
      g_object_unref(info);
      continue;
    }
    const char *id = g_app_info_get_id(info);
    const char *name = g_app_info_get_name(info);
    if (!id || !name || !*id || !*name || g_hash_table_contains(seen, id)) {
      g_object_unref(info);
      continue;
    }
    g_hash_table_add(seen, g_strdup(id));
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
    append_term(terms, id);
    append_term(terms, description);
    append_term(terms, generic);
    append_term(terms, keywords);
    append_term(terms, categories);
    append_term(terms, executable);
    append_term(terms, command);
    app->folded_terms = fold_string(terms->str);
    app->folded_name = fold_string(name);
    app->folded_executable = fold_string(executable);
    app->folded_keywords = fold_string(keywords);
    g_string_free(terms, TRUE);
    g_free(generic);
    g_free(keywords);
    g_free(categories);
    g_ptr_array_add(mode->apps, app);
    g_object_unref(info);
  }
  g_list_free(all);
  g_hash_table_destroy(seen);
  g_ptr_array_sort(mode->apps, app_compare);
}

static void bookmark_free(gpointer data) {
  UBookmark *bookmark = data;
  g_free(bookmark->title);
  g_free(bookmark->url);
  g_free(bookmark->folded_terms);
  g_free(bookmark);
}

static void clipboard_entry_free(gpointer data) {
  UClipboardEntry *entry = data;
  g_free(entry->id);
  g_free(entry->preview);
  g_free(entry->folded);
  g_free(entry);
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

static void parse_bookmark_tsv(UMode *mode, const char *output) {
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

static void remove_job(UMode *mode, gpointer job) {
  if (mode) {
    g_ptr_array_remove(mode->jobs, job);
  }
}

static void bookmarks_changed(UMode *mode) {
  if (!mode || !mode->query || !*mode->query) {
    return;
  }
  rebuild_local_results(mode);
  merge_results(mode);
  schedule_reload(mode);
}

static void bookmark_process_done(const char *stdout_text,
                                  const char *stderr_text, int exit_status,
                                  gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  BookmarkJob *job = data;
  UMode *mode = job->mode;
  if (mode && success && stdout_text) {
    parse_bookmark_tsv(mode, stdout_text);
    bookmarks_changed(mode);
  }
  remove_job(mode, job);
  g_free(job);
}

static void start_bookmark_process(UMode *mode, const char *const *argv) {
  if (!mode || !mode->life_cancellable) {
    return;
  }
  BookmarkJob *job = g_new0(BookmarkJob, 1);
  job->mode = mode;
  g_ptr_array_add(mode->jobs, job);
  u_process_run(argv, mode->life_cancellable, bookmark_process_done, job);
}

static void start_chromium_bookmarks(UMode *mode) {
  char *jq = g_find_program_in_path("jq");
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
        start_bookmark_process(mode, argv);
      }
      g_free(path);
    }
    g_dir_close(dir);
    g_free(base);
  }
  g_free(jq);
}

static void start_firefox_bookmarks_from(UMode *mode, const char *root) {
  char *sqlite3 = g_find_program_in_path("sqlite3");
  if (!sqlite3) {
    return;
  }
  GDir *dir = g_dir_open(root, 0, NULL);
  if (!dir) {
    g_free(sqlite3);
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
    start_bookmark_process(mode, argv);
    g_free(path);
  }
  g_dir_close(dir);
  g_free(sqlite3);
}

static void start_bookmarks(UMode *mode) {
  if (!mode->config.bookmarks) {
    return;
  }
  start_chromium_bookmarks(mode);
  char *firefox = g_build_filename(g_get_home_dir(), ".mozilla", "firefox", NULL);
  start_firefox_bookmarks_from(mode, firefox);
  g_free(firefox);
  char *zen =
      g_build_filename(g_get_user_config_dir(), "zen", "Profiles", NULL);
  start_firefox_bookmarks_from(mode, zen);
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
    int score = u_app_match_score(
        app->folded_name, app->folded_executable, app->folded_keywords,
        app->folded_terms, folded_query);
    if (score > 0) {
      add_app_result(mode, app, score);
    }
  }
}

static void add_web(UMode *mode, const char *query, const char *folded_query) {
  if (!mode->config.web ||
      mode->analysis.explicit_provider == U_PROVIDER_GITHUB ||
      mode->analysis.explicit_provider == U_PROVIDER_CLIPBOARD) {
    return;
  }
  if (u_query_is_url(query)) {
    char *url = u_normalize_url(query);
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
      } else if (g_ascii_strcasecmp(bang, "arch") == 0 ||
                 g_ascii_strcasecmp(bang, "archwiki") == 0) {
        engine = "arch";
      } else if (g_ascii_strcasecmp(bang, "man") == 0) {
        engine = "man";
      } else if (g_ascii_strcasecmp(bang, "docs") == 0 ||
                 g_ascii_strcasecmp(bang, "devdocs") == 0) {
        engine = "docs";
      } else if (g_ascii_strcasecmp(bang, "pkg") == 0 ||
                 g_ascii_strcasecmp(bang, "packages") == 0) {
        engine = "pkg";
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
    } else if (strstr(template, "wiki.archlinux.org")) {
      label = "Arch Wiki";
    } else if (strstr(template, "man.archlinux.org")) {
      label = "Arch man pages";
    } else if (strstr(template, "devdocs.io")) {
      label = "DevDocs";
    } else if (strstr(template, "archlinux.org/packages")) {
      label = "Arch packages";
    } else {
      label = "Web";
    }
    char *title = g_strdup_printf("Search %s for %s", label, text);
    char *description = g_strdup_printf("Web · search · %s", label);
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

static gboolean dictionary_retry_active(gint64 failed_at, gint64 now) {
  return failed_at > 0 && now >= failed_at &&
         now - failed_at < 30 * G_USEC_PER_SEC;
}

static char *truncate_utf8_chars(const char *text, glong max_chars) {
  if (!text) {
    return g_strdup("");
  }
  if (g_utf8_strlen(text, -1) <= max_chars) {
    return g_strdup(text);
  }
  const char *end = g_utf8_offset_to_pointer(text, max_chars);
  char *prefix = g_strndup(text, end - text);
  char *result = g_strconcat(prefix, "…", NULL);
  g_free(prefix);
  return result;
}

static void add_dictionary_entries(UMode *mode, const char *word,
                                   GPtrArray *entries, gboolean async) {
  if (!entries || entries->len == 0) {
    return;
  }
  char *external = u_dictionary_external_url(word);
  for (guint i = 0; i < entries->len; i++) {
    UDictionaryEntry *entry = g_ptr_array_index(entries, i);
    const char *display_word =
        entry->word && *entry->word ? entry->word : word;
    char *title =
        i == 0 ? g_strdup(display_word)
               : g_strdup_printf("%s · %d", display_word, (int)i + 1);
    GString *description =
        g_string_new("Dictionary · full definition");
    if (entry->part_of_speech && *entry->part_of_speech) {
      g_string_append(description, " · ");
      g_string_append(description, entry->part_of_speech);
    }
    if (entry->phonetic && *entry->phonetic) {
      g_string_append(description, " /");
      g_string_append(description, entry->phonetic);
      g_string_append(description, "/");
    }
    if (entry->definition && *entry->definition) {
      g_string_append(description, " · ");
      g_string_append(description, entry->definition);
    }
    if (entry->example && *entry->example) {
      g_string_append(description, " — “");
      g_string_append(description, entry->example);
      g_string_append(description, "”");
    }
    char *description_text = truncate_utf8_chars(description->str, 220);
    g_string_free(description, TRUE);
    char *copy_text = g_strdup_printf("%s — %s", display_word,
                                      entry->definition ? entry->definition
                                                        : "");
    UResult *result = u_result_new(
        U_PROVIDER_DICTIONARY, title, description_text,
        "accessories-dictionary", i == 0 ? 800 : 760, U_ACTION_OPEN_URI,
        external, copy_text);
    if (i == 0) {
      u_result_set_dedup_key(result, external);
    } else {
      char *key = g_strdup_printf("%s#%d", external, (int)i + 1);
      u_result_set_dedup_key(result, key);
      g_free(key);
    }
    if (async) {
      add_async_result(mode, result);
    } else {
      add_result(mode, result);
    }
    g_free(copy_text);
    g_free(description_text);
    g_free(title);
  }
  g_free(external);
}

static void dictionary_finished(const char *stdout_text,
                                const char *stderr_text, int exit_status,
                                gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  DictionaryJob *job = data;
  UMode *mode = job->mode;
  GPtrArray *entries =
      mode && success && stdout_text
          ? u_dictionary_parse_response(stdout_text, job->word)
          : NULL;
  if (mode) {
    gboolean current = mode->query_generation == job->generation;
    if (entries) {
      if (current) {
        g_clear_pointer(&mode->dictionary_pending_word, g_free);
        mode->dictionary_pending_generation = 0;
        g_clear_pointer(&mode->dictionary_failed_word, g_free);
        mode->dictionary_failed_at = 0;
        add_dictionary_entries(mode, job->word, entries, TRUE);
        merge_results(mode);
        schedule_reload(mode);
      }
      u_dictionary_cache_put(&mode->dictionary_cache, job->word, entries,
                             g_get_monotonic_time());
    } else if (current &&
               g_strcmp0(mode->dictionary_pending_word, job->word) == 0) {
      g_clear_pointer(&mode->dictionary_pending_word, g_free);
      mode->dictionary_pending_generation = 0;
      g_free(mode->dictionary_failed_word);
      mode->dictionary_failed_word = g_strdup(job->word);
      mode->dictionary_failed_at = g_get_monotonic_time();
    }
  }
  remove_job(mode, job);
  g_free(job->word);
  g_free(job);
}

static void start_dictionary(UMode *mode, const char *word) {
  if (!mode->config.dictionary || !word || !*word) {
    return;
  }
  if (g_strcmp0(mode->dictionary_pending_word, word) == 0 &&
      mode->dictionary_pending_generation == mode->query_generation) {
    return;
  }
  gint64 now = g_get_monotonic_time();
  if (g_strcmp0(mode->dictionary_failed_word, word) == 0 &&
      dictionary_retry_active(mode->dictionary_failed_at, now)) {
    return;
  }
  char *fetcher_path = NULL;
  UFetcher fetcher = u_fetch_find(&fetcher_path);
  if (fetcher == U_FETCH_NONE) {
    g_free(fetcher_path);
    return;
  }
  char *url = u_dictionary_api_url(word);
  char **argv = u_fetch_build_argv(fetcher, fetcher_path, url,
                                   mode->config.dictionary_timeout_seconds);
  if (!argv) {
    g_free(url);
    g_free(fetcher_path);
    return;
  }
  DictionaryJob *job = g_new0(DictionaryJob, 1);
  job->mode = mode;
  job->generation = mode->query_generation;
  job->word = g_strdup(word);
  g_free(mode->dictionary_pending_word);
  mode->dictionary_pending_word = g_strdup(word);
  mode->dictionary_pending_generation = mode->query_generation;
  g_ptr_array_add(mode->jobs, job);
  u_process_run((const char *const *)argv, mode->query_cancellable,
                dictionary_finished, job);
  g_strfreev(argv);
  g_free(url);
  g_free(fetcher_path);
}

static void add_dictionary(UMode *mode, const char *query,
                           const char *folded_query) {
  if (!mode->config.dictionary || mode->analysis.dictionary_score < 600) {
    return;
  }
  char *word = u_dictionary_word_from_query(query, folded_query);
  if (!word || !*word) {
    g_free(word);
    return;
  }
  char *dictionary_url = u_dictionary_external_url(word);
  char *wiktionary_url = u_wiktionary_url(word);
  char *title = g_strdup_printf("Open definition for %s", word);
  UResult *fallback = u_result_new(
      U_PROVIDER_DICTIONARY, title, "Dictionary · open reference",
      "accessories-dictionary", 450, U_ACTION_OPEN_URI, dictionary_url,
      dictionary_url);
  u_result_set_dedup_key(fallback, dictionary_url);
  add_result(mode, fallback);
  char *wiktionary_title = g_strdup_printf("Look up “%s” on Wiktionary", word);
  add_result(mode, u_result_new(U_PROVIDER_DICTIONARY, wiktionary_title,
                                "Dictionary · open Wiktionary",
                                "accessories-dictionary", 430,
                                U_ACTION_OPEN_URI, wiktionary_url,
                                wiktionary_url));
  gboolean fresh = FALSE;
  GPtrArray *entries = u_dictionary_cache_get(
      &mode->dictionary_cache, word, g_get_monotonic_time(), &fresh);
  if (entries && fresh) {
    add_dictionary_entries(mode, word, entries, FALSE);
  } else {
    start_dictionary(mode, word);
  }
  g_free(wiktionary_title);
  g_free(title);
  g_free(dictionary_url);
  g_free(wiktionary_url);
  g_free(word);
}

static void add_github_category(UMode *mode, const char *search,
                                UGitHubCategory category, int score) {
  const char *type = "repositories";
  const char *label = "repositories";
  const char *icon = "applications-internet";
  if (category == U_GITHUB_USERS) {
    type = "users";
    label = "users";
    icon = "system-users";
  } else if (category == U_GITHUB_ISSUES) {
    type = "issues";
    label = "issues";
    icon = "dialog-information";
  }
  char *url = u_github_search_url(search, type);
  char *title = g_strdup_printf("Search GitHub %s for %s", label, search);
  char *description = g_strdup_printf("GitHub · %s", label);
  add_result(mode, u_result_new(U_PROVIDER_GITHUB, title, description, icon,
                                score, U_ACTION_OPEN_URI, url, url));
  g_free(description);
  g_free(title);
  g_free(url);
}

static void add_github(UMode *mode, const char *query,
                       const char *folded_query) {
  if (!mode->config.github || mode->analysis.github_score < 600) {
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
  UGitHubCategory category = U_GITHUB_REPOS;
  char *search = u_github_strip_category(term, &category);
  if (!search) {
    return;
  }

  char *repo_url = u_github_repo_url(search);
  if (repo_url) {
    char *title = g_strdup_printf("Open GitHub repository %s", search);
    add_result(mode, u_result_new(U_PROVIDER_GITHUB, title,
                                  "GitHub · repository", "applications-internet",
                                  700, U_ACTION_OPEN_URI, repo_url, repo_url));
    g_free(title);
  }

  UGitHubCategory order[3];
  if (category == U_GITHUB_ISSUES) {
    order[0] = U_GITHUB_ISSUES;
    order[1] = U_GITHUB_REPOS;
    order[2] = U_GITHUB_USERS;
  } else if (category == U_GITHUB_USERS) {
    order[0] = U_GITHUB_USERS;
    order[1] = U_GITHUB_REPOS;
    order[2] = U_GITHUB_ISSUES;
  } else {
    order[0] = U_GITHUB_REPOS;
    order[1] = U_GITHUB_USERS;
    order[2] = U_GITHUB_ISSUES;
  }
  const int scores[3] = {650, 500, 450};
  for (int i = 0; i < 3; i++) {
    add_github_category(mode, search, order[i], scores[i]);
  }
  g_free(repo_url);
  g_free(search);
}

static void weather_finished(const char *stdout_text,
                             const char *stderr_text, int exit_status,
                             gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  WeatherJob *job = data;
  UMode *mode = job->mode;
  char *summary =
      mode && success ? u_weather_parse_response(stdout_text) : NULL;
  if (mode) {
    gboolean current = mode->query_generation == job->generation;
    if (summary) {
      u_weather_cache_put(&mode->weather_cache, job->city, summary,
                          g_get_monotonic_time());
      if (current) {
        g_clear_pointer(&mode->weather_pending_city, g_free);
        mode->weather_pending_generation = 0;
        g_clear_pointer(&mode->weather_failed_city, g_free);
        mode->weather_failed_at = 0;
        char *url = u_weather_forecast_url(job->city);
        add_async_result(mode,
                         u_result_new(U_PROVIDER_WEATHER, summary,
                                      "Weather · open forecast", "weather-clear",
                                      700,
                                      U_ACTION_OPEN_URI, url, summary));
        g_free(url);
        merge_results(mode);
        schedule_reload(mode);
      }
    } else if (current &&
               g_strcmp0(mode->weather_pending_city, job->city) == 0) {
      g_clear_pointer(&mode->weather_pending_city, g_free);
      mode->weather_pending_generation = 0;
      g_free(mode->weather_failed_city);
      mode->weather_failed_city = g_strdup(job->city);
      mode->weather_failed_at = g_get_monotonic_time();
    }
  }
  g_free(summary);
  remove_job(mode, job);
  g_free(job->city);
  g_free(job);
}

static void start_weather(UMode *mode, const char *city) {
  if (!mode->config.weather || !city || !*city) {
    return;
  }
  if (g_strcmp0(mode->weather_pending_city, city) == 0 &&
      mode->weather_pending_generation == mode->query_generation) {
    return;
  }
  gint64 now = g_get_monotonic_time();
  if (g_strcmp0(mode->weather_failed_city, city) == 0 &&
      u_weather_retry_backoff_active(mode->weather_failed_at, now, 30)) {
    return;
  }
  char *fetcher_path = NULL;
  UFetcher fetcher = u_fetch_find(&fetcher_path);
  if (fetcher == U_FETCH_NONE) {
    g_free(fetcher_path);
    return;
  }
  char *url = u_weather_summary_url(city);
  char **argv = u_fetch_build_argv(
      fetcher, fetcher_path, url, mode->config.weather_timeout_seconds);
  if (!argv) {
    g_free(url);
    g_free(fetcher_path);
    return;
  }
  WeatherJob *job = g_new0(WeatherJob, 1);
  job->mode = mode;
  job->generation = mode->query_generation;
  job->city = g_strdup(city);
  g_free(mode->weather_pending_city);
  mode->weather_pending_city = g_strdup(city);
  mode->weather_pending_generation = mode->query_generation;
  g_ptr_array_add(mode->jobs, job);
  u_process_run((const char *const *)argv, mode->query_cancellable,
                weather_finished, job);
  g_strfreev(argv);
  g_free(url);
  g_free(fetcher_path);
}

static void add_weather(UMode *mode, const char *query,
                        const char *folded_query) {
  if (!mode->config.weather || mode->analysis.weather_score < 600) {
    return;
  }
  char *city = u_weather_location_from_query(
      query, folded_query, mode->config.weather_location);
  if (!city || !*city) {
    add_result(mode, u_result_new(
                         U_PROVIDER_WEATHER, "Weather · specify a location",
                         "Use weather in <city> or set [weather] location",
                         "weather-clear", 500, U_ACTION_NONE, NULL, NULL));
    g_free(city);
    return;
  }
  char *forecast = u_weather_forecast_url(city);
  char *fallback_title = g_strdup_printf("Open detailed forecast for %s", city);
  add_result(mode, u_result_new(U_PROVIDER_WEATHER, fallback_title,
                                "Weather · open forecast", "weather-clear", 450,
                                U_ACTION_OPEN_URI, forecast, forecast));
  g_free(fallback_title);
  gboolean fresh = FALSE;
  const char *summary = u_weather_cache_get(
      &mode->weather_cache, city, g_get_monotonic_time(), &fresh);
  if (summary && fresh) {
    add_result(mode, u_result_new(U_PROVIDER_WEATHER, summary,
                                  "Weather · open forecast", "weather-clear", 700,
                                  U_ACTION_OPEN_URI, forecast, summary));
  } else {
    start_weather(mode, city);
  }
  g_free(forecast);
  g_free(city);
}

static void add_path_result(UMode *mode, const char *query) {
  char *expanded = u_expand_path(query);
  if (!expanded || !g_file_test(expanded, G_FILE_TEST_EXISTS)) {
    g_free(expanded);
    return;
  }
  char *base = g_path_get_basename(expanded);
  gboolean is_dir = g_file_test(expanded, G_FILE_TEST_IS_DIR);
  char *description = g_strdup_printf("File · open · %s", expanded);
  add_result(mode, u_result_new(U_PROVIDER_FILES, base, description,
                                is_dir ? "folder" : "text-x-generic", 1200,
                                U_ACTION_OPEN_PATH, expanded, expanded));
  g_free(description);
  g_free(base);
  g_free(expanded);
}

static void add_bookmarks(UMode *mode, const char *folded_query) {
  if (!mode->config.bookmarks || !*folded_query) {
    return;
  }
  for (guint i = 0; i < mode->bookmarks->len; i++) {
    UBookmark *bookmark = g_ptr_array_index(mode->bookmarks, i);
    int score = u_match_score_folded(bookmark->folded_terms,
                                     bookmark->folded_terms, folded_query);
    if (score > 0) {
      add_result(mode, u_result_new(U_PROVIDER_BOOKMARKS, bookmark->title,
                                    "Bookmark · open", "user-bookmarks", score,
                                    U_ACTION_OPEN_URI, bookmark->url,
                                    bookmark->url));
    }
  }
}

static void clipboard_process_done(const char *stdout_text,
                                   const char *stderr_text, int exit_status,
                                   gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  ClipboardJob *job = data;
  UMode *mode = job->mode;
  if (mode) {
    mode->clipboard_loading = FALSE;
    if (success && stdout_text) {
      g_ptr_array_set_size(mode->clipboard_entries, 0);
      guint entry_limit =
          MAX((guint)mode->config.clipboard_max_results * 20, 50u);
      char **lines = g_strsplit(stdout_text, "\n", -1);
      for (int i = 0; lines[i] && mode->clipboard_entries->len < entry_limit;
           i++) {
        char *tab = strchr(lines[i], '\t');
        if (!tab) {
          continue;
        }
        *tab = '\0';
        const char *preview = tab + 1;
        if (!*preview || !*lines[i]) {
          continue;
        }
        UClipboardEntry *entry = g_new0(UClipboardEntry, 1);
        entry->id = g_strdup(lines[i]);
        entry->preview = g_strdup(preview);
        entry->folded = fold_string(preview);
        g_ptr_array_add(mode->clipboard_entries, entry);
      }
      g_strfreev(lines);
    }
    mode->clipboard_loaded_at = g_get_monotonic_time();
    if (mode->query && *mode->query) {
      rebuild_local_results(mode);
      merge_results(mode);
      schedule_reload(mode);
    }
  }
  remove_job(mode, job);
  g_free(job);
}

static void ensure_clipboard_cache(UMode *mode) {
  if (!mode || !mode->config.clipboard || mode->clipboard_loading) {
    return;
  }
  gint64 now = g_get_monotonic_time();
  if (mode->clipboard_loaded_at != 0 &&
      now - mode->clipboard_loaded_at <
          (gint64)mode->config.clipboard_cache_seconds * G_USEC_PER_SEC) {
    return;
  }
  char *cliphist = g_find_program_in_path("cliphist");
  if (!cliphist || !mode->life_cancellable) {
    g_free(cliphist);
    return;
  }
  const char *argv[] = {cliphist, "list", NULL};
  ClipboardJob *job = g_new0(ClipboardJob, 1);
  job->mode = mode;
  mode->clipboard_loading = TRUE;
  g_ptr_array_add(mode->jobs, job);
  u_process_run(argv, mode->life_cancellable, clipboard_process_done, job);
  g_free(cliphist);
}

static void add_clipboard(UMode *mode, const char *query,
                          const char *folded_query) {
  if (!mode->config.clipboard) {
    return;
  }
  gboolean explicit = mode->analysis.explicit_provider == U_PROVIDER_CLIPBOARD;
  if (!explicit && !mode->config.clipboard_automatic) {
    return;
  }
  if (!*folded_query || strlen(folded_query) < 3) {
    return;
  }
  ensure_clipboard_cache(mode);
  if (mode->clipboard_entries->len == 0) {
    return;
  }
  const char *term = query;
  if (explicit && g_str_has_prefix(folded_query, "clipboard ")) {
    term = query + 10;
  } else if (explicit && g_str_has_prefix(folded_query, "clip ")) {
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
  int added = 0;
  for (guint i = 0;
       i < mode->clipboard_entries->len &&
       added < mode->config.clipboard_max_results;
       i++) {
    UClipboardEntry *entry = g_ptr_array_index(mode->clipboard_entries, i);
    int score = u_match_score_folded(entry->folded, entry->folded,
                                     search_folded);
    if (score == 0) {
      continue;
    }
    if (!explicit) {
      score = MIN(score, 300);
    }
    char *title = g_strdup(entry->preview);
    if (strlen(title) > 90) {
      title[90] = '\0';
    }
    add_result(mode, u_result_new(U_PROVIDER_CLIPBOARD, title,
                                  "Clipboard · copy", "edit-paste", score,
                                  U_ACTION_CLIPBOARD, entry->id,
                                  entry->preview));
    g_free(title);
    added++;
  }
  g_free(search_folded);
}

static gboolean currency_retry_backoff_active(gint64 failed_at, gint64 now) {
  if (failed_at <= 0 || now < failed_at) {
    return FALSE;
  }
  return now - failed_at < (gint64)60 * G_USEC_PER_SEC;
}

static gboolean add_currency_result(UMode *mode, const UCurrencyQuery *query,
                                    const UCurrencyRates *rates,
                                    gboolean fresh) {
  double rate = 1.0;
  const char *rate_date = NULL;
  gboolean same_currency = g_strcmp0(query->base, query->quote) == 0;
  if (!same_currency &&
      (!rates || !u_currency_rate(rates, query->quote, &rate, &rate_date))) {
    return FALSE;
  }
  double converted = query->amount * rate;
  if (!isfinite(converted)) {
    return FALSE;
  }
  char *source = u_calc_format(query->amount);
  char *target =
      u_currency_format_amount(converted, u_currency_minor_units(query->quote));
  if (!source || !target) {
    g_free(source);
    g_free(target);
    return FALSE;
  }
  char *title = g_strdup_printf("%s %s = %s %s", source, query->base, target,
                                query->quote);
  char *description;
  if (same_currency) {
    description = g_strdup("Same currency · no conversion");
  } else if (fresh) {
    description =
        g_strdup_printf("Ref %s", rate_date ? rate_date : "unknown");
  } else {
    description =
        g_strdup_printf("Cached %s", rate_date ? rate_date : "unknown");
  }
  add_result(mode, u_result_new(U_PROVIDER_CALC, title, description,
                                "accessories-calculator", fresh ? 1000 : 950,
                                U_ACTION_COPY, title, title));
  g_free(description);
  g_free(title);
  g_free(target);
  g_free(source);
  return TRUE;
}

static void currency_finished(const char *stdout_text, const char *stderr_text,
                              int exit_status, gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  CurrencyJob *job = data;
  UMode *mode = job->mode;
  if (mode) {
    UCurrencyRates *rates = NULL;
    if (success && stdout_text) {
      rates = u_currency_parse_rates(stdout_text, job->base);
    }
    if (rates) {
      u_currency_cache_put(&mode->currency_cache, rates,
                           g_get_monotonic_time());
      g_clear_pointer(&mode->currency_failed_base, g_free);
      mode->currency_failed_at = 0;
    } else {
      g_free(mode->currency_failed_base);
      mode->currency_failed_base = g_strdup(job->base);
      mode->currency_failed_at = g_get_monotonic_time();
    }
    if (mode->currency_pending_base &&
        g_strcmp0(mode->currency_pending_base, job->base) == 0 &&
        mode->currency_pending_generation == job->generation) {
      g_clear_pointer(&mode->currency_pending_base, g_free);
      mode->currency_pending_generation = 0;
    }
    if (mode->query_generation == job->generation) {
      rebuild_local_results(mode);
      merge_results(mode);
      schedule_reload(mode);
    }
  }
  remove_job(mode, job);
  g_free(job->base);
  g_free(job);
}

static gboolean start_currency(UMode *mode, const char *base) {
  char *fetcher_path = NULL;
  UFetcher fetcher = u_fetch_find(&fetcher_path);
  if (fetcher == U_FETCH_NONE) {
    g_free(fetcher_path);
    mode->currency_notice = "currency conversion needs curl or wget";
    return FALSE;
  }
  char *url = u_currency_rates_url(base);
  char **argv = u_fetch_build_argv(fetcher, fetcher_path, url,
                                   mode->config.currency_timeout_seconds);
  g_free(url);
  g_free(fetcher_path);
  if (!argv) {
    mode->currency_notice = "currency conversion unavailable";
    return FALSE;
  }
  CurrencyJob *job = g_new0(CurrencyJob, 1);
  job->mode = mode;
  job->generation = mode->query_generation;
  job->base = g_strdup(base);
  g_free(mode->currency_pending_base);
  mode->currency_pending_base = g_strdup(base);
  mode->currency_pending_generation = job->generation;
  mode->currency_notice = "fetching exchange rate";
  g_ptr_array_add(mode->jobs, job);
  u_process_run((const char *const *)argv, mode->query_cancellable,
                currency_finished, job);
  g_strfreev(argv);
  return TRUE;
}

static void add_currency(UMode *mode, const UCurrencyQuery *query) {
  gint64 now = g_get_monotonic_time();
  if (g_strcmp0(query->base, query->quote) == 0) {
    add_currency_result(mode, query, NULL, TRUE);
    return;
  }
  gboolean fresh = FALSE;
  UCurrencyRates *rates =
      u_currency_cache_get(&mode->currency_cache, query->base, now, &fresh);
  if (rates) {
    if (!add_currency_result(mode, query, rates, fresh)) {
      mode->currency_notice = "currency rate unavailable";
      return;
    }
    if (fresh) {
      return;
    }
    if (mode->currency_pending_base &&
        g_strcmp0(mode->currency_pending_base, query->base) == 0 &&
        mode->currency_pending_generation == mode->query_generation) {
      mode->currency_notice = "fetching exchange rate";
      return;
    }
    if (mode->currency_failed_base &&
        g_strcmp0(mode->currency_failed_base, query->base) == 0 &&
        currency_retry_backoff_active(mode->currency_failed_at, now)) {
      mode->currency_notice = "refresh failed; using cached rate";
      return;
    }
    start_currency(mode, query->base);
    return;
  }
  if (mode->currency_pending_base &&
      g_strcmp0(mode->currency_pending_base, query->base) == 0 &&
      mode->currency_pending_generation == mode->query_generation) {
    mode->currency_notice = "fetching exchange rate";
    return;
  }
  if (mode->currency_failed_base &&
      g_strcmp0(mode->currency_failed_base, query->base) == 0 &&
      currency_retry_backoff_active(mode->currency_failed_at, now)) {
    mode->currency_notice = "currency rate unavailable";
    return;
  }
  start_currency(mode, query->base);
}

static void add_calculator(UMode *mode, const char *query) {
  if (!mode->config.calculator) {
    return;
  }
  UCurrencyQuery currency;
  if (u_currency_parse_query(query, &currency)) {
    add_currency(mode, &currency);
    return;
  }
  if (mode->analysis.calc_score < 600) {
    return;
  }
  double value = 0;
  if (u_calc_evaluate(query, &value) != U_CALC_OK) {
    return;
  }
  char *text = u_calc_format(value);
  if (!text) {
    return;
  }
  char *description = g_strdup("Calculator · copy result");
  add_result(mode, u_result_new(U_PROVIDER_CALC, text, description,
                                "accessories-calculator", 1000, U_ACTION_COPY,
                                text, text));
  g_free(description);
  g_free(text);
}

static void rebuild_local_results(UMode *mode) {
  g_ptr_array_set_size(mode->local_results, 0);
  mode->currency_notice = NULL;
  char *query = g_strdup(mode->query ? mode->query : "");
  char *folded = fold_string(query);
  if (mode->analysis.intent == U_INTENT_EMPTY) {
    add_apps(mode, folded);
    g_free(query);
    g_free(folded);
    return;
  }
  add_apps(mode, folded);
  add_calculator(mode, query);
  add_web(mode, query, folded);
  add_dictionary(mode, query, folded);
  add_github(mode, query, folded);
  add_weather(mode, query, folded);
  add_bookmarks(mode, folded);
  if (mode->analysis.path_like) {
    add_path_result(mode, query);
  }
  add_clipboard(mode, query, folded);
  g_free(query);
  g_free(folded);
}

static void merge_results(UMode *mode) {
  g_ptr_array_set_size(mode->results, 0);
  for (guint i = 0; i < mode->local_results->len; i++) {
    g_ptr_array_add(mode->results, g_ptr_array_index(mode->local_results, i));
  }
  for (guint i = 0; i < mode->async_results->len; i++) {
    g_ptr_array_add(mode->results, g_ptr_array_index(mode->async_results, i));
  }
  u_results_dedup(mode->results);
  g_ptr_array_sort(mode->results, u_result_compare);
  if (mode->results->len > (guint)mode->config.max_results) {
    g_ptr_array_set_size(mode->results, mode->config.max_results);
  }
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

static void files_finished(const char *stdout_text, const char *stderr_text,
                           int exit_status, gboolean success, gpointer data) {
  (void)stderr_text;
  (void)exit_status;
  FileJob *job = data;
  UMode *mode = job->mode;
  if (mode && success && mode->query_generation == job->generation &&
      stdout_text) {
    GHashTable *seen =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    char **lines = g_strsplit(stdout_text, "\n", -1);
    for (int i = 0; lines[i]; i++) {
      if (!*lines[i] || g_hash_table_contains(seen, lines[i])) {
        continue;
      }
      g_hash_table_add(seen, g_strdup(lines[i]));
      const char *path = lines[i];
      char *base = g_path_get_basename(path);
      char *parent = g_path_get_dirname(path);
      gboolean is_dir = g_file_test(path, G_FILE_TEST_IS_DIR);
      char *description = g_strdup_printf("File · open · %s", parent);
      add_async_result(mode,
                       u_result_new(U_PROVIDER_FILES, base, description,
                                    is_dir ? "folder" : "text-x-generic",
                                    u_file_match_score(path, mode->query),
                                    U_ACTION_OPEN_PATH, path, path));
      g_free(description);
      g_free(parent);
      g_free(base);
    }
    g_strfreev(lines);
    g_hash_table_destroy(seen);
    merge_results(mode);
    schedule_reload(mode);
  }
  remove_job(mode, job);
  g_free(job);
}

static void start_files(UMode *mode) {
  if (!mode->config.files || mode->analysis.path_like ||
      !mode->query || strlen(mode->query) < 3 ||
      (mode->analysis.file_score < 100 && mode->analysis.app_score < 500)) {
    return;
  }
  char *fd = g_find_program_in_path("fd");
  if (!fd) {
    return;
  }
  char *roots =
      g_strdup(mode->config.file_roots ? mode->config.file_roots : "~");
  char **root_list = g_strsplit_set(roots, ":;", -1);
  GPtrArray *args = g_ptr_array_new();
  g_ptr_array_add(args, (gpointer)fd);
  g_ptr_array_add(args, (gpointer)"--color=never");
  g_ptr_array_add(args, (gpointer)"--hidden");
  g_ptr_array_add(args, (gpointer)(u_query_has_glob(mode->query) ? "--glob"
                                                                 : "--fixed-strings"));
  char max_results[16];
  char max_depth[16];
  g_snprintf(max_results, sizeof(max_results), "%d",
             mode->config.file_max_results);
  g_snprintf(max_depth, sizeof(max_depth), "%d", mode->config.file_max_depth);
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
  g_ptr_array_add(args, (gpointer)mode->query);
  guint root_start = args->len;
  guint root_count = 0;
  for (int i = 0; root_list[i]; i++) {
    if (!*root_list[i]) {
      continue;
    }
    char *root = u_expand_path(root_list[i]);
    if (g_file_test(root, G_FILE_TEST_IS_DIR)) {
      g_ptr_array_add(args, root);
      root_count++;
    } else {
      g_free(root);
    }
  }
  g_ptr_array_add(args, NULL);
  if (root_count == 0) {
    g_ptr_array_free(args, TRUE);
    g_strfreev(root_list);
    g_free(roots);
    g_free(fd);
    return;
  }
  FileJob *job = g_new0(FileJob, 1);
  job->mode = mode;
  job->generation = mode->query_generation;
  g_ptr_array_add(mode->jobs, job);
  u_process_run((const char *const *)args->pdata, mode->query_cancellable,
                files_finished, job);
  for (guint i = root_start; i < args->len - 1; i++) {
    g_free(g_ptr_array_index(args, i));
  }
  g_ptr_array_free(args, TRUE);
  g_strfreev(root_list);
  g_free(roots);
  g_free(fd);
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
    g_free((gpointer)program);
    return FALSE;
  }
  gboolean ok = g_subprocess_communicate_utf8(process, text, NULL, NULL, NULL,
                                              &error);
  if (error) {
    g_error_free(error);
  }
  g_object_unref(process);
  g_free((gpointer)program);
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

static gboolean reveal_with_file_manager(const char *path) {
  char *uri = u_file_uri(path);
  if (!uri) {
    return FALSE;
  }
  GError *error = NULL;
  GDBusConnection *bus =
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!bus) {
    g_clear_error(&error);
    g_free(uri);
    return FALSE;
  }
  const gchar *uris[] = {uri, NULL};
  GVariant *reply = g_dbus_connection_call_sync(
      bus, "org.freedesktop.FileManager1",
      "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1",
      "ShowItems", g_variant_new("(^ass)", uris, ""), NULL,
      G_DBUS_CALL_FLAGS_NONE, 750, NULL, &error);
  g_clear_error(&error);
  g_object_unref(bus);
  g_free(uri);
  if (reply) {
    g_variant_unref(reply);
    return TRUE;
  }
  return FALSE;
}

static void reveal_path(UMode *mode, const char *path) {
  if (!path || !*path) {
    return;
  }
  if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
    open_path(mode, path);
    return;
  }
  if (g_file_test(path, G_FILE_TEST_EXISTS) &&
      reveal_with_file_manager(path)) {
    return;
  }
  char *parent = u_file_parent(path);
  if (parent && g_file_test(parent, G_FILE_TEST_IS_DIR)) {
    open_path(mode, parent);
  }
  g_free(parent);
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
  char *cliphist = g_find_program_in_path("cliphist");
  if (!cliphist || !id) {
    g_free(cliphist);
    return;
  }
  const char *argv[] = {cliphist, "decode", id, NULL};
  char *output = NULL;
  int exit_status = -1;
  if (u_process_capture_sync_limited(argv, U_PROCESS_DEFAULT_MAX_OUTPUT,
                                     5000, &output, &exit_status) &&
      output && g_utf8_validate(output, -1, NULL)) {
    copy_to_clipboard(output);
  }
  g_free(output);
  g_free(cliphist);
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
  mode->query_generation++;
  if (mode->query_cancellable) {
    g_cancellable_cancel(mode->query_cancellable);
    g_object_unref(mode->query_cancellable);
  }
  mode->query_cancellable = g_cancellable_new();
  g_ptr_array_set_size(mode->async_results, 0);
  u_analyze_query(mode->query, &mode->analysis);
  rebuild_local_results(mode);
  start_files(mode);
  merge_results(mode);
  return TRUE;
}

static int universal_init(Mode *sw) {
  if (mode_get_private_data(sw)) {
    return TRUE;
  }
  UMode *mode = g_new0(UMode, 1);
  u_config_init(&mode->config);
  u_config_load(&mode->config, g_getenv("ROFI_UNIVERSAL_CONFIG"));
  mode->results = g_ptr_array_new();
  mode->local_results = g_ptr_array_new_with_free_func(u_result_free);
  mode->async_results = g_ptr_array_new_with_free_func(u_result_free);
  mode->apps = g_ptr_array_new_with_free_func(app_free);
  mode->bookmarks = g_ptr_array_new_with_free_func(bookmark_free);
  mode->clipboard_entries =
      g_ptr_array_new_with_free_func(clipboard_entry_free);
  u_weather_cache_init(&mode->weather_cache,
                       mode->config.weather_cache_seconds, 64);
  u_dictionary_cache_init(&mode->dictionary_cache,
                          mode->config.dictionary_cache_seconds, 32);
  mode->jobs = g_ptr_array_new();
  mode->life_cancellable = g_cancellable_new();
  mode->query_cancellable = g_cancellable_new();
  mode->query = g_strdup("");
  u_analyze_query(mode->query, &mode->analysis);
  u_currency_cache_init(&mode->currency_cache,
                        mode->config.currency_cache_seconds, 8);
  load_apps(mode);
  rebuild_local_results(mode);
  merge_results(mode);
  start_bookmarks(mode);
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
    UMode **owner = g_ptr_array_index(mode->jobs, i);
    *owner = NULL;
  }
  if (mode->query_cancellable) {
    g_cancellable_cancel(mode->query_cancellable);
    g_object_unref(mode->query_cancellable);
  }
  if (mode->life_cancellable) {
    g_cancellable_cancel(mode->life_cancellable);
    g_object_unref(mode->life_cancellable);
  }
  g_ptr_array_free(mode->jobs, TRUE);
  g_ptr_array_free(mode->results, TRUE);
  g_ptr_array_free(mode->local_results, TRUE);
  g_ptr_array_free(mode->async_results, TRUE);
  g_ptr_array_free(mode->apps, TRUE);
  g_ptr_array_free(mode->bookmarks, TRUE);
  g_ptr_array_free(mode->clipboard_entries, TRUE);
  u_weather_cache_clear(&mode->weather_cache);
  u_dictionary_cache_clear(&mode->dictionary_cache);
  u_currency_cache_clear(&mode->currency_cache);
  g_free(mode->weather_pending_city);
  g_free(mode->weather_failed_city);
  g_free(mode->dictionary_pending_word);
  g_free(mode->dictionary_failed_word);
  g_free(mode->currency_pending_base);
  g_free(mode->currency_failed_base);
  g_free(mode->query);
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
  return u_result_display_text(g_ptr_array_index(mode->results, selected));
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
  gboolean has_path = FALSE;
  for (guint i = 0; i < mode->results->len; i++) {
    UResult *result = g_ptr_array_index(mode->results, i);
    if (result->action == U_ACTION_OPEN_PATH) {
      has_path = TRUE;
      break;
    }
  }
  return u_status_message(mode->results->len,
                          !mode->query || !*mode->query, has_path,
                          mode->currency_notice);
}

static ModeMode universal_result(Mode *sw, int menu_entry, char **input,
                                 unsigned int selected) {
  (void)input;
  UMode *mode = mode_get_private_data(sw);
  if (!mode || selected >= mode->results->len) {
    return MODE_EXIT;
  }
  UResult *result = g_ptr_array_index(mode->results, selected);
  if (menu_entry & MENU_CUSTOM_COMMAND) {
    if ((menu_entry & MENU_LOWER_MASK) == 0 &&
        result->action == U_ACTION_OPEN_PATH) {
      reveal_path(mode, result->payload);
    }
    return MODE_EXIT;
  }
  if (menu_entry & MENU_OK) {
    execute_result(mode, result,
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

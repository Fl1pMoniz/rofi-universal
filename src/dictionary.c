#include "dictionary.h"

#include <json-glib/json-glib.h>
#include <string.h>

typedef struct {
  GPtrArray *entries;
  gint64 fetched_at;
} UDictionaryCacheEntry;

static void dictionary_entry_free(gpointer data) {
  UDictionaryEntry *entry = data;
  g_free(entry->word);
  g_free(entry->phonetic);
  g_free(entry->part_of_speech);
  g_free(entry->definition);
  g_free(entry->example);
  g_free(entry);
}

static void dictionary_cache_entry_free(gpointer data) {
  UDictionaryCacheEntry *entry = data;
  if (entry->entries) {
    u_dictionary_entries_free(entry->entries);
  }
  g_free(entry);
}

static const char *object_string(JsonObject *object, const char *key) {
  if (!object || !json_object_has_member(object, key)) {
    return NULL;
  }
  JsonNode *node = json_object_get_member(object, key);
  if (!node || !JSON_NODE_HOLDS_VALUE(node) ||
      json_node_get_value_type(node) != G_TYPE_STRING) {
    return NULL;
  }
  const char *value = json_node_get_string(node);
  return value && *value ? value : NULL;
}

static JsonArray *object_array(JsonObject *object, const char *key) {
  if (!object || !json_object_has_member(object, key)) {
    return NULL;
  }
  JsonNode *node = json_object_get_member(object, key);
  if (!node || !JSON_NODE_HOLDS_ARRAY(node)) {
    return NULL;
  }
  return json_node_get_array(node);
}

static const char *article_phonetic(JsonObject *article) {
  const char *phonetic = object_string(article, "phonetic");
  if (phonetic) {
    return phonetic;
  }
  JsonArray *phonetics = object_array(article, "phonetics");
  if (!phonetics || json_array_get_length(phonetics) == 0) {
    return NULL;
  }
  JsonNode *node = json_array_get_element(phonetics, 0);
  if (!node || !JSON_NODE_HOLDS_OBJECT(node)) {
    return NULL;
  }
  return object_string(json_node_get_object(node), "text");
}

char *u_dictionary_api_url(const char *word) {
  char *encoded = g_uri_escape_string(word ? word : "", NULL, TRUE);
  char *url = g_strdup_printf(
      "https://api.dictionaryapi.dev/api/v2/entries/en/%s", encoded);
  g_free(encoded);
  return url;
}

char *u_dictionary_external_url(const char *word) {
  char *encoded = g_uri_escape_string(word ? word : "", NULL, TRUE);
  char *url =
      g_strdup_printf("https://www.dictionary.com/browse/%s", encoded);
  g_free(encoded);
  return url;
}

char *u_wiktionary_url(const char *word) {
  char *encoded = g_uri_escape_string(word ? word : "", NULL, TRUE);
  char *url = g_strdup_printf(
      "https://en.wiktionary.org/w/index.php?search=%s", encoded);
  g_free(encoded);
  return url;
}

GPtrArray *u_dictionary_parse_response(const char *raw,
                                       const char *fallback_word) {
  if (!raw || !*raw || !g_utf8_validate(raw, -1, NULL)) {
    return NULL;
  }
  JsonParser *parser = json_parser_new();
  GError *error = NULL;
  if (!json_parser_load_from_data(parser, raw, -1, &error)) {
    g_clear_error(&error);
    g_object_unref(parser);
    return NULL;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
    g_object_unref(parser);
    return NULL;
  }
  JsonArray *articles = json_node_get_array(root);
  if (!articles || json_array_get_length(articles) == 0) {
    g_object_unref(parser);
    return NULL;
  }
  GPtrArray *entries = g_ptr_array_new_with_free_func(dictionary_entry_free);
  for (guint i = 0; i < json_array_get_length(articles) && entries->len < 2;
       i++) {
    JsonNode *article_node = json_array_get_element(articles, i);
    if (!article_node || !JSON_NODE_HOLDS_OBJECT(article_node)) {
      continue;
    }
    JsonObject *article = json_node_get_object(article_node);
    const char *word = object_string(article, "word");
    const char *phonetic = article_phonetic(article);
    JsonArray *meanings = object_array(article, "meanings");
    if (!meanings) {
      continue;
    }
    for (guint j = 0;
         j < json_array_get_length(meanings) && entries->len < 2; j++) {
      JsonNode *meaning_node = json_array_get_element(meanings, j);
      if (!meaning_node || !JSON_NODE_HOLDS_OBJECT(meaning_node)) {
        continue;
      }
      JsonObject *meaning = json_node_get_object(meaning_node);
      const char *part_of_speech = object_string(meaning, "partOfSpeech");
      JsonArray *definitions = object_array(meaning, "definitions");
      if (!definitions) {
        continue;
      }
      for (guint k = 0;
           k < json_array_get_length(definitions) && entries->len < 2; k++) {
        JsonNode *definition_node = json_array_get_element(definitions, k);
        if (!definition_node || !JSON_NODE_HOLDS_OBJECT(definition_node)) {
          continue;
        }
        JsonObject *definition_object = json_node_get_object(definition_node);
        const char *definition =
            object_string(definition_object, "definition");
        if (!definition) {
          continue;
        }
        UDictionaryEntry *entry = g_new0(UDictionaryEntry, 1);
        entry->word = g_strdup(word ? word
                                    : fallback_word ? fallback_word : "");
        entry->phonetic = g_strdup(phonetic ? phonetic : "");
        entry->part_of_speech =
            g_strdup(part_of_speech ? part_of_speech : "");
        entry->definition = g_strdup(definition);
        const char *example = object_string(definition_object, "example");
        entry->example = g_strdup(example ? example : "");
        g_ptr_array_add(entries, entry);
      }
    }
  }
  g_object_unref(parser);
  if (entries->len == 0) {
    u_dictionary_entries_free(entries);
    return NULL;
  }
  return entries;
}

void u_dictionary_entries_free(GPtrArray *entries) {
  if (entries) {
    g_ptr_array_free(entries, TRUE);
  }
}

void u_dictionary_cache_init(UDictionaryCache *cache, int ttl_seconds,
                             guint capacity) {
  cache->ttl_seconds = ttl_seconds > 0 ? ttl_seconds : 86400;
  cache->capacity = capacity > 0 ? capacity : 32;
  cache->entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                         dictionary_cache_entry_free);
}

void u_dictionary_cache_clear(UDictionaryCache *cache) {
  if (cache && cache->entries) {
    g_hash_table_destroy(cache->entries);
    cache->entries = NULL;
  }
}

GPtrArray *u_dictionary_cache_get(UDictionaryCache *cache, const char *word,
                                  gint64 now, gboolean *fresh) {
  if (fresh) {
    *fresh = FALSE;
  }
  if (!cache || !cache->entries || !word) {
    return NULL;
  }
  UDictionaryCacheEntry *entry = g_hash_table_lookup(cache->entries, word);
  if (!entry) {
    return NULL;
  }
  if (fresh) {
    *fresh = now >= entry->fetched_at &&
             now - entry->fetched_at <
                 (gint64)cache->ttl_seconds * G_USEC_PER_SEC;
  }
  return entry->entries;
}

void u_dictionary_cache_put(UDictionaryCache *cache, const char *word,
                            GPtrArray *entries, gint64 now) {
  if (!cache || !cache->entries || !word || !entries) {
    return;
  }
  if (g_hash_table_size(cache->entries) >= cache->capacity) {
    const char *oldest_key = NULL;
    gint64 oldest_time = G_MAXINT64;
    GHashTableIter iter;
    gpointer key;
    gpointer value;
    g_hash_table_iter_init(&iter, cache->entries);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
      UDictionaryCacheEntry *entry = value;
      if (entry->fetched_at < oldest_time) {
        oldest_time = entry->fetched_at;
        oldest_key = key;
      }
    }
    if (oldest_key) {
      g_hash_table_remove(cache->entries, oldest_key);
    }
  }
  UDictionaryCacheEntry *entry = g_new0(UDictionaryCacheEntry, 1);
  entry->entries = entries;
  entry->fetched_at = now;
  g_hash_table_replace(cache->entries, g_strdup(word), entry);
}

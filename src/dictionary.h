#ifndef ROFI_UNIVERSAL_DICTIONARY_H
#define ROFI_UNIVERSAL_DICTIONARY_H

#include <glib.h>

typedef struct {
  char *word;
  char *phonetic;
  char *part_of_speech;
  char *definition;
  char *example;
} UDictionaryEntry;

typedef struct {
  GHashTable *entries;
  int ttl_seconds;
  guint capacity;
} UDictionaryCache;

char *u_dictionary_api_url(const char *word);
char *u_dictionary_external_url(const char *word);
char *u_wiktionary_url(const char *word);
GPtrArray *u_dictionary_parse_response(const char *raw,
                                       const char *fallback_word);
void u_dictionary_entries_free(GPtrArray *entries);

void u_dictionary_cache_init(UDictionaryCache *cache, int ttl_seconds,
                             guint capacity);
void u_dictionary_cache_clear(UDictionaryCache *cache);
GPtrArray *u_dictionary_cache_get(UDictionaryCache *cache, const char *word,
                                  gint64 now, gboolean *fresh);
void u_dictionary_cache_put(UDictionaryCache *cache, const char *word,
                            GPtrArray *entries, gint64 now);

#endif

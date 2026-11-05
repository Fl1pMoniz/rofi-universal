#include "dictionary.h"

#include <glib.h>

static GPtrArray *make_entries(const char *word) {
  char *json = g_strdup_printf(
      "[{\"word\":\"%s\",\"meanings\":[{\"definitions\":"
      "[{\"definition\":\"definition\"}]}]}]",
      word);
  GPtrArray *entries = u_dictionary_parse_response(json, word);
  g_free(json);
  g_assert_nonnull(entries);
  return entries;
}

static void test_urls(void) {
  char *url = u_dictionary_api_url("New York");
  g_assert_cmpstr(url, ==,
                  "https://api.dictionaryapi.dev/api/v2/entries/en/New%20York");
  g_free(url);
  url = u_dictionary_external_url("ephemeral");
  g_assert_cmpstr(url, ==,
                  "https://www.dictionary.com/browse/ephemeral");
  g_free(url);
  url = u_wiktionary_url("ephemeral");
  g_assert_cmpstr(
      url, ==,
      "https://en.wiktionary.org/w/index.php?search=ephemeral");
  g_free(url);
}

static void test_parse_valid(void) {
  const char *json =
      "[{\"word\":\"ephemeral\",\"phonetic\":\"/x/\",\"meanings\":["
      "{\"partOfSpeech\":\"adjective\",\"definitions\":["
      "{\"definition\":\"Lasting for a short time.\","
      "\"example\":\"ephemeral fame\"},"
      "{\"definition\":\"Existing for only one day.\"}]}]}]";
  GPtrArray *entries = u_dictionary_parse_response(json, NULL);
  g_assert_nonnull(entries);
  g_assert_cmpuint(entries->len, ==, 2);
  UDictionaryEntry *first = g_ptr_array_index(entries, 0);
  g_assert_cmpstr(first->word, ==, "ephemeral");
  g_assert_cmpstr(first->phonetic, ==, "/x/");
  g_assert_cmpstr(first->part_of_speech, ==, "adjective");
  g_assert_cmpstr(first->definition, ==, "Lasting for a short time.");
  g_assert_cmpstr(first->example, ==, "ephemeral fame");
  UDictionaryEntry *second = g_ptr_array_index(entries, 1);
  g_assert_cmpstr(second->definition, ==, "Existing for only one day.");
  u_dictionary_entries_free(entries);
}

static void test_parse_phonetics_array(void) {
  const char *json =
      "[{\"word\":\"hello\",\"phonetics\":[{\"text\":\"/həˈləʊ/\"}],"
      "\"meanings\":[{\"definitions\":[{\"definition\":\"A greeting.\"}]}]}]";
  GPtrArray *entries = u_dictionary_parse_response(json, NULL);
  g_assert_nonnull(entries);
  g_assert_cmpstr(((UDictionaryEntry *)g_ptr_array_index(entries, 0))->phonetic,
                  ==, "/həˈləʊ/");
  u_dictionary_entries_free(entries);
}

static void test_parse_fallbacks(void) {
  const char *json =
      "[{\"meanings\":[{\"definitions\":[{\"definition\":\"A thing.\"}]}]}]";
  GPtrArray *entries = u_dictionary_parse_response(json, "fallback");
  g_assert_nonnull(entries);
  g_assert_cmpuint(entries->len, ==, 1);
  UDictionaryEntry *entry = g_ptr_array_index(entries, 0);
  g_assert_cmpstr(entry->word, ==, "fallback");
  u_dictionary_entries_free(entries);
}

static void test_parse_invalid(void) {
  g_assert_null(u_dictionary_parse_response(NULL, "x"));
  g_assert_null(u_dictionary_parse_response("", "x"));
  g_assert_null(u_dictionary_parse_response("{", "x"));
  g_assert_null(u_dictionary_parse_response("{\"title\":\"No Definitions Found\"}",
                                            "x"));
  g_assert_null(u_dictionary_parse_response("[]", "x"));
  g_assert_null(u_dictionary_parse_response("\xff", "x"));
}

static void test_cache(void) {
  UDictionaryCache cache;
  u_dictionary_cache_init(&cache, 60, 2);
  gboolean fresh = TRUE;
  g_assert_null(u_dictionary_cache_get(&cache, "one", 1 * G_USEC_PER_SEC,
                                       &fresh));
  g_assert_false(fresh);
  u_dictionary_cache_put(&cache, "one", make_entries("one"),
                         1 * G_USEC_PER_SEC);
  GPtrArray *entries = u_dictionary_cache_get(
      &cache, "one", 60 * G_USEC_PER_SEC, &fresh);
  g_assert_nonnull(entries);
  g_assert_true(fresh);
  g_assert_cmpstr(((UDictionaryEntry *)g_ptr_array_index(entries, 0))->word, ==,
                  "one");
  entries = u_dictionary_cache_get(&cache, "one", 62 * G_USEC_PER_SEC, &fresh);
  g_assert_nonnull(entries);
  g_assert_false(fresh);

  u_dictionary_cache_put(&cache, "two", make_entries("two"),
                         2 * G_USEC_PER_SEC);
  u_dictionary_cache_put(&cache, "three", make_entries("three"),
                         3 * G_USEC_PER_SEC);
  g_assert_null(u_dictionary_cache_get(&cache, "one", 3 * G_USEC_PER_SEC,
                                       &fresh));
  g_assert_nonnull(u_dictionary_cache_get(&cache, "two", 3 * G_USEC_PER_SEC,
                                          &fresh));
  g_assert_nonnull(u_dictionary_cache_get(&cache, "three", 3 * G_USEC_PER_SEC,
                                          &fresh));
  u_dictionary_cache_clear(&cache);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/dictionary/urls", test_urls);
  g_test_add_func("/dictionary/parse-valid", test_parse_valid);
  g_test_add_func("/dictionary/parse-phonetics", test_parse_phonetics_array);
  g_test_add_func("/dictionary/parse-fallbacks", test_parse_fallbacks);
  g_test_add_func("/dictionary/parse-invalid", test_parse_invalid);
  g_test_add_func("/dictionary/cache", test_cache);
  return g_test_run();
}

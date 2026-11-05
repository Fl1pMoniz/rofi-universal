# Architecture

`rofi-universal` is a native Rofi mode plugin. It deliberately avoids a daemon,
database, indexing service or dynamic provider framework.

```text
src/
  plugin.c       Rofi mode callbacks, provider implementations and actions
  query.c        Query analysis, provider relevance and scoring signals
  process.c      Cancellable async, output-limited subprocess capture
  fetch.c        curl/wget selection and command construction
  weather.c      Weather location, URL and response parsing
  dictionary.c   Dictionary URL, JSON parsing and bounded cache
  universal.c    Result model, URL building and INI configuration
  universal.h    Shared interfaces
```

## Query analysis

`u_analyze_query()` produces independent scores for applications, calculator,
web, files, clipboard, dictionary, GitHub and weather. A query can be relevant to
multiple providers at once. The analysis also exposes convenience fields such as
`path_like`, `url_like`, the primary `intent` and an `explicit_provider` used to
avoid duplicate shortcut results.

URL analysis recognizes explicit schemes, `www.` prefixes and bare common
domains. Bare domains are normalized to HTTPS before opening, while common file
extensions remain file/path searches. `src/query.c` contains deterministic rules
only. There is no AI model, token budget or network call in classification.
Natural-language queries containing
words such as `features`, `configuration`, `guide`, `documentation`, `examples`,
`alternatives`, `comparison`, `review`, `reference`, `manual`, `download`,
`commands`, `extensions`, `themes`, `plugins`, `addons`, `packages` or `api`
receive a stronger web signal without suppressing application or file matches.

## Ranking

Each provider creates `UResult` objects with a base match score. `plugin.c` adds:

- provider relevance from the query analysis,
- a small local-action bonus for applications, calculations, files and clipboard.

Application matching uses `u_app_match_score()` across name, executable, keyword
and desktop-term fields. The strongest field wins, but description-only terms are
discounted and executable-prefix matches are weighted below name-prefix matches,
so `steam games` ranks Steam itself above catalogue entries. File results use
`u_file_match_score()` to rank filename hits above parent-path hits.

`u_result_display_text()` renders the consistent title/description text used by
Rofi. Presentation stays plain text so both tested Rofi ABI versions behave
identically; no fragile pango markup or custom theme is introduced.

`u_results_dedup()` collapses results with the same provider and identity, keeping
the higher score. It uses an optional `dedup_key` when present, otherwise the
payload. Inline weather and the external forecast action share a URL, so the
inline summary replaces the fallback when present. Inline dictionary definitions
use per-sense dedup keys so multiple definitions remain visible while the
redundant Dictionary.com fallback is replaced.

`u_score_signals()` and `u_provider_relevance()` are testable in `test_core.c`.
Sorting is deterministic: score, provider, then title.

## Actions

The normal `Enter` action is provider-specific. `Shift+Return` uses Rofi's
`MENU_CUSTOM_ACTION` for the existing copy behavior. `Alt+1` (`kb-custom-1`,
`MENU_CUSTOM_COMMAND` index 0) reveals a selected file: the plugin calls
`org.freedesktop.FileManager1.ShowItems` with a file URI and a 750 ms timeout,
then falls back to opening the containing directory through the default desktop
handler. Directories open normally. No file-manager executable is hardcoded.

The reveal call is an explicit action rather than typing-time work. It is allowed
to make a short synchronous D-Bus/default-handler call while Rofi exits; it does
not affect search responsiveness. URI/parent construction and path-normalization
helpers are unit-tested.

Clipboard decode uses a bounded, synchronous `GSubprocess` capture after an
explicit copy action. It caps output at 4 MiB and refuses binary or invalid UTF-8
content. It remains separate from the async plugin jobs so that plugin teardown
cannot cancel a requested copy. Clipboard contents are never logged.

## Async lifecycle

`src/process.c` wraps `GSubprocess` with cancellable asynchronous stdout/stderr
reads and reports a success flag, exit status and a cancellation flag. Output is
streamed into bounded strings: stdout and stderr each default to 4 MiB per
process. Exceeding a limit cancels the reads, force-exits the child, marks the
operation failed and delivers only the bounded partial output. An optional
timeout is available through the limited API and is covered by tests. Cancellation
and timeout both stop reads promptly even when a grandchild process still holds a
pipe open.

The plugin keeps:

- a `life_cancellable` for plugin-lifetime work (bookmark and clipboard loading),
- a `query_cancellable` replaced on every query change,
- a `query_generation` counter.

Query-specific callbacks (calculator, file search and inline weather) are applied
only when their generation still matches the current query. Old callbacks are
ignored after cancellation. Bookmark and clipboard results are not query-specific;
when they arrive they rebuild local results and merge with any current async
results.

Inline weather uses a dedicated `WeatherJob` (mode pointer, query generation and
city). The request is a `curl` or `wget` subprocess with a bounded timeout. Parsed
summaries are stored in `src/weather.c`'s bounded in-memory cache for
`[weather] cache_seconds`; the oldest entry is evicted when the cache is full.
Failed cities back off briefly, and stale-generation results may be cached but
are never inserted into the current result list. If curl/wget or the network is
unavailable, the external wttr.in action remains.

`src/weather.c` owns the bounded cache and response parsing. `src/fetch.c` owns
curl/wget selection and command construction. These are tested without network
access. The process helper reports non-zero exits as failures, so failed curl/wget
requests never become weather results.

Inline dictionary uses the same fetch and process helpers. `src/dictionary.c`
parses dictionaryapi.dev JSON with JSON-GLib, extracts up to two definitions,
and maintains a bounded in-memory cache with oldest-entry eviction. The plugin
uses a query generation and pending-word generation, like weather, so obsolete
dictionary requests cannot overwrite newer results. Failed words back off briefly.
The browser fallback is always available.

Clipboard decode/copy remains a synchronous explicit-action path. This avoids
adding teardown-sensitive asynchronous work for an operation that is already
initiated by the user; the large-item buffering caveat is documented.

On plugin destruction, all jobs have their mode pointer cleared, cancellables are
cancelled, and result/cache arrays are released. Late callbacks free their job
without touching the destroyed mode.

`tests/benchmark_core.c` is an optional local benchmark target, separate from CI,
for repeated query analysis and application matching.

`tests/test_rapid_queries.py` exercises this path under Xvfb. It uses a fake
fetcher for both weather and dictionary requests, sends several rapid queries
with `xdotool`, closes Rofi while work is pending, and checks for clean teardown
and the absence of crash markers. It is a lifecycle smoke test rather than a
row-level assertion.

## Result ownership

- `local_results` owns synchronously generated results.
- `async_results` owns results produced by calculator, file search, inline
  weather and inline dictionary.
- `results` is a borrowed pointer array used by Rofi for display and action.

`merge_results()` rebuilds the borrowed array from the two owner arrays, sorts it
and truncates it to `max_results`. Provider functions never see unrelated
provider state beyond the shared mode configuration and caches.

## Provider code

Provider implementations remain in `plugin.c` because they share the live mode
state, result ordering and action dispatch. Introducing a dynamic provider
framework would add complexity without a demonstrated need. New providers should
use `UResult`, `u_score_signals()` and `u_process_run()`.

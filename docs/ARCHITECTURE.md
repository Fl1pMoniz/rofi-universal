# Architecture

`rofi-universal` is a native Rofi mode plugin. It deliberately avoids a daemon,
database, indexing service or dynamic provider framework.

```text
src/
  plugin.c       Rofi mode callbacks, provider implementations and actions
  query.c        Query analysis, provider relevance and scoring signals
  process.c      Cancellable async subprocess capture
  universal.c    Result model, URL building and INI configuration
  universal.h    Shared interfaces
```

## Query analysis

`u_analyze_query()` produces independent scores for applications, calculator,
web, files, clipboard, dictionary, GitHub and weather. A query can be relevant to
multiple providers at once. The analysis also exposes convenience fields such as
`path_like`, `url_like`, the primary `intent` and an `explicit_provider` used to
avoid duplicate shortcut results.

`src/query.c` contains deterministic rules only. There is no AI model, token
budget or network call in classification.

## Ranking

Each provider creates `UResult` objects with a base match score. `plugin.c` adds:

- provider relevance from the query analysis,
- a small local-action bonus for applications, calculations, files and clipboard.

`u_score_signals()` and `u_provider_relevance()` are testable in `test_core.c`.
Sorting is deterministic: score, provider, then title.

## Async lifecycle

`src/process.c` wraps `GSubprocess` with a cancellable callback that captures
stdout/stderr and reports a success flag, exit status and a cancellation flag.
Cancellation force-exits the child process.

The plugin keeps:

- a `life_cancellable` for plugin-lifetime work (bookmark and clipboard loading),
- a `query_cancellable` replaced on every query change,
- a `query_generation` counter.

Query-specific callbacks (calculator and file search) are applied only when their
generation still matches the current query. Old callbacks are ignored after
cancellation. Bookmark and clipboard results are not query-specific; when they
arrive they rebuild local results and merge with any current async results.

On plugin destruction, all jobs have their mode pointer cleared, cancellables are
cancelled, and result/cache arrays are released. Late callbacks free their job
without touching the destroyed mode.

## Result ownership

- `local_results` owns synchronously generated results.
- `async_results` owns results produced by calculator and file search.
- `results` is a borrowed pointer array used by Rofi for display and action.

`merge_results()` rebuilds the borrowed array from the two owner arrays, sorts it
and truncates it to `max_results`. Provider functions never see unrelated
provider state beyond the shared mode configuration and caches.

## Provider code

Provider implementations remain in `plugin.c` because they share the live mode
state, result ordering and action dispatch. Introducing a dynamic provider
framework would add complexity without a demonstrated need. New providers should
use `UResult`, `u_score_signals()` and `u_process_run()`.

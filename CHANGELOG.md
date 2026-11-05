# Changelog

All notable changes are documented here. The project follows semantic versioning
once regular releases begin.

## [0.6.0] - 2026-11-05

### Added

- Bare-domain URL recognition for common web domains outside explicit schemes
  and `www.` prefixes. `github.com/rofi` now becomes `https://github.com/rofi`
  and opens as a URL rather than being treated as a local path.
- Expanded natural-language web signals with `commands`, `extensions`, `themes`,
  `plugins`, `addons` and `packages`.
- Deterministic presentation helper for title/description rendering, with
  regression tests for empty and missing descriptions.
- Clearer action-oriented descriptions for applications, files, calculator,
  clipboard, web, dictionary and weather fallbacks.
- Optional `benchmark-core` target for deterministic local query-analysis and
  application-matching measurements. It is not a CI gate.

### Changed

- `terminal commands`, `firefox extensions` and `hyprland themes` now favor web
  results over the application prefix match, while `steam games` still favors
  Steam itself.
- Result presentation remains plain text compatible with Rofi 1.7.9 and 2.0.0;
  native pango attribute styling was not used.

### Investigated / retained

- Clipboard decode/copy remains a synchronous explicit-action path. It avoids
  adding lifecycle complexity around plugin teardown; the large-item caveat
  remains documented.
- No new configuration options were added; existing configuration remains
  compatible.

### Deferred

- A reveal-in-file-manager secondary action was not added because Shift+Enter is
  already the documented copy action and a new binding would risk conflicting
  with existing Rofi behavior.
- Inline GitHub API results and web previews remain deferred for reliability
  reasons.

## [0.5.0] - 2026-11-05

### Added

- Inline dictionary definitions from dictionaryapi.dev, including part of speech,
  phonetic text, examples and up to two definitions. Natural forms such as
  `definition of ephemeral`, `ephemeral meaning` and `what does ephemeral mean`
  are recognized alongside `define ephemeral`.
- Bounded dictionary cache with configurable TTL and timeout, stale-request
  rejection, retry backoff and browser fallback.
- Bounded subprocess output: stdout and stderr are streamed and capped at 4 MiB
  each. Exceeding a limit terminates the process and marks the operation failed.
- Optional timeout support in the process helper, with deterministic tests for
  timeout, continuous output, partial reads, large stderr and invalid UTF-8.
- GitHub category-led queries: `github issues rofi`, `github users torvalds` and
  `github repos rofi plugins` put the requested category first.
- Per-result deduplication keys so multiple dictionary senses remain visible
  while the redundant external-reference fallback is collapsed.
- Shared `src/fetch.c` curl/wget selection and command construction.
- JSON-GLib dependency for defensive JSON parsing.

### Changed

- Inline dictionary results open the detailed Dictionary.com reference; the
  Wiktionary and Dictionary.com browser actions remain fallbacks.
- Process cancellation and timeout stop reads immediately instead of waiting for
  grandchildren that still hold inherited pipes open.
- GitHub search actions use more useful repository/user/issue labels and scores.

### Fixed

- Subprocess output is no longer unbounded in memory.
- A timed-out or cancelled subprocess no longer waits for surviving child
  processes to close inherited pipes.
- Dictionary responses with malformed JSON, invalid UTF-8 or missing fields are
  rejected safely.

### Deferred

- GitHub inline API results remain deferred because unauthenticated search rate
  limits make typing-driven requests unreliable; browser actions remain.
- Web-search result previews remain deferred because no reliable documented
  source is available without scraping.

## [0.4.0] - 2026-11-05

### Added

- Application match scoring across names, generic names, keywords, desktop IDs,
  executables and command terms. Query-leading app names now surface the app for
  queries such as `steam games` without giving every Steam game the same rank.
- File matching ranked by filename versus parent path, plus glob search support
  for patterns such as `*.png`, `report?.pdf` and `[ab].txt`.
- Web shortcuts for Arch Wiki (`!arch`), Arch man pages (`!man`), DevDocs
  (`!docs`) and Arch packages (`!pkg`).
- GitHub repository, user and issue search actions, plus direct `owner/repo`
  opening.
- Duplicate suppression for results with the same provider and payload. Inline
  weather now replaces the redundant forecast action when both are present.
- Xvfb rapid-query, cancellation and teardown smoke test in CI.

### Changed

- Expanded natural-language web signals with terms such as `examples`,
  `alternatives`, `comparison`, `review`, `reference`, `manual`, `download` and
  `api`.
- Generic application matches are weighted below direct name and executable
  matches, so `steam games` ranks Steam itself above its catalogue entries.
- Inline weather descriptions now say `Weather · forecast` because selecting the
  inline result opens the detailed forecast.

### Fixed

- Weak executable-prefix matches can no longer outrank the named application.
- Empty or malformed weather responses and process failures remain rejected.

### Deferred

- Subprocess stdout is still fully buffered; a streaming output cap was not
  introduced because it needs a larger process-lifecycle change.
- Inline dictionary definitions and search-engine previews remain deferred.

## [0.3.1] - 2026-11-05

### Added

- Arch Linux packaging validation in CI, including `makepkg`, `namcap`, package
  installation, plugin loading and removal.
- Fedora 42 and 44 RPM package lifecycle tests: build, install, plugin loading,
  removal and user-configuration preservation.
- Deterministic source-tarball generation and pinned SHA-256 verification for
  release archives.
- Weather cache, retry-backoff, fetcher-selection and command-construction tests.
- Rofi plugin loading tests under Xvfb with Rofi 1.7.9 (Fedora 42) and Rofi
  2.0.0 (Fedora 44 and Arch Linux).

### Changed

- Moved weather cache state into `src/weather.c` with bounded capacity and
  oldest-entry eviction.
- Weather response parsing now rejects invalid UTF-8, missing numeric
  temperature fields and responses without a temperature unit.
- The install/uninstall scripts now recognise multiarch `lib/*/rofi` plugin
  directories.
- The Arch PKGBUILD uses a pinned checksum for the immutable release archive.
- The RPM spec builds from the same verified release archive.

### Fixed

- Non-zero subprocess exits are now reported as failures instead of successful
  communication.
- Failed weather requests cannot leave the provider stuck; retry backoff is
  bounded and cache entries remain limited.
- The Arch package no longer uses the `SKIP` checksum placeholder.
- User configuration is preserved across package removal and installer
  uninstallation.

## [0.3.0] - 2026-11-05

### Added

- Inline weather summaries for `weather in <city>` using wttr.in's documented
  text format. Requests run asynchronously with a configurable timeout, are
  cancelled on query changes, and are cached in memory.
- `[weather] location`, `cache_seconds` and `timeout_seconds` configuration.
- External detailed-forecast fallback when inline weather is unavailable.
- RPM spec and Arch Linux PKGBUILD under `packaging/`, plus packaging notes.
- Weather unit tests using fixtures, with no live network dependency.
- CI coverage for Fedora 42 and 44 with `-Dwerror=true`, Clang, GCC
  `-fanalyzer`, ASan/UBSan and RPM build validation.

### Changed

- Web relevance now increases for natural-language queries containing terms such
  as `features`, `configuration`, `guide`, `documentation`, `install` or `error`.
- A bare `weather` query no longer silently assumes an IP-based location. Set
  `[weather] location` or include a city in the query.
- Weather results now show an inline summary and a separate external forecast
  action instead of only opening wttr.in.
- Expanded query regression tests for `python 3.14 features`, `25% of 180`,
  `weather in Lisbon`, `github hyprwhispr`, `steam` and `report.pdf`.

### Fixed

- Corrected configuration documentation that still described digits as an
  exclusive calculator trigger.
- Weather retrieval never blocks the Rofi UI and is cancelled when the query
  changes.

## [0.2.0] - 2026-11-05

### Changed

- Replaced the digit-based calculator heuristic with independent query analysis
  signals for application, calculation, web, file, clipboard, dictionary, GitHub
  and weather relevance.
- Queries such as `python3`, `firefox 123`, `2 monitors` and
  `hyprland 2 monitor` no longer trigger calculator results.
- Ranking now combines match quality, provider relevance and a local-action
  preference. Multiple providers can contribute results for the same query.
- Moved query analysis and scoring into `src/query.c`.
- Added a cancellable asynchronous process module in `src/process.c`.

### Added

- Asynchronous, cancellable file search, clipboard refresh and bookmark loading.
- Query-generation checks reject stale callbacks and obsolete subprocesses.
- Optional low-priority automatic clipboard matching
  (`[clipboard] automatic=true`).
- In-memory clipboard cache with configurable `cache_seconds`.
- Application desktop-ID deduplication and executable/keyword match signals.
- Process lifecycle tests and expanded query/ranking/configuration tests.
- Fedora CI workflow.

### Fixed

- File and clipboard search no longer block the Rofi UI.
- Bookmark loading no longer delays plugin initialization.
- Stale qalc/file results cannot replace results for a newer query.
- Obsolete subprocesses are cancelled when the query changes.
- Clipboard history is no longer queried repeatedly on every keystroke.

## [0.1.0] - 2026-11-05

Initial public release.

### Added

- Native Rofi plugin providing one live search mode.
- Automatic intent detection and relevance ranking.
- Applications, calculator, web search, files, clipboard, dictionary, GitHub,
  weather and bookmark providers.
- Asynchronous `qalc` integration for calculations, units and currency.
- `!g`, `!ddg`, `!w`, `!yt`, `!gh`, `!aur`, `!maps` and `!define` web shortcuts.
- Direct path opening and optional `fd` file search.
- `clip ` prefixed clipboard history through `cliphist`.
- Chromium-family and Firefox/Zen bookmark loading when `jq`/`sqlite3` exist.
- INI configuration with provider toggles, custom web engines and file roots.
- Unit tests for intent detection, URL building, ranking and configuration.
- Install and uninstall scripts, example configuration and documentation.

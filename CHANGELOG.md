# Changelog

All notable changes are documented here. The project follows semantic versioning
once regular releases begin.

## [Unreleased]

### Added

- The launcher now starts on the `universal` tab while retaining Rofi's
  apps/files/term/window modes. The `calc` tab is included automatically when
  `libcalc.so` is available, and `ROFI_UNIVERSAL_EXTRA_MODI` can append optional
  modes such as `web:<script>`.

## [1.1.0] - 2026-11-05

### Added

- Native C mathematical calculator with arithmetic, parentheses, powers,
  percentages, hexadecimal/binary/octal integers, constants and documented math
  functions.
- Inline Frankfurter currency conversion for explicit ISO 4217 codes, with
  asynchronous fetching, bounded in-memory caching by base currency, provider
  rate dates, stale-cache labelling and graceful failure states.
- Calculator and currency unit test suites plus a headless Xvfb integration test.
- Currency configuration for cache lifetime and request timeout.

### Validation

- 8 Meson suites / 94 tests cover native arithmetic, formatting, limits,
  currency parsing, JSON validation, precision, cache behavior, query intent,
  configuration and existing providers.
- Xvfb integration covers native calculations, inline Frankfurter conversion,
  cache reuse, slow-request isolation, failure handling, rapid query/teardown,
  file reveal, clipboard actions and FileManager1.
- GCC and Clang release builds, GCC `-fanalyzer`, ASan/UBSan, Fedora RPM,
  Arch package, Debian-family source builds, plugin loading and install/uninstall
  lifecycles remain part of the release validation matrix.

### Changed

- Removed the external calculator runtime dependency and its configuration.
- Currency results are reference rates from a public provider, not bank or live
  trading rates.
- Currency rates retain their individual provider dates because a Frankfurter
  response may contain mixed dates across currencies; narrow Rofi themes show a
  compact `Ref YYYY-MM-DD` or `Cached YYYY-MM-DD` description.
- Calculator results are evaluated locally and synchronously; currency requests
  use the existing cancellable process/fetch infrastructure.
- Documentation, example configuration, packaging metadata and CI tests now
  describe the native calculator and currency provider.

### Limitations

- Currency conversion requires `curl` or `wget` and network access when no fresh
  cached rate exists. Cached stale rates are explicitly labelled.
- Currency accepts explicit ISO 4217 codes only; ambiguous symbols such as `$`
  are rejected.
- Unit conversion and base-conversion commands are not part of v1.1.0.
- The native parser does not support implicit multiplication, variables or
  equations.
- Debian stable and Ubuntu 24.04 remain unsupported because they ship Rofi
  1.7.5.

## [1.0.0] - 2026-11-05

### Stable

- First stable release. No new providers or user-facing features were added in
  this milestone.
- Final audit completed with 6 Meson suites / 77 tests, GCC and Clang builds,
  GCC `-fanalyzer`, ASan/UBSan, Xvfb integration tests, Fedora RPM lifecycle,
  Arch package lifecycle, and Ubuntu 26.04 LTS / Debian testing source-install
  and plugin-load checks.
- Real FileManager1 reveal was manually verified on Hyprland/Wayland with
  Dolphin 26.08.1 for paths containing spaces and Unicode. Directory and
  missing-file fallback behavior was manually verified through Thunar. Exact
  item selection was not programmatically asserted.
- The provider support matrix and installation boundaries are documented in
  `docs/PROVIDERS.md` and `docs/INSTALL.md`.
- Existing configuration and keyboard behavior remain compatible: Enter,
  Shift+Enter and Alt+1 retain their established actions.

### Limitations

- Debian stable and Ubuntu 24.04 ship Rofi 1.7.5 and remain unsupported.
- Debian-family installation is source-only; no `.deb` package is provided.
- Clipboard remains text-only with a 5-second timeout and 4 MiB cap.
- GitHub inline API results and web previews remain deferred.
- FileManager1 compatibility is verified with the tested Dolphin/Thunar setup,
  not every Linux desktop.
- Benchmarks remain local reference measurements, not guarantees.

## [0.9.0] - 2026-11-05

### Added

- Source build, install, Rofi plugin loading and uninstall verified on Ubuntu
  26.04 LTS and Debian testing with Rofi 2.0.0.
- CI source-install jobs for Ubuntu 26.04 LTS and Debian testing.
- Provider support matrix in `docs/PROVIDERS.md` covering dependencies, network
  behavior, result actions, fallbacks and limitations.
- Configuration compatibility tests for empty files, invalid value types and
  unknown keys.

### Changed

- README and installation documentation now state the tested Debian/Ubuntu
  versions and explicitly mark Debian stable and Ubuntu 24.04 as unsupported
  because they ship Rofi 1.7.5, below the required 1.7.6.
- No `.deb` package is provided; Debian-family installation uses the verified
  source installer.

### Investigated / retained

- Debian stable and Ubuntu 24.04 were not claimed as supported because their
  Rofi version does not meet the plugin requirement.
- Full Debian packaging was not added; maintaining a `.deb` without an
  established package archive would add complexity without improving the
  verified source path.
- FileManager1 success remains mock-tested rather than tested against every
  desktop file manager.
- GitHub inline API results and web previews remain deferred.

## [0.8.0] - 2026-11-05

### Added

- Clipboard decode now has a bounded 5-second timeout in addition to the 4 MiB
  output cap. The sync capture uses the existing async process helper on a
  private main context, so timeout cancellation and child termination reuse the
  tested lifecycle code.
- Binary data and embedded NUL output are rejected by the shared process helper
  before provider callbacks can parse them.
- Structured query regression suite covering application, web, calculator, file,
  URL, GitHub, weather, dictionary, shortcut, negative, empty and Unicode cases.
- Calculator recognition for hexadecimal, binary and octal literals such as
  `0xff`, `0b1010` and `0o755`.
- Mock `org.freedesktop.FileManager1` success-path test under `dbus-run-session`
  that verifies the exact file URI sent by the reveal action.
- Expanded benchmark target for file matching, display formatting and result
  deduplication, with reproducible methodology in `docs/BENCHMARKS.md`.

### Changed

- Benchmark documentation now records environment, iteration counts and
  methodology; results remain local reference measurements, not CI thresholds.
- CI now runs clipboard timeout tests, the query regression suite, the
  FileManager1 success path and the existing FileManager1 fallback test.

### Investigated / retained

- File reveal continues to use FileManager1 with a default-handler fallback.
- Clipboard remains text-only; binary and oversized content are refused.
- GitHub inline API results remain deferred because unauthenticated search rate
  limits make typing-driven requests unreliable.
- No configuration changes were required.

## [0.7.0] - 2026-11-05

### Added

- `Alt+1` file action: reveal a selected file through
  `org.freedesktop.FileManager1`, with a fallback to opening its containing
  directory through the desktop's default handler. Directories open normally.
- `u_file_uri()` and `u_file_parent()` helpers with tests for spaces, Unicode,
  absolute paths and parent-directory resolution.
- Bounded synchronous clipboard decoding for explicit copy actions. Decoding is
  capped at 4 MiB; binary data and invalid UTF-8 are refused instead of being
  silently corrupted.
- Process-layer synchronous capture tests for small, exact-limit, oversized,
  binary, non-zero and missing-executable cases.
- `Alt+1` discoverability hint in the Rofi mode message when file results are
  present.
- A dedicated query-regression dataset covering application, web, calculator,
  file, URL, GitHub, weather, dictionary, shortcut and negative cases.
- Benchmark coverage for file matching, display formatting and result
  deduplication, with methodology documented in `docs/BENCHMARKS.md`.

### Changed

- File result descriptions now explicitly say `File · open · <path>`.
- Clipboard result descriptions now say `Clipboard · copy`.
- Clipboard behavior remains text-only; image/binary clipboard entries are not
  copied by the inline action.
- No new configuration options were added.

### Investigated / retained

- File-manager reveal uses the standard FileManager1 D-Bus interface and a
  default-handler fallback; no file-manager executable is hardcoded.
- Clipboard decode remains synchronous because it is an explicit action and the
  operation is now bounded. It is not moved into plugin-lifetime async jobs,
  avoiding teardown races that could cancel the copy.
- GitHub inline API results remain deferred because unauthenticated search rate
  limits make typing-driven requests unreliable.

### Deferred

- Cross-desktop automated file-manager reveal testing was not added; URI/parent
  helpers are unit-tested, while the D-Bus/default-handler action depends on the
  live desktop environment.
- Clipboard image/binary support remains out of scope.

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

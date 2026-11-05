# Changelog

All notable changes are documented here. The project follows semantic versioning
once regular releases begin.

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

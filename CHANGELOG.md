# Changelog

All notable changes are documented here. The project follows semantic versioning
once regular releases begin.

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

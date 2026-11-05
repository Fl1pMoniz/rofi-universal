# Changelog

All notable changes are documented here. The project follows semantic versioning
once regular releases begin.

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

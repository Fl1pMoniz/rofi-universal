# rofi-universal

**One launcher. One search field. Multiple capabilities. Automatic intent detection.**

`rofi-universal` is a single Rofi mode that combines applications, calculations,
currency and unit conversions, web searches, files, clipboard history, bookmarks
and a few productivity shortcuts into one live search interface. It is inspired
by KDE KRunner and macOS Spotlight, but stays lightweight and uses ordinary
command-line tools instead of a background daemon or an AI service.

Unlike Rofi script modes, it is a native Rofi plugin. Results refresh as you
type, and you do not need to switch modes or remember a prefix for normal use.

## Features

- **Live combined results** from all enabled providers as you type.
- **Independent provider relevance scoring** with simple, maintainable rules:
  intents are no longer exclusive, so one query can produce useful app, file,
  calculator, web and clipboard results at the same time.
- **Applications** from the freedesktop desktop-entry database.
- **Calculator** through `qalc` from Qalculate!: arithmetic, equations,
  percentages, units and currency conversion (`500 EUR to USD`, `10 km to miles`).
- **Web search** with DuckDuckGo, Google, Brave, Bing, Startpage or a custom URL
  template; supports URLs and `!g`, `!ddg`, `!w`, `!yt`, `!gh`, `!aur`, `!maps`
  and `!define` shortcuts.
- **Files** through direct path expansion and asynchronous optional `fd` search.
- **Clipboard history** through `cliphist`, with explicit `clip ` search and
  optional low-priority automatic matching. The history is cached in memory and
  never written to disk by the launcher.
- **Dictionary** shortcuts for `define <word>`.
- **GitHub** search for `github <query>` or `gh <query>`.
- **Weather** shortcuts for `weather in <city>` via wttr.in.
- **Bookmarks** from Chromium-family JSON stores and Firefox/Zen SQLite stores
  when `jq` and `sqlite3` are available, loaded in the background.
- **Keyboard-first**: `Enter` runs the selected result; `Shift+Enter` copies its
  URL, path, value or title to the clipboard.
- **Non-blocking lifecycle**: file searches, clipboard refresh, bookmark loading
  and `qalc` run asynchronously. New queries cancel obsolete work and stale
  callbacks are rejected by generation checks.

## Screenshot

![rofi-universal showing a currency calculation and a web action](docs/screenshot.png)

## Demo

![Animated demo: applications, calculator, currency, units, web search, files, dictionary, GitHub, weather and clipboard](docs/demo.gif)

## Requirements

### Required

- Rofi **1.7.6+** with plugin support (tested with Rofi **2.0.0** on Fedora/Nobara
  and Hyprland/Wayland; CI builds against Fedora 42's Rofi package)
- GLib/GIO development headers, a C compiler, Meson and Ninja
- `qalc` from [Qalculate!](https://qalculate.github.io/) for calculations,
  units and currency conversion

### Optional providers

| Provider | Dependency |
| --- | --- |
| Files | `fd` (recommended), `timeout` |
| Clipboard | `cliphist`, `wl-clipboard` (Wayland) or `xclip`/`xsel` (X11) |
| Bookmarks | `jq` for Chromium-family, `sqlite3` for Firefox/Zen |
| URL/file opening | `xdg-open` or the desktop's default URL handler |
| Web search | a browser registered as the default URL handler |

## Install

### Fedora / Nobara

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel qalculate
# Optional:
sudo dnf install fd-find cliphist wl-clipboard jq sqlite
```

### Arch Linux

```sh
sudo pacman -S --needed rofi meson ninja gcc pkgconf glib2 cairo qalculate
# Optional:
sudo pacman -S --needed fd cliphist wl-clipboard jq sqlite
```

### Debian / Ubuntu

```sh
sudo apt install rofi rofi-dev meson ninja-build build-essential pkg-config \
  libglib2.0-dev libcairo2-dev qalc
# Optional:
sudo apt install fd-find cliphist wl-clipboard jq sqlite3
```

### Build and install

```sh
git clone https://github.com/Fl1pMoniz/rofi-universal.git
cd rofi-universal
./scripts/install.sh
```

The default prefix is `~/.local`. Use another prefix with `PREFIX=/usr/local`.
The installer builds the plugin, installs `libuniversal.so` under
`$PREFIX/lib/rofi`, installs the `rofi-universal` launcher, and copies an example
configuration to `~/.config/rofi-universal/config.ini` if one does not exist.

Run it:

```sh
rofi-universal
```

Or add a desktop shortcut. For Hyprland:

```ini
bindd = SUPER, SPACE, Universal search, exec, rofi-universal
```

For Sway/i3:

```text
bindsym $mod+space exec --no-startup-id rofi-universal
```

### Run without installing

```sh
meson setup build
meson compile -C build
ROFI_PLUGIN_PATH="$PWD/build" rofi -show universal -modi universal
```

### Uninstall

```sh
./scripts/uninstall.sh
```

Your configuration in `~/.config/rofi-universal/` is preserved.

## Configuration

The configuration file is INI-based. The default path is:

```text
~/.config/rofi-universal/config.ini
```

Override it with `ROFI_UNIVERSAL_CONFIG=/path/to/config.ini`. To use a specific
Rofi theme, set `ROFI_UNIVERSAL_ROFI_CONFIG=/path/to/config.rasi` or pass Rofi
arguments after `rofi-universal`, for example:

```sh
rofi-universal -config ~/.config/rofi/config.rasi
```

Example:

```ini
[general]
max_results=40
qalc_binary=qalc
web_engine=duckduckgo
open_command=xdg-open

[providers]
applications=true
calculator=true
web=true
files=true
clipboard=true
dictionary=true
github=true
weather=true
bookmarks=true

[files]
roots=~
max_depth=4
max_results=8

[clipboard]
automatic=false
cache_seconds=30
max_results=10
```

`web_engine` accepts `duckduckgo`, `google`, `brave`, `bing`, `startpage`,
`wikipedia`, `youtube`, `github`, `aur`, `openstreetmap`, `dictionary`,
`wiktionary`, or a custom template containing `%s` or `{query}`.

Disable a provider by setting it to `false`. Disabled providers add no results
and are not queried.

## Keyboard shortcuts

| Key | Action |
| --- | --- |
| `Enter` | Open the selected result / copy a calculation |
| `Shift+Enter` | Copy the selected result's URL, path, value or title |
| `Up` / `Down` | Move selection |
| `PageUp` / `PageDown` | Move by a page |
| `Home` / `End` | Jump to the first/last result |
| `Escape` | Close the launcher |

## Example queries

| Query | Result |
| --- | --- |
| `25 * 18` | Calculator result |
| `500 EUR to USD` | Currency conversion |
| `10 km to miles` | Unit conversion |
| `firefox` | Application (when installed) |
| `zen` / `kitty` | Application match, with files below |
| `~/Downloads/report.pdf` | Direct file or directory open |
| `define ephemeral` | Dictionary links |
| `github hyprwhispr` | GitHub repository search |
| `weather in Tokyo` | Weather shortcut |
| `hyprland configuration` | Web search |
| `!yt linux customization` | YouTube search |
| `clip github` | Search clipboard history |
| `python` | Applications, files and web results ranked together |

## How it works

`rofi-universal` is a native Rofi mode plugin. It uses Rofi's
`_preprocess_input` callback, which runs on every edit, to rebuild a shared result
list and call `rofi_view_reload()`. That is what gives live updates in one mode.

Source layout:

```text
src/
  plugin.c       Rofi callbacks, providers, actions and result merging
  query.c        Query analysis, provider relevance and scoring signals
  process.c      Cancellable asynchronous subprocess capture
  universal.c    Result model, URL building and configuration
  universal.h    Shared interfaces
tests/
  test_core.c    Query, ranking, URL and configuration tests
  test_process.c Async process success, cancellation and failure tests
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the provider and async
lifecycle details.

Query analysis produces independent signals for applications, calculations, web
search, files, clipboard, dictionary, GitHub and weather. A query can be relevant
to several providers; there is no single provider-suppressing intent. Ranking
combines match quality, provider relevance and a local-action bonus:

1. Exact title, prefix and token matches produce a match score.
2. The query analysis adds provider relevance.
3. Local providers get a small preference over external actions.
4. Results are sorted deterministically by score, provider and title.

Async work uses a query-generation counter and a per-query `GCancellable`.
Applications and local analysis render immediately. `qalc`, `fd`, cliphist refresh
and bookmark loading run through `src/process.c`; obsolete processes are cancelled
and callbacks for older generations are ignored. Bookmark and clipboard data are
held only in memory for the plugin's lifetime.

No prompt is sent to an AI model. No daemon runs in the background. Network access
only happens when `qalc` refreshes exchange rates or when the user chooses a web
action. Bookmark stores are read locally.

## Extending

Providers share the live mode state, so their result-building code remains in
`src/plugin.c` with a shared `UResult` representation. Query signals and scoring
live in `src/query.c`; reusable process execution lives in `src/process.c`.

1. Add a score/action in the provider function.
2. Add a `U_PROVIDER_*` value in `src/universal.h` if needed.
3. Add provider relevance in `src/query.c`.
4. Use `u_process_run()` for subprocess work; never block the UI.
5. Add a test in `tests/test_core.c` or `tests/test_process.c`.
6. Run `meson test -C build` and an interactive smoke test.

Please keep new providers local-first and dependency-tolerant. A missing tool
must never prevent the rest of the launcher from working.

## Testing

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

The core tests check query analysis, URL encoding, provider relevance, ranking,
configuration and invalid input. The process tests check asynchronous success,
cancellation and spawn failures without network access. An interactive smoke test
can be run on Wayland:

```sh
ROFI_PLUGIN_PATH="$PWD/build" rofi -show universal -modi universal
```

Then try the example queries above.

## Troubleshooting

**`Mode 'universal' not found`**
The plugin path is wrong. `rofi-universal` sets `ROFI_PLUGIN_PATH` automatically.
If launching Rofi directly, point `-plugin-path` or `ROFI_PLUGIN_PATH` at the
directory containing `libuniversal.so`.

**Calculator does nothing**
Install `qalculate` and make sure `qalc` is on `PATH`, or set `qalc_binary` in the
configuration. The first currency lookup may take a moment while qalc refreshes
exchange rates.

**Files do not appear**
Install `fd`. File search is limited by `max_depth`, `max_results` and a short
timeout so the launcher stays responsive. Direct paths always work.

**Clipboard results are empty**
Install `cliphist` and `wl-clipboard`. Explicit search uses the `clip ` prefix.
Set `[clipboard] automatic=true` to include low-ranked clipboard matches in normal
searches. Automatic matching is off by default because clipboard history may
contain sensitive data. Disable the provider entirely with
`providers.clipboard=false`.

**Bookmarks are missing**
Only Chromium-family JSON bookmarks and Firefox/Zen SQLite bookmarks are read.
Install `jq` and/or `sqlite3`.

**Theme looks wrong**
Pass your Rofi config with `ROFI_UNIVERSAL_ROFI_CONFIG` or `-config`. The plugin
does not ship a theme and preserves the look of your existing Rofi setup.

## License

Original code is licensed under the [MIT License](LICENSE).
Copyright (c) 2026 Fl1pMoniz.

External tools are invoked through their command-line interfaces and are not
bundled. Their licenses remain their own: Rofi (MIT/X11), Qalculate!/qalc
(GPL-2.0-or-later), fd (MIT/Apache-2.0 and Unicode-DFS-2016), cliphist
(GPL-3.0-only), jq (MIT and ICU/CC-BY-3.0), SQLite (public domain),
wl-clipboard (GPL-3.0-or-later), xclip/xsel (GPL/MIT as applicable).

## Acknowledgements

- [Rofi](https://github.com/davatorium/rofi) for the plugin API and the original
  launcher experience.
- [rofi-calc](https://github.com/svenstaro/rofi-calc) for demonstrating live
  plugin updates; this project does not copy its source.
- [Qalculate!](https://qalculate.github.io/) for the calculation engine.
- [rofi-web-search](https://github.com/pdonadeo/rofi-web-search) for showing what
  a small web-search mode can do.

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md) and
[CHANGELOG.md](CHANGELOG.md).

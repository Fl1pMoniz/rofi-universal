# Configuration

`rofi-universal` reads an INI file from:

```text
~/.config/rofi-universal/config.ini
```

You can override the path with `ROFI_UNIVERSAL_CONFIG`. The installer copies
`data/config.example.ini` there on first install; existing files are never
overwritten.

## General

```ini
[general]
max_results=40
qalc_binary=qalc
web_engine=duckduckgo
open_command=xdg-open
```

- `max_results` is clamped to 5–200.
- `qalc_binary` may be a command name or an absolute path.
- `web_engine` accepts a built-in name or a URL template containing `%s` or
  `{query}`.
- `open_command` is used only when the desktop cannot open a URI itself.

Built-in web engines: `duckduckgo`, `google`, `brave`, `bing`, `startpage`,
`wikipedia`, `youtube`, `github`, `aur`, `openstreetmap`, `dictionary`,
`wiktionary`.

Custom example:

```ini
web_engine=https://search.example/?q=%s
```

## Providers

```ini
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
```

Set any value to `false` to disable the provider.

By default the clipboard provider is only queried when the input starts with
`clip ` or `clipboard `. Set `[clipboard] automatic=true` to include matching
entries in normal searches at a deliberately low rank. This avoids leaking
clipboard contents into unrelated searches. The provider also respects the
dedicated `clipboard=false` switch.

## Files

```ini
[files]
roots=~
max_depth=4
max_results=8
```

`roots` is a colon-separated list. Each root is searched with `fd` when the query
looks like an application or file name. The search is bounded by a short timeout,
`max_depth` and `max_results`. Direct paths such as `~/Downloads/report.pdf` are
opened without `fd`.

## Clipboard

```ini
[clipboard]
automatic=false
cache_seconds=30
max_results=10
```

Only entries matching `clip <text>` are shown by default. Selecting an entry
decodes it with `cliphist` and copies it to the Wayland clipboard with `wl-copy`
(or X11 `xclip`/`xsel`).

Set `automatic=true` to also include clipboard matches in normal searches. These
results are deliberately ranked below applications and calculations. Automatic
matching is off by default because clipboard history can contain passwords or
private text. The cliphist list is cached in memory for `cache_seconds` and is
never written to a persistent cache by the launcher.

## Weather

```ini
[weather]
location=
cache_seconds=600
timeout_seconds=4
```

`weather in <city>` asks wttr.in for a compact summary and displays it directly in
Rofi when `curl` or `wget` is available. The request runs asynchronously with
`timeout_seconds` and is cancelled when the query changes. A detailed forecast
action remains available, and is the fallback when inline retrieval fails.

`location` is used only for a bare `weather` query. If it is empty, use
`weather in <city>`; the plugin does not silently guess a location from the
network address. `cache_seconds` controls the in-memory cache. No weather data is
written to disk by the plugin.

## Ranking and query signals

Query analysis is intentionally small and predictable:

- URLs, leading `!` bangs and `github`/`gh` prefixes are recognized as web/GitHub
  signals.
- `define`, `weather`, `clip` and path queries have dedicated signals.
- Calculator signals require arithmetic, percentage, unit, currency or base
  conversion patterns; a bare number inside a name such as `python3` or
  `firefox 123` is not a calculation.
- A single word with no special form gets a strong application signal.
- Multi-word natural-language queries get a web signal; words such as `features`,
  `configuration`, `guide`, `documentation`, `install` and `error` strengthen it.

Each provider receives an independent relevance signal. A query can be relevant
to applications, files, calculations, web search and clipboard at the same time;
there is no single intent that suppresses the others. Providers score matches by
exact title, prefix, keyword, executable and token matches. Ranking combines
provider relevance, match quality and a small local-action preference.

Local results render immediately. qalc, fd, clipboard refresh, bookmark loading
and inline weather retrieval run asynchronously. A new query cancels obsolete
work, and callbacks from older query generations are ignored.

## Rofi theme

`rofi-universal` does not replace your Rofi theme. Use the launcher's
`ROFI_UNIVERSAL_ROFI_CONFIG` environment variable:

```sh
ROFI_UNIVERSAL_ROFI_CONFIG="$HOME/.config/rofi/config.rasi" rofi-universal
```

or pass a config directly:

```sh
rofi-universal -config "$HOME/.config/rofi/config.rasi"
```

The mode is named `universal`, so a manual launch looks like:

```sh
ROFI_PLUGIN_PATH="$HOME/.local/lib/rofi" \
  rofi -show universal -modi universal
```

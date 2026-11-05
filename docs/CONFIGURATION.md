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

The clipboard provider is only queried when the input starts with `clip ` or
`clipboard `. This avoids leaking clipboard contents into unrelated searches.
It also respects the dedicated `clipboard=false` switch.

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
max_results=10
```

Only entries matching `clip <text>` are shown. Selecting an entry decodes it with
`cliphist` and copies it to the Wayland clipboard with `wl-copy` (or X11
`xclip`/`xsel`).

## Ranking and intent detection

Intent detection is intentionally small and predictable:

- URLs, leading `!` bangs and `github`/`gh` prefixes are recognized as web/GitHub
  intents.
- `define`, `weather`, `clip` and path queries have dedicated intents.
- Queries containing digits or math functions go to the calculator.
- A single word with no special form is treated as an application query.
- Other multi-word queries are web queries.

The detected intent adds a provider-specific score boost. Providers also score
matches by exact title, prefix and substring. Local results never wait for the
network. qalc runs asynchronously and its result is inserted when ready.

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

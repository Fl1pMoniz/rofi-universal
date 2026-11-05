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
`wiktionary`, `arch`, `man`, `docs`, `pkg`.

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
opened without `fd`. Filename matches rank above parent-path matches. Queries
containing glob characters use `fd --glob`, for example `*.png`, `report?.pdf` or
`[ab].txt`.

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

## GitHub and web shortcuts

`github <query>` or `gh <query>` shows repository, user and issue search actions.
Category-led forms put the requested category first:

```text
github issues rofi
github users torvalds
github repos rofi plugins
```

A query in `owner/repository` form also offers direct repository opening:
`github Fl1pMoniz/rofi-universal`.

Web bangs include `!arch` (Arch Wiki), `!man` (Arch man pages), `!docs`
(DevDocs) and `!pkg` (Arch packages), in addition to the general search-engine
and site-specific bangs listed above. These open browser actions; they do not
scrape result pages.

## Dictionary

```ini
[dictionary]
cache_seconds=86400
timeout_seconds=4
```

Dictionary queries are recognized in these forms:

```text
define ephemeral
definition ephemeral
definition of ephemeral
ephemeral meaning
ephemeral definition
what does ephemeral mean
meaning of ephemeral
```

When `curl` or `wget` is available, the plugin requests a compact definition
from the public dictionaryapi.dev service and shows up to two definitions inline
with part of speech, phonetics and examples where available. Selecting an inline
result opens a detailed Dictionary.com reference; `Shift+Enter` copies the
definition. Wiktionary and Dictionary.com browser actions remain as fallbacks.

`cache_seconds` is clamped to 60–604800 seconds. `timeout_seconds` is clamped to
1–15 seconds. Only the requested word is sent to dictionaryapi.dev; no local
files, application lists or clipboard contents are sent. Results are cached in
memory only.

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
network address. `cache_seconds` controls the bounded in-memory cache; the
oldest entry is evicted when the cache is full. No weather data is written to
disk by the plugin.

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
  `configuration`, `guide`, `documentation`, `install`, `examples`,
  `alternatives`, `comparison`, `review`, `reference`, `manual`, `download` and
  `error` strengthen it.
- Application matching uses the strongest name, executable, keyword or desktop
  term match. Description-only matches are discounted, and a query that begins
  with an application name gets a lower boost than an exact application match.

Each provider receives an independent relevance signal. A query can be relevant
to applications, files, calculations, web search and clipboard at the same time;
there is no single intent that suppresses the others. Providers score matches by
exact title, prefix, keyword, executable and token matches. Ranking combines
provider relevance, match quality and a small local-action preference.

Results with the same provider and target are deduplicated, keeping the higher
score. For example, an inline weather summary replaces the redundant external
forecast action when both point to the same forecast URL. Inline dictionary
results use per-sense deduplication keys so multiple definitions remain visible
while the redundant Dictionary.com fallback is replaced.

Subprocess stdout and stderr are each capped at 4 MiB. Exceeding the cap marks the
operation as failed and terminates the child; truncated output is never parsed as
a successful provider result.

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

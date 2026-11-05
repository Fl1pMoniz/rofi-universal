# Provider support matrix

This matrix describes the providers shipped with rofi-universal 0.9.0. Status
reflects the current implementation and test coverage, not a promise of
compatibility with every desktop environment.

| Provider | Status | Dependencies | Network | Result action | Fallback |
| --- | --- | --- | --- | --- | --- |
| Applications | Stable | freedesktop desktop entries via GIO | No | Launch application | No result if no matching entry |
| Files | Stable with limitations | `fd` optional; default desktop handler | No | Open file/directory; Alt+1 reveal | Direct path open; containing-directory fallback |
| Calculator | Stable with limitations | `qalc` required | qalc exchange rates may use network | Copy result | No calculator result; web fallback |
| Web search | Stable with limitations | Default browser/URL handler | Only when browser opens a search | Open browser URL | No inline result |
| Clipboard | Stable with limitations | `cliphist`; `wl-copy` or `xclip`/`xsel` | No | Copy selected text | No result when unavailable |
| Dictionary | Stable with limitations | `curl` or `wget`; dictionaryapi.dev | Yes, requested word only | Open detailed reference | Browser links |
| GitHub | Stable with limitations | Default browser/URL handler | Only when browser opens a search | Open GitHub search/repository | No inline API results |
| Weather | Stable with limitations | `curl` or `wget`; wttr.in | Yes, requested city only | Open detailed forecast | External forecast link |
| Bookmarks | Stable with limitations | `jq` and/or `sqlite3` | No | Open bookmark | Provider unavailable if tools/formats absent |

## Applications

- Examples: `firefox`, `steam`, `terminal`, `browser`.
- Matches names, generic names, keywords, desktop IDs and executables.
- Hidden entries are excluded through `g_app_info_should_show()`.
- Launching uses GDesktopAppInfo and argument arrays, never shell interpolation.
- A failed launch currently produces no result-side error message.

## Files

- Examples: `report.pdf`, `~/Downloads/report.pdf`, `config.ini`, `*.png`.
- Direct paths are opened without `fd`; other searches use bounded asynchronous
  `fd` with configured roots, depth and result limits.
- File name matches rank above parent-path matches.
- Alt+1 reveals through `org.freedesktop.FileManager1` when available.
- If FileManager1 is unavailable, the containing directory is opened through the
  default desktop handler.
- File-manager success depends on the live desktop; CI uses a mock service for
  the protocol and a fake handler for the fallback.
- No reveal-in-file-manager action is available when both FileManager1 and the
  default handler are unavailable.

## Calculator

- Examples: `25 * 18`, `25% of 180`, `10 km to miles`, `10 EUR to USD`,
  `0xff`, `0b1010`, `0o755`.
- Uses `qalc` and preserves qalc's formatting.
- The first currency conversion may refresh exchange rates and require network
  access.
- Calculation detection is deterministic and intentionally conservative.
- If `qalc` is missing or fails, no calculator result is shown.

## Web search

- Examples: `linux hyprland configuration`, `!yt linux customization`,
  `!arch package-name`, `github.com/rofi`.
- Supports configured engines and the documented bang shortcuts.
- Opens the default browser or URL handler.
- No HTML scraping, search-result previews or undocumented search APIs.
- A missing browser or URL handler fails silently at the action boundary.

## Clipboard

- Examples: `clip github`, `clip token`.
- Uses `cliphist` for history and `wl-copy`, `xclip` or `xsel` for copying.
- Automatic matching in normal searches is opt-in and off by default.
- History is held in memory only and never logged or persisted by the launcher.
- Decoding is text-only, capped at 4 MiB and has a 5-second timeout.
- Binary, embedded-NUL and invalid-UTF-8 selections are refused.
- Clipboard contents are not sent to network providers.

## Dictionary

- Examples: `define ephemeral`, `definition of ephemeral`, `ephemeral meaning`,
  `what does ephemeral mean`.
- Uses the public dictionaryapi.dev v2 endpoint through `curl` or `wget`.
- Only the requested word is sent; no API key is required.
- Shows up to two definitions inline with part of speech, phonetics and examples
  where available.
- Results are cached in memory; failed lookups back off briefly.
- Dictionary.com and Wiktionary browser links remain as fallbacks.
- The inline provider is English-only.

## GitHub

- Examples: `github hyprwhispr`, `github issues rofi`,
  `github owner/repository`.
- Opens repository, user and issue searches in the browser.
- Category-led queries put the requested category first.
- No GitHub token, API call, rate-limit handling or inline API results are used.
- The current browser-based behavior is intentional and reliable; inline API
  results remain deferred.

## Weather

- Examples: `weather in Tokyo`, `weather` with `[weather] location`.
- Uses wttr.in through `curl` or `wget` and sends only the requested city.
- Shows a compact inline summary when retrieval succeeds.
- Results are cached in memory; failed cities back off briefly.
- A detailed forecast action remains available as the fallback.
- `[weather] location` is used only for a bare `weather` query; no IP-based
  location is assumed.

## Bookmarks

- Loads Chromium-family JSON bookmarks with `jq` and Firefox/Zen SQLite
  bookmarks with `sqlite3`.
- Loads in the background and matches bookmark titles and URLs.
- Uses the default browser/URL handler to open results.
- Unsupported bookmark stores or missing helper tools simply yield no bookmark
  results.
- Bookmarks remain subject to the supported store formats; there is no browser
  extension or account integration.

## Disabling providers

Every provider can be disabled under `[providers]` in the configuration file.
Missing optional dependencies do not prevent local applications, files or
calculations from working.

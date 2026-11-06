# Provider support matrix

This matrix describes the providers shipped with rofi-universal 1.1.0. Status
reflects the current implementation and test coverage, not a promise of
compatibility with every desktop environment.

| Provider | Status | Dependencies | Network | Result action | Fallback |
| --- | --- | --- | --- | --- | --- |
| Applications | Stable | freedesktop desktop entries via GIO | No | Launch application | No result if no matching entry |
| Files | Stable with limitations | `fd` optional; default desktop handler | No | Open file/directory; Alt+1 reveal | Direct path open; containing-directory fallback |
| Calculator | Stable | Native C evaluator; no external binary | No | Copy result | Invalid expressions yield no calculator result |
| Currency | Stable with limitations | `curl` or `wget`; Frankfurter | Yes, base currency only | Copy converted amount | Cached rate; status notice when unavailable |
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
  the protocol and a fake handler for the fallback. Manual Hyprland/Wayland
  testing with Dolphin 26.08.1 opened Dolphin through `ShowItems` for a temporary
  file containing spaces and Unicode; a directory and a missing-file fallback
  opened through Thunar's default handler. Exact item selection was not
  programmatically asserted.
- No reveal-in-file-manager action is available when both FileManager1 and the
  default handler are unavailable.

## Calculator

- Examples: `25 * 18`, `100 / 4`, `sqrt(144)`, `2^10`, `(25 + 15) * 2`,
  `25% of 180`, `0xff`, `0b1010`, `0o755`.
- Implemented natively in C; it never invokes a shell, another calculator, Python, JavaScript
  or another program.
- Supports `+`, `-`, `*`, `/`, `%`, `^`, parentheses, unary signs, decimals,
  scientific notation, `pi`, `e`, and `sqrt`, `abs`, `sin`, `cos`, `tan`,
  `asin`, `acos`, `atan`, `log`, `log10`, `ln`, `exp`, `floor`, `ceil`,
  `round`.
- Exponentiation is right-associative. `-2^2` is `-(2^2)` and `2^3^2` is
  `2^(3^2)`. Implicit multiplication is not supported. `log` and `ln` are
  natural logarithms; `log10` is base 10. `round` rounds halfway cases away
  from zero.
- `%` is binary modulo when followed by an operand. A trailing `%` is a
  percentage and `X% of Y` is supported.
- Division/modulo by zero, invalid domains, overflow, non-finite results,
  excessive length, excessive nesting and excessive token counts are rejected.
- Results are formatted with a locale-independent decimal separator and no
  unnecessary trailing zeroes.
- No external calculator executable is required at build, install or runtime.
- Unit conversion and base-conversion commands are not part of v1.1.0.

## Currency

- Examples: `300 eur to USD`, `100 USD to EUR`, `50 gbp in eur`,
  `1000 JPY to USD`, `25 usd in eur`.
- Uses the public Frankfurter v2 API at `api.frankfurter.dev`.
- Only the base currency code is sent. No API key, account or credentials are
  required.
- Results are asynchronous and cached in memory by base currency. Fresh results
  show a compact `Ref YYYY-MM-DD` description; the provider is Frankfurter.
- Cached-but-expired rates may be shown while a refresh is attempted or after a
  refresh failure; their description says `Cached YYYY-MM-DD`.
- If no usable cached rate exists and the request fails, no conversion result is
  shown and the status line reports the failure. A normal web search may remain
  available, but it is not presented as a conversion.
- Frankfurter rates are blended reference rates, not bank, commercial,
  transaction or live trading rates. Rates update daily and may lag the market.
- Currency symbols such as `$` are not accepted. Use explicit ISO 4217 codes.
- Zero and negative amounts are accepted and converted arithmetically.
- Supported codes: AED, AFN, ALL, AMD, ANG, AOA, ARS, AUD, AWG, AZN, BAM, BBD, BDT, BGN, BHD, BIF, BMD, BND, BOB, BRL, BSD, BTN, BWP, BYN, BZD, CAD, CDF, CHF, CLP, CNY, COP, CRC, CUP, CVE, CZK, DJF, DKK, DOP, DZD, EGP, ERN, ETB, EUR, FJD, FKP, GBP, GEL, GHS, GIP, GMD, GNF, GTQ, GYD, HKD, HNL, HTG, HUF, IDR, ILS, INR, IQD, IRR, ISK, JMD, JOD, JPY, KES, KGS, KHR, KMF, KPW, KRW, KWD, KYD, KZT, LAK, LBP, LKR, LRD, LSL, LYD, MAD, MDL, MGA, MKD, MMK, MNT, MOP, MRU, MUR, MVR, MWK, MXN, MYR, MZN, NAD, NGN, NIO, NOK, NPR, NZD, OMR, PAB, PEN, PGK, PHP, PKR, PLN, PYG, QAR, RON, RSD, RUB, RWF, SAR, SBD, SCR, SDG, SEK, SGD, SHP, SLE, SOS, SRD, SSP, STN, SVC, SYP, SZL, THB, TJS, TMT, TND, TOP, TRY, TTD, TWD, TZS, UAH, UGX, USD, UYU, UZS, VES, VND, VUV, WST, XAF, XCD, XOF, XPF, YER, ZAR, ZMW.
- Currencies with zero minor units: BIF, CLP, DJF, GNF, ISK, JPY, KMF, KRW, PYG, RWF, UGX, VND, VUV, XAF, XOF, XPF.
- Currencies with three minor units: BHD, IQD, JOD, KWD, LYD, OMR, TND.

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

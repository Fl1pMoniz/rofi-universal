# Security policy

## Supported versions

The latest release on the `main` branch receives security fixes.

## Reporting a vulnerability

Please report security issues privately through GitHub's **Security advisories**
for this repository. Do not open a public issue for a vulnerability before a fix
is available.

Include:

- A description of the issue and its impact.
- Steps to reproduce.
- The affected version or commit.
- Whether the issue requires a malicious local user, a crafted query, a crafted
  desktop file, or another precondition.

## Security notes

`rofi-universal` is a local launcher, but it touches sensitive surfaces:

- **Clipboard history** is searched explicitly with the `clip ` prefix. Optional
  automatic matching is disabled by default (`[clipboard] automatic=false`) and
  gives clipboard results a low ranking. Clipboard entries can contain passwords,
  tokens or private text. Users can disable the provider with
  `providers.clipboard=false`. The cliphist list is held in memory only and is not
  logged or written to a persistent cache by the launcher.
- **Subprocesses** are spawned with argument arrays, not through a shell. The
  configured `open_command` is parsed with `g_shell_parse_argv`; treat the
  configuration file as trusted local input. Asynchronous subprocesses are
  cancellable, non-zero exits are treated as failures, and callbacks are ignored
  when their query generation is stale or the plugin has been destroyed.
  Subprocess stdout is buffered in memory; queries and expected provider output
  are small, but extremely large output from an optional external tool is not
  incrementally capped.
- **Network access** is limited to qalc exchange-rate updates, optional inline
  weather retrieval, and opening web actions chosen by the user. Inline weather
  sends the requested city to wttr.in through `curl` or `wget`; it is not used
  for queries without a weather signal. Weather summaries are cached only in
  memory. The launcher does not send queries to an AI service.
- **Desktop files and bookmarks** are local data. A malicious desktop entry can
  still launch a malicious command when selected; the launcher does not execute
  desktop entries without a user action.
- **Calculation output** comes from qalc and is copied to the clipboard only when
  the user selects the calculation result.

No credentials or secrets are collected by the project. Do not include secrets in
bug reports, screenshots, logs or test fixtures.

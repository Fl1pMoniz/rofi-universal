# Contributing

Thanks for helping improve `rofi-universal`.

## Development setup

Required tools:

- Rofi 1.7.6+ with development headers (`rofi-devel`, `rofi-dev`, or the headers
  included in the distribution's Rofi package)
- Meson, Ninja, a C11 compiler, pkg-config
- GLib/GIO and Cairo development headers
- `qalc` for calculator integration (optional for building, required for the
  calculator provider at runtime)

On Fedora/Nobara:

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel qalculate
```

Build and test:

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

Run from the build tree:

```sh
ROFI_PLUGIN_PATH="$PWD/build" rofi -show universal -modi universal
```

## Pull requests

- Keep the launcher lightweight. Do not add a daemon, database or AI service for
  basic search classification.
- Prefer documented command-line tools over copied source code.
- Local results must not block on network requests.
- Missing optional dependencies must degrade gracefully.
- Add or update tests in `tests/test_core.c` for query analysis, ranking,
  configuration or URL changes. Add process tests in `tests/test_process.c` for
  async behavior.
- Run `meson test -C build --print-errorlogs` and a manual Rofi smoke test before
  opening a pull request.
- Keep documentation and `data/config.example.ini` in sync with configuration
  changes.
- Follow the existing C style: 2-space indentation, GLib memory helpers, no
  unnecessary abstractions.

## Adding a provider

1. Add a provider enum and icon/name mapping in `src/universal.h` and
   `src/universal.c` if necessary.
2. Add intent detection and a ranking boost in `src/universal.c`.
3. Add a provider function and call it from `rebuild_results()` in
   `src/plugin.c`.
4. Use `UResult` actions instead of shell command strings. Do not introduce
   shell interpolation of user input.
5. Add a test and document the provider in `README.md`,
   `docs/CONFIGURATION.md`, and the example configuration.

## Licensing

By contributing, you agree that original code you add is licensed under the MIT
License. Do not copy code from projects with incompatible or unclear licenses.
External command-line tools are fine to integrate through their documented
interfaces; retain their attribution in the README and release notes.

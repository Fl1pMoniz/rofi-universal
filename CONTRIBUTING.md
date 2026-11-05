# Contributing

Thanks for helping improve `rofi-universal`.

## Development setup

Required tools:

- Rofi 1.7.6+ with development headers (`rofi-devel`, `rofi-dev`, or the headers
  included in the distribution's Rofi package)
- Meson, Ninja, a C11 compiler, pkg-config
- GLib/GIO, JSON-GLib and Cairo development headers
- `qalc` for calculator integration (optional for building, required for the
  calculator provider at runtime)

On Fedora/Nobara:

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel json-glib-devel qalculate
```

Build and test:

```sh
meson setup build --buildtype=release -Dwerror=true
meson compile -C build
meson test -C build --print-errorlogs
```

Packaging files are under `packaging/`. The Fedora RPM and Arch PKGBUILD are
built and lifecycle-tested in CI. Release archives are generated with
`scripts/make-source-tarball.sh`; update the pinned checksum and release-asset URL
together and follow `packaging/README.md`.

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
- Add or update tests in `tests/test_core.c` for query analysis, application/file
  matching, dictionary query forms, ranking, deduplication, configuration or URL
  changes. Add presentation/display-text tests in `tests/test_core.c`. Add
  file URI/parent tests in `tests/test_core.c`, process/output-limit and bounded
  synchronous clipboard-capture tests in `tests/test_process.c`, weather
  parsing/cache tests in `tests/test_weather.c`, fetcher tests in
  `tests/test_fetch.c`, and dictionary JSON/cache tests in
  `tests/test_dictionary.c`.
- Run `tests/test_rapid_queries.py` under Xvfb when changing lifecycle, weather
  request handling or plugin teardown. It requires `xdotool`.
- Run `tests/test_file_actions.py` under Xvfb when changing file actions. It
  verifies the Alt+1 directory fallback with a fake handler.
- Run `tests/test_filemanager_dbus.py` under `dbus-run-session` and Xvfb when
  changing FileManager1 reveal behavior. It uses a mock service and requires
  PyGObject.
- Run `tests/test_clipboard_actions.py` under Xvfb when changing clipboard
  decode/copy behavior. It uses fake cliphist/wl-copy tools.
- Use `meson compile -C build benchmark-core && ./build/benchmark-core` for a
  quick local query-analysis/matching/formatting/dedup sanity check; benchmarks
  are not CI gates. Methodology and limitations are in `docs/BENCHMARKS.md`.
- Run `meson test -C build --print-errorlogs` and a manual Rofi smoke test before
  opening a pull request. CI also runs source-install/plugin-load checks on
  Ubuntu 26.04 LTS and Debian testing.
- Keep documentation and `data/config.example.ini` in sync with configuration
  changes.
- Follow the existing C style: 2-space indentation, GLib memory helpers, no
  unnecessary abstractions.

## Adding a provider

1. Add a provider enum and icon/name mapping in `src/universal.h` and
   `src/universal.c` if necessary.
2. Add independent query signals and relevance in `src/query.c`.
3. Add a provider function and call it from `rebuild_local_results()` in
   `src/plugin.c`.
4. Use `UResult` actions instead of shell command strings. Do not introduce
   shell interpolation of user input.
5. Use `u_process_run()` for asynchronous work and reject stale query
   generations.
6. Add a test and document the provider in `README.md`,
   `docs/CONFIGURATION.md`, and the example configuration.
7. If results can collide with an existing provider action, use a stable payload
   or `u_result_set_dedup_key()` so `u_results_dedup()` can keep the
   higher-scoring result.
8. Bound subprocess output with `u_process_run()` or
   `u_process_run_limited()`; do not parse output from a failed/truncated process.
9. Network providers must use the shared fetch/process helpers, cache bounded
   results in memory, and reject stale query generations.
10. File actions should use the default desktop handler or the standard
    FileManager1 D-Bus interface; do not hardcode a file-manager executable or
    invoke a shell.

## Licensing

By contributing, you agree that original code you add is licensed under the MIT
License. Do not copy code from projects with incompatible or unclear licenses.
External command-line tools are fine to integrate through their documented
interfaces; retain their attribution in the README and release notes.

# Installation

## Tested environments

- Fedora 42 with Rofi 1.7.9: CI compile, Xvfb plugin load and clean binary RPM
  install/upgrade/removal.
- Fedora 44 with Rofi 2.0.0: CI compile, Xvfb plugin load and clean binary RPM
  install/upgrade/removal.
- Arch Linux current with Rofi 2.0.0: CI package build, install, Xvfb plugin
  load, `namcap` and removal.
- Ubuntu 26.04 LTS with Rofi 2.0.0: source build, install, plugin load and
  uninstall verified.
- Debian testing with Rofi 2.0.0: source build, install, plugin load and
  uninstall verified.
- Local Wayland/Hyprland with Rofi 2.0.0.

Debian stable and Ubuntu 24.04 ship Rofi 1.7.5 and are not supported.

See [PROVIDERS.md](PROVIDERS.md) for provider-specific dependencies and status.

The calculator is implemented natively and has no external runtime dependency.
Currency conversion is optional and uses `curl` or `wget` with network access to
`api.frankfurter.dev`; it is disabled when no fetcher or network is available.

## Distribution dependencies

### Fedora / Nobara — prebuilt RPM

Tagged releases provide x86_64 RPMs for Fedora 42 and Fedora 44. Install the
package matching your Fedora release; no compiler, Meson, Ninja or development
headers are needed:

```sh
sudo dnf install ./rofi-universal-<version>-<release>.fcXX.x86_64.rpm
rpm -q rofi-universal
rofi -show universal -modi universal
```

Verify the RPM against the release `SHA256SUMS` before installation. Upgrade
with `sudo dnf upgrade ./rofi-universal-...rpm` and remove with
`sudo dnf remove rofi-universal`. The RPM does not install or remove user
configuration. Fedora 42 and 44 use different Rofi ABIs, so use the matching
package and do not mix `fc42`/`fc44` RPMs. Nobara releases matching those Fedora
versions can use the same package. aarch64 packages are not yet published.

The release RPMs are currently unsigned. SHA-256 verifies integrity over HTTPS,
not publisher identity. Do not disable DNF GPG checks globally or use
`--nogpgcheck` as a normal installation method; if your policy requires signed
packages, use the source-build fallback or wait for a signed release. DNF may
warn that OpenPGP checks were skipped for the local unsigned RPM; that is not a
signature verification.

If no compatible RPM exists, use the source-build fallback below.

### Fedora / Nobara — source-build fallback

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel json-glib-devel
```

Optional providers:

```sh
sudo dnf install fd-find cliphist wl-clipboard jq sqlite curl
```

### Arch Linux

```sh
sudo pacman -S --needed rofi meson ninja gcc pkgconf glib2 cairo json-glib
sudo pacman -S --needed fd cliphist wl-clipboard jq sqlite curl
```

### Ubuntu 26.04 LTS / Debian testing

```sh
sudo apt install rofi rofi-dev meson ninja-build build-essential pkg-config \
  libglib2.0-dev libcairo2-dev libjson-glib-dev
sudo apt install fd-find cliphist wl-clipboard jq sqlite3 curl
```

Source build, install, plugin loading and uninstall were verified in Ubuntu
26.04 LTS and Debian testing with Rofi 2.0.0. No `.deb` package is currently
provided.

Debian stable and Ubuntu 24.04 ship Rofi 1.7.5, below the required Rofi 1.7.6,
and are not supported. CI also runs the source-install and plugin-load lifecycle
on Ubuntu 26.04 LTS and Debian testing.

## Build and install

The recommended prefix is `~/.local` so no root access is needed:

```sh
git clone https://github.com/Fl1pMoniz/rofi-universal.git
cd rofi-universal
./scripts/install.sh
```

Install system-wide:

```sh
PREFIX=/usr/local ./scripts/install.sh
```

The script runs Meson, installs the plugin and launcher, and copies
`data/config.example.ini` to `~/.config/rofi-universal/config.ini` if needed.

## Manual build

```sh
meson setup build --prefix="$HOME/.local" --buildtype=release
meson compile -C build
meson install -C build
```

The plugin is installed in the Meson plugin directory, such as
`$PREFIX/lib/rofi/libuniversal.so`, `$PREFIX/lib64/rofi/libuniversal.so` or a
multiarch `$PREFIX/lib/*/rofi/libuniversal.so`. The `rofi-universal` launcher
discovers all of these locations.

## Distribution packages

The tag-driven `.github/workflows/release-rpm.yml` workflow builds Fedora 42/44
x86_64 RPMs, verifies the source checksum, tests clean installation, upgrade and
removal with no build tooling present, and uploads the RPM plus `SHA256SUMS` as
a workflow artifact. It does not publish a release automatically.

Fedora/Nobara RPM packaging lives in `packaging/rofi-universal.spec`:

```sh
sudo dnf builddep packaging/rofi-universal.spec
rpmbuild -ba packaging/rofi-universal.spec
```

Arch Linux packaging lives in `packaging/PKGBUILD`:

```sh
cd packaging
makepkg -si
```

The RPM is built, installed, plugin-loaded, upgraded and removed in Fedora 42/44
CI. The PKGBUILD is built, installed, plugin-loaded and removed in an Arch Linux
container CI. Neither package is published in official Fedora/Nobara or Arch
repositories. No `.deb` package is provided;
Ubuntu 26.04 LTS and Debian testing use the source installer. See
[../packaging/README.md](../packaging/README.md) for release archive and checksum
details.

## Running without installation

```sh
meson setup build
meson compile -C build
ROFI_PLUGIN_PATH="$PWD/build" rofi -show universal -modi universal
```

Pass your Rofi configuration explicitly:

```sh
ROFI_PLUGIN_PATH="$PWD/build" rofi \
  -config "$HOME/.config/rofi/config.rasi" \
  -show universal -modi universal
```

## Desktop shortcut

Hyprland:

```ini
bindd = SUPER, SPACE, Universal search, exec, rofi-universal
```

Sway/i3:

```text
bindsym $mod+space exec --no-startup-id rofi-universal
```

GNOME/KDE custom shortcut: bind the command `rofi-universal`.

## Uninstallation

```sh
./scripts/uninstall.sh
```

This removes the plugin, launcher and installed example configuration. It
leaves `~/.config/rofi-universal/` untouched. Remove that directory manually if
you also want to remove your configuration.

For distribution packages, use the package manager's removal command
(`sudo dnf remove rofi-universal` or `sudo pacman -R rofi-universal`). Your
configuration directory is not removed by the package.

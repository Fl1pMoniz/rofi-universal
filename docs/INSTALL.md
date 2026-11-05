# Installation

## Tested environments

- Fedora 42 with Rofi 1.7.9: CI compile and Xvfb plugin load.
- Fedora 44 with Rofi 2.0.0: CI compile and Xvfb plugin load.
- Arch Linux current with Rofi 2.0.0: CI package build, install, Xvfb plugin
  load, `namcap` and removal.
- Local Wayland/Hyprland with Rofi 2.0.0.

The Debian/Ubuntu instructions are generic Meson build instructions and are not
covered by this repository's CI matrix.

## Distribution dependencies

### Fedora / Nobara

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel json-glib-devel qalculate
```

Optional providers:

```sh
sudo dnf install fd-find cliphist wl-clipboard jq sqlite curl
```

### Arch Linux

```sh
sudo pacman -S --needed rofi meson ninja gcc pkgconf glib2 cairo json-glib qalculate
sudo pacman -S --needed fd cliphist wl-clipboard jq sqlite curl
```

### Debian / Ubuntu

```sh
sudo apt install rofi rofi-dev meson ninja-build build-essential pkg-config \
  libglib2.0-dev libcairo2-dev libjson-glib-dev qalc
sudo apt install fd-find cliphist wl-clipboard jq sqlite3 curl
```

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

The RPM is built, installed and removed in Fedora 42/44 CI. The PKGBUILD is
built, installed, plugin-loaded and removed in an Arch Linux container CI. Neither
package is published in official repositories. See
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

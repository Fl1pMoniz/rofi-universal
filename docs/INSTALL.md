# Installation

## Distribution dependencies

### Fedora / Nobara

```sh
sudo dnf install rofi rofi-devel meson ninja-build gcc pkg-config \
  glib2-devel cairo-devel qalculate
```

Optional providers:

```sh
sudo dnf install fd-find cliphist wl-clipboard jq sqlite
```

### Arch Linux

```sh
sudo pacman -S --needed rofi meson ninja gcc pkgconf glib2 cairo qalculate
sudo pacman -S --needed fd cliphist wl-clipboard jq sqlite
```

### Debian / Ubuntu

```sh
sudo apt install rofi rofi-dev meson ninja-build build-essential pkg-config \
  libglib2.0-dev libcairo2-dev qalc
sudo apt install fd-find cliphist wl-clipboard jq sqlite3
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

The plugin is installed in `$PREFIX/lib/rofi/libuniversal.so` or
`$PREFIX/lib64/rofi/libuniversal.so`. The `rofi-universal` launcher discovers both
locations.

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

This removes the plugin and launcher. It leaves `~/.config/rofi-universal/`
untouched. Remove that directory manually if you also want to remove your
configuration.

#!/bin/sh
# Build and install rofi-universal into PREFIX (default: ~/.local).
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
prefix=${PREFIX:-"$HOME/.local"}
builddir=${BUILDDIR:-"$root/build"}

if [ -f "$builddir/build.ninja" ]; then
    meson setup --reconfigure "$builddir" --prefix="$prefix" --buildtype=release
else
    meson setup "$builddir" --prefix="$prefix" --buildtype=release
fi
meson compile -C "$builddir"
meson install -C "$builddir"

configdir=${XDG_CONFIG_HOME:-"$HOME/.config"}/rofi-universal
mkdir -p "$configdir"
if [ ! -e "$configdir/config.ini" ]; then
    cp "$root/data/config.example.ini" "$configdir/config.ini"
fi
printf 'Installed rofi-universal.\n'
printf 'Plugin directory: under %s/lib*/rofi\n' "$prefix"
printf 'Launcher: %s/bin/rofi-universal\n' "$prefix"
printf 'Configuration: %s\n' "$configdir/config.ini"

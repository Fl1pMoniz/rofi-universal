#!/bin/sh
# Remove installed files. Configuration and user data are preserved.
set -eu
prefix=${PREFIX:-"$HOME/.local"}
removed=0
for path in \
    "$prefix/lib/rofi/libuniversal.so" \
    "$prefix/lib64/rofi/libuniversal.so" \
    "$prefix/bin/rofi-universal"; do
    if [ -e "$path" ]; then
        rm -f "$path"
        printf 'Removed %s\n' "$path"
        removed=1
    fi
done
if [ "$removed" -eq 0 ]; then
    printf 'No installed rofi-universal files found under %s\n' "$prefix"
fi
printf 'User configuration was left untouched.\n'

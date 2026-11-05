#!/bin/sh
# Create a deterministic release source tarball from tracked files.
# Packaging metadata is intentionally excluded so a checksum can be
# recorded in packaging/PKGBUILD without changing the archive.
set -eu
version=${1:?usage: $0 VERSION [OUTPUT]}
output=${2:-"$PWD/rofi-universal-$version.tar.gz"}
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
list=$(mktemp)
trap 'rm -f "$list"' EXIT INT TERM
git ls-files -z -- . ':(exclude)packaging' >"$list"
tar --null --files-from="$list" --format=gnu --sort=name \
    --mtime=@1793836800 --owner=0 --group=0 --numeric-owner \
    --transform="s,^,rofi-universal-$version/," -cf - |
    gzip -n >"$output"
sha256sum "$output"

#!/bin/sh
# Create a deterministic release source tarball from a Git ref.
# Packaging metadata is intentionally excluded so a checksum can be
# recorded in packaging/PKGBUILD without changing the archive.
set -eu
version=${1:?usage: $0 VERSION [OUTPUT] [REF]}
output=${2:-"$PWD/rofi-universal-$version.tar.gz"}
ref=${3:-HEAD}
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT INT TERM
git archive --format=tar --prefix="rofi-universal-$version/" "$ref" -- . \
    ':(exclude)packaging' >"$tmp"
gzip -n -c "$tmp" >"$output"
sha256sum "$output"

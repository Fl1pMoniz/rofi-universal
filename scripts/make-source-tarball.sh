#!/bin/sh
# Create a deterministic release source archive from a clean checkout.
# Packaging and CI metadata are excluded so their checksums and configuration
# do not change the archive consumed by the package builds.
set -eu
version=${1:?usage: $0 VERSION [OUTPUT]}
output=${2:-"$PWD/rofi-universal-$version.tar.gz"}
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
list=$(mktemp)
trap 'rm -f "$list"' EXIT INT TERM
find . -type f \
    ! -path './.git/*' \
    ! -path './.github/*' \
    ! -path './packaging/*' \
    ! -path './build/*' \
    ! -path './build-*/*' \
    ! -name '*.o' \
    ! -name '*.so' \
    ! -name '*.tar.gz' \
    -printf '%P\0' | LC_ALL=C sort -z >"$list"
test -s "$list"
tar --null --files-from="$list" --format=gnu --sort=name \
    --mtime=@1793836800 --owner=0 --group=0 --numeric-owner \
    --transform="s,^,rofi-universal-$version/," -cf - |
    gzip -n >"$output"
sha256sum "$output"

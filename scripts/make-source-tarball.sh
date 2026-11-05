#!/bin/sh
# Create a deterministic release source archive from a clean checkout.
# Packaging and CI metadata are excluded so their checksums and configuration
# do not change the archive consumed by the package builds.
set -eu
umask 022
version=${1:?usage: $0 VERSION [OUTPUT]}
output=${2:-"$PWD/rofi-universal-$version.tar"}
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"
list=$(mktemp)
stage=$(mktemp -d)
trap 'rm -rf "$list" "$stage"' EXIT INT TERM
find . -type f \
    ! -path './.git/*' \
    ! -path './.github/*' \
    ! -path './packaging/*' \
    ! -path './build/*' \
    ! -path './build-*/*' \
    ! -path '*/__pycache__/*' \
    ! -name '*.pyc' \
    ! -name '*.o' \
    ! -name '*.so' \
    ! -name '*.tar' \
    ! -name '*.tar.gz' \
    -printf '%P\n' | LC_ALL=C sort >"$list"
test -s "$list"
destroot="$stage/rofi-universal-$version"
while IFS= read -r file; do
    [ -n "$file" ] || continue
    dest="$destroot/$file"
    mkdir -p "$(dirname "$dest")"
    cp "$file" "$dest"
    case "$file" in
        scripts/*|data/rofi-universal|tests/test_integration.py|tests/test_rapid_queries.py)
            chmod 0755 "$dest"
            ;;
        *)
            chmod 0644 "$dest"
            ;;
    esac
done <"$list"
tar -C "$stage" --format=ustar --sort=name \
    --mtime=@1793836800 --owner=0 --group=0 --numeric-owner \
    -cf "$output" "rofi-universal-$version"
sha256sum "$output"

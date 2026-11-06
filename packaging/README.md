# Packaging

These files package the Meson build for distributions that are not covered by
the generic installer. They are maintained in-tree so that release tarballs and
downstream review use the same build description as the project. Debian-family
installations currently use the source installer; no `.deb` package is provided.

## Source archives and checksums

Release archives are generated from the immutable `vVERSION` tag with:

```sh
scripts/make-source-tarball.sh VERSION /tmp/rofi-universal-VERSION.tar
```

The script excludes `packaging/`, `release/` and `dist/` so packaging metadata
and downloaded release output cannot change the archive. The regression test
`tests/test_source_archive.py` verifies deterministic generation and the absence
of RPMs or checksum manifests in the source archive. The resulting tarball is attached to the
GitHub release as `rofi-universal-VERSION.tar`. Both the RPM spec and the
PKGBUILD consume that same asset.

To update packaging for a release:

1. Set the version in `meson.build`, the spec and the PKGBUILD.
2. Generate the source archive from the release commit.
3. Put its SHA-256 in `packaging/PKGBUILD`.
4. Verify the RPM builds from the same archive.
5. Attach the archive to the release.
6. Confirm the release asset URL and checksum independently.

## Fedora / Nobara

The RPM spec builds a binary package from the verified release archive:

```sh
sudo dnf builddep packaging/rofi-universal.spec
rpmbuild -ba packaging/rofi-universal.spec
```

`Requires: rofi >= 1.7.6` and `Requires: json-glib` are mandatory. Dynamic
library dependencies are also generated from the built plugin. Currency, file,
clipboard, bookmark and inline weather/dictionary tools are weak `Recommends`, so
the plugin installs and degrades gracefully without them. The native calculator
requires no external runtime tool. qalc is not a dependency.

The tag-driven `.github/workflows/release-rpm.yml` workflow builds x86_64 RPMs
for Fedora 42 and Fedora 44, verifies the source archive checksum, tests in a
clean runtime container without GCC/Meson/Ninja/pkg-config, validates install,
upgrade and removal, and uploads the release RPM and `SHA256SUMS` as workflow
artifacts. The upgrade-test RPM is uploaded separately as a test artifact. The
workflow does not publish a release automatically.

The RPM installs:

- `/usr/bin/rofi-universal`
- `/usr/lib64/rofi/libuniversal.so` on x86_64 Fedora
- license, documentation and an example configuration file

It does not install user configuration or build tools. Fedora 42 and Fedora 44
use different Rofi ABIs, so use the RPM built for the matching release; do not
mix `fc42` and `fc44` packages. Nobara releases based on those Fedora versions
can use the matching RPM. aarch64 is not currently built or tested.

Release RPMs are unsigned at this stage. Publish and verify `SHA256SUMS` over
HTTPS, but do not represent checksums as equivalent to a cryptographic package
signature. The workflow uses `--nogpgcheck` only for isolated unsigned test
RPMs; the normal user instructions must not disable DNF GPG checks globally.

## Arch Linux

The PKGBUILD builds the same release archive with the standard `/usr` prefix:

```sh
cd packaging
makepkg -si
```

The package is built, installed, loaded under Xvfb and removed in an Arch Linux
container in CI. `namcap` is run on the resulting package. `json-glib` is a
runtime dependency; optional provider tools are listed in `optdepends`.

## Validation status

- RPM: built for x86_64 on Fedora 42 (Rofi 1.7.9) and Fedora 44 (Rofi 2.0.0);
  clean install, plugin load, calculator/currency integration, upgrade and
  removal tested without build tooling installed.
- Arch package: built and installed in an Arch Linux container with Rofi 2.0.0;
  `namcap`, plugin load and removal are exercised in CI.
- `namcap` currently reports W-level notes for the `/bin/sh` launcher, the host
  Rofi dependency and default glibc detection. There are no E-level findings.
- Neither package is published in official distribution repositories.

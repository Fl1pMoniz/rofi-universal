# Packaging

These files package the Meson build for distributions that are not covered by
the generic installer. They are maintained in-tree so that release tarballs and
downstream review use the same build description as the project.

## Source archives and checksums

Release archives are generated from the immutable `vVERSION` tag with:

```sh
scripts/make-source-tarball.sh VERSION /tmp/rofi-universal-VERSION.tar
```

The script excludes `packaging/` so the checksum can be recorded in the
PKGBUILD without changing the archive. The resulting tarball is attached to the
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

The RPM spec builds from the verified release archive:

```sh
sudo dnf builddep packaging/rofi-universal.spec
rpmbuild -ba packaging/rofi-universal.spec
```

`Requires: rofi` is mandatory. Calculator, file, clipboard, bookmark and inline
weather dependencies are weak `Recommends`, so the plugin installs and degrades
gracefully without them. The RPM is built, installed and removed in Fedora 42
and Fedora 44 CI. It is not published in Fedora or Nobara repositories.

## Arch Linux

The PKGBUILD builds the same release archive with the standard `/usr` prefix:

```sh
cd packaging
makepkg -si
```

The package is built, installed, loaded under Xvfb and removed in an Arch Linux
container in CI. `namcap` is run on the resulting package. Optional dependencies
are listed in `optdepends`; they are not required to install the plugin.

## Validation status

- RPM: built and installed in Fedora 42 (Rofi 1.7.9) and Fedora 44 (Rofi 2.0.0)
  CI environments.
- Arch package: built and installed in an Arch Linux container with Rofi 2.0.0;
  `namcap`, plugin load and removal are exercised in CI.
- `namcap` currently reports W-level notes for the `/bin/sh` launcher, the host
  Rofi dependency and default glibc detection. There are no E-level findings.
- Neither package is published in official distribution repositories.

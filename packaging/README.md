# Packaging

These files package the Meson build for distributions that are not covered by
the generic installer. They are maintained in-tree so that release tarballs and
downstream review use the same build description as the project.

## Fedora / Nobara

The RPM spec builds from the tagged release tarball:

```sh
sudo dnf builddep packaging/rofi-universal.spec
rpmbuild -ba packaging/rofi-universal.spec
```

`Requires: rofi` is mandatory. Calculator, file, clipboard, bookmark and inline
weather dependencies are weak `Recommends`, so the plugin installs and degrades
gracefully without them. The package is not currently published in Fedora or
Nobara repositories.

## Arch Linux

The PKGBUILD builds the tagged release tarball with the standard `/usr` prefix:

```sh
cd packaging
makepkg -si
```

Update `sha256sums` after the v0.3.0 release tag exists. `qalculate`, `fd`,
`cliphist`, `wl-clipboard`, `jq`, `sqlite`, `curl` and `wget` are optional
dependencies; the PKGBUILD lists them in `optdepends`.

## Validation status

The Fedora RPM build and test are exercised in CI. The PKGBUILD is provided for
Arch users and reviewed for syntax, but it is not built on Arch Linux in this
repository's CI; do not assume it has been tested on a real Arch system.

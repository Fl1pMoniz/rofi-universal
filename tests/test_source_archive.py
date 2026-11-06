#!/usr/bin/env python3
"""Regression test for deterministic, release-artifact-free source archives."""
from pathlib import Path
import hashlib
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(
    r"version:\s*'([^']+)'", (ROOT / 'meson.build').read_text()
).group(1)
ARCHIVE = f'rofi-universal-{VERSION}.tar'
EXPECTED = re.search(
    r"sha256sums=\('([0-9a-f]+)'\)", (ROOT / 'packaging/PKGBUILD').read_text()
).group(1)
IGNORE = shutil.ignore_patterns(
    '.git', 'build', 'build-*', '__pycache__', '*.pyc', '*.o', '*.so', '*.tar',
    '*.rpm', 'release', 'dist')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archive(name, checkout, output):
    subprocess.run(
        [str(checkout / 'scripts/make-source-tarball.sh'), VERSION, str(output)],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    assert output.is_file(), name
    return digest(output)


with tempfile.TemporaryDirectory(prefix='ru-source-archive-') as tmp:
    base = Path(tmp)
    checkout = base / 'checkout'
    shutil.copytree(ROOT, checkout, ignore=IGNORE)

    clean = base / 'clean.tar'
    clean_hash = archive('clean', checkout, clean)
    assert clean_hash == EXPECTED, (clean_hash, EXPECTED)

    repeated = base / 'repeated.tar'
    assert archive('repeated', checkout, repeated) == clean_hash

    # Simulate release-bundle output inside the checkout. These files must not
    # affect the source archive if the archive is generated after downloads.
    release = checkout / 'release'
    release.mkdir()
    (release / f'rofi-universal-{VERSION}-1.fc42.x86_64.rpm').write_bytes(b'dummy rpm')
    (release / 'SHA256SUMS').write_text('dummy\n')

    contaminated = base / 'contaminated.tar'
    contaminated_hash = archive('contaminated', checkout, contaminated)
    assert contaminated_hash == clean_hash, (contaminated_hash, clean_hash)

    names = subprocess.check_output(['tar', '-tf', str(contaminated)], text=True)
    assert '/release/' not in names
    assert not any(name.endswith('.rpm') for name in names.splitlines())
    assert not any(name.endswith('/SHA256SUMS') for name in names.splitlines())

print('Source archive determinism and release-artifact exclusion checks passed')

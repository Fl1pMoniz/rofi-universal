#!/usr/bin/env python3
"""Headless file-action test for Alt+1 reveal fallback.

The FileManager1 success path depends on a live desktop file manager. This test
verifies the deterministic fallback: when no FileManager1 handler is available,
Alt+1 opens the containing directory through the configured/default handler.
"""
from pathlib import Path
import os
import shlex
import shutil
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("ROFI_UNIVERSAL_BUILD", root / "build"))
plugin = build / "libuniversal.so"

if not plugin.is_file():
    raise SystemExit(f"missing plugin: {plugin} (run meson compile -C build)")
if not os.environ.get("DISPLAY"):
    print("no X display; skipping file-action test")
    raise SystemExit(0)

xdotool = shutil.which("xdotool")
rofi = shutil.which("rofi")
if not xdotool or not rofi:
    print("xdotool or rofi not found; skipping file-action test")
    raise SystemExit(0)

with tempfile.TemporaryDirectory(prefix="rofi-universal-files-") as tmp:
    tmpdir = Path(tmp)
    bindir = tmpdir / "bin"
    bindir.mkdir()
    target = tmpdir / "report final.pdf"
    target.write_text("test\n")
    log = tmpdir / "open.log"

    fake_open = tmpdir / "fake-open"
    fake_open.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" >> {shlex.quote(str(log))}\n"
    )
    fake_open.chmod(0o755)

    fake_xdg = bindir / "xdg-open"
    fake_xdg.write_text("#!/bin/sh\nexit 1\n")
    fake_xdg.chmod(0o755)

    config = tmpdir / "config.ini"
    config.write_text(
        "[general]\n"
        f"open_command={fake_open}\n"
        "[providers]\n"
        "applications=false\ncalculator=false\nweb=false\nfiles=true\n"
        "clipboard=false\ndictionary=false\ngithub=false\nweather=false\n"
        "bookmarks=false\n"
    )

    env = dict(os.environ)
    env["ROFI_PLUGIN_PATH"] = str(build)
    env["ROFI_UNIVERSAL_CONFIG"] = str(config)
    env["PATH"] = f"{bindir}:{env.get('PATH', '')}"

    process = subprocess.Popen(
        [rofi, "-no-config", "-show", "universal", "-modi", "universal"],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        time.sleep(1.0)
        subprocess.run([xdotool, "type", "--delay", "8", str(target)],
                       timeout=5, check=False)
        time.sleep(0.4)
        subprocess.run([xdotool, "key", "alt+1"], timeout=5, check=False)
        try:
            stdout, stderr = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise SystemExit("file-action test: Rofi did not exit after Alt+1")

        combined = (stdout or "") + (stderr or "")
        for marker in ("Segmentation fault", "double free", "GLib-ERROR",
                       "CRITICAL"):
            if marker in combined:
                raise SystemExit(
                    f"file-action test: crash marker {marker!r}:\n{combined}"
                )
        if not log.exists() or not log.read_text().strip():
            raise SystemExit(
                "file-action test: Alt+1 did not open the containing directory"
            )
        expected_parent = str(tmpdir)
        if expected_parent not in log.read_text():
            raise SystemExit(
                "file-action test: unexpected open target:\n" + log.read_text()
            )
        print("File-action reveal fallback test passed")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()

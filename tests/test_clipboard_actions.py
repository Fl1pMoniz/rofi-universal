#!/usr/bin/env python3
"""Headless clipboard decode/copy integration test.

Uses fake cliphist and wl-copy executables so CI never touches the real
clipboard database or clipboard.
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
    print("no X display; skipping clipboard-action test")
    raise SystemExit(0)

xdotool = shutil.which("xdotool")
rofi = shutil.which("rofi")
if not xdotool or not rofi:
    print("xdotool or rofi not found; skipping clipboard-action test")
    raise SystemExit(0)

with tempfile.TemporaryDirectory(prefix="rofi-universal-clipboard-") as tmp:
    tmpdir = Path(tmp)
    bindir = tmpdir / "bin"
    bindir.mkdir()
    log = tmpdir / "clipboard.log"

    cliphist = bindir / "cliphist"
    cliphist.write_text(
        "#!/bin/sh\n"
        "case \"$1\" in\n"
        "  list) printf '1\\tclipboard token\\n' ;;\n"
        "  decode) printf 'clipboard token\\n' ;;\n"
        "  *) exit 1 ;;\n"
        "esac\n"
    )
    cliphist.chmod(0o755)

    wl_copy = bindir / "wl-copy"
    wl_copy.write_text(
        "#!/bin/sh\n"
        f"cat > {shlex.quote(str(log))}\n"
    )
    wl_copy.chmod(0o755)

    config = tmpdir / "config.ini"
    config.write_text(
        "[providers]\n"
        "applications=false\ncalculator=false\nweb=false\nfiles=false\n"
        "clipboard=true\ndictionary=false\ngithub=false\nweather=false\n"
        "bookmarks=false\n"
        "[clipboard]\nautomatic=true\n"
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
        subprocess.run([xdotool, "type", "--delay", "8", "clip token"],
                       timeout=5, check=False)
        time.sleep(1.0)
        subprocess.run([xdotool, "key", "Return"], timeout=5, check=False)
        try:
            stdout, stderr = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise SystemExit("clipboard-action test: Rofi did not exit")

        combined = (stdout or "") + (stderr or "")
        for marker in ("Segmentation fault", "double free", "GLib-ERROR",
                       "CRITICAL"):
            if marker in combined:
                raise SystemExit(
                    f"clipboard-action test: crash marker {marker!r}:\n{combined}"
                )
        copied = log.read_text() if log.exists() else ""
        if "clipboard token" not in copied:
            raise SystemExit(
                "clipboard-action test: decoded clipboard text was not copied"
            )
        print("Clipboard decode/copy integration test passed")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()

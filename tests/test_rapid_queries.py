#!/usr/bin/env python3
"""Rapid-query, cancellation and teardown smoke test.

This is intentionally a smoke test: it exercises Rofi, the plugin, a fake
weather fetcher and plugin destruction while a request is pending. It does not
assert individual rows because Rofi does not expose that state to the harness.
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
    print("no X display; skipping rapid-query test")
    raise SystemExit(0)

xdotool = shutil.which("xdotool")
rofi = shutil.which("rofi")
if not xdotool or not rofi:
    print("xdotool or rofi not found; skipping rapid-query test")
    raise SystemExit(0)

with tempfile.TemporaryDirectory(prefix="rofi-universal-rapid-") as tmp:
    tmpdir = Path(tmp)
    bindir = tmpdir / "bin"
    bindir.mkdir()
    log = tmpdir / "curl.log"
    curl = bindir / "curl"
    curl.write_text(
        "#!/bin/sh\n"
        f"printf 'start\\n' >> {shlex.quote(str(log))}\n"
        "sleep 1.5\n"
        "printf 'Testville: +20°C, wind 5km/h, humidity 50%%\\n'\n"
    )
    curl.chmod(0o755)

    config = tmpdir / "config.ini"
    config.write_text(
        "[providers]\n"
        "applications=false\ncalculator=false\nweb=false\nfiles=false\n"
        "clipboard=false\ndictionary=false\ngithub=false\nweather=true\n"
        "bookmarks=false\n"
        "[weather]\ntimeout_seconds=1\n"
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

        def replace_query(text):
            subprocess.run([xdotool, "key", "--repeat", "64", "BackSpace"],
                           timeout=5, check=False)
            subprocess.run([xdotool, "type", "--delay", "8", text], timeout=5,
                           check=False)

        replace_query("weather in Lisbon")
        time.sleep(0.15)
        replace_query("weather in Porto")
        time.sleep(0.15)
        replace_query("weather in Faro")
        time.sleep(0.15)
        subprocess.run([xdotool, "key", "Escape"], timeout=5, check=False)

        try:
            stdout, stderr = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise SystemExit("rapid-query test: Rofi did not exit after Escape")

        combined = (stdout or "") + (stderr or "")
        for marker in ("Segmentation fault", "double free", "corrupted",
                       "GLib-ERROR", "CRITICAL"):
            if marker in combined:
                raise SystemExit(
                    f"rapid-query test: crash marker {marker!r}:\n{combined}"
                )
        starts = len(log.read_text().splitlines()) if log.exists() else 0
        if starts < 2:
            raise SystemExit(
                f"rapid-query test: expected multiple weather requests, saw {starts}"
            )
        print(f"Rapid-query and teardown smoke test passed ({starts} requests started)")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()

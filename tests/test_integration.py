#!/usr/bin/env python3
"""Optional integration smoke test for the built plugin and qalc."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("ROFI_UNIVERSAL_BUILD", root / "build"))
plugin = build / "libuniversal.so"

if not plugin.is_file():
    raise SystemExit(f"missing plugin: {plugin} (run meson compile -C build)")

qalc = shutil.which("qalc")
if qalc:
    out = subprocess.check_output([qalc, "-t", "25 * 18"], text=True, timeout=15)
    if "450" not in out:
        raise SystemExit(f"qalc returned unexpected output: {out!r}")
    print("qalc calculator check passed")
else:
    print("qalc not found; skipping calculator check")

if not (os.environ.get("WAYLAND_DISPLAY") or os.environ.get("DISPLAY")):
    print("no graphical session; skipping Rofi plugin load check")
    raise SystemExit(0)

env = dict(os.environ, ROFI_PLUGIN_PATH=str(build))
process = subprocess.Popen(
    ["rofi", "-no-config", "-show", "universal", "-modi", "universal"],
    env=env,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
)
time.sleep(1.5)
if process.poll() is not None:
    stdout, stderr = process.communicate()
    combined = stdout + stderr
    if "universal" in combined or "plugin" in combined.lower():
        raise SystemExit(f"Rofi failed to load the plugin:\n{combined}")
    raise SystemExit(f"Rofi exited unexpectedly:\n{combined}")
process.terminate()
try:
    stdout, stderr = process.communicate(timeout=5)
except subprocess.TimeoutExpired:
    process.kill()
    stdout, stderr = process.communicate()
combined = (stdout or "") + (stderr or "")
for marker in ("Failed to open", "ABI version", "Mode 'universal' not found"):
    if marker in combined:
        raise SystemExit(f"Rofi plugin error ({marker}):\n{combined}")
print("Rofi plugin load check passed")

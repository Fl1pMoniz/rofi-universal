#!/usr/bin/env python3
"""Mock FileManager1 success-path integration test.

Runs under dbus-run-session and an X display. A small in-process GIO service
implements org.freedesktop.FileManager1.ShowItems and records the URI it gets.
"""
from pathlib import Path
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

try:
    import gi
    gi.require_version("Gio", "2.0")
    from gi.repository import Gio, GLib
except Exception:
    print("PyGObject/GIO not available; skipping FileManager1 test")
    raise SystemExit(0)

XML = """
<node>
  <interface name="org.freedesktop.FileManager1">
    <method name="ShowItems">
      <arg type="as" direction="in"/>
      <arg type="s" direction="in"/>
    </method>
  </interface>
</node>
"""


def mock_service(log_path):
    loop = GLib.MainLoop()

    def on_method_call(connection, sender, object_path, interface_name,
                       method_name, parameters, invocation):
        uris, _startup = parameters.unpack()
        with open(log_path, "a", encoding="utf-8") as handle:
            for uri in uris:
                handle.write(uri + "\n")
        invocation.return_value(None)

    def on_bus_acquired(connection, name):
        node = Gio.DBusNodeInfo.new_for_xml(XML)
        connection.register_object("/org/freedesktop/FileManager1",
                                   node.interfaces[0], on_method_call, None,
                                   None)

    def on_name_lost(connection, name):
        loop.quit()

    Gio.bus_own_name(Gio.BusType.SESSION, "org.freedesktop.FileManager1",
                     Gio.BusNameOwnerFlags.NONE, on_bus_acquired, None,
                     on_name_lost)
    loop.run()


if "--mock" in sys.argv:
    mock_service(sys.argv[sys.argv.index("--mock") + 1])
    raise SystemExit(0)

root = Path(__file__).resolve().parents[1]
build = Path(os.environ.get("ROFI_UNIVERSAL_BUILD", root / "build"))
plugin = build / "libuniversal.so"

if not plugin.is_file():
    raise SystemExit(f"missing plugin: {plugin} (run meson compile -C build)")
if not os.environ.get("DISPLAY"):
    print("no X display; skipping FileManager1 test")
    raise SystemExit(0)
if not os.environ.get("DBUS_SESSION_BUS_ADDRESS"):
    print("no session bus; run this test under dbus-run-session")
    raise SystemExit(0)

xdotool = shutil.which("xdotool")
rofi = shutil.which("rofi")
if not xdotool or not rofi:
    print("xdotool or rofi not found; skipping FileManager1 test")
    raise SystemExit(0)

with tempfile.TemporaryDirectory(prefix="rofi-universal-fm-") as tmp:
    tmpdir = Path(tmp)
    target = tmpdir / "report final.pdf"
    target.write_text("test\n")
    mock_log = tmpdir / "filemanager.log"
    fallback_log = tmpdir / "fallback.log"

    fake_open = tmpdir / "fake-open"
    fake_open.write_text(
        "#!/bin/sh\n"
        f"printf '%s\\n' \"$@\" >> {shlex.quote(str(fallback_log))}\n"
    )
    fake_open.chmod(0o755)

    config = tmpdir / "config.ini"
    config.write_text(
        "[general]\n"
        f"open_command={fake_open}\n"
        "[providers]\n"
        "applications=false\ncalculator=false\nweb=false\nfiles=true\n"
        "clipboard=false\ndictionary=false\ngithub=false\nweather=false\n"
        "bookmarks=false\n"
    )

    mock = subprocess.Popen(
        [sys.executable, __file__, "--mock", str(mock_log)],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    time.sleep(0.7)

    env = dict(os.environ)
    env["ROFI_PLUGIN_PATH"] = str(build)
    env["ROFI_UNIVERSAL_CONFIG"] = str(config)
    process = subprocess.Popen(
        [rofi, "-no-config", "-show", "universal", "-modi", "universal"],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
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
            raise SystemExit("FileManager1 test: Rofi did not exit after Alt+1")

        combined = (stdout or "") + (stderr or "")
        for marker in ("Segmentation fault", "double free", "GLib-ERROR",
                       "CRITICAL"):
            if marker in combined:
                raise SystemExit(
                    f"FileManager1 test: crash marker {marker!r}:\n{combined}"
                )
        expected = target.as_uri()
        if not mock_log.exists() or expected not in mock_log.read_text():
            fallback = fallback_log.read_text() if fallback_log.exists() else ""
            raise SystemExit(
                "FileManager1 test: ShowItems did not receive the file URI\n"
                f"expected: {expected}\nfallback: {fallback}"
            )
        print("FileManager1 ShowItems success-path test passed")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()
        mock.terminate()
        try:
            mock.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            mock.kill()
            mock.communicate()

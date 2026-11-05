#!/usr/bin/env python3
"""Headless native-calculator and currency-provider integration test.

Uses fake clipboard and curl helpers, so it never requires an external
calculator, a live exchange-rate service or access to the real clipboard.
"""
from pathlib import Path
import os
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
    print("no X display; skipping calculator/currency integration test")
    raise SystemExit(0)

xdotool = shutil.which("xdotool")
rofi = shutil.which("rofi")
if not xdotool or not rofi:
    print("xdotool or rofi not found; skipping calculator/currency test")
    raise SystemExit(0)

CRASH_MARKERS = (
    "Segmentation fault",
    "double free",
    "GLib-ERROR",
    "CRITICAL",
)


def write_executable(path, contents):
    path.write_text(contents)
    path.chmod(0o755)


def make_env(config, bindir):
    env = dict(os.environ)
    env["ROFI_PLUGIN_PATH"] = str(build)
    env["ROFI_UNIVERSAL_CONFIG"] = str(config)
    env["PATH"] = f"{bindir}:{env.get('PATH', '')}"
    return env


def run_rofi(env, query, key):
    process = subprocess.Popen(
        [rofi, "-no-config", "-show", "universal", "-modi", "universal"],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        time.sleep(1.0)
        subprocess.run([xdotool, "type", "--delay", "8", query],
                       timeout=5, check=False)
        time.sleep(1.2)
        subprocess.run([xdotool, "key", key], timeout=5, check=False)
        try:
            stdout, stderr = process.communicate(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()
            raise SystemExit("calculator/currency test: Rofi did not exit")
        combined = (stdout or "") + (stderr or "")
        for marker in CRASH_MARKERS:
            if marker in combined:
                raise SystemExit(
                    f"calculator/currency test: crash marker {marker!r}:\n"
                    f"{combined}"
                )
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()


def base_config(tmpdir, calculator=True, web=False, clipboard=False,
                files=False):
    config = tmpdir / "config.ini"
    config.write_text(
        "[providers]\n"
        f"applications=false\ncalculator={'true' if calculator else 'false'}\n"
        f"web={'true' if web else 'false'}\n"
        f"files={'true' if files else 'false'}\n"
        "clipboard=false\ndictionary=false\ngithub=false\nweather=false\n"
        "bookmarks=false\n"
    )
    return config


def test_native_arithmetic():
    with tempfile.TemporaryDirectory(prefix="rofi-universal-calc-") as tmp:
        tmpdir = Path(tmp)
        bindir = tmpdir / "bin"
        bindir.mkdir()
        copy_log = tmpdir / "copy.log"
        write_executable(
            bindir / "wl-copy",
            "#!/bin/sh\n"
            f"cat > {copy_log}\n",
        )
        config = base_config(tmpdir)
        run_rofi(make_env(config, bindir), "25 * 18", "Return")
        copied = copy_log.read_text() if copy_log.exists() else ""
        if "450" not in copied:
            raise SystemExit(
                "calculator/currency test: native arithmetic did not copy 450:\n"
                f"{copied}"
            )
        print("Native calculator integration test passed")


def test_currency_success():
    with tempfile.TemporaryDirectory(prefix="rofi-universal-currency-") as tmp:
        tmpdir = Path(tmp)
        bindir = tmpdir / "bin"
        bindir.mkdir()
        copy_log = tmpdir / "copy.log"
        curl_log = tmpdir / "curl.log"
        write_executable(
            bindir / "curl",
            "#!/bin/sh\n"
            f"printf '%s\\n' \"$*\" >> {curl_log}\n"
            "printf '%s' '[{\"date\":\"2026-11-05\",\"base\":\"EUR\","
            "\"quote\":\"USD\",\"rate\":1.1251}]'\n",
        )
        write_executable(
            bindir / "wl-copy",
            "#!/bin/sh\n"
            f"cat > {copy_log}\n",
        )
        config = base_config(tmpdir)
        run_rofi(make_env(config, bindir), "300 eur to USD", "Return")
        requested = curl_log.read_text() if curl_log.exists() else ""
        copied = copy_log.read_text() if copy_log.exists() else ""
        if "api.frankfurter.dev/v2/rates?base=EUR" not in requested:
            raise SystemExit(
                "calculator/currency test: currency provider was not requested:\n"
                f"{requested}"
            )
        if "300 EUR = 337.53 USD" not in copied:
            raise SystemExit(
                "calculator/currency test: converted amount was not copied:\n"
                f"{copied}"
            )
        print("Inline currency conversion integration test passed")


def test_currency_failure_is_safe():
    with tempfile.TemporaryDirectory(prefix="rofi-universal-currency-fail-") as tmp:
        tmpdir = Path(tmp)
        bindir = tmpdir / "bin"
        bindir.mkdir()
        copy_log = tmpdir / "copy.log"
        write_executable(bindir / "curl", "#!/bin/sh\nexit 1\n")
        write_executable(
            bindir / "wl-copy",
            "#!/bin/sh\n"
            f"cat > {copy_log}\n",
        )
        config = base_config(tmpdir)
        run_rofi(make_env(config, bindir), "300 eur to USD", "Escape")
        copied = copy_log.read_text() if copy_log.exists() else ""
        if copied:
            raise SystemExit(
                "calculator/currency test: failed currency request produced a "
                f"copied result:\n{copied}"
            )
        print("Currency failure integration smoke test passed")


def test_currency_cache_reuse():
    with tempfile.TemporaryDirectory(prefix="rofi-universal-currency-cache-") as tmp:
        tmpdir = Path(tmp)
        bindir = tmpdir / "bin"
        bindir.mkdir()
        copy_log = tmpdir / "copy.log"
        curl_log = tmpdir / "curl.log"
        write_executable(
            bindir / "curl",
            "#!/bin/sh\n"
            f"printf 'request\\n' >> {curl_log}\n"
            "printf '%s' '[{\"date\":\"2026-11-05\",\"base\":\"EUR\","
            "\"quote\":\"USD\",\"rate\":1.1251}]'\n",
        )
        write_executable(
            bindir / "wl-copy",
            "#!/bin/sh\n"
            f"cat > {copy_log}\n",
        )
        config = base_config(tmpdir)
        env = make_env(config, bindir)
        process = subprocess.Popen(
            [rofi, "-no-config", "-show", "universal", "-modi", "universal"],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            time.sleep(1.0)
            subprocess.run([xdotool, "type", "--delay", "8", "300 eur to USD"],
                           timeout=5, check=False)
            time.sleep(1.2)
            subprocess.run([xdotool, "key", "--repeat", "64", "BackSpace"],
                           timeout=5, check=False)
            subprocess.run([xdotool, "type", "--delay", "8", "100 eur to USD"],
                           timeout=5, check=False)
            time.sleep(0.4)
            subprocess.run([xdotool, "key", "Return"], timeout=5, check=False)
            try:
                stdout, stderr = process.communicate(timeout=8)
            except subprocess.TimeoutExpired:
                process.kill()
                process.communicate()
                raise SystemExit("calculator/currency test: Rofi did not exit")
            combined = (stdout or "") + (stderr or "")
            for marker in CRASH_MARKERS:
                if marker in combined:
                    raise SystemExit(
                        f"calculator/currency test: crash marker {marker!r}:\n"
                        f"{combined}"
                    )
        finally:
            if process.poll() is None:
                process.kill()
                process.communicate()
        requests = curl_log.read_text().splitlines() if curl_log.exists() else []
        copied = copy_log.read_text() if copy_log.exists() else ""
        if len(requests) != 1:
            raise SystemExit(
                "calculator/currency test: expected one cached currency request, "
                f"saw {len(requests)}:\n" + "\n".join(requests)
            )
        if "100 EUR = 112.51 USD" not in copied:
            raise SystemExit(
                "calculator/currency test: cached conversion was not copied:\n"
                f"{copied}"
            )
        print("Currency cache reuse integration test passed")


def test_slow_currency_does_not_block_arithmetic():
    with tempfile.TemporaryDirectory(prefix="rofi-universal-currency-slow-") as tmp:
        tmpdir = Path(tmp)
        bindir = tmpdir / "bin"
        bindir.mkdir()
        copy_log = tmpdir / "copy.log"
        write_executable(
            bindir / "curl",
            "#!/bin/sh\n"
            "sleep 3\n"
            "printf '%s' '[{\"date\":\"2026-11-05\",\"base\":\"EUR\","
            "\"quote\":\"USD\",\"rate\":1.1251}]'\n",
        )
        write_executable(
            bindir / "wl-copy",
            "#!/bin/sh\n"
            f"cat > {copy_log}\n",
        )
        config = base_config(tmpdir)
        env = make_env(config, bindir)
        process = subprocess.Popen(
            [rofi, "-no-config", "-show", "universal", "-modi", "universal"],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            time.sleep(1.0)
            subprocess.run([xdotool, "type", "--delay", "8", "300 eur to USD"],
                           timeout=5, check=False)
            time.sleep(0.2)
            subprocess.run([xdotool, "key", "--repeat", "64", "BackSpace"],
                           timeout=5, check=False)
            subprocess.run([xdotool, "type", "--delay", "8", "25 * 18"],
                           timeout=5, check=False)
            time.sleep(0.5)
            subprocess.run([xdotool, "key", "Return"], timeout=5, check=False)
            try:
                stdout, stderr = process.communicate(timeout=8)
            except subprocess.TimeoutExpired:
                process.kill()
                process.communicate()
                raise SystemExit("calculator/currency test: Rofi did not exit")
            combined = (stdout or "") + (stderr or "")
            for marker in CRASH_MARKERS:
                if marker in combined:
                    raise SystemExit(
                        f"calculator/currency test: crash marker {marker!r}:\n"
                        f"{combined}"
                    )
        finally:
            if process.poll() is None:
                process.kill()
                process.communicate()
        copied = copy_log.read_text() if copy_log.exists() else ""
        if "450" not in copied:
            raise SystemExit(
                "calculator/currency test: slow currency request blocked "
                f"arithmetic:\n{copied}"
            )
        print("Slow currency request does not block arithmetic")


test_native_arithmetic()
test_currency_success()
test_currency_failure_is_safe()
test_currency_cache_reuse()
test_slow_currency_does_not_block_arithmetic()

#!/usr/bin/env python3
"""PS5 Vulkan Template - a test run when the console has no klog capture.

    run-without-klog.py RUN_DIR TIMEOUT

Launches the deployed title through the control payload (ps5vkctl), follows its
own test-results.txt over FTP, and ends when the title has exited on its own: a
run ends through the title's own exit, and closing it is only the watchdog for
a hang (TIMEOUT seconds). The results file lands in RUN_DIR. Without klog, a
crash shows as a run that stopped before its "ends" line, and the driver's own
messages are not seen: use run-title.py's klog capture when the console has it.

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""
import io
import json
import os
import sys
import time
from ftplib import FTP, error_perm
from pathlib import Path

PS5 = Path(__file__).resolve().parent.parent
VULKAN = Path(os.environ.get("PS5_VULKAN_DIR", PS5.parent.parent / "PS5_Vulkan")).resolve()
sys.path.insert(0, str(VULKAN / "tools"))
import ps5_console  # noqa: E402

TITLE = json.loads((PS5 / "sce_sys" / "param.json").read_text())["titleId"]


def read_results(settings):
    sink = io.BytesIO()
    try:
        with FTP() as ftp:
            ftp.connect(settings["host"], settings["ftp_port"], timeout=15)
            ftp.login(settings["ftp_user"], settings["ftp_password"] or "codex")
            ftp.retrbinary(f"RETR /data/homebrew/{TITLE}/test-results.txt", sink.write)
    except (error_perm, OSError, EOFError):
        return None
    return sink.getvalue().decode(errors="replace")


def running(settings):
    return " count=0 " not in ps5_console.ps5vkctl_command(settings, "procs", timeout=20) + " "


def main():
    run_dir, timeout = Path(sys.argv[1]), float(sys.argv[2])
    settings = ps5_console.load_settings()
    if running(settings):
        sys.exit("the console is running something: not starting")
    with FTP() as ftp:  # no stale results may pass for this run's
        ftp.connect(settings["host"], settings["ftp_port"], timeout=15)
        ftp.login(settings["ftp_user"], settings["ftp_password"] or "codex")
        try:
            ftp.delete(f"/data/homebrew/{TITLE}/test-results.txt")
        except error_perm:
            pass
    reply = ps5_console.ps5vkctl_command(settings, f"launch {TITLE}", timeout=60)
    print(f"launch: {reply}", flush=True)
    if not reply.startswith("ok"):
        sys.exit(1)
    start, shown, text, ended = time.monotonic(), 0, "", "timed out"
    time.sleep(3)
    while time.monotonic() - start < timeout:
        text = read_results(settings) or text
        lines = text.splitlines()
        for line in lines[shown:]:
            print(line, flush=True)
        shown = len(lines)
        if not running(settings):
            ended = "the title exited on its own" if "ends: status" in text else "the title stopped before its end (a crash?)"
            break
        time.sleep(2)
    else:
        print(f"watchdog: closing {TITLE}: {ps5_console.ps5vkctl_command(settings, f'kill {TITLE}', timeout=60)}")
    text = read_results(settings) or text
    for line in text.splitlines()[shown:]:
        print(line)
    (run_dir / "test-results.txt").write_text(text)
    print(f"closing: {ended}")
    return 0 if "ends: status" in text else 1


if __name__ == "__main__":
    sys.exit(main())

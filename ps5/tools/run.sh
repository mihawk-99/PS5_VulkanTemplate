#!/usr/bin/env bash
# PS5 Vulkan Template - a test run on the console: each sample draws N frames,
# its last frame comes back as a screenshot, and klog is checked.
#
#   ps5/tools/run.sh                      every sample linked in, 300 frames each
#   ps5/tools/run.sh menu                 the samples in the menu
#   ps5/tools/run.sh bloom deferred       those samples
#   FRAMES=600 ps5/tools/run.sh ...       another budget
#   ps5/tools/run.sh --menu               no test: the menu, until Quit
#
# PS5_Vulkan's run tool checks that the console runs nothing, launches the title
# through the control payload (ps5vkctl), captures klog until the title's
# "ends" line, a crash record or the title's exit, and symbolises a crash. The
# run ends through the title's own exit: test-run.txt is the title's own stop,
# and the launch that reads it deletes it. Then tools/check-run.py reads the
# klog and the screenshots (klog/<run>/) and prints a verdict for each sample.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
vulkan=$(cd -- "${PS5_VULKAN_DIR:-$root/../PS5_Vulkan}" && pwd)
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
# klog's prefix: the title's name, as a regular expression
prefix=$(python3 -c 'import json,re,sys; print(re.escape("[" + json.load(open(sys.argv[1]))["localizedParameters"]["en-US"]["titleName"] + "]"))' "$ps5/sce_sys/param.json")
run_dir="$root/klog/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$run_dir"
output="$run_dir/klog.log"

samples=()
if [[ ${1:-} != --menu ]]; then
    (( $# )) || set -- all
    mapfile -t samples < <(python3 "$ps5/tools/check-run.py" --resolve "$@")
    (( ${#samples[@]} )) || { echo "no sample matches: $*" >&2; exit 2; }
    test_run=$(printf 'frames %s\nscreenshot\nsamples %s\n' "${FRAMES:-300}" "${samples[*]}")
    python3 - "$vulkan/tools" "$title_id" "$test_run" <<'PY'
import io, sys
from ftplib import FTP
sys.path.insert(0, sys.argv[1])
import ps5_console
settings = ps5_console.load_settings()
ftp = FTP()
ftp.connect(settings["host"], settings["ftp_port"], timeout=15)
ftp.login(settings["ftp_user"], settings["ftp_password"])
ftp.storbinary(f"STOR /data/homebrew/{sys.argv[2]}/test-run.txt", io.BytesIO(f"{sys.argv[3]}\n".encode()))
ftp.quit()
print("test run: " + sys.argv[3].replace("\n", "; "))
PY
    timeout=$(( 60 + 60 * ${#samples[@]} ))
else
    timeout=86400
fi
klog_port=$(python3 -c "import sys; sys.path.insert(0, sys.argv[1]); import ps5_console, socket; s = ps5_console.load_settings(); socket.create_connection((s['host'], s['klog_port']), 3).close(); print('open')" "$vulkan/tools" 2>/dev/null || true)
if [[ $klog_port == open ]]; then
    python3 "$vulkan/tools/run-title.py" "$title_id" \
        --until "$prefix ends" --timeout "${RUN_TIMEOUT:-$timeout}" \
        --echo "$prefix (sample|samples|test run|ends|screenshot)|fatal|FAILED|Fatal" \
        --elf "$root/build/ps5/link/llvm-pie.elf" --output "$output"
elif (( ${#samples[@]} )); then
    echo "no klog capture on the console (port closed): following the title's own results file"
    python3 "$ps5/tools/run-without-klog.py" "$run_dir" "${RUN_TIMEOUT:-$timeout}" || true
else
    echo "no klog capture on the console, and --menu has nothing else to follow" >&2
    exit 2
fi
if (( ${#samples[@]} )); then
    exec python3 "$ps5/tools/check-run.py" --check "$run_dir" "${samples[@]}"
fi

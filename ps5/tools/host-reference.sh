#!/usr/bin/env bash
# PS5 Vulkan Samples - the reference pictures: the title's code on this PC.
#
#   ps5/tools/host-reference.sh [all|menu|ID...]     FRAMES=300 by default
#
# Builds the title's code for Linux (PS5_HOST_REFERENCE: the same samples, base
# class, launcher and test runs, with a headless surface instead of the
# console's display and no pad), runs a test run on the PC's Vulkan driver, and
# leaves each sample's last frame in klog/host-<time>/<id>.png, at the size
# the console's screenshots have. A picture from another driver tells a RADV
# bug on the console from a sample or asset problem. Run ps5/tools/build.sh
# first: the assets come from the console build's title folder.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
build="$root/build/ps5-host"
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
title_id=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["titleId"])' "$ps5/sce_sys/param.json")
[[ -d $root/dist/$title_id/assets ]] || { echo "no dist/$title_id/assets: run ps5/tools/build.sh first" >&2; exit 2; }

if [[ ! -f $build/build.ninja ]]; then
    cmake -S "$ps5" -B "$build" -G Ninja -DCMAKE_MAKE_PROGRAM="$ninja" -DPS5_HOST_REFERENCE=ON \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
        -DPS5_HOST_APP_ROOT="$build/app" > "$build.configure.log" 2>&1 || { cat "$build.configure.log" >&2; exit 1; }
fi
"$ninja" -C "$build" ps5-samples-host | tail -1
mkdir -p "$build/app"
ln -sfn "$root/dist/$title_id/assets" "$build/app/assets"
ln -sfn "$root/shaders" "$build/app/shaders"

(( $# )) || set -- all
mapfile -t samples < <(python3 "$ps5/tools/check-run.py" --resolve "$@")
printf 'frames %s\nscreenshot\nsamples %s\n' "${FRAMES:-300}" "${samples[*]}" > "$build/app/test-run.txt"
out="$root/klog/host-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$out"
status=0
"$build/ps5-samples-host" 2>&1 | tee "$out/log.txt" | grep -E "\] (sample|samples|test run|ends)|Fatal|rror" || true
grep -q "ends: status 0" "$out/log.txt" || status=1
for sample in "${samples[@]}"; do
    ppm="$build/app/screenshots/$sample.ppm"
    [[ -f $ppm ]] && python3 -c 'import sys; from PIL import Image; Image.open(sys.argv[1]).save(sys.argv[2])' "$ppm" "$out/$sample.png"
done
echo "pictures: $out/*.png"
exit $status

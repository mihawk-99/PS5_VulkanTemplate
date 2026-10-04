#!/usr/bin/env bash
# PS5 Vulkan Template - a preview of the title's own screens, recorded on this PC.
#
#   ps5/tools/record-reel.sh
#
# Runs the host reference build (ps5/tools/host-reference.sh builds it) three
# times, each a test run with a scripted pad (test-run.txt's press lines): the start screen moving across its
# cards, Designs stepping through the gallery with R1, Themes moving down the
# list. Every frame (1920x1080 at the test runs' fixed 60 frames a second) is
# piped to ffmpeg; the three parts are kept as near-lossless masters in
# build/reel/, joined with short cross-fades, and encoded as build/reel/reel.mp4.
# The pictures are the PC driver's; the console draws the same frames within
# the distances ps5/README.md records. (The README's clip is the console's own
# output, through a capture card: skills/ps5-console/references/test-runs.md.)
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
ps5=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
root=$(dirname "$ps5")
build="$root/build/ps5-host"
out="$root/build/reel"
ninja=$(command -v ninja || echo "$HOME/.local/bin/ninja")
[[ -f $build/build.ninja ]] || { echo "run ps5/tools/host-reference.sh once first" >&2; exit 2; }
"$ninja" -C "$build" ps5-samples-host | tail -1
mkdir -p "$out"

# The scripted pad, as test-run.txt's "press <program> <frame> <action>" lines
script="$out/presses.txt"
{
    printf 'press start %d %s\n' 45 right 95 right 150 left
    for i in $(seq 0 11); do printf 'press uikit %d r1\n' $((50 + 60 * i)); done
    for i in $(seq 0 12); do printf 'press themes %d down\n' $((40 + 22 * i)); done
} > "$script"

# One part: a test run of one program for some frames, every frame recorded
record() {
    local name=$1 program=$2 frames=$3
    { printf 'frames %s\nsamples %s\n' "$frames" "$program"; cat "$script"; } > "$build/app/test-run.txt"
    PS5_RECORD="ffmpeg -loglevel error -y -f rawvideo -pix_fmt rgb24 -s 1920x1080 -r 60 -i - -c:v libx264 -preset veryfast -crf 8 -pix_fmt yuv444p $out/$name.mp4" \
        "$build/ps5-samples-host" 2>&1 | grep -E "\] (sample|ends)" || true
}
record 1-start start 200
record 2-designs uikit 780
record 3-themes themes 330

# Joined with 0.3 s cross-fades (each offset is where the next part starts to show)
length() { ffprobe -v error -show_entries format=duration -of csv=p=0 "$1"; }
a=$(length "$out/1-start.mp4")
b=$(length "$out/2-designs.mp4")
first=$(python3 -c "print(round($a - 0.3, 3))")
second=$(python3 -c "print(round($a + $b - 0.6, 3))")
ffmpeg -loglevel error -y -i "$out/1-start.mp4" -i "$out/2-designs.mp4" -i "$out/3-themes.mp4" \
    -filter_complex "[0][1]xfade=transition=fade:duration=0.3:offset=$first[ab];[ab][2]xfade=transition=fade:duration=0.3:offset=$second[v]" \
    -map "[v]" -c:v libx264 -preset slow -crf 8 -pix_fmt yuv444p "$out/reel-master.mp4"

ffmpeg -loglevel error -y -i "$out/reel-master.mp4" -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p \
    -movflags +faststart "$out/reel.mp4"
printf '==> %s: %s frames at 60 fps\n' "build/reel/reel.mp4" \
    "$(ffprobe -v error -count_frames -select_streams v -show_entries stream=nb_read_frames -of csv=p=0 "$out/reel.mp4")"

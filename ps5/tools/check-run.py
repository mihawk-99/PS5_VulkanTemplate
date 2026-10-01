#!/usr/bin/env python3
"""PS5 Vulkan Template - read a test run: klog, screenshots, a verdict per sample.

    check-run.py --resolve all|menu|launcher|ID...   the sample ids a run covers
                                               ("launcher": the menu itself)
    check-run.py --check RUN_DIR ID...         fetch the screenshots of the
                                               samples that ended well, check
                                               klog and the pictures, print a
                                               table; exit 1 on any failure

A sample passes when its klog line says "ok", klog holds no crash record, no
GPU fault and no RADV error line, its screenshot exists and is not one flat
colour, and, when ps5/reference/<id>.png exists (the host reference's picture
of frame 300, tools/host-reference.sh --save) and the run drew 300 frames, the
screenshot is close to it.
The table goes to RUN_DIR/summary.txt too.

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""

import argparse
import io
import re
import sys
from ftplib import FTP, error_perm
from pathlib import Path

PS5 = Path(__file__).resolve().parent.parent
ROOT = PS5.parent
VULKAN = Path(__import__("os").environ.get("PS5_VULKAN_DIR", ROOT.parent / "PS5_Vulkan")).resolve()
TITLE_ID = __import__("json").loads((PS5 / "sce_sys" / "param.json").read_text())["titleId"]

PARAM = __import__("json").loads((PS5 / "sce_sys" / "param.json").read_text())
TITLE_NAME = PARAM["localizedParameters"]["en-US"]["titleName"]
SAMPLE_LINE = re.compile(r"(?:\[" + re.escape(TITLE_NAME) + r"\] |^)sample (\w+): (ok|FAILED)(.*)", re.M)
CRASH = re.compile(r"A user thread receives a fatal signal|mDBG: Sending signal|GPU_FAULT|gpu fault", re.I)
DRIVER_ERROR = re.compile(r"\bradv(/ps5)?: .*(error|fail)|MESA: error|amdgpu: .*fail|Fatal : VkResult", re.I)
# Mean absolute difference (0..255 per channel) a screenshot may have from its reference
REFERENCE_TOLERANCE = 12.0


def samples_in_build():
    listing = ROOT / "build" / "ps5" / "samples.txt"
    if not listing.is_file():
        sys.exit("no build/ps5/samples.txt: build the title first (ps5/tools/build.sh)")
    return [s for s in listing.read_text().strip().split(";") if s]


def samples_in_menu():
    text = (PS5 / "src" / "samples.cpp").read_text()
    return [m.group(1) for m in re.finditer(r"\n\s*SAMPLE\((\w+),\s*true,", text)]


def resolve(words):
    built = samples_in_build()
    chosen = []
    for word in words:
        ids = built if word == "all" else samples_in_menu() if word == "menu" else [word]
        for sample in ids:
            if sample not in built and sample != "launcher":  # "launcher": the menu itself
                sys.exit(f"{sample} is not linked into the title (ps5/src/samples.cpp)")
            if sample not in chosen:
                chosen.append(sample)
    print("\n".join(chosen))


def fetch(names, run_dir):
    sys.path.insert(0, str(VULKAN / "tools"))
    import ps5_console
    settings = ps5_console.load_settings()
    fetched = {}
    with FTP() as ftp:
        ftp.connect(settings["host"], settings["ftp_port"], timeout=30)
        ftp.login(settings["ftp_user"], settings["ftp_password"] or "codex")
        for name in names:
            sink = io.BytesIO()
            try:
                ftp.retrbinary(f"RETR /data/homebrew/{TITLE_ID}/screenshots/{name}.ppm", sink.write)
            except error_perm:
                continue
            path = run_dir / f"{name}.ppm"
            path.write_bytes(sink.getvalue())
            fetched[name] = path
    return fetched


def picture_checks(sample, ppm, run_dir, compare):
    """Flatness and, when there is one and the run's frame budget is the
    references' (300), the distance from the reference."""
    try:
        from PIL import Image, ImageStat
    except ImportError:
        return [], "no Pillow: picture not checked"
    image = Image.open(ppm).convert("RGB")
    png = run_dir / f"{sample}.png"
    image.save(png)
    problems = []
    stddev = sum(ImageStat.Stat(image).stddev) / 3
    if stddev < 2.0:
        problems.append(f"flat picture (stddev {stddev:.1f})")
    note = f"stddev {stddev:.0f}"
    reference = PS5 / "reference" / f"{sample}.png"
    if reference.is_file() and compare:
        ref = Image.open(reference).convert("RGB")
        small = image.resize(ref.size, Image.BILINEAR)
        diff = sum(ImageStat.Stat(__import__("PIL.ImageChops", fromlist=["x"]).difference(small, ref)).mean) / 3
        note += f", {diff:.1f} from the reference"
        if diff > REFERENCE_TOLERANCE:
            problems.append(f"{diff:.1f} from the reference (tolerance {REFERENCE_TOLERANCE})")
    return problems, note


def check(run_dir, samples):
    run_dir = Path(run_dir)
    # klog, or without a klog capture the title's own test-results.txt
    source = run_dir / "klog.log"
    if not source.is_file() or not SAMPLE_LINE.search(source.read_text(errors="replace")):
        source = run_dir / "test-results.txt"
    log = source.read_text(errors="replace") if source.is_file() else ""
    results = {m.group(1): (m.group(2), m.group(3).strip(" ,:")) for m in SAMPLE_LINE.finditer(log)}
    budget = re.search(r"test run: \d+ samples, (\d+) frames each", log)
    # The references (tools/host-reference.sh --save) show frame 300 of each sample
    compare = bool(budget) and budget.group(1) == "300"
    crashes = [line for line in log.splitlines() if CRASH.search(line)]
    driver_errors = [line for line in log.splitlines() if DRIVER_ERROR.search(line)]
    fetched = fetch([s for s in samples if results.get(s, ("",))[0] == "ok"], run_dir)

    rows, failed = [], 0
    for sample in samples:
        status, detail = results.get(sample, ("missing", "no klog line"))
        problems = [] if status == "ok" else [f"{status}: {detail}"]
        note = ""
        if status == "ok":
            if sample not in fetched:
                problems.append("no screenshot")
            else:
                more, note = picture_checks(sample, fetched[sample], run_dir, compare)
                problems += more
        fps = re.search(r"([\d.]+) fps", detail)
        verdict = "PASS" if not problems else "FAIL"
        failed += verdict == "FAIL"
        rows.append(f"{verdict}  {sample:<20} {fps.group(1) + ' fps' if fps else '':>11}  "
                    f"{'; '.join(problems) if problems else note}")
    lines = rows + [""]
    if crashes:
        failed += 1
        lines += ["crash records in klog:"] + [f"  {c}" for c in crashes[:5]]
    if driver_errors:
        failed += 1
        lines += ["driver error lines in klog:"] + [f"  {e}" for e in driver_errors[:10]]
    lines.append(f"{len(samples) - sum(r.startswith('FAIL') for r in rows)} of {len(samples)} samples pass"
                 + ("" if not (crashes or driver_errors) else ", klog is not clean")
                 + ("" if source.name == "klog.log" else " (no klog capture: crash records and driver messages were not checked)"))
    text = "\n".join(lines)
    (run_dir / "summary.txt").write_text(text + "\n")
    print(text)
    print(f"pictures: {run_dir}/*.png")
    return 1 if failed else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--resolve", nargs="+")
    parser.add_argument("--check", nargs="+", metavar=("RUN_DIR", "ID"))
    args = parser.parse_args()
    if args.resolve:
        resolve(args.resolve)
        return 0
    if args.check:
        return check(args.check[0], args.check[1:])
    parser.error("nothing to do")


if __name__ == "__main__":
    sys.exit(main())

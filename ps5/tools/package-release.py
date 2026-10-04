#!/usr/bin/env python3
"""PS5 Vulkan Template - a release: the title's ZIP and the source of every part in it.

    package-release.py TAG [--date YYYY-MM-DD]     writes dist/release-TAG/

It takes the built title folder (dist/<TITLE_ID>, from ps5/tools/build.sh, whose
link staged licenses/components.json) and writes:

  <Project>-TAG.zip     the title folder: directory entries included, deflate, Unix modes
                        kept, every timestamp the release date; tested, unpacked and
                        compared with the folder
  source/<part>-<rev>.tar.gz
                        each part's source at the revision the title was built from
                        (git archive, with the submodules the build used); a part
                        fixed upstream is named in SOURCES.txt with its address
  source/SHA256SUMS, source/SOURCES.txt
  SHA256SUMS            the ZIP's and the archives' digests

It refuses a title whose parts were built from uncommitted source (the notices
record "dirty"), and a folder holding anything a release must never carry (a test
file, a screenshot, klog, .env). The release itself (tag, GitHub release, read-back)
follows skills/ps5-release.

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""
import argparse
import datetime
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

PS5 = Path(__file__).resolve().parent.parent
ROOT = PS5.parent
NEVER = ("test-run.txt", "test-results.txt", "screenshots", "klog", ".env", "radv-shader-cache")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def make_zip(folder, zip_path, when):
    stamp = (when.year, when.month, when.day, 0, 0, 0)
    entries = sorted(folder.rglob("*"))
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        root = zipfile.ZipInfo(folder.name + "/", stamp)
        root.external_attr = (stat.S_IFDIR | 0o755) << 16
        z.writestr(root, b"")
        for path in entries:
            name = folder.name + "/" + path.relative_to(folder).as_posix()
            mode = path.stat().st_mode
            if path.is_dir():
                info = zipfile.ZipInfo(name + "/", stamp)
                info.external_attr = (stat.S_IFDIR | (mode & 0o777)) << 16
                z.writestr(info, b"")
            else:
                info = zipfile.ZipInfo(name, stamp)
                info.external_attr = (stat.S_IFREG | (mode & 0o777)) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, path.read_bytes(), compresslevel=9)


def check_zip(zip_path, folder):
    with zipfile.ZipFile(zip_path) as z:
        bad = z.testzip()
        if bad:
            raise SystemExit(f"{zip_path.name}: {bad} is damaged")
        with tempfile.TemporaryDirectory() as tmp:
            z.extractall(tmp)
            unpacked = Path(tmp) / folder.name
            for path in folder.rglob("*"):
                if path.is_file() and sha256(path) != sha256(unpacked / path.relative_to(folder)):
                    raise SystemExit(f"{zip_path.name}: {path.relative_to(folder)} differs once unpacked")
            count = sum(1 for p in unpacked.rglob("*") if p.is_file())
    return count


def archive(repo, revision, prefix, dest):
    with open(dest, "wb") as out:
        subprocess.run(["git", "-C", str(repo), "archive", "--format=tar.gz", f"--prefix={prefix}/", revision],
                       stdout=out, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("tag")
    parser.add_argument("--date", default=datetime.date.today().isoformat())
    args = parser.parse_args()
    when = datetime.date.fromisoformat(args.date)

    param = json.loads((PS5 / "sce_sys/param.json").read_text())
    title_id = param["titleId"]
    folder = ROOT / "dist" / title_id
    parts = json.loads((folder / "licenses/components.json").read_text())
    dirty = [p["id"] for p in parts if p["source"].get("dirty")]
    if dirty:
        raise SystemExit(f"built from uncommitted source: {', '.join(dirty)}; commit, rebuild, then package")
    found = [str(p.relative_to(folder)) for p in folder.rglob("*") if p.name in NEVER]
    if found:
        raise SystemExit(f"the title folder holds what a release never carries: {found}")

    out = ROOT / "dist" / f"release-{args.tag}"
    shutil.rmtree(out, ignore_errors=True)
    (out / "source").mkdir(parents=True)
    project = ROOT.name
    zip_path = out / f"{project}-{args.tag}.zip"
    make_zip(folder, zip_path, when)
    files = check_zip(zip_path, folder)
    print(f"==> {zip_path.name}: {files} files, {zip_path.stat().st_size / 1e6:.1f} MB, tested and compared")

    sources = []
    for p in parts:
        src = p["source"]
        if src["kind"] == "git":
            name = f'{p["id"]}-{src["revision"][:12]}'
            archive(src["path"], src["revision"], name, out / "source" / f"{name}.tar.gz")
            sources.append((f"{name}.tar.gz", p["name"], f'{src["remote"]} at {src["revision"]}'))
            for sub, info in src.get("submodules", {}).items():
                sname = f'{p["id"]}-{Path(sub).name}-{info["revision"][:12]}'
                archive(Path(src["path"]) / sub, info["revision"], sname, out / "source" / f"{sname}.tar.gz")
                sources.append((f"{sname}.tar.gz", f'{sub}, a submodule of the title', f'{info["remote"]} at {info["revision"]}'))
            if src.get("built_with_sdk"):
                sdk = ROOT.parent / "PS5_PayloadSDK"
                sname = f'{p["id"]}-sdk-{src["built_with_sdk"][:12]}'
                archive(sdk, src["built_with_sdk"], sname, out / "source" / f"{sname}.tar.gz")
                sources.append((f"{sname}.tar.gz", "the payload SDK fork the RADV archive was built against",
                                f'https://github.com/mihawk-99/PS5_PayloadSDK at {src["built_with_sdk"]}'))
        else:
            sources.append(("(not attached)", p["name"], f'{src["revision"]}: {src["url"]}'))
    with open(out / "source/SOURCES.txt", "w") as f:
        f.write(f"The source of {param['localizedParameters']['en-US']['titleName']} {args.tag} ({title_id}), "
                f"one archive per part, at the revision the title was built from.\n\n")
        for file, what, where in sources:
            f.write(f"{file}\n  {what}\n  {where}\n\n")
    sums = [(sha256(a), a.name) for a in sorted((out / "source").glob("*.tar.gz"))]
    (out / "source/SHA256SUMS").write_text("".join(f"{h}  {n}\n" for h, n in sums))
    (out / "SHA256SUMS").write_text(f"{sha256(zip_path)}  {zip_path.name}\n" + "".join(f"{h}  source/{n}\n" for h, n in sums))
    total = sum(a.stat().st_size for a in (out / "source").glob("*.tar.gz"))
    print(f"==> source: {len(sums)} archives, {total / 1e6:.1f} MB; SHA256SUMS, SOURCES.txt")
    print(f"==> sha256 {sha256(zip_path)}  {zip_path.name}")


if __name__ == "__main__":
    main()

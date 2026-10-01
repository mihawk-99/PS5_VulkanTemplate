#!/usr/bin/env python3
"""PS5 Vulkan Samples - the assets the title ships, and their notices.

    build-assets.py --install dist/PPSA99130    lay the assets of the samples
                                                 linked in (build/ps5/samples.txt)
                                                 into the title folder's assets/
    build-assets.py --notices                   rewrite ps5/ASSETS.md

Every asset comes from ps5/assets.json, which names its origin and licence:
"pack" (the assets submodule, at the revision this repository pins),
or "generated" (made by ps5/tools/generate_assets.py, deterministically,
from geometry and noise written there or from CC0 files it downloads at pinned
SHA-256 hashes into build/ps5/downloads/). The
title's assets/NOTICES.txt lists what was installed, with the licences.

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""

import argparse
import hashlib
import json
import os
import shutil
import sys
import urllib.request
from pathlib import Path

PS5 = Path(__file__).resolve().parent.parent
ROOT = PS5.parent
BUILD = ROOT / "build" / "ps5"
DOWNLOADS = BUILD / "downloads"
GENERATED = BUILD / "generated"


def load_manifest():
    return json.loads((PS5 / "assets.json").read_text())


def linked_samples():
    listing = BUILD / "samples.txt"
    if not listing.is_file():
        sys.exit("no build/ps5/samples.txt: build the title first (ps5/tools/build.sh)")
    return [s for s in listing.read_text().strip().split(";") if s]


def download(url, sha256):
    DOWNLOADS.mkdir(parents=True, exist_ok=True)
    target = DOWNLOADS / sha256
    if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == sha256:
        return target
    print(f"download {url}")
    request = urllib.request.Request(url, headers={"User-Agent": "ps5-vulkan-samples-asset-build"})
    with urllib.request.urlopen(request, timeout=120) as response:
        data = response.read()
    actual = hashlib.sha256(data).hexdigest()
    if actual != sha256:
        sys.exit(f"{url}: sha256 {actual}, expected {sha256}")
    target.write_bytes(data)
    return target


def source_path(asset, manifest):
    kind = asset["from"]
    if kind == "pack":
        path = ROOT / "assets" / asset["source"]
        if not path.exists():
            sys.exit(f"{path} is missing: git submodule update --init assets")
        return path
    if kind == "generated":
        sys.path.insert(0, str(PS5 / "tools"))
        import generate_assets
        generate_assets.build(asset["generator"], GENERATED, download)
        path = GENERATED / asset["path"]
        if not path.exists():
            sys.exit(f"generator {asset['generator']} did not make {asset['path']}")
        return path
    sys.exit(f"{asset['path']}: unknown source kind {kind}")


def copy_asset(source, target):
    """A file or a folder; a .gltf brings the buffers and images it names."""
    target.parent.mkdir(parents=True, exist_ok=True)
    if source.is_dir():
        shutil.copytree(source, target, copy_function=shutil.copy2,
                        ignore=shutil.ignore_patterns("screenshot", "README.md"), dirs_exist_ok=True)
        return
    shutil.copy2(source, target)
    if source.suffix == ".gltf":
        gltf = json.loads(source.read_text())
        for item in gltf.get("buffers", []) + gltf.get("images", []):
            uri = item.get("uri", "")
            if uri and not uri.startswith("data:"):
                (target.parent / uri).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source.parent / uri, target.parent / uri)


def install(folder):
    manifest = load_manifest()
    samples = set(linked_samples())
    destination = Path(folder) / "assets"
    staging = Path(folder) / "assets.new"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    installed = []
    for asset in manifest["assets"]:
        if "*" not in asset["used_by"] and not samples.intersection(asset["used_by"]):
            continue
        copy_asset(source_path(asset, manifest), staging / asset["path"])
        if asset.get("licence_file"):
            licence = PS5 / asset["licence_file"]
            (staging / "LICENSES").mkdir(exist_ok=True)
            shutil.copy2(licence, staging / "LICENSES" / licence.name)
        installed.append(asset)
    (staging / "NOTICES.txt").write_text(notices_text(installed))
    if destination.exists():
        shutil.rmtree(destination)
    staging.rename(destination)
    size = sum(f.stat().st_size for f in destination.rglob("*") if f.is_file())
    print(f"==> assets: {len(installed)} entries, {size / 1e6:.1f} MB in {destination}")


def notices_text(assets):
    lines = ["PS5 Vulkan Samples - the assets in this folder, their authors and licences.", ""]
    for asset in assets:
        lines.append(f"{asset['path']}: {asset['title']}, {asset['author']}. {asset['licence']}. {asset['url']}")
        if asset.get("changes"):
            lines.append(f"    Changes: {asset['changes']}")
        if asset.get("licence_file"):
            lines.append(f"    Licence text: LICENSES/{Path(asset['licence_file']).name}")
    lines += ["", "CC0-1.0: https://creativecommons.org/publicdomain/zero/1.0/",
              "CC-BY-3.0: https://creativecommons.org/licenses/by/3.0/",
              "MIT: made for PS5 Vulkan Samples (github.com/mihawk-99/PS5_VulkanSamples), under its LICENSE.md"]
    return "\n".join(lines) + "\n"


def write_notices():
    manifest = load_manifest()
    out = ["# Assets", "",
           "The assets the PS5 title ships, written from `ps5/assets.json` by",
           "`ps5/tools/build-assets.py --notices`. Only assets with a clear licence are",
           "shipped. The rest of the asset pack (the `assets` submodule) stays out of the",
           "title, and the samples that use it get replacements, listed here.", "",
           "| Path under `/app0/assets/` | Asset | Author | Licence | Samples |",
           "| --- | --- | --- | --- | --- |"]
    for asset in manifest["assets"]:
        used = ", ".join(asset["used_by"]).replace("*", "all")
        title = f"[{asset['title']}]({asset['url']})"
        if asset.get("changes"):
            title += f" ({asset['changes']})"
        out.append(f"| `{asset['path']}` | {title} | {asset['author']} | {asset['licence']} | {used} |")
    (PS5 / "ASSETS.md").write_text("\n".join(out) + "\n")
    print(f"wrote {PS5 / 'ASSETS.md'}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--install", metavar="TITLE_FOLDER")
    parser.add_argument("--notices", action="store_true")
    args = parser.parse_args()
    if not args.install and not args.notices:
        parser.error("nothing to do")
    if args.notices:
        write_notices()
    if args.install:
        install(args.install)


if __name__ == "__main__":
    main()

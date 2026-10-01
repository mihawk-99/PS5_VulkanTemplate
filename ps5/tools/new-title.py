#!/usr/bin/env python3
"""PS5 Vulkan Samples - make a new PS5 homebrew title on this foundation.

    ps5/tools/new-title.py DIRECTORY --title-id PPSA12345 --name "My Title" [--refresh 60] [--force]

The new title is this repository's foundation with one program: the base class
(base/, with its PS5 hooks), the libraries it uses (external/), the PS5 layer
(ps5/: the launch, the pad, klog, test runs, the build, the tools) and the
starter sample (examples/starter/) renamed to the title's own program. It is
built, deployed, run and checked exactly as the samples are, and a technique
from any sample can be copied into it as it stands, since every sample is a
class on the same base. The foundation is copied at this repository's last
commit (git archive), and its revision is recorded in the new title's README.

The title id is PPSA and five digits, unique on the console: the ids my titles
use are refused. The name is what the home screen shows: up to 40 letters,
digits, spaces, '.', '_' or '-'. --refresh 60 leaves out param.json's
high-frame-rate bits (the default asks for 119.88 Hz).

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""

import argparse
import datetime
import io
import json
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

PS5 = Path(__file__).resolve().parent.parent
ROOT = PS5.parent

TAKEN = {
    "PPSA99002": "ProsperoLight",
    "PPSA99008": "ProsperoEden",
    "PPSA99010": "PS5 vkQuake",
    "PPSA99014": "PS5_Vulkan's RADV test title",
    "PPSA99015": "PS5_Vulkan's CTS title",
    "PPSA99100": "the old ps5-homebrew-template",
    "PPSA99130": "PS5 Vulkan Samples",
    "PPSA99169": "PS5 RetroArch",
    "PPSA99988": "PS5_Vulkan's test runner",
    "PPSA99996": "a PS5_Vulkan canary",
    "PPSA99997": "a PS5_Vulkan canary",
    "PPSA99998": "a PS5_Vulkan canary",
    "PPSA99999": "PS5_Vulkan's default profile",
}

# What the foundation is: these paths of this repository, at its last commit
FOUNDATION = ["base", "external", "ps5", "shaders/glsl/base", "LICENSE.md", ".gitignore"]
# ... less what belongs to the samples title only
LEFT_OUT = re.compile(r"^(base/Vulkan(Android|OHOS)\.|ps5/(reference/|README\.md|ASSETS\.md|sce_sys/|assets\.json|tools/new-title\.py))")


def param_json(title_id, name, refresh):
    label = re.sub(r"[^A-Z0-9]", "", name.upper())[:16].ljust(16, "0")
    return {
        "ageLevel": {"default": 0},
        "applicationCategoryType": 0,
        "applicationDrmType": "free",
        "attribute": 0,
        "attribute2": 0,
        # 0x80040: high-frame-rate output. With it, RADV's VideoOut swapchain
        # asks for 119.88 Hz and falls back to 59.94 Hz when the display stays
        # there (PS5_Mesa, wsi_common_videoout.c).
        "attribute3": 0x80040 if refresh == 120 else 0,
        "conceptId": title_id[4:],
        "contentBadgeType": 1,
        "contentId": f"UP9000-{title_id}_00-{label}",
        "contentVersion": "01.000.000",
        "downloadDataSize": 256,
        "gameIntent": {"permittedIntents": [{"intentType": "launchActivity"}]},
        "localizedParameters": {"defaultLanguage": "en-US", "en-US": {"titleName": name}},
        "masterVersion": "01.00",
        "pubtools": {
            "creationDate": datetime.date.today().strftime("%Y-%m-%d 00:00:00"),
            "loudnessSnd0": "-28.00",
            "toolVersion": "2.00",
        },
        "requiredSystemSoftwareVersion": "0x0000000000000000",
        "sdkVersion": "0x0000000000000000",
        "titleId": title_id,
        "versionFileUri": "",
    }


def icon(path, name):
    """A 512x512 placeholder icon: a dark gradient and the title's initials."""
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("note: Pillow is not installed; add ps5/sce_sys/icon0.png (512x512 PNG) yourself")
        return
    img = Image.new("RGB", (512, 512))
    pixels = img.load()
    for y in range(512):
        for x in range(512):
            t = (x + y) / 1022.0
            pixels[x, y] = (int(12 + 70 * t), int(10 + 10 * t), int(40 + 110 * t))
    draw = ImageDraw.Draw(img)
    initials = "".join(word[0] for word in name.split()[:2]).upper() or "PS"
    font = None
    for candidate in ("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
                      "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
        if Path(candidate).is_file():
            font = ImageFont.truetype(candidate, 200)
            break
    if font is None:
        font = ImageFont.load_default()
    box = draw.textbbox((0, 0), initials, font=font)
    draw.text(((512 - (box[2] - box[0])) / 2 - box[0], (512 - (box[3] - box[1])) / 2 - box[1]),
              initials, fill=(240, 236, 248), font=font)
    img.save(path)


def export(target):
    """The foundation's files at this repository's last commit, and glm (a submodule) at its own."""
    archive = subprocess.run(["git", "-C", str(ROOT), "archive", "HEAD", *FOUNDATION, "examples/starter",
                              "shaders/glsl/starter"], capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        members = [m for m in tar.getmembers() if not LEFT_OUT.match(m.name)]
        tar.extractall(target, members=members, filter="data")
    glm = subprocess.run(["git", "-C", str(ROOT / "external/glm"), "archive", "--prefix=external/glm/", "HEAD"],
                         capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(glm)) as tar:
        tar.extractall(target, filter="data")
    return subprocess.run(["git", "-C", str(ROOT), "rev-parse", "HEAD"], capture_output=True, text=True,
                          check=True).stdout.strip()


def program_id(name):
    word = re.sub(r"[^a-z0-9]", "", name.lower())
    return word if word and word[0].isalpha() else "title" + word


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--title-id", required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--refresh", type=int, choices=(60, 120), default=120)
    parser.add_argument("--force", action="store_true", help="accept a title id from the taken list")
    args = parser.parse_args()

    if not re.fullmatch(r"PPSA\d{5}", args.title_id):
        sys.exit("--title-id: PPSA and five digits")
    if args.title_id in TAKEN and not args.force:
        sys.exit(f"{args.title_id} is {TAKEN[args.title_id]}'s title id; pick another")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9 ._-]{0,39}", args.name):
        sys.exit("--name: 1 to 40 letters, digits, spaces, '.', '_' or '-'")
    if subprocess.run(["git", "-C", str(ROOT), "status", "--porcelain", "--", *FOUNDATION, "examples/starter",
                       "shaders/glsl/starter"], capture_output=True, text=True).stdout.strip():
        print("note: the foundation has uncommitted changes; the new title gets its last commit")
    target = args.directory.resolve()
    if target.exists() and any(target.iterdir()):
        sys.exit(f"{target} exists and is not empty")
    target.mkdir(parents=True, exist_ok=True)
    revision = export(target)
    pid = program_id(args.name)

    # The program: the starter, renamed
    (target / "examples/starter").rename(target / f"examples/{pid}")
    (target / f"examples/{pid}/starter.cpp").rename(target / f"examples/{pid}/{pid}.cpp")
    (target / "shaders/glsl/starter").rename(target / f"shaders/glsl/{pid}")
    program = target / f"examples/{pid}/{pid}.cpp"
    text = program.read_text()
    text = text.replace(" * Starter - the program a new PS5 homebrew grows from.", f" * {args.name} - its program, made from the PS5 Vulkan Samples starter.")
    text = text.replace('title = "Starter";', f'title = "{args.name}";')
    text = text.replace('"starter/model.', f'"{pid}/model.')
    program.write_text(text)

    # The programs it links: this one
    samples = target / "ps5/src/samples.cpp"
    text = samples.read_text()
    start = text.index("#define PS5_SAMPLES")
    end = text.index("#define SAMPLE(id, menu, title, description) \\\n\tnamespace")
    text = (text[:start] + "#define PS5_SAMPLES \\\n"
            f"\tSAMPLE({pid}, true, \"{args.name}\", \"{args.name}\")\n\n" + text[end:])
    samples.write_text(text)

    # Its identity
    (target / "ps5/sce_sys").mkdir(parents=True, exist_ok=True)
    (target / "ps5/sce_sys/param.json").write_text(json.dumps(param_json(args.title_id, args.name, args.refresh), indent=2) + "\n")
    icon(target / "ps5/sce_sys/icon0.png", args.name)

    # Its assets: the font (kept in the repository) and the starter's model
    manifest = json.loads((PS5 / "assets.json").read_text())
    kept = []
    for asset in manifest["assets"]:
        if asset["path"] == "Roboto-Medium.ttf":
            (target / "ps5/assets").mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / "assets" / asset["source"], target / "ps5/assets/Roboto-Medium.ttf")
            kept.append(dict(asset, **{"from": "file", "source": "ps5/assets/Roboto-Medium.ttf"}))
        elif "starter" in asset["used_by"]:
            kept.append(dict(asset, used_by=[pid]))
    manifest["comment"] = (f"The assets {args.name} ships, each with its origin and licence (ps5/tools/build-assets.py). "
                           "Add each new one here, with its licence: only assets with a clear licence are shipped.")
    manifest["sources"]["file"] = "a file kept in this repository"
    manifest["assets"] = kept
    (target / "ps5/assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (target / ".gitignore").write_text("build/\ndist/\nklog/\n.deps/\nimgui.ini\n")

    (target / "README.md").write_text(README.format(name=args.name, title_id=args.title_id, pid=pid,
                                                    revision=revision, short=revision[:8]))
    subprocess.run(["python3", str(target / "ps5/tools/build-assets.py"), "--notices"], check=True,
                   capture_output=True)
    subprocess.run(["git", "init", "-q", "-b", "main", str(target)], check=True)
    subprocess.run(["git", "-C", str(target), "add", "-A"], check=True)
    subprocess.run(["git", "-C", str(target), "commit", "-q", "-m",
                    f"{args.name}: a new title on the PS5 Vulkan Samples foundation\n\n"
                    f"Made by ps5/tools/new-title.py from PS5_VulkanSamples {revision[:12]}: the base class,\n"
                    f"the PS5 layer and its tools, and the starter as the program ({pid})."], check=True)
    print(f"==> {args.name} ({args.title_id}) in {target}")
    print(f"    program: examples/{pid}/{pid}.cpp, shaders/glsl/{pid}/")
    print("    next: ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh")


README = """# {name}

A PS5 homebrew title (`{title_id}`) on RADV, made from the
[PS5 Vulkan Samples](https://github.com/mihawk-99/PS5_VulkanSamples) foundation at
`{short}`: Sascha Willems' Vulkan example base class with its PS5 hooks, the PS5 layer
(the launch, the pad, klog, test runs, the build and the console tools), and one
program, `examples/{pid}/{pid}.cpp`, grown from the starter sample.

## Building and running

It builds against my PS5 stack, checked out beside it: PS5_Vulkan (the RADV release
archive, the link recipe, the native tool and `libc.prx`) and the payload SDK fork
(`ps5/tools/setup-sdk.sh` installs it at the pinned revision).

```bash
ps5/tools/build.sh              # dist/{title_id}/: eboot.bin, sce_sys, shaders, assets
ps5/tools/deploy.sh             # upload what changed, over the console's FTP server
ps5/tools/run.sh                # a test run: 300 frames, the last one saved and checked
ps5/tools/run.sh --menu         # no test: the program, until it ends itself
ps5/tools/host-reference.sh     # the same frames on this PC's Vulkan driver
```

A launch from the home screen starts the program at once. The sticks move the camera,
OPTIONS or the touch pad shows and hides the settings window, the D-pad and CROSS
change them, and its Quit ends the title.

## Growing it

- **The program** is one class on the base class, `examples/{pid}/{pid}.cpp`, with its
  shaders in `shaders/glsl/{pid}/` (GLSL beside the SPIR-V it loads: after changing one,
  `ps5/tools/compile-shaders.sh {pid}`).
- **A technique from PS5 Vulkan Samples** (shadows, deferred lighting, bloom, MSAA,
  instancing, indirect draws, compute, bindless textures, mesh shaders, ray queries)
  copies across as it stands: every sample is a class on the same base.
- **Assets** go in `ps5/assets.json`, each with its origin and licence; only assets
  with a clear licence are shipped (`ps5/ASSETS.md` is written from it).
- **More programs**: another `SAMPLE(...)` line in `ps5/src/samples.cpp` turns the
  title into a menu of them, as PS5 Vulkan Samples is.
- How the foundation works, its test runs and its tools: PS5 Vulkan Samples'
  `ps5/README.md`.

## Licences

The foundation is MIT (`LICENSE.md`, Sascha Willems'; the PS5 layer is mine, under the
same licence). The assets keep their own (`ps5/ASSETS.md`). The built title links the
PS5 platform layer of my payload SDK fork, which is GPL-3.0, so the title as
distributed is under GPL-3.0.
"""


if __name__ == "__main__":
    main()

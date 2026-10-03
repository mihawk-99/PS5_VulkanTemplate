#!/usr/bin/env python3
"""PS5 Vulkan Template - make a new PS5 homebrew title on this foundation.

    ps5/tools/new-title.py DIRECTORY --title-id PPSA12345 --name "My Title" [--refresh 60] [--force]
    ps5/tools/new-title.py DIRECTORY --title-id PPSA12345 --name "My Title" --ui DESIGN

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

--ui DESIGN starts from the UI module instead of the 3D starter: the program is
the UI starter (examples/uiscreen/), and its screen is a copy of one of the UI
kit's designs (aurora, settings, store, hud...: --ui list names them), in the
program's kit/screen.cpp, to change at will. The UI module, the program and the
kit's assets are GPL-3.0-or-later (ps5/ui/README.md); without --ui the new
title has none of them, and its source stays MIT.

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
LEFT_OUT = re.compile(r"^(base/Vulkan(Android|OHOS)\.|ps5/(reference(/|$)|README\.md|ASSETS\.md|sce_sys(/|$)|assets\.json|tools/new-title\.py))")
# ... and, for a title without --ui, the GPL-3.0 UI module
UI_ONLY = re.compile(r"^ps5/(ui(/|$)|licenses/GPL-3\.0\.txt$)")
# The program a title grows from: the 3D starter, or the UI starter
STARTERS = {False: ["examples/starter", "shaders/glsl/starter"], True: ["examples/uiscreen"]}


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


def export(target, ui):
    """The foundation's files at this repository's last commit, and glm (a submodule) at its own."""
    archive = subprocess.run(["git", "-C", str(ROOT), "archive", "HEAD", *FOUNDATION, *STARTERS[ui]],
                             capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        members = [m for m in tar.getmembers()
                   if not LEFT_OUT.match(m.name) and (ui or not UI_ONLY.match(m.name))]
        tar.extractall(target, members=members, filter="data")
    glm = subprocess.run(["git", "-C", str(ROOT / "external/glm"), "archive", "--prefix=external/glm/", "HEAD"],
                         capture_output=True, check=True).stdout
    with tarfile.open(fileobj=io.BytesIO(glm)) as tar:
        tar.extractall(target, filter="data")
    return subprocess.run(["git", "-C", str(ROOT), "rev-parse", "HEAD"], capture_output=True, text=True,
                          check=True).stdout.strip()


def kit_designs(kit):
    """The kit's designs a title can start from: one file each (the component
    library is a gallery of pages, not a screen)."""
    return sorted(p.stem for p in (kit / "src/concepts").glob("*.cpp") if p.stem not in ("registry", "components"))


SCREEN_TAIL = """
// The title's screen, for its program (ps5/ui/kit.hpp declares it)
namespace ps5ui
{{
std::unique_ptr<hui::app::Concept> make_screen(hui::app::Context &context)
{{
    return hui::screen::make_{design}(context);
}}
}} // namespace ps5ui
"""


def screen_source(kit, design, name, pid):
    """The kit's design, as the title's own screen: its namespace renamed so it
    cannot meet the kit's copy, and ps5ui::make_screen to make it."""
    text = (kit / f"src/concepts/{design}.cpp").read_text()
    revision = (kit / ".revision").read_text().strip()
    lines = text.split("\n")
    spdx = next(i for i, line in enumerate(lines) if "SPDX-License-Identifier" in line)
    lines[spdx + 1:spdx + 1] = [
        "//",
        f"// Copied into {name} by PS5_VulkanTemplate's new-title.py from PS5_VKHomebrewUI",
        f"// {revision[:7]} (src/concepts/{design}.cpp), in a namespace of its own: change it at",
        f"// will. Its program (../{pid}.cpp) draws it through ps5ui::make_screen, below.",
    ]
    text = "\n".join(lines)
    text = text.replace("namespace hui::concepts", "namespace hui::screen")
    text = text.replace(f'"src/concepts/{design}.cpp"', f'"examples/{pid}/kit/screen.cpp"')
    return text.rstrip("\n") + "\n" + SCREEN_TAIL.format(design=design)


def ui_program(text, name, pid, design):
    """The UI starter, as the title's program: its name, and its own screen."""
    swaps = [
        (" * UI starter - the program a homebrew whose face is one of the kit's designs grows from.",
         f" * {name} - its program, made from the PS5 Vulkan Template's UI starter."),
        (" * program). Here the design is the kit's own Aurora Shelf, a console home\n"
         " * screen; ps5/tools/new-title.py --ui <design> makes a title whose program is\n"
         " * this file and whose design is a copy of the one named, in kit/screen.cpp,\n"
         " * to change at will.",
         f" * program). The design is this title's copy of the kit's \"{design}\", in\n"
         " * kit/screen.cpp, to change at will."),
        ("// The design on screen\n", "// The design on screen: this title's own, in kit/screen.cpp\n"),
        ("\treturn hui::concepts::make_aurora(context);", "\treturn ps5ui::make_screen(context);"),
        ('title = "UI starter";', f'title = "{name}";'),
        ('name = "uiscreen";', f'name = "{pid}";'),
    ]
    for old, new in swaps:
        if old not in text:
            sys.exit(f"examples/uiscreen/uiscreen.cpp no longer has: {old.splitlines()[0]}")
        text = text.replace(old, new)
    return text


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
    parser.add_argument("--ui", metavar="DESIGN", help="start from the UI kit's design DESIGN ('list' names them)")
    args = parser.parse_args()
    ui = args.ui is not None
    if ui:
        kit = Path(subprocess.run(["bash", str(PS5 / "ui/setup-kit.sh")], capture_output=True, text=True,
                                  check=True).stdout.strip())
        designs = kit_designs(kit)
        if args.ui not in designs:
            sys.exit(("" if args.ui == "list" else f"--ui: no design named {args.ui}. ")
                     + "The designs: " + ", ".join(designs))

    if not re.fullmatch(r"PPSA\d{5}", args.title_id):
        sys.exit("--title-id: PPSA and five digits")
    if args.title_id in TAKEN and not args.force:
        sys.exit(f"{args.title_id} is {TAKEN[args.title_id]}'s title id; pick another")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9 ._-]{0,39}", args.name):
        sys.exit("--name: 1 to 40 letters, digits, spaces, '.', '_' or '-'")
    if subprocess.run(["git", "-C", str(ROOT), "status", "--porcelain", "--", *FOUNDATION, *STARTERS[ui]],
                      capture_output=True, text=True).stdout.strip():
        print("note: the foundation has uncommitted changes; the new title gets its last commit")
    target = args.directory.resolve()
    if target.exists() and any(target.iterdir()):
        sys.exit(f"{target} exists and is not empty")
    target.mkdir(parents=True, exist_ok=True)
    revision = export(target, ui)
    pid = program_id(args.name)

    if ui:
        # The program: the UI starter, renamed, and the design it draws, copied
        (target / "examples/uiscreen").rename(target / f"examples/{pid}")
        (target / f"examples/{pid}/uiscreen.cpp").rename(target / f"examples/{pid}/{pid}.cpp")
        program = target / f"examples/{pid}/{pid}.cpp"
        program.write_text(ui_program(program.read_text(), args.name, pid, args.ui))
        (target / f"examples/{pid}/kit").mkdir()
        (target / f"examples/{pid}/kit/screen.cpp").write_text(screen_source(kit, args.ui, args.name, pid))
    else:
        # The program: the starter, renamed
        (target / "examples/starter").rename(target / f"examples/{pid}")
        (target / f"examples/{pid}/starter.cpp").rename(target / f"examples/{pid}/{pid}.cpp")
        (target / "shaders/glsl/starter").rename(target / f"shaders/glsl/{pid}")
        program = target / f"examples/{pid}/{pid}.cpp"
        text = program.read_text()
        text = text.replace(" * Starter - the program a new PS5 homebrew grows from.", f" * {args.name} - its program, made from the PS5 Vulkan Template starter.")
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
    # ... with no variants: those are the samples title's
    text = re.sub(r"#define PS5_VARIANTS \\\n(\tVARIANTS\(.*\)( \\)?\n)+", "#define PS5_VARIANTS\n", text)
    samples.write_text(text)

    # Its identity
    (target / "ps5/sce_sys").mkdir(parents=True, exist_ok=True)
    (target / "ps5/sce_sys/param.json").write_text(json.dumps(param_json(args.title_id, args.name, args.refresh), indent=2) + "\n")
    icon(target / "ps5/sce_sys/icon0.png", args.name)

    # Its assets: the overlay's font and the starter's model, fetched or made at build time
    manifest = json.loads((PS5 / "assets.json").read_text())
    kept = []
    for asset in manifest["assets"]:
        if asset["path"] == "Roboto-Medium.ttf":
            kept.append(asset)
        elif ui and "@ui" in asset["used_by"]:
            kept.append(asset)
        elif not ui and "starter" in asset["used_by"]:
            kept.append(dict(asset, used_by=[pid]))
    manifest["comment"] = (f"The assets {args.name} ships, each with its origin and licence (ps5/tools/build-assets.py). "
                           "Add each new one here, with its licence: only assets with a clear licence are shipped.")
    manifest["sources"]["file"] = "a file kept in this repository (ps5/assets/)"
    manifest["assets"] = kept
    if not ui:
        manifest["sources"].pop("kit", None)
    (target / "ps5/assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (target / ".gitignore").write_text("build/\ndist/\nklog/\n.deps/\nimgui.ini\n")

    (target / "README.md").write_text((UI_README if ui else README).format(
        name=args.name, title_id=args.title_id, pid=pid, revision=revision, short=revision[:8], design=args.ui))
    subprocess.run(["python3", str(target / "ps5/tools/build-assets.py"), "--notices"], check=True,
                   capture_output=True)
    subprocess.run(["git", "init", "-q", "-b", "main", str(target)], check=True)
    subprocess.run(["git", "-C", str(target), "add", "-A"], check=True)
    subprocess.run(["git", "-C", str(target), "commit", "-q", "-m",
                    f"{args.name}: a new title on the PS5 Vulkan Template foundation\n\n"
                    f"Made by ps5/tools/new-title.py from PS5_VulkanTemplate {revision[:12]}: the base class,\n"
                    + (f"the PS5 layer and its tools, and the UI starter as the program ({pid}), with\n"
                       f"the UI kit's \"{args.ui}\" design as its screen." if ui else
                       f"the PS5 layer and its tools, and the starter as the program ({pid}).")], check=True)
    print(f"==> {args.name} ({args.title_id}) in {target}")
    if ui:
        print(f"    program: examples/{pid}/{pid}.cpp, its screen examples/{pid}/kit/screen.cpp ({args.ui})")
    else:
        print(f"    program: examples/{pid}/{pid}.cpp, shaders/glsl/{pid}/")
    print("    next: ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh")


README = """# {name}

A PS5 homebrew title (`{title_id}`) on RADV, made from the
[PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate) foundation at
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
- **A technique from the template's samples** (shadows, deferred lighting, bloom, MSAA,
  instancing, indirect draws, compute, bindless textures, mesh shaders, ray queries)
  copies across as it stands: every sample is a class on the same base.
- **Assets** go in `ps5/assets.json`, each with its origin and licence; only assets
  with a clear licence are shipped (`ps5/ASSETS.md` is written from it).
- **More programs**: another `SAMPLE(...)` line in `ps5/src/samples.cpp` turns the
  title into a menu of them, as PS5 Vulkan Samples is.
- How the foundation works, its test runs and its tools: PS5_VulkanTemplate's
  `ps5/README.md`; the agent skills for this stack are in its `skills/`.

## Licences

The foundation is MIT (`LICENSE.md`, Sascha Willems'; the PS5 layer is mine, under the
same licence). The assets keep their own (`ps5/ASSETS.md`). The built title links the
PS5 platform layer of my payload SDK fork, which is GPL-3.0, so the title as
distributed is under GPL-3.0.
"""


UI_README = """# {name}

A PS5 homebrew title (`{title_id}`) on RADV, made from the
[PS5 Vulkan Template](https://github.com/mihawk-99/PS5_VulkanTemplate) foundation at
`{short}` with its UI module: Sascha Willems' Vulkan example base class with its PS5
hooks, the PS5 layer (the launch, the pad, sound, klog, test runs, the build and the
console tools), BlackBearReloaded's UI kit drawn with Vulkan (through my fork
PS5_VKHomebrewUI), and one program, `examples/{pid}/{pid}.cpp`, whose screen is its
own copy of the kit's "{design}" design, `examples/{pid}/kit/screen.cpp`.

## Building and running

It builds against my PS5 stack, checked out beside it: PS5_Vulkan (the RADV release
archive, the link recipe, the native tool and `libc.prx`), the payload SDK fork
(`ps5/tools/setup-sdk.sh` installs it at the pinned revision) and PS5_VKHomebrewUI
(`ps5/ui/setup-kit.sh` exports the kit at the pinned revision, from GitHub when it is
not beside it).

```bash
ps5/tools/build.sh              # dist/{title_id}/: eboot.bin, sce_sys, assets (the kit's fonts and sounds)
ps5/tools/deploy.sh             # upload what changed, over the console's FTP server
ps5/tools/run.sh                # a test run: 300 frames, the last one saved and checked
ps5/tools/run.sh --menu         # no test: the program, until it ends itself
ps5/tools/host-reference.sh     # the same frames on this PC's Vulkan driver
```

A launch from the home screen shows the screen at once. Every button is the
design's; holding OPTIONS for a second ends the title.

## Growing it

- **The screen** is `examples/{pid}/kit/screen.cpp`: a class with `update(input, dt,
  feedback)` and a const `draw(frame)`, assembled from the kit's components and
  themes. The kit's guides are in PS5_VKHomebrewUI's `docs/` (CRAFT.md first, then
  COMPONENTS.md, THEMES.md and KIT.md), its headers in `.deps/hui/src/`.
- **The program** around it, `examples/{pid}/{pid}.cpp`, owns the frame: input, sound,
  the light bar, the layers. A 3D scene goes under the screen as in the template's
  `uioverlay` sample (`kit.renderer.import_texture`).
- **Assets** go in `ps5/assets.json`, each with its origin and licence; only assets
  with a clear licence are shipped (`ps5/ASSETS.md` is written from it).
- How the foundation and the UI module work: PS5_VulkanTemplate's `ps5/README.md`
  and `ps5/ui/README.md`; the agent skills for this stack are in its `skills/`.

## Licences

The UI kit is GPL-3.0-or-later, and so are this title's program, its screen, the UI
module (`ps5/ui/`) and the kit's sounds (`ps5/licenses/GPL-3.0.txt`); the kit's fonts
are OFL-1.1 and Bitstream Vera (`ps5/ASSETS.md`). The rest of the foundation is MIT
(`LICENSE.md`, Sascha Willems'; the PS5 layer is mine, under the same licence). The
title as distributed is under GPL-3.0.
"""


if __name__ == "__main__":
    main()

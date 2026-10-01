# PS5 Vulkan Template

**Everything needed to build a PlayStation 5 homebrew powered by Vulkan, in one
repository.** Titles made from it render through RADV, Mesa's Vulkan driver, which my
[PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa) fork builds for the console with a
PS5 winsys. RADV is linked into each title by [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan),
on the platform layer of my [payload SDK fork](https://github.com/mihawk-99/PS5_PayloadSDK).
On the console it reports Vulkan 1.4 and presents at 3840x2160, at 119.88 Hz when the
TV allows it.

| What | Where |
| --- | --- |
| **A generator** that makes a new title, with its own program, ready to build, run and test on the console | [`ps5/tools/new-title.py`](ps5/tools/new-title.py) |
| **The foundation** every title is made on: Sascha Willems' Vulkan example base class with its PS5 hooks, and the PS5 layer (launch, pad, klog, display, test runs, shell exit) | [`base/`](base), [`ps5/src/`](ps5/src) |
| **Sixteen samples proven on the console**, one class each on the same base, built into one title, PS5 Vulkan Samples: glTF, mipmaps, PBR, shadows, deferred lighting, bloom, MSAA, instancing, indirect draws, compute particles, bindless textures, dynamic rendering, ImGui, mesh shaders, ray queries, and the starter | [`examples/`](examples), the table in [`ps5/README.md`](ps5/README.md) |
| **A test suite** for titles and for the driver: each program for a set number of frames, its last frame compared with the same frame drawn by the PC's Vulkan driver, klog checked | [`ps5/tools/run.sh`](ps5/tools/run.sh), [`ps5/tools/host-reference.sh`](ps5/tools/host-reference.sh) |
| **Agent skills** for this stack: starting and building titles, the console, porting, releases | [`skills/`](skills) |
| **Build and console tools**: setup, build, deploy, run, verdicts | [`ps5/tools/`](ps5/tools) |

## Start

On a Linux PC (Arch or CachyOS are what I use), beside an empty folder for the stack:

```bash
git clone https://github.com/mihawk-99/PS5_VulkanTemplate.git
cd PS5_VulkanTemplate
ps5/tools/bootstrap.sh --check     # what the host has, and what is missing
ps5/tools/bootstrap.sh             # clone PS5_Vulkan, PS5_Mesa, PS5_PayloadSDK beside it, build what is missing
```

The bootstrap builds RADV once (a long build), PS5_Vulkan's native tool and `libc.prx`,
the console's control payload, installs the pinned payload SDK and fetches glm. It never touches the
console. Then give the console's address to the tools, in PS5_Vulkan's ignored `.env`:

```bash
printf 'PS5_HOST=<the console'"'"'s address>\n' >> ../PS5_Vulkan/.env
```

**The console** needs a homebrew environment of its own (this repository does not set
one up): an enabler such as [etaHEN](https://github.com/etaHEN/etaHEN),
[ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) for titles in
`/data/homebrew`, [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv) on port 2121 and
[klogsrv](https://github.com/ps5-payload-dev/klogsrv) on port 3232. Once per boot, load
PS5_Vulkan's control payload, which launches and stops titles for the tools:

```bash
(cd ../PS5_Vulkan && python3 tools/ps5_console.py deploy-payload)   # then load it with the console's payload loader
```

## Make a homebrew

```bash
python3 ps5/tools/new-title.py ../PS5_MyGame --title-id PPSA99121 --name "My Game"
cd ../PS5_MyGame
ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
```

The new title is the foundation with one program, `examples/mygame/mygame.cpp`, grown
from the starter: a textured, lit glTF model, a camera on the sticks, a settings window
on the pad. Launched from the home screen it starts at once; `ps5/tools/run.sh` runs it
for 300 frames, brings its last frame back and checks klog. Replace the model, grow the
class, and take whatever a sample shows: every sample is a class on the same base, so
its code and shaders copy across as they stand. The new title's README says how it is
laid out.

The title id is `PPSA` and five digits, unique on the console; the generator refuses
those my titles use.

## Run the samples

```bash
ps5/tools/build.sh && ps5/tools/deploy.sh   # the PS5 Vulkan Samples title, PPSA99130
ps5/tools/run.sh launcher all               # every sample and the menu: a verdict for each
ps5/tools/host-reference.sh                 # the same frames on this PC's Vulkan driver
```

Launched from the home screen, PS5 Vulkan Samples shows a menu of them. On my console
(2026-10-01, RADV 0b2d6d1) every one held 119.9 fps at 3840x2160 and drew its frame 300
within 4.5 levels in 255 of the PC's. How the port works, its tools and how to add a
sample: [`ps5/README.md`](ps5/README.md). The assets and their licences:
[`ps5/ASSETS.md`](ps5/ASSETS.md).

## With an agent

The skills in [`skills/`](skills) teach an agent this stack: how a title is built and
linked, the console's contracts (how a title ends, the display, the pad), the platform
layer, running and debugging on the console, porting existing code, releases. Link them
into the agent's skill folder:

```bash
for skill in ps5-homebrew ps5-console ps5-porting ps5-release; do
    ln -sfn "$PWD/skills/$skill" ~/.claude/skills/$skill
done
```

## Layout

| Path | What it is |
| --- | --- |
| `base/` | Sascha Willems' example base class, with the PS5 hooks (`VK_EXAMPLE_PS5`) |
| `examples/` | upstream's examples; the sixteen in `ps5/src/samples.cpp` build for the console, `examples/starter/` is the one new titles grow from |
| `shaders/glsl/` | their GLSL and the SPIR-V they load (`ps5/tools/compile-shaders.sh <id>`) |
| `external/` | glm, Dear ImGui, libktx, tinygltf, the Vulkan headers |
| `ps5/` | the PS5 port: its sources, build, tools, assets, identity and README |
| `skills/` | the agent skills |
| `assets/` | upstream's asset pack, a submodule that is not needed: the title fetches the files it ships (those with a clear licence) at pinned hashes, and replaces the rest |
| `README.upstream.md` | upstream's README: the examples, their credits |

## Fork, credits, licences

This is a fork of Sascha Willems' [Vulkan examples](https://github.com/SaschaWillems/Vulkan)
(MIT), which keeps upstream's history on `main` and reaches it through an `upstream`
remote. Upstream's files change as little as the port allows, each change marked `PS5`
or guarded by `VK_EXAMPLE_PS5`; upstream's README is [`README.upstream.md`](README.upstream.md).
Upstream's code and what I add are MIT (`LICENSE.md`). The assets keep their own
licences (`ps5/ASSETS.md`). A built title links the PS5 platform layer, which is
GPL-3.0, so a title as distributed is under GPL-3.0; RADV (Mesa) is MIT.

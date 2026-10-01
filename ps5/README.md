# The PS5 port

Sascha Willems' Vulkan examples as one PS5 homebrew title, **PS5 Vulkan Samples**
(`PPSA99130`), on RADV linked into the title. Launched from the home screen it shows a
menu of samples; a test run draws a set number of frames of each sample, saves the
last frame and checks klog. The title is a reference for writing Vulkan on the
console, a driver test suite run beside the CTS after RADV changes, and a showcase.

## The samples

A sample goes in the menu only once it has been proven on the console: it starts,
draws what the host reference draws, holds its frame rate, and ends cleanly. Until
then test runs reach it and the menu does not (`src/samples.cpp`, the second field).

| Sample | What it shows | Assets | Menu |
| --- | --- | --- | --- |
| `gltfloading` | a glTF scene: meshes, materials, textures, node hierarchy | Flight Helmet | not yet |
| `texturemipmapgen` | a mip chain made at run time with blits, and the samplers that read it | a tunnel, Metal Plate | not yet |
| `pbrtexture` | metal and roughness maps lit by an HDR environment (IBL) | Vintage Video Camera, Kloofendal sky | not yet |
| `shadowmapping` | a directional light's depth map, filtered with PCF | two scenes made here | not yet |
| `deferred` | a G-buffer in several render targets, then the lights in one pass | Knight, Cobblestone Floor 01 | not yet |
| `bloom` | bright parts blurred in two separable passes and added back | Retro UFO, a starfield made here | not yet |
| `multisampling` | MSAA with resolve attachments, and sample-rate shading | Lantern | not yet |
| `instancing` | thousands of rocks in one draw, with per-instance data | rocks and a planet made here | not yet |
| `indirectdraw` | draws read from a GPU buffer, many meshes and instances each | plants and ground made here, Dry Ground 01 | not yet |
| `computeparticles` | particles moved by a compute shader and drawn as points | sprites made here | not yet |
| `descriptorindexing` | bindless textures: one descriptor array indexed per object | made by the sample | not yet |
| `dynamicrendering` | rendering without render pass and framebuffer objects | Lantern | not yet |
| `imgui` | Dear ImGui over a 3D scene, with windows of its own | shapes made here | not yet |
| `meshshader` | geometry from task and mesh shaders, with no vertex input | none | not yet |
| `rayquery` | shadows traced from a fragment shader against an acceleration structure | a scene made here | not yet |

The assets, their authors and licences: [`ASSETS.md`](ASSETS.md).

## Controls

In the menu, the D-pad chooses and CROSS starts. In a sample:

| Pad | Does |
| --- | --- |
| left stick | turns the camera around the scene (walks, for a first-person camera) |
| right stick | zooms (looks around, for a first-person camera) |
| D-pad, CROSS, CIRCLE | move through the sample's settings, change one, back |
| L1, R1 | the previous or next settings window |
| L2, R2 | change a slider slower or faster |
| TOUCH PAD | hide or show the settings |
| OPTIONS | back to the menu |

## Building and running

The title builds against my PS5 stack, checked out beside this repository:
[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan) (its SDK fork installed in
`.deps`, its RADV release archive, its native tool and `libc.prx`) and the Mesa fork
it builds RADV from. The assets need Python with numpy and Pillow, and download their
CC0 sources once (into `build/ps5/downloads/`, checked against pinned hashes).

```bash
git submodule update --init         # glm, and the asset pack
ps5/tools/build.sh                  # dist/PPSA99130/: eboot.bin, sce_sys, shaders, assets
ps5/tools/deploy.sh                 # upload what changed, over the console's FTP server
ps5/tools/run.sh                    # a test run of every sample, 300 frames each
ps5/tools/run.sh menu               # the samples in the menu
ps5/tools/run.sh bloom deferred     # those two
ps5/tools/run.sh --menu             # no test: the menu, until Quit
```

The console's address comes from PS5_Vulkan's `.env`, and its tools do the work: the
run tool checks the console runs nothing, launches the title through the control
payload, and captures klog until the title's `ends` line, a crash record or its exit.

**A test run** ends through the title's own exit. `run.sh` writes `test-run.txt` into
the title folder; the launch that reads it deletes it, runs each sample in turn on an
instance and a device of its own, saves each one's last frame in `screenshots/`, logs
one line for each, and ends. Then `tools/check-run.py` fetches the pictures and prints
a verdict for each sample. A sample passes when its klog line says `ok`, klog holds no
crash record, GPU fault or RADV error line, and its picture exists and is not one flat
colour (and, when `reference/<id>.png` exists, is close to it). The run's klog, the
pictures as PNG and `summary.txt` stay in `klog/<time>/`;
`tools/contact-sheet.py klog/<time>` puts the pictures on a few sheets.

**Test runs are deterministic.** The launcher sets `benchmark.active`, which upstream's
samples read as "seed the random generators with 0", and the render loop steps time by
1/60 s a frame whatever the frame rate, so frame 300 shows the same moment on any
driver. Two host runs differ by less than 2 levels in 255 (computeparticles, whose
points scatter, by 4.4).

**The host reference.** `ps5/tools/host-reference.sh [samples]` builds the same code for
Linux, with a headless surface instead of the console's display and no pad, and runs a
test run on the PC's Vulkan driver. Its pictures, in `klog/host-<time>/`, are what the
console's should look like: a difference that the host does not show is RADV's to
explain, one that it shows too is the sample's or the asset's. `--save` keeps them,
at 480x270, as `reference/<id>.png`, which `check-run.py` compares a 300-frame run's
pictures with (a mean difference of 12 in 255 fails).

**After a RADV change**, run every sample (`ps5/tools/run.sh`) with the CTS gate, and
compare the frame rates with the table above and the pictures with the host's.

## How the port works

The examples stay upstream's. What the port changes in upstream's files is guarded by
`VK_EXAMPLE_PS5`:

- **One title.** `VULKAN_EXAMPLE_MAIN()` defines `createExample()` instead of `main`
  (`base/Entrypoints.h`). The build compiles each sample inside a namespace of its own
  (`src/wrap.cpp.in`), so fifteen classes called `VulkanExample` and their helper types
  stay apart; `src/prelude.h` includes every header a sample may include first, so none
  lands inside a namespace. `src/samples.cpp` lists the samples linked in.
- **Vulkan without a loader.** Every unit includes volk first (`-include volk.h`); the
  title initialises it from RADV's `vk_icdGetInstanceProcAddr`, and the base class loads
  the instance's and the device's commands once each exists.
- **The display.** The base class's direct-to-display path (`VK_KHR_display`) at the
  console's one mode, 3840x2160, at 119.88 Hz when `param.json` asks for it.
- **No `exit`.** `vks::tools::exitFatal` throws instead (the console reports an `exit`
  as a crash), and the launcher ends that sample and carries on. A missing shader is
  fatal rather than a null module handed to the driver.
- **Input, test runs, screenshots.** `src/example_ps5.cpp`: the pad to the camera and
  to ImGui's gamepad navigation; the frame budget; the last frame copied from the
  swapchain image before it is presented, written at half size as a PPM.
- **The launcher** (`src/launcher.cpp`) is itself an example on the base class, with its
  own ImGui window. Each sample then starts on a fresh instance and device.
- **Assets.** Paths point at the replacements in `/app0/assets/` (`ps5/assets.json`).
  tinygltf has no `wordexp` on the console (`__PROSPERO__`), and `gltfloading` leaves
  tinygltf's and stb_image's code to `base/VulkanglTFModel.cpp`.

The title's own parts:

| File | What it is |
| --- | --- |
| `CMakeLists.txt` | the build: the payload SDK's CMake toolchain, the samples from `src/samples.cpp`, and the host reference variant |
| `src/main.cpp` | the title: the menu loop, test runs, one sample's run |
| `src/launcher.cpp` | the menu |
| `src/samples.cpp` | the samples linked in, and which are in the menu |
| `src/example_ps5.cpp` | the base class's console parts: pad, screenshot |
| `src/platform.c`, `platform.h` | klog, the splash, the pad, the shell exit (from the ps5-homebrew skill's template) |
| `src/platform_host.c` | the same on a PC, for the host reference |
| `tools/build.sh`, `link-title.sh` | configure, compile, link with RADV, sign, package |
| `tools/build-assets.py`, `generate_assets.py`, `assets.json` | the assets and their notices |
| `tools/deploy.sh`, `run.sh`, `check-run.py`, `contact-sheet.py` | the console: upload, test runs, verdicts |
| `tools/host-reference.sh` | the reference pictures |

## Adding a sample

1. Add a `SAMPLE(id, false, "title", "description")` line to `src/samples.cpp`. The
   build compiles `examples/<id>/<id>.cpp` (or `main.cpp`) and packages
   `shaders/glsl/<id>/`.
2. Check its assets: anything from the asset pack without a clear licence gets a
   replacement in `assets.json` (made by `generate_assets.py`, or a CC0 source at a
   pinned hash), and the sample's path changes to it, with a note.
3. `tools/host-reference.sh <id>`, and look at the picture.
4. `tools/build.sh && tools/deploy.sh && tools/run.sh <id>`, and compare.
5. Once it passes on the console, set its second field to `true`, and record it here.

## Licences

Upstream's code is MIT (`LICENSE.md`), and so is what I add in `ps5/`. The assets
keep their own licences (`ASSETS.md`; the title's `assets/NOTICES.txt` lists those
installed). The built title links the PS5 platform layer of my payload SDK fork, which
is GPL-3.0, so the title as distributed is under GPL-3.0; RADV (Mesa) is MIT.

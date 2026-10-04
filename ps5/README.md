# The PS5 port

Sascha Willems' Vulkan examples as one PS5 homebrew title, **Vulkan Template**
(`PPSA99130`), on RADV linked into the title. Launched from the home screen it opens on
a start screen drawn with the UI module: **Samples**, the menu of Vulkan samples;
**Designs**, the UI kit's gallery of twenty-one designs; and **Themes**, which picks the
title's look from the kit's thirty themes (the start screen, the Samples menu and the
samples' settings windows wear it). A title built without `ps5/ui/` opens on a plain
menu. A test run draws a set number of frames of each sample, saves the last frame and
checks klog. The title is a reference for writing Vulkan on the
console, a driver test suite run beside the CTS after RADV changes, a showcase, and
the foundation every new homebrew is made on (`tools/new-title.py`, below).

## The samples

A sample goes in the menu only once it has been proven on the console: it starts,
draws what the host reference draws, holds its frame rate, and ends cleanly. Until
then test runs reach it and the menu does not (`src/samples.cpp`, the second field).

All sixteen are proven, last on 2026-10-03 with RADV 15a5e99 and SDK fork adc8dd7, in
two launches of the title (the second a reopen, with the same figures), their settings
windows in the title's default theme: a test run of 300 frames each, at 3840x2160 on
the 119.88 Hz display. Each held 119.9 fps after
its first 150 frames, its last frame differed from the host reference's by at most 4.5
levels in 255 (the "from the host" column, a mean over the picture at 480x270), klog
held no crash record, GPU fault or driver error, and the title exited on its own. The
title's own screens pass the same run: the start screen (`start`, 1.6 from the host),
the Samples menu (`launcher`, 1.3) and Themes (`themes`, 0.9). The settings windows'
new look moved every sample's picture, so their references were saved again
(`host-reference.sh --save`) before these runs.

| Sample | What it shows | Assets | Console | From the host |
| --- | --- | --- | --- | --- |
| `starter` | the program a new homebrew grows from: a textured, lit glTF model with dynamic rendering, a settings window | Lantern | 119.9 fps | 0.9 |
| `gltfloading` | a glTF scene: meshes, materials, textures, node hierarchy | Flight Helmet | 119.9 fps | 0.6 |
| `texturemipmapgen` | a mip chain made at run time with blits, and the samplers that read it | a tunnel, Metal Plate | 119.9 fps | 0.8 |
| `pbrtexture` | metal and roughness maps lit by an HDR environment (IBL) | Vintage Video Camera, Kloofendal sky | 119.9 fps | 2.0 |
| `shadowmapping` | a directional light's depth map, filtered with PCF | two scenes made here | 119.9 fps | 0.7 |
| `deferred` | a G-buffer in several render targets, then the lights in one pass | Knight, Cobblestone Floor 01 | 119.9 fps | 1.3 |
| `bloom` | bright parts blurred in two separable passes and added back | Retro UFO, a starfield made here | 119.9 fps | 0.7 |
| `multisampling` | MSAA with resolve attachments, and sample-rate shading | Lantern | 119.9 fps | 1.8 |
| `instancing` | thousands of rocks in one draw, with per-instance data | rocks and a planet made here | 119.9 fps | 1.9 |
| `indirectdraw` | draws read from a GPU buffer, many meshes and instances each | plants and ground made here, Dry Ground 01 | 119.9 fps | 3.3 |
| `computeparticles` | particles moved by a compute shader and drawn as points | sprites made here | 119.9 fps | 4.5 |
| `descriptorindexing` | bindless textures: one descriptor array indexed per object | made by the sample | 119.9 fps | 1.0 |
| `dynamicrendering` | rendering without render pass and framebuffer objects | Lantern | 119.9 fps | 0.6 |
| `imgui` | Dear ImGui over a 3D scene, with windows of its own | shapes made here | 119.9 fps | 0.5 |
| `meshshader` | geometry from task and mesh shaders, with no vertex input | none | 119.9 fps | 0.5 |
| `rayquery` | shadows traced from a fragment shader against an acceleration structure | a scene made here | 119.9 fps | 0.5 |

The assets, their authors and licences: [`ASSETS.md`](ASSETS.md).

### The UI module

Three more programs draw with the UI module ([`ui/README.md`](ui/README.md)):
BlackBearReloaded's ps5-homebrew-ui kit, drawn with Vulkan through my fork
PS5_VKHomebrewUI. The module is GPL-3.0-or-later and built only when `ps5/ui/` is
there; a title made without `--ui` has none of it. `uioverlay` is in the Samples menu,
the gallery is the start screen's Designs, and test runs reach all three.

They were proven on 2026-10-03 with RADV 15a5e99, SDK fork adc8dd7 and
PS5_VKHomebrewUI 4451c26, in the same launches as a full run of the sixteen samples
above and the title's own screens (`start`, `launcher`, `themes`: 43 programs a
launch, 43 passes in each of two launches): each held 119.9 fps at 3840x2160, drew
within 3.3 levels of the host's picture (the screens within 1.6), opened the
console's audio output, and left klog clean. How the sounds and the rumble feel is
not something a run checks.

| Program | What it shows | Console | From the host |
| --- | --- | --- | --- |
| `uikit` | the kit's gallery: twenty-one designs (L1/R1), the Theme Lab's thirty themes, the component library, the kit's sounds and music | 119.9 fps | 1.2 |
| `uikit.<design>` | the gallery started on one design, playing the kit's own tour of it: all twenty-one (`aurora` ... `toolbox`, `samples.cpp`'s `VARIANTS`) | 119.9 fps each | 0.4 to 3.3 |
| `uioverlay` | a HUD and a frosted pause menu over the starter's lantern, drawn off-screen and blurred by the glass (`import_texture`); the kit on dynamic rendering | 119.9 fps | 0.2 |
| `uiscreen` | the UI starter: one design full screen, the program a `--ui` title grows from | 119.9 fps | 1.2 |

In the three, every button is the program's (OPTIONS pauses or belongs to the
design), and holding OPTIONS for a second goes back to the menu.

## Controls

On the start screen the D-pad chooses Samples, Designs, Themes or Quit and CROSS opens
it. CIRCLE in the Samples menu or in Themes returns to it; in a design, OPTIONS held for
a second does (a plate with a filling ring shows the hold), since a press of OPTIONS
belongs to the design. In Themes, CROSS makes the focused theme the title's.
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
[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan) (its RADV release archive, its
link recipe, its native tool and `libc.prx`), the Mesa fork it builds RADV from, and
my [payload SDK fork](https://github.com/mihawk-99/PS5_PayloadSDK), which
`tools/setup-sdk.sh` installs into `.deps/` at the revision it pins (adc8dd7, which
adds the platform layer's `/data` client; a pin is never older than fa69d00, the first
with its C-locale `localeconv`). The assets need Python with numpy and Pillow, and download their
CC0 sources once (into `build/ps5/downloads/`, checked against pinned hashes).

```bash
ps5/tools/bootstrap.sh              # once: PS5_Vulkan, PS5_Mesa, PS5_PayloadSDK beside it, glm
ps5/tools/build.sh                  # dist/PPSA99130/: eboot.bin, sce_sys, shaders, assets
ps5/tools/deploy.sh                 # upload what changed, over the console's FTP server
ps5/tools/run.sh                    # a test run of every sample, 300 frames each
ps5/tools/run.sh menu               # the samples in the menu
ps5/tools/run.sh bloom deferred     # those two
ps5/tools/run.sh start launcher all # the start screen and the menu too
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
`tools/contact-sheet.py klog/<time>` puts the pictures on a few sheets, and
`tools/compare-run.py klog/<time> klog/host-<time>` sets them beside the host's with
their difference. The title also writes its lines to `test-results.txt` in its folder as
it goes: when the console has no klog capture, `run.sh` follows that file instead
(`tools/run-without-klog.py`), and the verdicts say that crash records and driver
messages went unchecked.

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

**After a RADV change**, run every sample (`ps5/tools/run.sh start launcher all`) with the
CTS gate, and compare the frame rates with the table above and the pictures with the
host's. The suite earns its place: its first console run drew three samples black
where the host drew them in colour. The console's `localeconv()` reports an empty
decimal point, and tinygltf's JSON parser, which builds numbers for `strtod` from it,
read every glTF material colour of 0.62 as 0. The fix went where such gaps go, into
the shared platform layer (SDK fork fa69d00) and PS5_Vulkan's link recipe, for every
title that links them.

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
| `src/samples.cpp` | the samples linked in, and which are in the menu (one alone: the title is that program) |
| `src/example_ps5.cpp` | the base class's console parts: pad, screenshot |
| `src/platform.c`, `platform.h` | klog, the splash, the pad, the shell exit (from the ps5-homebrew skill's template) |
| `src/platform_host.c` | the same on a PC, for the host reference |
| `tools/build.sh`, `link-title.sh`, `setup-sdk.sh` | configure, compile, link with RADV, sign, package; the SDK pin |
| `tools/new-title.py`, `compile-shaders.sh` | a new title on this foundation; a program's GLSL to the SPIR-V it loads |
| `tools/build-assets.py`, `generate_assets.py`, `assets.json` | the assets and their notices |
| `tools/deploy.sh`, `run.sh`, `run-without-klog.py`, `check-run.py` | the console: upload, test runs, verdicts |
| `tools/contact-sheet.py`, `compare-run.py` | the pictures, together and beside the host's |
| `tools/host-reference.sh`, `reference/` | the reference pictures (the host build links libc++ 18.1.8, as the console's SDK, so the samples' random scenes match) |
| `tools/record-reel.sh` | a preview of the title's own screens recorded on this PC: the host reference build pipes every frame of scripted test runs to ffmpeg |

## Starting a homebrew

```bash
python3 ps5/tools/new-title.py ../PS5_MyTitle --title-id PPSA99121 --name "My Title"
```

A new title is this foundation with one program: `base/` with its PS5 hooks,
`external/`, the PS5 layer and its tools in `ps5/`, and the starter renamed to the
title's own program (`examples/<id>/<id>.cpp`, `shaders/glsl/<id>/`), with its own
`param.json`, icon and asset list. It is built, deployed, run and checked as the
samples are, and a sample's technique copies into it as it stands, since both are
classes on the same base. A title with one program starts it at once, with no menu;
OPTIONS then shows and hides the settings window, and the program's Quit ends the
title. The foundation is copied at this repository's last commit, so a fix to it is
made here, where every sample tests it, and committed first. Proven on 2026-10-01:
a title made this way built from nothing in 31 s and passed its test run on the
console (119.9 fps, 0.6 from its host reference, a clean exit).

With `--ui <design>` (`--ui list` names the twenty), the program is the UI starter
instead, and its screen is the title's own copy of the kit's design in
`examples/<id>/kit/screen.cpp`; the title carries the GPL-3.0 UI module and the
kit's assets. On 2026-10-03 a `--ui settings` title and a plain one, made from the
working tree, built and passed their host references; the UI starter they grow
from passed on the console (above), the generated `--ui` title has not been run
there yet.

## Adding a sample

1. Add a `SAMPLE(id, false, "title", "description")` line to `src/samples.cpp`. The
   build compiles `examples/<id>/<id>.cpp` (or `main.cpp`) and packages
   `shaders/glsl/<id>/`.
2. Check its assets: anything from the asset pack without a clear licence gets a
   replacement in `assets.json` (made by `generate_assets.py`, or a CC0 source at a
   pinned hash), and the sample's path changes to it, with a note.
3. `tools/host-reference.sh <id>`, and look at the picture.
4. `tools/build.sh && tools/deploy.sh && tools/run.sh <id>`, and compare.
5. Once it passes on the console, set its second field to `true`, keep its reference
   (`tools/host-reference.sh --save <id>`), and record it here.

## Licences

Upstream's code is MIT (`LICENSE.md`), and so is what I add in `ps5/`. The assets
keep their own licences (`ASSETS.md`; the title's `assets/NOTICES.txt` lists those
installed). The built title links the PS5 platform layer of my payload SDK fork, which
is GPL-3.0, so the title as distributed is under GPL-3.0; RADV (Mesa) is MIT. The UI
module (`ps5/ui/`), the programs that draw with it and the kit's sounds are
GPL-3.0-or-later, its fonts OFL-1.1 and Bitstream Vera (`ui/README.md`).

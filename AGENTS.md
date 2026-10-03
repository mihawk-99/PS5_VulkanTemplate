# AGENTS.md

Instructions for an AI agent working in this repository, whether to make a new
PlayStation 5 homebrew from it, to grow one, or to change the repository itself.
Read this file first, then the skill that fits the task (below). The human README is
[`README.md`](README.md); how the port works is [`ps5/README.md`](ps5/README.md).

## What this repository is

PS5 Vulkan Template is everything needed to build a PS5 homebrew title that renders
with Vulkan 1.4, in one place:

- **a foundation**: Sascha Willems' Vulkan example base class (`base/`) with PS5
  hooks, and a PS5 layer (`ps5/src/`) that launches the program, reads the pad, sends
  output to klog, drives the display, runs tests and ends the title correctly;
- **a generator**, `ps5/tools/new-title.py`, which makes a new title: the
  foundation plus one program grown from the starter sample;
- **sixteen samples proven on the console**, built into one title, PS5 Vulkan
  Samples (`PPSA99130`), each a class on the same base: a library of techniques that
  copy into any title as they stand;
- **a UI module**, `ps5/ui/` (GPL-3.0-or-later, opt-in): BlackBearReloaded's UI kit
  (themes, components, twenty-one complete designs, sound) drawn with Vulkan through
  my fork PS5_VKHomebrewUI, with three programs on it (`uikit`, `uioverlay`,
  `uiscreen`) and `new-title.py --ui <design>` for a title whose face is a design;
- **a test suite**: each program runs for a frame budget, its last frame is saved and
  compared with the same frame drawn by the PC's Vulkan driver, and klog is checked;
- **agent skills** for this stack, in `skills/`;
- **build and console tools**, in `ps5/tools/`.

The driver is RADV, Mesa's Vulkan driver, built for the console from my PS5_Mesa
fork by PS5_Vulkan, linked into each title (the console has no Vulkan of its own and
a title cannot load one). It runs on the platform layer of my payload SDK fork,
PS5_PayloadSDK. Those three repositories are checked out beside this one:

```text
  PS5_VulkanTemplate (this)  ->  a title: eboot.bin, sce_sys/, sce_module/libc.prx, shaders/, assets/
     links  RADV release archive ........ PS5_Vulkan builds it from PS5_Mesa (tools/build-radv.sh)
     links  the platform layer ........... libps5platform.a, from PS5_PayloadSDK at the pin in ps5/tools/setup-sdk.sh
     uses   the link recipe, CRT, libc.prx, native tool, console tools ... PS5_Vulkan
```

I am the maintainer; the documents speak in my voice ("I", "my console").

## The skills, and when to read each

The skills in `skills/` hold what is true of every title and was expensive to learn.
If your tool loads skills from a folder, link each `skills/<name>` there (README.md,
"With an agent"); otherwise read `skills/<name>/SKILL.md` yourself when its task
comes up, and the reference files it names.

| Skill | Read it before |
| --- | --- |
| [`skills/ps5-homebrew`](skills/ps5-homebrew/SKILL.md) | starting or building a title, linking RADV, packaging, the display, pad, audio, memory, threads, files, deciding which project a fix belongs in. References: `new-title.md`, `stack.md`, `vulkan-on-radv.md`, `platform-contracts.md`, `platform-layer.md`, `title-packaging.md`, `toolchain.md`, `runtime-surface.md`, `driver-work.md`, `ui-kit.md` |
| [`skills/ps5-console`](skills/ps5-console/SKILL.md) | anything that touches the console: deploying, launching, klog, stopping a run, crashes, measuring, regression runs. References: `console-tools.md`, `test-runs.md`, `crash-reading.md` |
| [`skills/ps5-porting`](skills/ps5-porting/SKILL.md) | bringing existing software over: forks and pins, cross-building, memory and JIT, files, loading code without `dlopen`. References: `forks.md`, `memory-and-jit.md`, `files-and-io.md`, `loading-code.md` |
| [`skills/ps5-release`](skills/ps5-release/SKILL.md) | anything that leaves the machine as a release: licences, notices, what never ships, the build order, release notes. References: `licensing.md`, `release-notes.md` |

## Rules that are never broken

1. **A title never calls `exit()` and never returns from `_start`.** It asks the
   shell to close it. The foundation does this (`catchReturnFromMain` in
   `ps5/src/platform.c`): a program ends by returning from its render loop (set the
   base class's `quit`), and a fatal error goes through `vks::tools::exitFatal`,
   which throws on the console. Never add `exit`, `abort` or `std::exit` to a
   program. (`skills/ps5-homebrew/references/platform-contracts.md`)
2. **RADV is linked only with PS5_Vulkan's recipe** (`tools/radv-link.sh`, called by
   `ps5/tools/link-title.sh`). A hand-written link builds and then calls through NULL.
3. **Fixes go where they belong.** A libc function the console lacks, a memory,
   thread or file rule: the platform layer in PS5_PayloadSDK, with a host test, then a
   new pin. A driver bug: PS5_Mesa, a general fix proven by CTS runs, then a new pin
   in PS5_Vulkan. A fix to the foundation: this repository, where every sample tests
   it. Never a workaround in one title for another project's bug.
4. **Nothing private leaves the machine.** The console's address and ports live only
   in PS5_Vulkan's ignored `.env`; mask addresses in anything you print or paste.
   Never commit, upload or ship console firmware or anything taken from it, game
   data, BIOS files, keys or saves. Call only exported functions of the console's
   system software, and write down only exported names and behaviour measured on the
   console.
5. **Only assets with a clear licence ship**, each recorded in `ps5/assets.json` with
   its author, licence and URL (the notices in `ps5/ASSETS.md` and the title's
   `assets/NOTICES.txt` are written from it). A file with no stated licence, or one
   that asks not to be redistributed, stays out, whatever its quality.
6. **Prove it on the console before claiming it.** A README line, a commit message or
   a report says what a run showed, at the same level of detail whether it passed or
   failed. "Builds" is not "works"; "reported frames" is not "drew the right frames".
7. **Console runs end through the title's own exit.** Check the console is idle
   before launching; a test run ends itself after its frame budget. Closing a title
   with the control payload is only a watchdog for a hang.
8. **Work on `main`; never rewrite published history or force-push.** Upstream's
   files change as little as the port allows: a PS5 change is guarded by
   `VK_EXAMPLE_PS5` or marked with a `PS5` comment, so upstream merges stay clean.
9. **The UI kit stays a GPL-3.0 module.** It lives in `ps5/ui/`, the programs that
   draw with it, and `.deps/hui` (re-exported from the pin in `ps5/ui/setup-kit.sh`,
   never patched); kit files keep their SPDX headers. Nothing of it goes into the MIT
   starter or the MIT foundation files, so a title made without `--ui` stays MIT at
   source. A kit fix goes to PS5_VKHomebrewUI, then a new pin.
   (`skills/ps5-homebrew/references/ui-kit.md`)

## Setting up (once per machine)

From this repository's folder, on a Linux PC (Arch or CachyOS are what I use):

```bash
ps5/tools/bootstrap.sh --check    # report: host tools, the three projects, their builds, the console's address
ps5/tools/bootstrap.sh            # clone what is missing beside this repository, build what is missing
```

It needs: git, cmake, ninja, meson, clang 18 or later, python3 with numpy, Pillow and
mako, glslangValidator, rsync, curl, zstd, make. It builds PS5_Vulkan's payload SDK,
native tool and `libc.prx` (`make`; PS5_Vulkan's own Hello World may fail to link
there, which a title does not need), the RADV release archive (`tools/build-radv.sh
release`, a long build, done only when missing; `--rebuild-radv` to rebuild a stale
one), the control payload ps5vkctl, glm (a submodule) and this repository's pinned
payload SDK (`ps5/tools/setup-sdk.sh`, into `.deps/`). It never touches the console.
The asset pack submodule (`assets/`) is not needed.

Then the console's address, which a person gives you:

```bash
printf 'PS5_HOST=<address>\n' >> ../PS5_Vulkan/.env    # FTP_PORT=2121, KLOG_PORT=3232, PS5VKCTL_PORT=9111 are the defaults
```

**The console** must be running its homebrew environment, which a person sets up
after each boot: an enabler (etaHEN), ShadowMountPlus (titles in `/data/homebrew`),
ftpsrv on port 2121, klogsrv on port 3232, an ELF loader on port 9021, and
PS5_Vulkan's control payload ps5vkctl on port 9111. If the loader answers and
ps5vkctl does not, you may load ps5vkctl yourself (it is this stack's own payload):
`<PS5_Vulkan>/.deps/native/ps5-payload-sdk/bin/prospero-deploy -h <host> -p 9021
<PS5_Vulkan>/build/ps5vkctl/ps5vkctl.elf`, then `python3 tools/ps5_console.py
payload` in PS5_Vulkan answers `ok ps5vkctl`. Never load third-party payloads, and
never change the console's settings or system software.

## Task: make a new homebrew

```bash
python3 ps5/tools/new-title.py ../PS5_MyGame --title-id PPSA99121 --name "My Game" [--refresh 60]
python3 ps5/tools/new-title.py ../PS5_MyApp --title-id PPSA99122 --name "My App" --ui settings   # a UI title
cd ../PS5_MyGame
ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
```

- **The title id** is `PPSA` and five digits, unique on the console. The generator
  refuses the ids my titles use (`TAKEN` in `new-title.py`: 99002, 99008, 99010,
  99014, 99015, 99100, 99130, 99169, 99988, 99996 to 99999). Ask which id to use if
  the person has not said.
- **The name** is what the home screen shows (1 to 40 letters, digits, spaces, `.`,
  `_`, `-`) and klog's prefix (`[My Game] ...`).
- **`--refresh 60`** leaves out the 120 Hz bits in `param.json`; the default asks for
  119.88 Hz (the display falls back to 59.94 Hz by itself where it must).
- **`--ui <design>`** starts from the UI module instead of the 3D starter: the
  program is the UI starter (`examples/uiscreen`) and its screen is the title's own
  copy of one of the kit's designs (`--ui list` names the twenty), in
  `examples/<id>/kit/screen.cpp`. The title then has `ps5/ui/` and the kit's assets
  and is GPL-3.0-or-later at source (`ps5/ui/README.md`); ask before choosing it for
  someone who has not.
- **The foundation is copied from this repository's last commit** (`git archive
  HEAD`). Commit a foundation change here first if the new title should have it.
- The new title is a git repository on `main` with one commit, beside the others
  (its tools find `../PS5_Vulkan` and `../PS5_PayloadSDK`; elsewhere set
  `PS5_VULKAN_DIR` and `PS5_PAYLOAD_SDK_FORK`).

What it contains:

| Path | What it is | Change it? |
| --- | --- | --- |
| `examples/<id>/<id>.cpp` | **the program**: one `VulkanExample` class on `VulkanExampleBase`, grown from the starter | yes: this is the title |
| `shaders/glsl/<id>/` | its GLSL and the SPIR-V it loads | yes; recompile with `ps5/tools/compile-shaders.sh <id>` and commit both |
| `ps5/assets.json` | the assets it ships, with licences | yes, for every asset |
| `ps5/sce_sys/param.json`, `icon0.png` | identity and icon (`pic0.dds`/`pic1.dds` optional: `title-packaging.md`) | yes |
| `ps5/src/samples.cpp` | the programs linked in: one line, `SAMPLE(<id>, true, ...)` | to add programs |
| `base/`, `external/`, `ps5/src/` (rest), `ps5/tools/`, `ps5/CMakeLists.txt` | the foundation | only to fix it, and the fix belongs in PS5_VulkanTemplate first |
| `README.md`, `LICENSE.md`, `ps5/ASSETS.md` | the title's README, MIT, the asset notices | README yes; regenerate the notices |

How it behaves: launched from the home screen, the program starts at once (one
program, no menu). The left stick turns the camera (or walks, for a first-person
camera), the right stick zooms (or looks); OPTIONS or the touch pad shows and hides
the settings window; the D-pad moves through it, CROSS acts, CIRCLE backs out, L1/R1
change windows, L2/R2 change a slider slower or faster; the program's Quit ends the
title. With more than one program in `samples.cpp`, a menu comes first and OPTIONS
returns to it.

## Task: grow the program

The program is an ordinary Sascha Willems example. Its shape:

```cpp
class VulkanExample : public VulkanExampleBase {
    VulkanExample()                  // title, camera, apiVersion, features and extensions wanted
    void getEnabledFeatures()        // turn on device features the program uses (check deviceFeatures first)
    void getEnabledExtensions()      // device extensions, when they depend on what is supported
    void prepare()                   // VulkanExampleBase::prepare(), then assets, buffers, descriptors, pipelines; prepared = true
    void buildCommandBuffer()        // record drawCmdBuffers[currentBuffer] for swapChain.images[currentImageIndex]
    void render()                    // prepareFrame(); update; buildCommandBuffer(); submitFrame();
    void OnUpdateUIOverlay(vks::UIOverlay *overlay)  // the settings window: header, checkBox, sliderFloat, button...
    ~VulkanExample()                 // destroy what prepare created (if (device) ...)
};
VULKAN_EXAMPLE_MAIN()                // on the console: defines createExample() for the launcher
```

- **Replace the model**: put its entry in `ps5/assets.json` and the path in
  `loadAssets()` (`getAssetPath()` is `/app0/assets/`). The glTF loader
  (`vkglTF::Model`, `base/VulkanglTFModel.*`) reads **`.gltf` with its buffers and
  images beside it, not `.glb`**. Textures for `vks::Texture2D` and friends are
  **KTX 1** files (`base/VulkanTexture.*`). `ps5/tools/generate_assets.py` has a KTX
  writer, a glTF writer, mip chains, cube maps from equirectangular HDRs, and
  converters to copy from.
- **The world's Y axis points down**: `vkglTF::FileLoadingFlags::FlipY` turns glTF's
  Y-up content the way the samples' cameras expect. A cube map seen through a skybox
  has its sky at -Y. Front faces are counter-clockwise, as glTF's.
- **Rendering**: the starter uses Vulkan 1.3 dynamic rendering (`useDynamicRendering
  = true`, `beginDynamicRendering`/`endDynamicRendering`; `requiresStencil = true`
  because those helpers transition the stencil aspect too). Most samples use the base
  class's render pass and framebuffers instead (`renderPass`, `frameBuffers`). Either
  works; keep to one in a program.
- **Frames in flight**: two (`maxConcurrentFrames`); index per-frame resources by
  `currentBuffer`, the swapchain image by `currentImageIndex`.
- **Time**: animate with `frameTimer` (seconds per frame) or `timer` (0..1, scaled by
  `timerSpeed`). In a test run both advance as if at 60 fps, so pictures repeat.
- **Input** arrives through the base class: the camera is moved for you, ImGui gets
  the pad. `gamePadState.axisLeft`/`axisRight` hold the sticks (-1..1) for anything
  else; `ps5_pad()` (`ps5/src/ps5_samples.h`) gives every button. A program that
  wants the sticks for itself sets `camera.type` and handles them in `render()`.
- **Ending**: set `quit = true` (the starter's Quit button does). Save and release
  everything before the render loop returns.
- **Files**: write only under `/app0/` or `/data/`, `chmod` folders 0777 and files
  0666 so FTP can reach them. A sandboxed title cannot see `/data` until it asks the Lapy
  daemon: `ps5_elevation_request` (`ps5platform/elevation.h`, `skills/ps5-homebrew/references/platform-layer.md`);
  without a running daemon it times out and the title keeps to `/app0`.
- **C++20, exceptions and RTTI** are available; so are `std::thread`, `std::random_device`
  and libc++. `dlopen` is not (`skills/ps5-porting/references/loading-code.md`).
- **A libc call beyond the basics** may not exist on the console: check
  `skills/ps5-homebrew/references/runtime-surface.md` and the platform layer
  (`ps5platform/*.h`) before relying on it.

## Task: copy a technique from a sample

Each sample in `examples/<sample>/` is a complete class on the same base. To bring,
say, shadow mapping into the program:

1. Read `examples/shadowmapping/shadowmapping.cpp` and its shaders in
   `shaders/glsl/shadowmapping/`.
2. Copy the members (offscreen framebuffer, pipelines, uniform data), the setup
   functions and their calls in `prepare()`, the commands in `buildCommandBuffer()`
   and the uniforms in `render()`.
3. Copy the shaders into `shaders/glsl/<id>/`, change the `loadShader(...)` paths to
   `"<id>/<name>.spv"`, and recompile if you change them (`ps5/tools/compile-shaders.sh <id>`).
4. Copy what its constructor and `getEnabledFeatures()`/`getEnabledExtensions()`
   turn on, and its assets (with their `ps5/assets.json` entries, from the
   repository's `ps5/assets.json`).
5. Reconcile the rendering path (render pass or dynamic rendering, above), then
   build, run, and compare the picture with what the sample draws.

| Technique | Sample |
| --- | --- |
| glTF scenes with materials and textures | `gltfloading` |
| a texture's mip chain made with blits; samplers | `texturemipmapgen` |
| PBR with image based lighting (irradiance, prefiltered environment, BRDF table) | `pbrtexture` |
| directional shadow maps with PCF | `shadowmapping` |
| a G-buffer and many lights | `deferred` |
| bloom: offscreen passes, separable blur | `bloom` |
| MSAA with resolve, sample-rate shading | `multisampling` |
| instancing with per-instance vertex data | `instancing` |
| indirect draws from a GPU buffer | `indirectdraw` |
| compute shaders writing a storage buffer drawn as points | `computeparticles` |
| bindless textures (descriptor indexing) | `descriptorindexing` |
| dynamic rendering without render passes | `dynamicrendering`, `starter` |
| Dear ImGui windows of a program's own | `imgui` |
| task and mesh shaders | `meshshader` |
| ray queries against an acceleration structure | `rayquery` |
| a console-grade interface: themes, components, sound, frosted glass (the UI kit) | `uikit`, `uiscreen` |
| a HUD and a pause menu over a 3D scene the glass blurs (`import_texture`) | `uioverlay` |

Upstream has about eighty more examples in `examples/` that are not built for the
console. Any of them can be tried by adding its line to `ps5/src/samples.cpp` (next
task); prove it before trusting it.

## Task: add a sample to PS5 Vulkan Samples (this repository)

1. Add `SAMPLE(<id>, false, "Title", "One line")` to `ps5/src/samples.cpp`. The build
   compiles `examples/<id>/<id>.cpp` (or `main.cpp`) inside a namespace of its own
   (`ps5/src/wrap.cpp.in`) and packages `shaders/glsl/<id>/`. A header the sample
   includes that is not in `ps5/src/prelude.h` must be added there, or it lands
   inside the namespace and fails to compile.
2. Check its assets. Anything from the asset pack whose licence is unclear gets a
   replacement in `ps5/assets.json` (made by `generate_assets.py`, or a CC0 source at
   a pinned SHA-256), and the sample's path changes to it with the comment
   `// PS5 fork: assets with a clear licence replace the asset pack's here (ps5/ASSETS.md)`.
   Then `python3 ps5/tools/build-assets.py --notices`.
3. `ps5/tools/host-reference.sh <id>` and look at the picture (PC driver, headless).
4. `ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh <id>`, and compare.
5. Once it passes on the console, set its second field to `true` (the menu), keep its
   reference (`ps5/tools/host-reference.sh --save <id>`), and add its row, with the
   measured frame rate and difference, to the table in `ps5/README.md`.

## Task: test on the console

```bash
ps5/tools/run.sh                     # every program linked in, 300 frames each
ps5/tools/run.sh launcher all        # PS5 Vulkan Samples: the menu too
ps5/tools/run.sh <id> [<id>...]      # those
FRAMES=600 ps5/tools/run.sh <id>     # another budget (no reference comparison: references are frame 300)
ps5/tools/run.sh --menu              # no test: launch normally and capture klog until the title ends
```

What happens: `run.sh` writes `test-run.txt` into the title folder (`frames N`,
`screenshot`, `samples ...`); PS5_Vulkan's `run-title.py` checks the console is idle,
launches through ps5vkctl and captures klog until the title's `ends: status N` line,
a crash record or its exit; the launch that read the file deletes it, runs each
program on an instance and device of its own, saves each last frame to
`/app0/screenshots/<id>.ppm` (1920x1080), and ends through the shell. Then
`ps5/tools/check-run.py` fetches the pictures and prints a verdict per program:

- **PASS** when the program's line says `ok`, klog has no crash record, GPU fault or
  driver error line, its picture is not one flat colour (standard deviation of 2 or
  more), and, for a 300-frame run with a `ps5/reference/<id>.png`, its picture is
  within a mean of 12 levels in 255 of the reference;
- the frame rate column is measured from the middle of the run to the start of the
  last frame (the screenshot's readback is not counted);
- everything stays in `klog/<YYYYmmdd-HHMMSS>/`: `klog.log`, `<id>.ppm` and `.png`,
  `summary.txt`.

Look at the pictures, not only the verdicts: `python3 ps5/tools/contact-sheet.py
klog/<run>` puts them six to a sheet, and `python3 ps5/tools/compare-run.py klog/<run>
klog/host-<run> [ids]` sets each beside the PC's with their difference.

**Test runs are deterministic**: the launch sets `benchmark.active` (the samples'
random generators get seed 0), time advances 1/60 s a frame, and the pad is ignored.
**The host reference** (`ps5/tools/host-reference.sh [ids]`, `--save` to keep them as
`ps5/reference/`) builds the same code for Linux with a headless surface and libc++
18.1.8 (as the console's SDK; libstdc++'s random distributions differ) and runs the
same test on the PC's driver. A difference only the console shows is RADV's or the
platform's to explain; one the PC shows too is the program's or the asset's.

**Without klog** (klogsrv not running): `run.sh` follows the title's own
`test-results.txt` instead (`ps5/tools/run-without-klog.py`); crash records and
driver messages then go unchecked, and the report must say so.

**Speed**: a run ends the moment the title does; the whole PS5 Vulkan Samples suite
takes about 50 s. Deploying sends only changed files (and `eboot.bin` always).

## Task: change the foundation (this repository)

What the PS5 port changed in upstream's files, all guarded by `VK_EXAMPLE_PS5`:

| File | What |
| --- | --- |
| `base/Entrypoints.h` | `VULKAN_EXAMPLE_MAIN()` defines `createExample()` instead of `main` |
| `base/vulkanexamplebase.h` | the `ps5` struct (frame budget, screenshot path, frames drawn, timing, own overlay, OPTIONS behaviour), `ps5HandleInput()`, the capture semaphore |
| `base/vulkanexamplebase.cpp` | volk loading after instance and device creation, the PS5 render loop (input, budget, fixed time step in tests), the overlay hook for the menu, the screenshot before present, 3840x2160 and the overlay at 2.5x, no benchmark loop |
| `base/VulkanTools.cpp` | `exitFatal` throws; a missing shader is fatal |
| `external/tinygltf/tiny_gltf.h` | no `wordexp` on the console (`__PROSPERO__`) |
| `examples/gltfloading/gltfloading.cpp` | tinygltf's and stb_image's code comes once, from `base/VulkanglTFModel.cpp` |
| twelve samples | asset paths to the replacements, each with its `PS5 fork` comment |

The PS5 layer (`ps5/src/`): `main.cpp` (launch, test runs, the menu loop, one
program's run), `launcher.cpp` (the menu), `samples.cpp` (the programs),
`example_ps5.cpp` (pad and screenshot for the base class), `platform.c`/`.h` (klog,
splash, pad, time, the shell exit), `platform_host.c` (the same on a PC),
`prelude.h` and `wrap.cpp.in` (one namespace a sample). The build: `ps5/CMakeLists.txt`
(the payload SDK's CMake toolchain, every unit compiled with `-include volk.h`,
`VK_NO_PROTOTYPES`, C++20, `-O2 -g1 -fno-omit-frame-pointer`), `ps5/tools/build.sh`
(configure once into `build/ps5/`, compile what changed, link, package, assets) and
`ps5/tools/link-title.sh` (RADV with PS5_Vulkan's recipe, the CRT, the AGC stubs,
the native tool's conversion and signing, the title folder in `dist/<TITLE_ID>/`).

After any change to the foundation: `ps5/tools/build.sh && ps5/tools/deploy.sh &&
ps5/tools/run.sh launcher all` must give a PASS for every program, and the README's
claims must match what the run measured. A change that moves pictures on purpose
gets new references (`ps5/tools/host-reference.sh --save`), named in the commit.
Titles made earlier keep their copy of the foundation; carry a fix into one by hand.

## Task: debug

- **klog** carries everything the title writes to stderr or `std::cout`, prefixed
  `[<title name>]`, and RADV's own messages. Read the run's `klog.log`.
- **A crash** (`A user thread receives a fatal signal`, a GPU fault) ends the run;
  `run-title.py` prints the crash record and symbolises the backtrace against
  `build/ps5/link/llvm-pie.elf`. Backtraces are frame-pointer walks. Read
  `skills/ps5-console/references/crash-reading.md`.
- **`sample <id>: FAILED after ...: <message>`** is a fatal error the program raised
  (`exitFatal`: a `VkResult`, a missing file or shader); the title carried on.
- **Wrong pictures**: run the same program in the host reference first. Both wrong:
  the program or its assets (winding, Y axis, texture format, a `.glb`). Only the
  console wrong: suspect the platform (the `localeconv` case below) or RADV; reduce
  it to the smallest program, and take a driver bug to PS5_Mesa with targeted CTS runs
  (`skills/ps5-homebrew/references/driver-work.md`).
- **Slow first frames** after a driver change are the shader cache being rebuilt;
  measure the second run.
- **RADV switches** (`RADV_DEBUG`, `ACO_DEBUG`, `MESA_SHADER_CACHE_DIR`) are set with
  `setenv` in `main` before the instance exists; the title has no shell to inherit
  them from. A test-only switch goes in a test file the launch reads, never in a default.

## The console, in facts a program designs around

| | |
| --- | --- |
| Vulkan | 1.4 (RADV): mesh and task shaders, ray queries and ray tracing pipelines, descriptor indexing, dynamic rendering among what it reports; absent features are reported absent (`PS5_Vulkan/docs/CTS_GAPS.md` is the live list). Query features as on any GPU |
| Display | one, 3840x2160, through `VK_KHR_display`; FIFO; 119.88 Hz with `param.json`'s high-frame-rate bits, else 59.94 Hz |
| Queues | graphics, compute and transfer families, all served from the graphics ring: no asynchronous compute |
| Shader cache | RADV's, in `/app0/radv-shader-cache/`; never depend on it being warm |
| Memory | direct memory (about 12 GiB, shared with the GPU) holds the heap; flexible memory is small; pages are 16 KiB |
| CPU state | a title starts with MXCSR 0x9fe0 (flush-to-zero, denormals-are-zero); `ps5platform/fp.h` sets IEEE where needed |
| Threads | `pthread` (wrapped: 2 MiB stacks in direct memory); thread-local storage is emulated |
| Files | the title's folder is `/app0/` (`/data/homebrew/<TITLE_ID>/`); USB and extended storage are hidden; sustained writes slow to about 2 MiB/s after 1.3 GiB |
| Code | no `dlopen`; JIT through `ps5platform/exec.h` |
| Ending | through the shell only (rule 1) |

## Pitfalls already paid for

- **`localeconv()` on the console reports an empty decimal point.** nlohmann::json
  (inside tinygltf) then read every number's integer part only: materials of 0.62
  drew black, node scales of 0.2 drew at 0. The platform layer's `ps5_localeconv`
  (SDK fork fa69d00 and later) fixes it, bound by the link (PS5_Vulkan's recipe, or
  `link-title.sh` when the recipe predates it). Keep the SDK pin (`ps5/tools/setup-sdk.sh`)
  at or after fa69d00.
- **A symbol check piped into `grep -q` under `set -o pipefail`** is a coin flip: grep
  stops at the match, the lister takes SIGPIPE, and pipefail reports a miss. The
  link's `localeconv` binding was decided that way, and after the SDK archive grew
  (adc8dd7) half the links came out with the console's `localeconv` and black glTF
  materials. Read the listing whole first (`grep -q ... <<<"$(llvm-nm ...)"`);
  `link-title.sh` now refuses a title whose `localeconv` is unbound.
- **A header first included inside a sample's namespace** breaks the build in strange
  ways (`std::` inside `sample_x::std`): add it to `ps5/src/prelude.h`.
- **A library's `*_IMPLEMENTATION` macro in a sample** duplicates code the base
  already links: guard it with `#if !defined(VK_EXAMPLE_PS5)`.
- **glTF**: the loader takes `.gltf` only; winding is counter-clockwise outward;
  Y is flipped on load.
- **Pictures that differ between console and PC only in random content** come from a
  different C++ library: keep the host reference on libc++ (it is, by default).
- **ImGui writes `imgui.ini`** unless told not to; the foundation turns it off.
- **A fresh checkout has no `build/`**; the tools create it.
- **Leftover test files**: a test run's `test-run.txt` is consumed by the next launch,
  even one started by hand. If a run failed to start, delete it over FTP or the next
  launch from the home screen becomes a test run.

## Conventions

- **Commits**: on `main`; a subject line that says what changed, and a body that says
  why and what was proven (on the console, with numbers). Never commit `.env`,
  `build/`, `dist/`, `klog/`, `.deps/`, screenshots of games, or anything rule 4 names.
- **Code**: match the file you are in. Upstream's files and the C++ PS5 sources use
  tabs and Sascha Willems' brace style; `ps5/src/platform.c`/`.h` use three spaces;
  comments say why, not what. New files carry the
  copyright and licence header the neighbouring files carry (MIT here).
- **Docs**: in the first person of the maintainer ("I"), never "the user" or "the
  owner"; numbers that change stay in the document that owns them, and others point
  at it.
- **Assets**: every new one gets its `ps5/assets.json` entry (path, source, author,
  licence, URL, changes) before it is used.

## Checklist: before saying a title works

- [ ] `ps5/tools/run.sh` gives PASS for every program, and "the title exited on its own".
- [ ] You looked at the pictures (and against the host reference's).
- [ ] The frame rate holds the display's (119.9 or 59.9 fps) where the program means to.
- [ ] Every asset is in `ps5/assets.json` with a clear licence; `ps5/ASSETS.md` is current.
- [ ] Every libc or POSIX call beyond the basics is covered (`runtime-surface.md`).
- [ ] The README states what was measured, and nothing that was not.

## Where the live answers are

| Question | Read |
| --- | --- |
| What does RADV report and do on the console now? | `../PS5_Vulkan/README.md` ("The RADV port"), `../PS5_Vulkan/docs/CTS_GAPS.md` |
| What does the platform layer offer? | `../PS5_PayloadSDK/platform/include/ps5platform/*.h`, `platform/docs/PROBE.md` |
| How is a title linked? | `../PS5_Vulkan/tools/radv-link.sh`, `ps5/tools/link-title.sh` |
| What did each sample measure on the console? | `ps5/README.md` |
| Which assets ship, under which licence? | `ps5/ASSETS.md`, `ps5/assets.json` |
| What do upstream's examples show? | `README.upstream.md`, each `examples/<name>/` |

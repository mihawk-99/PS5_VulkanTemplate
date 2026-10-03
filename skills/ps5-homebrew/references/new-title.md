# A new title

Every new title is made on one foundation: PS5_VulkanTemplate, the repository
these skills live in, the same code its PS5 Vulkan Samples title runs sixteen
programs on. A title is that foundation with
one program of its own, so what an agent learns in one title, or in any sample, holds
in every other.

## Make it

From PS5_VulkanTemplate's folder:

```bash
python3 ps5/tools/new-title.py ../PS5_MyTitle --title-id PPSA99121 --name "My Title" [--refresh 60]
cd ../PS5_MyTitle
ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
```

- **Put it beside `PS5_Vulkan` and `PS5_PayloadSDK`.** Its build finds them as
  `../`; elsewhere, set `PS5_VULKAN_DIR` and `PS5_PAYLOAD_SDK_FORK`.
- **The title id** is `PPSA` and five digits, unique on the console. The script
  refuses the ids my projects already use. A new id needs no registration step: the
  first launch of a freshly deployed folder works (proven with PPSA99120).
- **The name** is what the home screen shows: up to 40 letters, digits, spaces,
  `.`, `_` or `-`. It is also klog's prefix (`[My Title] ...`).
- **`--refresh 60`** leaves out the high-frame-rate bits in `param.json`. The default
  asks for 119.88 Hz (`title-packaging.md`).
- **`--ui <design>`** makes a title whose face is one of the UI kit's designs (`--ui
  list`): its program is the UI starter and its screen a copy of the design in
  `examples/<id>/kit/screen.cpp`. Such a title carries the GPL-3.0 UI module and also
  needs PS5_VKHomebrewUI beside it (or `PS5_VKHOMEBREWUI`; else the pin is fetched
  from GitHub). `ui-kit.md` says how a screen is built.
- **The foundation is copied at PS5_VulkanTemplate's last commit** (its README names the
  revision). Commit a change to the foundation there first if the new title should
  have it.

It makes a git repository on `main` with one commit. Before the first push, create
the GitHub repository and check what would leave the machine (no `.env`, no
captures, no game data).

Proven on 2026-10-01: a title made this way (PPSA99120, "Hello RADV") built from
nothing in 31 s, and its test run held 119.9 fps at 3840x2160 on the 119.88 Hz
display, its last frame 0.6 levels in 255 from the same frame on the PC's driver, and
it ended through the shell.

## What it holds

| Path | What it is |
| --- | --- |
| `examples/<id>/<id>.cpp` | the program: one class on the base class, grown from the starter sample (a textured, lit glTF model, the camera, a settings window with Quit) |
| `shaders/glsl/<id>/` | its shaders: GLSL beside the SPIR-V it loads (`ps5/tools/compile-shaders.sh <id>` after a change, then commit both) |
| `base/` | Sascha Willems' example base class with its PS5 hooks (`VK_EXAMPLE_PS5`): instance, device, the display, frames, the camera, the overlay, glTF models, textures, buffers |
| `external/` | glm, Dear ImGui, libktx, tinygltf, the Vulkan headers |
| `ps5/src/` | the PS5 layer: the launch and test runs (`main.cpp`), the console (`platform.c`: klog, the splash, the pad, the shell exit), the pad and screenshots for the base class (`example_ps5.cpp`), the programs linked in (`samples.cpp`) |
| `ps5/CMakeLists.txt`, `ps5/tools/` | the build (the payload SDK's CMake toolchain, RADV linked with PS5_Vulkan's recipe), the SDK pin (`setup-sdk.sh`), deploy, test runs and their verdicts, the host reference |
| `ps5/sce_sys/` | `param.json` (the identity, the one place it is written) and the icon |
| `ps5/assets.json`, `ps5/ASSETS.md`, `ps5/assets/` | the assets shipped, each with its origin and licence, and the notices written from them |
| `LICENSE.md`, `README.md` | MIT, the foundation's; how to build, run and grow it |

## What the launch does

1. **klog** (`platform_init`): standard error, and the samples' `std::cout`, go to
   klog with the title's name as prefix. Nothing else reads a title's output.
2. **The splash** is dismissed; it covers the title's frames until then.
3. **The pad** is opened, with retries (the service can lag the launch).
4. **Vulkan without a loader**: volk, initialised from `vk_icdGetInstanceProcAddr`,
   the RADV linked into the title (`vulkan-on-radv.md`).
5. **A test run** if `/app0/test-run.txt` is there (the run tool writes it; the launch
   deletes it): each program for the frame budget, a screenshot of the last frame, a
   line each in klog and in `test-results.txt`, then the end. Otherwise the program
   runs until it ends itself (its Quit), and the title ends with it.
6. **The display**: the base class's direct-to-display path, `VK_KHR_display`, the
   console's one mode, 3840x2160, at 119.88 Hz when `param.json` asks.
7. **The end**: `main` returns, and `catchReturnFromMain` asks the shell to close the
   title (`platform-contracts.md`, "Ending a title"). A fatal error in the program
   (`vks::tools::exitFatal`) throws instead of calling `exit`, and is reported.

In the program, the left stick turns the camera and the right one zooms (or walks and
looks, for a first-person camera); OPTIONS or the touch pad shows and hides the
settings window; the D-pad and CROSS change the settings.

## Growing it into a program

- **Replace the model, then grow the class.** `loadAssets` names it; add it to
  `ps5/assets.json` with its licence (a CC0 source at a pinned hash, made by
  `ps5/tools/generate_assets.py`, or a file kept in `ps5/assets/`).
- **Copy a technique from a sample.** Shadows (`shadowmapping`), deferred lighting,
  bloom, MSAA, PBR with image based lighting, instancing, indirect draws, compute
  particles, bindless textures, dynamic rendering, ImGui, mesh shaders, ray queries:
  each is a class on the same base, proven on the console, and its methods and
  shaders copy across as they stand.
- **More than one program**: another `SAMPLE(...)` line in `ps5/src/samples.cpp`
  makes the title a menu of them, as PS5 Vulkan Samples is.
- **C++20, exceptions and RTTI** are available: the link brings libc++, libc++abi
  and libunwind, and the CRT runs static constructors and destructors.
- **Libraries:** add their sources to `ps5/CMakeLists.txt` (built with the same
  toolchain), or build them into static archives with it. `dlopen` is refused to a
  title (`ps5-porting` skill, "Loading code").
- **Audio, save files, threads, memory:** `platform-contracts.md` and
  `platform-layer.md`.
- **Settings and logs** a person may want to read or edit over FTP: create them
  with mode 0666 (folders 0777), set with `chmod`. The console's FTP server is
  another process, and it needs them that open to fetch or replace them.
- **A fix to the foundation** (the base class, the PS5 layer, the tools) goes into
  PS5_VulkanTemplate first, where every sample tests it, then into the titles.

## Checks before calling a title working

- `ps5/tools/run.sh` ends with "the title exited on its own" and a PASS for each
  program: its klog line says ok, klog holds no crash record, GPU fault or RADV error,
  and its last frame is not one flat colour.
- `ps5/tools/host-reference.sh --save` once the picture is right, so later runs are
  compared with it (a mean difference over 12 levels in 255 fails). Test runs are
  deterministic (a fixed time step, fixed random seeds, the pad ignored), so the
  console's frame and the PC's should match closely.
- Look at the pictures (`klog/<run>/<id>.png`): a run that reports frames has not
  necessarily shown the right ones.
- The frame rate matches the display (119.9 or 59.9 fps) when the title is meant to
  keep up.
- Every libc or POSIX call beyond the basics is one the platform layer covers
  (`runtime-surface.md`).

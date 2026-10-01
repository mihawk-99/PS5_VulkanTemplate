---
name: ps5-homebrew
description: Creating or building a PlayStation 5 homebrew title on my stack - a native title that renders through RADV (Mesa's Vulkan driver with the PS5 winsys, PS5_Mesa/PS5_Vulkan) and runs on the PS5_PayloadSDK fork's platform layer. Use it to start a new title, link RADV, package eboot.bin and sce_sys/, use the display, pad, audio or memory, or decide which project a fix belongs in.
---

# PS5 homebrew on RADV

A title is a native x86-64 ELF for the console (`x86_64-sie-ps5`), signed into a
fake SELF (`eboot.bin`) and run from a folder under `/data/homebrew/<TITLE_ID>/` on a
jailbroken PS5. It renders through **RADV**, Mesa's Vulkan driver, linked into the
title: the console has no Vulkan of its own and a title cannot load one. RADV
reports Vulkan 1.4 on the console and presents through `VK_KHR_display` on the
console's VideoOut.

Five projects make that possible, each with one job. Know which owns what before
changing anything (`references/stack.md`):

| Project | Owns |
| --- | --- |
| `PS5_PayloadSDK` | the toolchain, and the **platform layer** (`libps5platform.a`): heap, executable memory, the libc the console lacks, klog, the machine context |
| `PS5_Mesa` | RADV with the PS5 winsys and the VideoOut WSI, on `main` |
| `PS5_Vulkan` | builds RADV's release archive at a pinned Mesa revision; the link recipe, the native packaging tool, the CTS on the console, the smoke test, the console tools (`ps5vkctl`, `run-title.py`, `deploy-title-folder.py`) |
| a title | its own code, `sce_sys/`, and a build that consumes the four above |
| `PS5_RetroArch`, `PS5_vkQuake`, `PS5_ProsperoEden`, `PS5_VulkanTemplate` | working titles to copy from |

## Start a new title

Don't start from an empty directory or from a copy of another title. Every new
title is made on one foundation, PS5_VulkanTemplate (the repository these skills
live in), the same code its sixteen samples run on, proven on the console (2026-10-01: Vulkan
1.4.354, 3840x2160 at 119.88 Hz, 119.9 fps, each sample's last frame matching the
PC's driver's, a clean exit through the shell):

```bash
# from PS5_VulkanTemplate's folder; ps5/tools/bootstrap.sh first, once
python3 ps5/tools/new-title.py ../PS5_MyTitle --title-id PPSA99121 --name "My Title"
cd ../PS5_MyTitle
ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
```

The title gets Sascha Willems' Vulkan example base class with its PS5 hooks, the
PS5 layer (klog, the splash, the pad, the `VK_KHR_display` display, test runs that
end in a screenshot, the shell exit, the build and the console tools), and one
program: the starter sample, a textured, lit glTF model, the camera on the sticks
and a settings window on the pad, as `examples/<id>/<id>.cpp`. Replace the model and
grow the class. Every sample is a class on the same base, so a technique (shadows,
deferred lighting, bloom, compute, mesh shaders, ray queries...) copies in as it
stands. What each part is for: `references/new-title.md`.

## Rules that cost runs to learn

1. **A title never calls `exit()` and never returns from `_start`.** It asks the
   shell to close it (`sceSystemServiceLoadExec("exit", NULL)`) from
   `catchReturnFromMain`, then waits. Anything else is reported as a crash over a
   run that worked. `references/platform-contracts.md`, "Ending a title".
2. **Link RADV with PS5_Vulkan's recipe, not your own.** `tools/radv-link.sh` links
   the archive whole, wraps the heap and threads, and binds the libc names the
   console lacks to the platform layer. A hand-written link builds and then calls
   through NULL. `references/vulkan-on-radv.md`.
3. **Platform gaps go into the platform layer, not the title.** A missing libc
   function, a memory or threading rule, a kernel call: add it to
   `PS5_PayloadSDK/platform/`, test it there, bump the pin. A per-title workaround is
   the next title's bug. `references/platform-layer.md`.
4. **Driver gaps go into PS5_Mesa, and they are general fixes.** A RADV bug is fixed
   for every application, with a targeted CTS run behind it, never worked around in
   one title or special-cased for one game. `references/driver-work.md`.
5. **Check the runtime surface before the first deploy.** A symbol the SDK's stubs
   satisfy may not exist on the console. `references/runtime-surface.md`.
6. **Prove it on the console before writing it down.** A claim in a README, a
   commit message or a release note is a claim until a run backs it, and the run is
   reported at the same detail whether it passed or failed.

Running, watching, stopping and debugging titles on the console has its own skill,
`ps5-console`. Bringing existing software (an engine, an emulator, a library) to
the console is `ps5-porting`. Publishing a release is `ps5-release`.

## Standing rules

- Keep the console's address and ports in PS5_Vulkan's ignored `.env` (every
  console tool reads it), never in a commit, a log you paste, or a
  document. Mask addresses when printing tool output.
- Never copy, commit or upload console firmware or anything taken from it: call
  only exported functions, and write down only exported names and behaviour
  measured on the console.
- Never commit, upload or ship game data, BIOS files, keys or saves.
- Every repository keeps its work on `main`, published history is never rewritten
  or force-pushed, and forks are pinned by revision in the consumer's build, not
  applied as patch series.
- Documentation speaks in the first person ("I"), never about "the owner" or "the
  user".

## Where the live answers are

Point at these; never copy their numbers into another project, because they change
with every round.

| Question | Read |
| --- | --- |
| What does RADV do on the console right now? | `PS5_Vulkan/README.md` ("The RADV port"), `docs/RADV_PHASE.md` |
| Which Vulkan features are missing, and why? | `PS5_Vulkan/docs/CTS_GAPS.md` |
| What has the console measured about memory, files, threads, FP? | `PS5_PayloadSDK/platform/docs/PROBE.md` |
| What does the platform layer offer? | `PS5_PayloadSDK/platform/include/ps5platform/*.h` |
| How is RADV linked? | `PS5_Vulkan/tools/radv-link.sh` |
| A whole emulator frontend as a worked example | `PS5_RetroArch/src/` (`main.cpp`, `core_loader_ps5.cpp`, `audio_ps5.cpp`, `input_ps5.cpp`) |
| A whole game as a worked example | `PS5_vkQuake/src/` |
| A Vulkan technique proven on the console (glTF, PBR, shadows, deferred, bloom, MSAA, instancing, indirect draws, compute, bindless, dynamic rendering, ImGui, mesh shaders, ray queries) | `PS5_VulkanTemplate/examples/<id>/`, the table in its `ps5/README.md` |

## References

| File | Read it when |
| --- | --- |
| `references/stack.md` | it is unclear which project owns an answer or a fix |
| `references/new-title.md` | starting a title, or growing its starter into a program |
| `references/vulkan-on-radv.md` | writing the renderer, linking RADV, choosing features |
| `references/platform-contracts.md` | ending the title, the display, the pad, audio, splash, FP state |
| `references/platform-layer.md` | memory, threads, files, libc gaps: what the platform layer gives |
| `references/title-packaging.md` | `param.json`, `sce_sys/` assets, signing, the title folder |
| `references/toolchain.md` | setting up a build, flags, a miscompile |
| `references/runtime-surface.md` | before the first deploy, or a call jumps to zero |
| `references/driver-work.md` | RADV itself is wrong or missing something |

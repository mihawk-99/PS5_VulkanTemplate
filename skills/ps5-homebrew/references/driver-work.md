# When RADV itself is the problem

## Decide whose bug it is

A title's rendering problem is the driver's only when the same Vulkan usage is
valid by the specification and renders correctly on a desktop RADV. Before
touching the driver:

1. **Reduce it.** The smallest program that shows it: one sample of
   PS5_VulkanTemplate (or its starter, or a title made with `new-title.py`), the smoke test
   (`PS5_Vulkan/radv/radv_smoke.c`), or a CTS group.
2. **Run the matching CTS cases on the console** with `PS5_Vulkan/tools/run-cts.py`
   (title PPSA99015): `--case 'dEQP-VK.pipeline.*'` for one pattern in one launch,
   `--caselist FILE` or `--mustpass GROUP...` for batched runs (resumable with
   `--run NAME`), `--env NAME=VALUE` for a driver switch. A CTS failure is a driver
   bug by definition. A clean CTS run with a broken title points back at the title,
   or at a gap the CTS does not cover.
3. **Compare with desktop RADV** where you can: the same Mesa revision, built for
   Linux, is the reference for "what RADV does".

## Rules for driver changes

- **General fixes only.** A fix makes RADV correct for every application. Never key
  behaviour on an application name, a game, or one title's usage pattern, and never
  work around a driver bug inside one title and leave the driver wrong.
- **Real capabilities only.** A feature, limit or extension the driver reports must
  work for every valid use. A capability that works for some shapes, or that a
  run-time refusal fences off, is not reported.
- **The CTS is the gate.** Every change is proven by targeted runs of the CTS groups
  it touches, on the console, and must not change a passing case. A full CTS run
  comes last, after every non-passing case and every "not supported" the port
  causes has been addressed by targeted runs.
- **Platform gaps are not driver changes.** Memory, threads, libc and kernel calls
  go into the platform layer (`platform-layer.md`), not into the Mesa fork.

## Where things are

| Part | Location |
| --- | --- |
| the PS5 winsys: memory, command submission, sync, platform setup | `PS5_Mesa/src/amd/vulkan/winsys/ps5/` |
| the display (VK_KHR_display on VideoOut, 120 Hz, pacing) | `PS5_Mesa/src/vulkan/wsi/wsi_common_videoout.c` |
| everything else | upstream RADV, ACO, NIR, Mesa's runtime: changed only where the console differs |
| the build and the pin | `PS5_Vulkan/tools/build-radv.sh` (`mesa_revision=`) |
| the round-by-round record | `PS5_Vulkan/docs/RADV_PHASE.md` |
| every non-passing case and every port-caused absence | `PS5_Vulkan/docs/CTS_GAPS.md` |

## The loop

1. Change PS5_Mesa on `main`, commit.
2. `tools/build-radv.sh` in PS5_Vulkan (assertions on) for the smoke test and the
   CTS; `tools/build-radv.sh release` for titles. Each build of a new revision
   changes RADV's cache identity, so titles recompile every pipeline once. The first
   run after a driver change is slow to start, and is not a regression.
3. Targeted CTS runs on the console; the smoke test (`tools/build-radv-title.sh`,
   PPSA99014); and the PS5 Vulkan Samples suite (`PS5_VulkanTemplate`:
   `ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh launcher all`),
   fifteen real workloads, each 300 frames, its last frame compared with the same
   frame on the PC's driver (`ps5/tools/host-reference.sh`). A sample whose picture
   differs from the PC's names the feature to look at.
4. Pin the revision in `tools/build-radv.sh`, rebuild the release archive, record
   the round in `docs/RADV_PHASE.md`.
5. **Rebuild and regression-run every title that links RADV** (RetroArch with every
   core, vkQuake, ProsperoEden, PS5 Vulkan Samples), compare with their published
   figures, and update their READMEs. A driver fix that breaks a title is not finished.

## Debugging aids

- RADV's messages reach klog through the title's stderr capture. `RADV_DEBUG` and
  `ACO_DEBUG` work as on Linux, set with `setenv` before the instance is made, or
  from a test file the title reads at launch (the smoke test reads
  `/app0/radv-smoke-env.txt`).
- A GPU fault or hang shows in klog as a GPU fault record and usually ends the run.
  Reduce it to one draw or dispatch before changing anything.
- Output too large for klog (`RADV_DEBUG=shaders`) goes to a file in the title
  folder instead (the smoke test's `RADV_SMOKE_STDERR`).

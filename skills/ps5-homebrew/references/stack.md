# The stack, and who owns which answer

Everything is checked out side by side in one folder, and the projects find
each other as siblings (`../PS5_Vulkan`, `../PS5_Mesa`, `../PS5_PayloadSDK`), each
overridable by an environment variable. All of them are on GitHub under
`mihawk-99`, all keep their work on `main`. PS5_VulkanTemplate's
`ps5/tools/bootstrap.sh` clones the others beside it and builds what a title needs.

```text
  a title (examples/<id>/, ps5/)
     |  links
     v
  RADV release archive ......... libvulkan_radeon.ps5.a: RADV + ACO + NIR + Mesa's
     |                           runtime + the PS5 winsys + the VideoOut WSI,
     |                           built by PS5_Vulkan from PS5_Mesa at a pinned revision
     v
  platform layer ................ libps5platform.a from the PS5_PayloadSDK fork:
     |                           heap, exec memory, shm arenas, libc gaps, klog,
     |                           FP state, machine context
     v
  payload SDK (v0.42 + fork) .... prospero-clang wrapper, lld, crt, libc/libc++
     |                           stubs, the console's link stubs (.so per module)
     v
  the console ................... libkernel, libc, AGC, VideoOut, AudioOut, Pad,
                                  UserService, SystemService - exported functions only
```

## The projects

**PS5_PayloadSDK** - my fork of ps5-payload-dev/sdk at v0.42. The release's
binaries are kept as released (they are what the console has accepted); the fork
adds `platform/` and fixes the machine context in `sys/_ucontext.h`. A project pins
one revision and installs it with `platform/tools/setup-sdk.sh` through its own
`tools/setup-native-dependencies.sh`, which records the revision in
`.deps/native/ps5-payload-sdk/.ps5-sdk-revision`.

**PS5_Mesa** - my Mesa fork (26.2.0 base): RADV built with `-Dradv-winsys=ps5`.
The PS5 parts are `src/amd/vulkan/winsys/ps5/` (memory from direct memory, command
streams submitted through AGC, sync, the platform setup) and
`src/vulkan/wsi/wsi_common_videoout.c` (the display). Everything else is upstream
RADV, unchanged but where the console differs.

**PS5_Vulkan** - the driver project. It:
- builds the archive titles link: `tools/build-radv.sh release` into
  `.deps/native/radv-release/lib/libvulkan_radeon.ps5.a`, with a `PROVENANCE.txt`
  naming the Mesa revision (`tools/build-radv.sh` without `release` keeps
  assertions on, for the smoke test and the CTS);
- pins that revision (`mesa_revision=` in `tools/build-radv.sh`);
- owns the link recipe every title uses (`tools/radv-link.sh`);
- carries the native packaging: the CRT and C++ runtime (`tooling/native/`), the
  ELF-to-SELF tool (`build/host/ps5-native-tool`), the clean-room `runtime/libc.prx`;
- runs the Khronos CTS on the console (`tools/run-cts.py`, title PPSA99015) and a
  smoke test (`radv/radv_smoke.c`, PPSA99014);
- owns the console tools every project uses: the control payload `ps5vkctl`,
  `tools/ps5_console.py`, `tools/run-title.py`, `tools/deploy-title-folder.py`;
- still builds ps5vk, the first driver, as `PS5_VULKAN_DRIVER=ps5vk` in the titles
  that kept it. New work never targets ps5vk.

**The titles** - PS5_RetroArch (PPSA99169, an emulator frontend loading libretro
cores, most of them forks of mine), PS5_vkQuake (PPSA99010), PS5_ProsperoEden
(PPSA99008, my experimental fork of BlackBearReloaded's Eden port, for benchmarking
the driver), and Vulkan Template (PPSA99130), which PS5_VulkanTemplate builds: my fork of
Sascha Willems' Vulkan examples, sixteen samples in one title, a reference for Vulkan
techniques on the console, a driver test suite, and the foundation every new title is
made on (`ps5/README.md` there). Each links the RADV archive
through `radv-link.sh`, with the payload SDK fork at a revision it pins itself.

**ps5-homebrew-template** - the old starting point. It links ps5vk, not RADV, and is
superseded by PS5_VulkanTemplate's `ps5/tools/new-title.py`, which makes new titles on
the foundation the samples run on.

## Which project gets the change

| The problem | It goes into |
| --- | --- |
| a libc function the console lacks or refuses; a memory, thread or file rule; a kernel call | the platform layer, `PS5_PayloadSDK/platform/`, with a host test and, for console behaviour, a probe |
| RADV renders wrongly, crashes, hangs, or lacks a feature the hardware has | `PS5_Mesa` (winsys or RADV), proven by targeted CTS runs, then a new pin in `PS5_Vulkan` (`references/driver-work.md`) |
| how titles link RADV | `PS5_Vulkan/tools/radv-link.sh` |
| the packaging tool, the CRT, `libc.prx` | `PS5_Vulkan/tooling/native/`, `runtime/` |
| the console tools (launch, kill, deploy, klog) | `PS5_Vulkan/tools/`, `payload/ps5vkctl/` |
| one program's behaviour | that title |
| a third-party library or emulator core | my fork of it, pinned by the title (`ps5-porting` skill) |

The test that decides: **would the next title need the same fix?** Then it does not
belong in this one.

After changing a shared layer (platform layer, RADV, link recipe), every title that
consumes it gets the new pin and a regression run on the console before the change
is called finished (`ps5-console` skill, "Regression runs").

## Volatile facts live in their owners' documents

Feature lists, CTS counts, performance figures and open gaps change every round.
Quote them from the owner's document at the moment you need them, with its date; do
not copy them into another repository or into this skill.

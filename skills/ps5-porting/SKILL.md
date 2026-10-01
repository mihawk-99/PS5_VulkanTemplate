---
name: ps5-porting
description: Bringing existing software to my PS5 homebrew stack - a game engine, an emulator or libretro core, or a library - as a native title or as part of one (PS5_RetroArch). Covers forking and pinning, cross-building, replacing the platform code, Vulkan renderers on RADV, JITs and fastmem, threads, files, and the console's limits. Use it before and during any port.
---

# Porting to the PS5

Most of a port is not the program: it is the program's assumptions about an
operating system the console only partly is. The console is FreeBSD-like underneath,
with no window system, no dynamic loading, a small flexible memory budget beside a
large direct-memory pool, throttled file writes, FTZ/DAZ floating point, and a libc
that lacks or refuses part of POSIX. Each of those has a known answer, written once
in the platform layer or in a project that already met it. Reuse them.

## The order of work

1. **Read the licence first.** It decides whether the port can ship and with what
   (`ps5-release` skill, `licensing.md`). A GPL-2.0-only program cannot be
   distributed linked with my GPL-3.0-or-later platform layer and frontend; it can
   still be built for my own console.
2. **Fork it** on GitHub as `mihawk-99/PS5_<Name>`, work on `main`, and pin the
   revision in the consuming project's build script (`references/forks.md`).
3. **Cross-build it unchanged first**, as a static archive with the SDK's compilers
   (`ps5-homebrew` skill, `toolchain.md`). The errors list the platform code to
   replace.
4. **Replace the platform code, not the program.** Keep the PS5 changes behind
   `__PROSPERO__` (the compiler defines it), in as few files as possible, the way
   upstream keeps its other platforms. Where a gap is the console's (libc, memory,
   threads), the answer goes in the platform layer, not in the fork.
5. **Bring it up on the console's skeleton**: klog capture, splash, display, pad,
   audio, the shell exit, as PS5_VulkanTemplate's PS5 layer does them (`ps5/src/`;
   `ps5-homebrew` skill), then the program's main loop.
6. **Prove each step on the console** (`ps5-console` skill), and keep the run's
   numbers.

## What usually needs replacing

| The program uses | On the console |
| --- | --- |
| a window, SDL, GLFW | the `VK_KHR_display` swapchain (`ps5-homebrew` skill, `vulkan-on-radv.md`); a small platform file for pad and audio |
| OpenGL | port its Vulkan backend if it has one (RADV is a full Vulkan 1.4). Otherwise BlackBearReloaded's ps5-opengl is the GL route (ProsperoEden's GL backend uses it) |
| `vkGetInstanceProcAddr` from a loader | one forwarding function to `vk_icdGetInstanceProcAddr` |
| `dlopen` of plugins or drivers | link statically, or load ELF code with an in-process loader (`references/loading-code.md`) |
| `mmap(PROT_EXEC)` for a JIT | `ps5platform/exec.h` (`references/memory-and-jit.md`) |
| guest memory with mirrors, fastmem | `ps5platform/shm.h` plus a SIGSEGV handler using the fork's machine context |
| `malloc` of hundreds of MB | the platform heap, in direct memory (the link recipe wraps malloc) |
| files anywhere, `$HOME`, `getcwd` | `/app0/` (the title folder) and `/data/`; no `getcwd` (`references/files-and-io.md`) |
| heavy file writes | throttled after about 1.3 GiB: write through the console's FTP server (`offload.h`) |
| denormals, IEEE rounding | set MXCSR per thread (`ps5platform/fp.h`): a title starts FTZ/DAZ |
| `exit()` | the shell exit (`ps5-homebrew` skill, `platform-contracts.md`) |
| networking | treat as unavailable until a probe proves otherwise |

## Rules for emulators and engines

- **Hold full speed while shaders compile.** Use asynchronous compilation: ubershaders,
  background pipeline compiles, pipeline libraries. Never rely on a warmed shader
  cache: the first launch of every game is someone's first impression.
- **Make the program faster rather than doing less.** When a game falls short of full
  speed, optimise the emulator for the console (its hot paths, its threading, the
  driver underneath) instead of underclocking the emulated CPU or skipping work that
  changes the game's behaviour.
- **No accuracy traded away silently.** A setting that trades accuracy for speed is
  named as such, and is not a default unless I chose it.
- **Defaults are the best picture that holds full speed** in the games tested (the
  highest internal resolution that keeps 100%), recorded with the measurement behind
  them.
- **Track upstream.** A fork that drifts behind upstream loses its fixes. Merge
  upstream regularly, and port upstream's features into a libretro fork rather than
  leaving it behind (LRPS2 follows PCSX2 this way).

## Worked examples

| Port | Look at |
| --- | --- |
| a whole game engine | PS5_vkQuake (`src/`: display, pad, audio, memory, exit, the RADV ICD forward) |
| an emulator frontend loading cores | PS5_RetroArch (`src/core_loader_ps5.cpp`, `core_imports_ps5.cpp`, `tools/build-*.sh`) |
| JIT emulators | PS5_RetroArch's PPSSPP, Dolphin, Azahar (Dynarmic), DeSmuME, Mupen64Plus, LRPS2 and RPCS3 forks |
| a large C++ application | PS5_ProsperoEden (Eden): its own build, RADV through `tools/radv-link-eden.sh` |

## References

| File | Read it when |
| --- | --- |
| `references/forks.md` | creating, pinning, updating or upstreaming a fork |
| `references/memory-and-jit.md` | heaps, direct memory, JIT code, guest memory, fastmem, signals |
| `references/files-and-io.md` | paths, saves, configs, write speed, directories |
| `references/loading-code.md` | plugins, dynamic libraries, cores, static linking |

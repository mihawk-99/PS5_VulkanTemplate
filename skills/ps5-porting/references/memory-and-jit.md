# Memory, JIT code and fastmem

Read `PS5_PayloadSDK/platform/docs/PROBE.md` for the measured numbers. The shapes
below are what every port has needed.

## Two pools, and which to use

| Pool | Size | What goes there |
| --- | --- | --- |
| **flexible memory** (the title's own budget, what plain `mmap` and libc's heap draw from) | small: a few hundred MiB free at start | little: let the libraries that insist on `mmap` have it |
| **direct memory** (shared with the GPU) | about 12 GiB | everything big: the heap, JIT caches, guest memory, loaded code, large buffers |

- **The heap.** The link recipe wraps `malloc` and its family into the platform heap
  (`ps5platform/heap.h`): dlmalloc in direct memory, an arena per allocating thread
  up to eight. A port's own allocator can sit on `ps5_heap_*`. libc's heap ran out
  long before the title in every project that relied on it (an 8 KiB `operator new`
  refused under the CTS; cores out of memory with hundreds of MB of flexible memory
  free).
- **More than 4 GiB at once** works in direct memory (PROBE.md, "More than 4 GiB at
  once").
- **Placement matters.** A mapping with no address can land in the Vulkan driver's
  GPU window. The platform layer places everything it maps; do the same with any
  `sceKernel*` mapping of your own, inside a range you reserved.
- **Pages are 16 KiB** (0x4000). Every `mprotect`, guard page and fastmem view works
  in 16 KiB units, which limits page-table-based fastmem schemes designed for 4 KiB
  pages.

## JIT code: `ps5platform/exec.h`

What the console measured: direct memory mapped read-write and then given execute
with `sceKernelMprotect` runs (read-execute, and read-write-execute alike). Asking
for execute at map time is refused. A protection change costs about 26 us. So:

- Get a code region from `exec.h` (placed, in direct memory, read-write-execute by
  default, so the JIT writes and runs without a protection change per block).
- Do not `mmap(PROT_EXEC)`: refused.
- Do not flip protections per emitted block: at 26 us each, a busy JIT spends frames
  on it.
- **Size the code cache for the worst case.** An early limit of 128 small JIT
  regions crashed a GameCube game between a mission and its menu. The title now
  hands out as many as a core asks for.

Proven by PPSSPP, Dolphin (its JIT and vertex loaders), Dynarmic (Azahar),
DeSmuME, Mupen64Plus (new dynarec and ParaLLEl-RSP), LRPS2 and RPCS3 in
PS5_RetroArch.

## Guest memory, mirrors, fastmem: `ps5platform/shm.h`

An emulator wants one block of guest memory visible at several addresses (mirrors, a
fastmem window where guest address = host base + offset). On the console:

- POSIX shared memory works, but **every view is charged to the small flexible
  budget**.
- One direct-memory allocation can be mapped any number of times, charged once to
  the direct pool. `shm.h` wraps that, inside reserved virtual ranges. It is Dolphin's
  MemArena generalised, and LRPS2's and PPSSPP's arenas use the same code.

## Signal handlers (fastmem faults, JIT guards)

- Install with `sigaction` and `SA_SIGINFO`. The handler reads registers from
  `ucontext_t`, and **must use the SDK fork's layout**: the console's context sits 48
  bytes later than FreeBSD's header says. With the fork's `sys/_ucontext.h`,
  `uc->uc_mcontext.mc_rip` (and the other `mc_*` registers) are right. Include
  `ps5platform/context.h` so a build against an upstream SDK fails instead of reading
  garbage.
- A handler that resumes execution sets `mc_rip` (and the registers it patched), and
  returns.
- Keep a fault the handler does not own fatal: chain to the previous handler, so the
  console's crash report still names the real fault.

## Threads

- `pthread_create` with no stack size gets 2 MiB in direct memory (the link
  recipe's wrap). Ask for more where the program needs it: emulators' CPU threads
  often do.
- **Set the FP state on every thread that needs IEEE behaviour.** A thread starts
  FTZ/DAZ (MXCSR 0x9fe0). `ps5platform/fp.h` has the IEEE state.
- Thread-local storage is emulated: a TLS access is a call. Keep hot per-thread
  state in a struct you pass, not in `thread_local` variables read in an inner loop.
- The console's CPU is an 8-core Zen 2 with SMT. The platform layer's topology probe
  measured which logical CPUs share a core: pin cooperating threads accordingly when
  it matters (PROBE.md, "Threads").

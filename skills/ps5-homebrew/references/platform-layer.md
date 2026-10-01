# The platform layer

`libps5platform.a`, built from `PS5_PayloadSDK/platform/` and installed with the SDK
fork (`target/lib/libps5platform.a`, `target/include/ps5platform/`). It is the one
place where everything every title needs from the console, and cannot take from
the released SDK, is written down and tested. Its statements about the console
come from its own probes (`platform/docs/PROBE.md`, with `evidence/`), never from
reading the system software.

## What it gives a title

| Header | What it is for |
| --- | --- |
| `heap.h` | malloc and its family in **direct memory** (dlmalloc mspaces, one per allocating thread, up to eight). libc's private heap runs out long before the title does. Titles get it through the link recipe's `--wrap=malloc` and its family, or call `ps5_heap_*` beside an allocator of their own. |
| `exec.h` | executable code in direct memory, for JITs: mapped read-write, then given execute with `sceKernelMprotect`. Asking for execute at map time is refused, and every region is placed (an unplaced mapping lands in the GPU window). A protection change costs about 26 us. |
| `shm.h` | one direct-memory object mapped at several addresses inside reserved virtual ranges: an emulator's guest memory with mirrors and a fastmem window. POSIX shared memory would charge every view to the small flexible budget. |
| `libc.h` | the libc functions the console lacks, refuses or faults in, as `ps5_*`: `access` (refused for every path), `statvfs`, the directory and `*at` families, `getpwuid_r`, `posix_fallocate`, `utimensat`/`futimens`, `clock_nanosleep`, `getaddrinfo`, `memfd_create`, `open_memstream`, `nl_langinfo`, the `xlocale` family, `regex` (musl's TRE), `backtrace`, `dladdr`, `__cxa_thread_atexit_impl`, `localeconv` (the console's reports an empty decimal point, and nlohmann::json then reads 0.62 as 0)... The link recipe binds libc's names to them. |
| `klog.h` | `ps5_klog_capture_stderr(prefix)`: standard error, line by line, into klog. A title may not `dup2` (EPERM), so the stream is moved, not the descriptor. |
| `fp.h` | the IEEE floating-point state (a title starts with FTZ/DAZ on). |
| `context.h` | the machine context as the console lays it out. The console's `ucontext_t` has 48 more bytes before `uc_mcontext` than FreeBSD's; the fork's `sys/_ucontext.h` carries them, so `uc->uc_mcontext.mc_rip` is the faulting instruction. Including `context.h` refuses to build against an upstream SDK. |
| `offload.h`, `ftp.h` | writing files through the console's own FTP server on 127.0.0.1. Once a title's write burst (about 1.3 GiB) is spent, its writes drop to about 2 MiB/s, while the server, another process, writes at about 7 MiB/s a connection and 22 MiB/s in all. |
| `kernel.h` | the exported kernel functions the layer and its consumers call, declared once. |
| `agc.h`, `videoout.h` | the AGC and VideoOut functions the GPU drivers call. |
| `probe.h` | the capability probe behind PROBE.md. |

## Measured facts a title designs around

From `PROBE.md`; read it for the numbers and the evidence:

- **Two pools.** Direct memory is large: about 12 GiB, shared with the GPU. The
  title's flexible memory is small: a few hundred MiB free at start. Put big things
  (heaps, JIT caches, guest memory, loaded code) in direct memory.
- **Virtual space** has to be reserved and placed deliberately. Mappings without an
  address land in the GPU driver's window.
- **Thread-local storage** is emulated (`__emutls_*`), and costs a call per access.
- **Writes are throttled** after the burst (above).
- **FP state** starts as FTZ/DAZ.

## Adding to it

When a title meets something the console lacks:

1. Check `libc.h` and the other headers first. It may already be there, waiting to
   be bound.
2. If not, add it to `platform/src/` with a host test (`make -C platform test`, against
   the host model of the console's kernel). If it states something about the
   console, add a probe and record the console's answer in `PROBE.md`.
3. Bind it: for a libc name, add it to the `--defsym` list in
   `PS5_Vulkan/tools/radv-link.sh`, so every title gets it.
4. Commit and push the fork, then bump the pinned revision in each consumer
   (`sdk_revision` in its `tools/setup-native-dependencies.sh`), reinstall, rebuild,
   and run the consumers on the console.

Rules the layer keeps: exported functions only, no raw system calls or internal
entry points, and nothing taken from the console's firmware.

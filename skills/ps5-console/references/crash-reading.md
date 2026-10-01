# Reading a console death

## Two records, and neither is complete alone

| Record | Answers |
| --- | --- |
| the title's own output: klog lines with its prefix, log files in its folder | what the program did and said |
| the kernel's records in klog | what happened to the process: signals, exits, the crash report, GPU faults |

A program's output ends at its last write; a fatal signal can arrive after it,
during shutdown. A clean-looking trace does not mean a clean end. Read both.

## The crash report

A fatal signal makes the console write a report into klog, starting at **"A user
thread receives a fatal signal"**. The lines to read:

```text
# reason: ...                      the signal and its cause
# fault address: ...               what was accessed
# rip: 0000000000430b34            the instruction
# backtrace:                       return addresses, one a line
# 0000000000431f20
# dynamic libraries:
# /app0/eboot.bin
#  xotext: 0000000000400000:00000000009f0000 nsegs: 4     the title's load base
```

The same report also produces a coredump and a `gpudump.elf` run. **The GPU dump is
routine, not evidence of a GPU fault.**

`run-title.py --elf build/llvm-pie.elf` symbolises the rip and the backtrace by
itself, as functions and lines.

## Symbolising by hand

1. **Subtract the load base.** The title is linked at 0 and loaded at the `xotext`
   base (0x400000 for `eboot.bin`). `rip 0x430b34` is `0x30b34` in the ELF. One
   project spent three rounds blaming the shader compiler for its own test runner's
   divide fault because it skipped this step.
2. **Use the ELF of the build that crashed** (`build/llvm-pie.elf`, before
   conversion). A map from another link gives confident, wrong names. Match the
   build identity the title printed first.
3. `llvm-addr2line -f -C -i -e build/llvm-pie.elf 0x30b34`.
4. **Cross-check:** if the instruction at that address cannot raise that signal, the
   base is wrong, or the address is a return address (one past the call).

## Backtraces are frame-pointer walks

So:
- **`-fno-omit-frame-pointer` is mandatory**, or there is no backtrace.
- **A call through NULL pushes no frame.** The walk then names the caller's call
  site, one frame above the culprit. When a backtrace names a function that plainly
  does not fault, ask what it called.

## What the signal usually means

| Seen | Usually |
| --- | --- |
| SIGSEGV, `rip` 0 or wild, fault address equal to `rip` | a call through an unfilled import: the runtime-surface class (`ps5-homebrew` skill, `runtime-surface.md`) |
| SIGSEGV with `rip: 0` and an **empty** backtrace at the end of a run | the title returned from `_start` |
| SIGSYS, `rip` in libkernel's range (`0x8xxxxxxxx`) | a system call refused to a title. Not a bug in your compiled code: route the call through the platform layer. At the end of a run, after a normal exit record, it is `exit()` |
| SIGBUS / SIGSEGV inside JIT-generated code | executable memory or a fastmem mapping not set up the platform layer's way (`ps5-porting` skill) |
| SIGFPE, SIGILL in a compiler | try the same input on the host first |
| a GPU fault record (`GPU_FAULT`, a page fault on the GPU) | the GPU read or wrote memory it should not have. Reduce to one draw or dispatch |

**When every run "crashes" at the end, suspect the exit path first.** Both wrong
endings (`exit()`, returning from `_start`) produce a crash report after a program
that worked (`ps5-homebrew` skill, `platform-contracts.md`, "Ending a title").

## A machine context in a signal handler

The console's `ucontext_t` puts the registers six 64-bit words later than FreeBSD's
header says. With the SDK fork's `sys/_ucontext.h` (and `ps5platform/context.h`),
`uc->uc_mcontext.mc_rip` is right. An emulator's fastmem handler written against an
upstream header reads garbage. Never carry an offset of your own.

## Hangs and stalls

- **No new klog lines and no progress for about 90 s** with the title still running:
  a hang. Note the last lines of both records, then close the title (low resolution
  only, never at 8K) and retry once from a clean launch before reporting.
- **A GPU hang** usually ends in a GPU fault record, or in the title stalling inside
  a fence wait. Reduce it to the smallest submission that hangs.
- **A title that is slow rather than stuck**: measure it (`test-runs.md`) before
  calling it hung.

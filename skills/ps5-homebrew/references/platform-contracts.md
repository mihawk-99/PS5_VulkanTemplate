# Platform contracts

What the console requires of a title, as opposed to what it merely allows: shapes
that fail at run time when they are wrong, not at compile time. Each one below is
in the PS5 layer every new title gets (PS5_VulkanTemplate's `ps5/src/`: `main.cpp`,
`platform.c`, `example_ps5.cpp`) or in a project it names.

## Ending a title: ask the shell, never exit

**The least guessable rule on the platform, and the most expensive to rediscover.**

| What the title does | What happens |
| --- | --- |
| `exit(status)` | SIGSYS in a libkernel stub, after the kernel has recorded a normal exit |
| returns from `_start` | SIGSEGV with `rip: 0` and an empty backtrace: the loader jumps to `_start`, so `ret` has nothing to return to |
| asks the shell to close it | the home screen comes back: the only correct ending |

```c
int sceSystemServiceLoadExec(const char *path, const char *const *argv);

void catchReturnFromMain(int status)        /* the CRT calls it after main returns */
{
   fflush(NULL);                            /* the last trace lines are in these buffers */
   (void)sceSystemServiceLoadExec("exit", NULL);
   for (;;)
      sceKernelUsleep(100000);              /* the shell closes us asynchronously */
}
```

`catchReturnFromMain` is a weak hook in PS5_Vulkan's CRT (`tooling/native/app_crt.cpp`):
defining it replaces the CRT's fallthrough to `exit`. Two consequences:
- **Everything that must be saved, flushed or released happens before `main`
  returns.** The hook is the last code that runs.
- **A program that wants to quit from deep inside** calls a `[[noreturn]]` function
  doing the same (PS5_vkQuake's `src/title_exit.cpp`), never `exit()`.

Every fatal signal makes the console write a crash report, a coredump and a
`gpudump.elf` run. A run that drew every frame and then ended wrongly reads, in
klog, exactly like one that died mid-frame: check how a title ends before believing
that it crashed.

## The splash

The shell's splash screen covers the title until the title calls
`sceSystemServiceHideSplashScreen()`. Call it at start-up, before the first present:
a title presenting perfectly behind it shows a black screen, which once cost a
series of runs to understand.

## The display

Through RADV's `VK_KHR_display` swapchain (`vulkan-on-radv.md`), at 3840x2160. The
title owns the console's one VideoOut through that swapchain; it does not open
VideoOut itself as well.

**120 Hz** needs both a request and a display that follows. The request is
`param.json`'s `attribute3` with bits `0x80040` set (`title-packaging.md`). The WSI
then measures the vblank period and falls back to 59.94 Hz when the display stays
there. A program that paces by time must therefore read the refresh it got (the
mode's `refreshRate`, or measured presents), not assume 120.

A PAL (50 Hz) program on a 119.88 or 59.94 Hz output alternates two and three
refreshes per frame. The console offers no 50 or 100 Hz mode.

## The pad

The console's pad library, with the console's own button numbering. The layout is
verified on hardware (ProsperoLight, PS5_vkQuake, PS5_RetroArch):

```c
int sceUserServiceInitialize(const void *params);       /* NULL */
int sceUserServiceGetInitialUser(int32_t *user_id);
int scePadInit(void);
int scePadOpen(int32_t user_id, int32_t port_type, int32_t index, const void *params);  /* user, 0, 0, NULL */
int scePadRead(int32_t handle, void *samples, int32_t capacity);  /* returns the samples since the last read */
```

- A sample is 120 bytes: `uint32 buttons` at 0, the stick bytes (left x/y, right
  x/y, 0..255 with 128 centred) and the triggers (0..255) at 4..9, `int32 connected`
  at 0x4c, `uint64 timestamp_us` at 0x50. Keep the static assertions on the layout.
- Buttons: L3 0x2, R3 0x4, OPTIONS 0x8, up 0x10, right 0x20, down 0x40, left 0x80,
  L1 0x400, R1 0x800, triangle 0x1000, circle 0x2000, cross 0x4000, square 0x8000,
  touch pad 0x100000. `0x80000000` means the shell has the pad (the home screen, a
  dialog): ignore that sample.
- Retry `scePadOpen` (10 tries, 100 ms apart). A title can start before the pad
  service has published the device.
- Translate the console's numbering once, at the platform boundary. Console
  constants do not leak into program code.

## Audio

`libSceAudioOut`, the way PS5_vkQuake and PS5_RetroArch drive it:

```c
int32_t sceAudioOutInit(void);                         /* 0x8026000e: already initialised, fine */
int32_t sceAudioOutOpen(int32_t user, int32_t type, int32_t index,
                        uint32_t grain, uint32_t rate, uint32_t format);
int32_t sceAudioOutOutput(int32_t port, const void *samples);  /* blocks for one grain */

port = sceAudioOutOpen(0xff, 0, 0, 256, 48000, 1);    /* system user, main port, 256 frames, 48 kHz, S16 stereo */
```

The shape that works is **a ring the program writes and a worker thread** that
blocks in `sceAudioOutOutput` one 256-frame grain at a time. Resample to 48 kHz in
the program. Treat an output that stops draining as a first-class case: a frame
stalled waiting for audio is harder to find in a log than silence.

## Floating-point state

A title starts with **MXCSR 0x9fe0**: flush-to-zero and denormals-are-zero on, where
Linux, FreeBSD and Windows start at 0x1f80. Code that needs IEEE denormals (an
emulator's FPU, a numerically careful library) sets the state it needs on each of
its threads. The platform layer's `ps5platform/fp.h` has the IEEE state. The CTS's
double-precision references broke on exactly this.

## Threads

- Through `pthread`, wrapped by the link recipe: a thread that asks for no stack size
  gets 2 MiB in direct memory, and join and detach free it.
- Thread-local storage is emulated (`-femutls`, `toolchain.md`).
- Affinity and priority go through the console's own calls. The platform layer's
  topology probe measured which cores share a physical core
  (`PS5_PayloadSDK/platform/docs/PROBE.md`, "Threads").

## Files and folders

- The title's folder is `/app0/` while it runs: `/data/homebrew/<TITLE_ID>/` on the
  console, which is also where FTP sees it.
- Everything a title creates must stay reachable over FTP: folders 0777, files at
  least 0666, set explicitly (`chmod`), whatever the umask says. The FTP server is
  another process, and a file only the title can read is a log nobody can fetch.
- USB drives and extended storage (`/mnt/...`) are hidden from a title by its
  sandbox. Keep what the title reads under `/app0` or `/data`.
- Sustained writes are throttled: about 1.3 GiB at full speed, then about 2 MiB/s.
  The platform layer's `offload.h` writes through the console's FTP server instead
  (`platform-layer.md`).

## Networking

Sockets exist, and the platform layer's FTP client uses them on 127.0.0.1. Treat
any other networking as unproven until a probe shows it works, and disable a
subsystem gracefully rather than retrying a refusal.

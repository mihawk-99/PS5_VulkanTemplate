# Files and I/O

## Where things are

| Path | What it is |
| --- | --- |
| `/app0/` | the title's folder while it runs: `/data/homebrew/<TITLE_ID>/` on the console and over FTP. Assets shipped with the title, plus anything the title writes (configs, saves, logs, caches) |
| `/data/...` | the console's data partition: shared folders a title and FTP can both reach |
| `/mnt/...` | USB drives and extended storage: **hidden by a title's sandbox**. A title cannot list them |

There is no current directory worth trusting (`getcwd` is not provided). Build paths
from `/app0/` explicitly, and give the program its base path at start-up (a config
directory, a content directory) instead of letting it derive one.

Give a port's own folders under the title folder (`config/`, `saves/`,
`savestates/`, `system/` for BIOS files the user supplies, `content/` for the user's
own games). Seed defaults only when no live configuration exists, so an update never
overwrites someone's settings.

## Permissions: everything reachable over FTP

People fetch logs, edit configs and copy saves over FTP, and the FTP server is
another process. So everything a title creates must be: folders **0777**, files at
least **0666**, set explicitly with `chmod` after creation, whatever the umask
gave. PS5_RetroArch also walks its managed folders at launch and repairs modes left
by older builds.

## The libc file functions the console lacks

The directory family (`opendir`, `readdir`, `fdopendir`...), the `*at` family
(`openat`, `fstatat`, `mkdirat`, `renameat`, `unlinkat`...), `access` (refused for
every path), `statvfs`, `posix_fallocate`, `utimensat`/`futimens`, `memfd_create`,
`open_memstream` (whose buffer appears at `fflush`/`fclose`): the platform layer
implements them, and a title linked with the recipe gets them under libc's names. A
`DIR` from the platform's `opendir` is its own: use the platform's `readdir` and
`closedir` with it (the recipe binds all of them together).

## Write speed

A title may write about **1.3 GiB at full speed**, then about **2 MiB/s**, however it
writes (`write`, `O_DIRECT`, a shared mapping) and with any number of threads. The
console's FTP server, another process, is not held to the title's budget: about
7 MiB/s a connection and 22 MiB/s in all. So:

- Keep routine writes small: saves, configs, logs.
- For big writes (a 3 GB game install, a large cache), use `ps5platform/offload.h`:
  it writes through the console's FTP server on 127.0.0.1, several files at once.
  RPCS3's installs in PS5_RetroArch went 8 times faster this way, byte for byte the
  same.
- Reads are not throttled this way.

## Logs

- A title's standard error reaches klog only through `ps5_klog_capture_stderr`.
- A log file in the title folder is the record that survives a run. Flush it on a
  timer (PS5_RetroArch flushes its trace every 250 ms): a buffered line is lost when
  the title dies.
- A log opened for append holds every run since deployment. Truncate it at start-up,
  or read from the tail.
- Users report problems with `retroarch.log`/`trace.txt`-style files. Tell them to
  copy these before relaunching, because a new launch replaces them.

## Networking

Sockets exist (the platform's FTP client uses them on 127.0.0.1). Anything beyond
that is unproven until probed: disable a port's networking gracefully, and never let
a retry loop stall a frame.

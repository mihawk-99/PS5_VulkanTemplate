---
name: ps5-console
description: Running, testing and debugging my PS5 homebrew titles on the console - deploying over FTP, launching and closing through the ps5vkctl payload, capturing klog, stopping runs cleanly, reading crashes, measuring frame rate and audio, and regression runs of every core or title. Use it for anything that touches the console itself.
---

# The console loop

One console, reached over the local network: an FTP server (port 2121), a klog
stream (port 3232), and my control payload **ps5vkctl** (port 9111), which launches
and closes titles. The address and ports live only in the ignored `.env` of
PS5_Vulkan (and of each project with its own tools). Never print them in a reply or
a document: mask addresses in any tool output you show.

## Before every run

1. **Is the console idle?** `ps5vkctl procs` must answer `count=0`. Something
   running may be me playing: never close it, never launch over it. Wait, poll every
   minute or so, and start when it is idle (or ask).
2. **One console job at a time.** Two runs restart the same title and fight over it,
   and the loser's klog reads like a crash.
3. **The build that runs is the build you think.** Deploy after building, and match
   the build identity the title prints against the build you are reading.

## A run

```bash
python3 PS5_Vulkan/tools/deploy-title-folder.py dist/<TITLE_ID>           # upload what changed, verify
python3 PS5_Vulkan/tools/run-title.py <TITLE_ID> --until '<end line regex>' \
    --elf build/llvm-pie.elf --echo '<prefix regex>' --timeout 600         # launch, capture, stop, symbolise
```

(A title made with PS5_VulkanTemplate's `new-title.py` wraps both: `ps5/tools/deploy.sh`,
`ps5/tools/run.sh`, which also fetches the last frame and gives a verdict.)

`run-title.py` checks the console is idle, arms the klog capture (dropping the
service's replayed backlog), launches through ps5vkctl, and ends **as soon as** the
end line matches, a crash record appears, or the title's process is gone. Only then,
after a grace period, does it close a title that is still running. Read
`references/console-tools.md` for every tool and the console's FTP quirks.

## How a run must end

- **Through the title's own exit.** A test run tells the title when to stop: a frame
  budget, a script's STOP, a stop file the title polls. The title then shuts down
  the way it would for a person and asks the shell to close it. Build that hook
  into every title (`references/test-runs.md`, "Test hooks").
- **Closing a title from outside (`ps5vkctl kill`) is a last-resort watchdog** for
  a hung title, and preferably at a low resolution. Never force-close a title
  rendering at 8K: two unclean power-offs followed exactly that, and one damaged
  `/data`.
- **Stop the moment the answer is in.** The end line, a crash signature (`fatal
  signal`, a GPU fault) or the title's exit ends the capture. Timeouts are only a
  watchdog for hangs. Poll with short intervals; never sit out a fixed sleep.
- **Run long jobs detached and poll them** (a background job, `nohup`). A run tied to
  the turn that started it dies with that turn.

## After a run

- Read both records: the title's own output (klog lines with its prefix, and any
  log file it writes in its folder) and the kernel's records. A trace ends at the
  last write; a fatal signal arrives after it.
- A crash: `references/crash-reading.md`.
- **Restore what the test replaced.** If a run deployed test files over an installed
  title (another title id, a test build over my real one), put a signed build back
  and prove it launches. An `eboot.bin` downloaded over FTP is a decrypted ELF, not a
  backup.
- Test files belong to one launch: a test hook file the title consumes at start, or
  one the run tool deletes afterwards. A file left behind turns my next launch by
  hand into a test run.

## Measuring

Frame rate, frame-time spikes, audio continuity and screenshots, from the title's
own records, not from watching: `references/test-runs.md`, "Measuring".

## Regression runs

After every major update - RADV or the Mesa fork, the platform layer or SDK fork,
a frontend, a title's build, any core - run **every core or title it reaches** on
the console before calling the work finished: each boots, renders correctly, holds
full speed with clean audio, and closes and reopens cleanly. Compare against the
figures its README claims, update the README in the same round, and fix or report
any regression found. An asset-only change (a background image) needs the host
checks, not a console run. `references/test-runs.md`, "Regression runs".

## Rules of evidence

- A claim is what a run showed: the command, what it returned, what it proves.
  Report failures at the same detail as successes.
- No tolerated corruption, no accuracy traded for speed without saying so.
- Raw captures (klog files, screenshots, dumps) are working files in ignored folders.
  What gets committed is small, machine-readable, and states what it is compared
  against.
- Never put game names in public documents, release notes or commit-visible
  evidence: describe the core, the system and the measurement.

## References

| File | Read it when |
| --- | --- |
| `references/console-tools.md` | using ps5vkctl, deploying, capturing klog, FTP |
| `references/crash-reading.md` | a title died, hung, or the GPU faulted |
| `references/test-runs.md` | adding test hooks, measuring, running a regression battery |

# The console tools

All of them are PS5_Vulkan's (`tools/`, `payload/ps5vkctl/`), and every other project
calls them or carries a copy. They read `PS5_HOST`, `FTP_PORT` (2121), `KLOG_PORT`
(3232), `PS5VKCTL_PORT` (9111), `PS5_FTP_USER` and `PS5_FTP_PASSWORD` from the
environment, then from PS5_Vulkan's ignored `.env`.

## ps5vkctl: the control payload

A payload that stays resident once loaded (once per console boot, with whatever
payload loader the console runs; build it with `tools/build-ps5vkctl.sh`, upload it
with `python3 tools/ps5_console.py deploy-payload`). It answers one command per
connection on port 9111:

| Command | Answer |
| --- | --- |
| `ping` | is the agent there |
| `status` | which application is running, and its title id |
| `procs` | that application's processes: `count=0` means idle |
| `users` | the user service's state |
| `launch <TITLE_ID>` | start a title (refused while another runs) |
| `kill <TITLE_ID>` | close it: suspend, SIGKILL its processes, then the shell's kill. It refuses unless the running title is the one named |
| `restart <TITLE_ID>` | close if running, then launch |

From Python:

```python
import sys; sys.path.insert(0, "PS5_Vulkan/tools"); import ps5_console
settings = ps5_console.load_settings()
print(ps5_console.ps5vkctl_command(settings, "procs", timeout=15))
```

A title cannot launch itself (the system refuses a launch while an app runs), and a
title after a GPU fault cannot be trusted to close itself. That is why the payload
exists. It is also why `kill` is a watchdog and not the way runs end
(`../SKILL.md`, "How a run must end").

A freshly deployed title id launches without any registration step (proven
2026-10-01 with a new id).

## Deploying

`python3 tools/deploy-title-folder.py dist/<TITLE_ID> [--always eboot.bin] [--all]`
uploads `dist/<TITLE_ID>/` to `/data/homebrew/<TITLE_ID>/`, skipping files whose
size matches unless forced, and reads every file back.

Projects with their own deploy (`PS5_RetroArch/tools/deploy-title.py`) also verify a
manifest of digests recorded at build time. Verify what you **built**, not what you
download: a deployed `eboot.bin` (and `libc.prx`) reads back as a plain ELF of
another length, the console's own form of an accepted title.

The console's FTP server deviates from the standard in three ways, each of which
breaks the obvious code:
- A successful `DELE` is answered **226**; `ftplib.delete()` accepts only 200/250
  and raises. Use `sendcmd("DELE ...")`.
- **Paths resolve from the root.** Use absolute paths everywhere.
- **A listing with a path argument is inconsistent.** Change directory, then list
  bare.

## Capturing klog

klog is a TCP stream, not a file: connect and read. On connect the service first
replays a backlog of older lines; the tools wait for it to go quiet and drop it, so
a capture holds only the run. Capture before the launch and through the end: the
most useful part is often what the kernel says after the title's last line.

- `python3 tools/ps5_console.py klog --output FILE`: capture by hand.
- `run-title.py` captures to `--output` (default `Klog_Logs/<title>-<time>.log`),
  prints the lines matching `--echo`, and stops on `--until`, a crash record, or the
  title's exit.

A title's standard error reaches klog only if the title captures it
(`ps5_klog_capture_stderr`, `ps5-homebrew` skill). Without that, the driver's
messages are lost.

## The title's own files

A title can write logs and results into its folder (`/app0/...`), fetched afterwards
over FTP from `/data/homebrew/<TITLE_ID>/` (`run-title.py --fetch NAME`). Two rules:

- **A log opened for append holds every run since the folder was deployed.** Take
  the run you want from the tail, or truncate at start-up.
- **Create files 0666 and folders 0777**, so FTP can read and replace them.

## Never block on a run

A long run (a CTS batch, a soak, a regression battery) goes in a detached job whose
log you poll every 20 to 60 s with one cheap command, doing other work in between.
No new klog bytes for about 90 s, with no end line, means a wedged title. Treat it as
a result: report it, then close the title (low resolution only) and retry once from a
clean launch.

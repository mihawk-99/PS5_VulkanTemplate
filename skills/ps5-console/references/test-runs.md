# Test runs: hooks, measuring, regression

## Test hooks: let the title run itself

A console run has no one at the pad. Every title needs a way to be driven, measured
and stopped without a person, and the way must never leak into an ordinary launch.

**The pattern: test files consumed by the launch.** The run tool writes a file into
the title folder just before launching. The title reads it at start-up and deletes
it, so the next launch by hand is ordinary. PS5_RetroArch goes one step further: the
run tool writes `/app0/test-run.txt`, and a launch that finds it missing deletes
every known test file left behind by an interrupted run.

Hooks worth building in, from the simplest:

| Hook | What it does | Example |
| --- | --- | --- |
| frame budget | draw N frames, then end through the normal exit, with a screenshot of the last | PS5_VulkanTemplate's `/app0/test-run.txt` (`frames 300`, `screenshot`, `samples all`), which its samples title and every title made with its `new-title.py` have |
| arguments file | extra command-line arguments, one a line | PS5_RetroArch's `/app0/args.txt` (`--max-frames=N`, content to load) |
| pad script | timed actions as if pressed on the pad, plus test actions | PS5_RetroArch's `/app0/pad-script.txt` |
| environment file | `NAME=VALUE` lines set before the driver starts | the smoke test's `/app0/radv-smoke-env.txt` |
| stop file | polled each frame: shut down cleanly | for runs with no fixed length |

PS5_RetroArch's pad script (`src/input_ps5.cpp`) is the fullest. Each line is `<seconds> <ACTION>`:
button names (`L3+R3`, `DOWN`, `B`...) press them, `MARK` measures the frames since
the last mark, `SCREENSHOT` saves a GPU screenshot, `SAVE_STATE` / `LOAD_STATE`,
`RELOAD` reloads the content, `OPTION <key> <value>` sets a core option, and `STOP`
ends the run through the frontend's own shutdown.

**Lines for a recording.** PS5_VulkanTemplate's `test-run.txt` also takes what a
recording of the title needs, still deterministic (`ps5/src/ps5_samples.h`):
`press <program> <frame> <left|right|up|down|cross|l1|r1>` presses for the UI kit's
screens (`start`, `launcher`, `themes`, `uikit`) at a frame of the run, `orbit 20`
turns every sample's camera 20 degrees a second (`orbit deferred 8` one program's;
a first-person camera moves round the scene's origin as well, so the scene stays in
view), `overlay off` hides the samples' settings windows (imgui keeps its own), and
`beat 0.592` gives programs that move with music the beat in seconds. References
are made without them.

### Recording through a capture card

What a 1080p60 USB capture card ("USB3 Video", a V4L2 device: raw YUYV 4:2:2 at
1920x1080, 60 fps, the best mode it offers) showed, measured on my console:

- **The title must present at 59.94 Hz.** With `param.json`'s high-frame-rate bits
  the title flips at 119.88 Hz while the link to a 60 Hz card runs at 60: every other
  frame never reaches the card, so a 1/60 s test step plays back at double speed.
  Record with those bits left out (`attribute3` 0, as `new-title.py --refresh 60`
  writes), and put them back after.
- **The card repeats frames.** It delivered about 62 frames a second from the
  console's 59.94, one exact copy every 30 or so (0 pixels changed; a real frame of a
  moving scene changes 40,000 or more). Drop them by exact equality where the picture
  moves; where it is still, map frames by their capture time at 59.94 Hz from a frame
  whose index is known (a scripted press lands on its frame within half a frame).
- **No dropped frames** with `ffmpeg -fps_mode passthrough -enc_time_base demux` and a
  fast lossless codec (Ut Video, about 375 Mbit/s); x264 lossless and the default
  frame-rate handling dropped a few. Keep the PC otherwise idle: a busy CPU starved
  the card's ALSA audio until ffmpeg gave up mid-run.
- **Colours**: the card sends full-range YUV with the BT.709 matrix, tagged as
  limited; converting it as full-range BT.709 matched the PC driver's frames to a mean
  of about 2 levels in 255.
- **The first program of a launch** may lose its first second under the launch's
  transition: put the program a recording needs whole second.

## Measuring

Measure from the title's own records, at the moment they happen, and print them to
klog:

- **Frame rate and spikes**: frames presented per window, and the longest gap
  between two frames. A frame-time over 40 ms is visible as a stutter at any rate:
  count them (PS5_RetroArch's `MARK` reports `fps`, `worst_ms`, `over_40ms`). Report
  windows after boot separately from the boot itself.
- **Audio continuity**: per window, how much the output played against silence it
  had to insert (PS5_RetroArch's `audio ps5: window played= silence=`). Under about
  98% after boot is audible crackle.
- **Pictures**: a GPU screenshot of the swapchain image, fetched over FTP and looked
  at. A run that reports frames has not necessarily shown the right ones. GPU
  screenshots do not capture overlays drawn outside the swapchain (a menu on another
  path). Better still, compare it with the same frame drawn by another driver:
  PS5_VulkanTemplate builds the title's own code for the PC with a headless surface
  (`ps5/tools/host-reference.sh`) and fails a sample whose picture is off by more
  than 12 levels in 255. For that, a test run must be deterministic: a fixed time
  step a frame, fixed random seeds, the pad ignored, and the same C++ library on
  both sides (libc++ 18, as the console's SDK: libstdc++'s random distributions
  give other numbers from the same seed).
- **GPU and submission cost**: the driver's own per-window lines where it has them
  (RADV's PS5 winsys prints where submission time goes every 10 s). Use them to
  compare settings (upscaling factor, threads), not as absolute figures.

**The first run after a driver change compiles every pipeline again** (RADV's cache
is keyed by the driver build). A slow start then is not a regression: warm the cache
with one run, and measure the next.

## Regression runs

After every major update (RADV or the Mesa fork, the platform layer or SDK fork, a
frontend, a title's build, any core), run every title and core it reaches, on the
console, before calling the work done. Asset-only changes are the exception.

For each core or title, with the game or content its battery names:
1. boot, then measure 10 to 25 s after boot;
2. open and close the menu (twice where it has one);
3. Close Content / quit to its menu, load the same content again, measure again;
4. a screenshot each time; audio windows throughout;
5. no `fatal signal`, no GPU fault, no RADV error lines.

Compare every figure with what the README and the last release notes claim. A
difference either gets fixed or gets reported, in the README and the notes of the
same round, never left for users to find. Run the battery as one detached job that
waits for the console to be idle and launches each case in turn, then read the
summary.

Write results without game titles in anything public: core, system, settings and
the numbers.

## Without klog

klog (klogsrv on port 3232) is a payload the console's environment loads, and it is
sometimes not there after a reboot. A title that also writes its test lines to a file
in its folder, flushed line by line, can still be run and judged: launch it through
ps5vkctl, poll `procs` until it has exited, fetch the file over FTP
(PS5_VulkanTemplate's `ps5/tools/run-without-klog.py`). Say in the report that crash
records and driver messages went unchecked.

## Long runs

Soaks (ten minutes or more), CTS batches and batteries run detached, logging to a
file, with progress judged from klog records. Poll them, and work on something else
meanwhile. Never launch a second console job while one runs.

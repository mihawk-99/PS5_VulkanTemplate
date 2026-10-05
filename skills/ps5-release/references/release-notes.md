# Release notes

PS5_RetroArch's notes are the model (`docs/releases/<tag>.md`, kept locally and
published as the release body). Keep the local file and the published body
identical.

## Skeleton

```markdown
# <Project> <tag>

<One line: what the program is.>

![Important warning](<raw URL of assets/readme/important-warning.png on main>)

## <Project> is a passion project, not a piracy project.

It exists so you can play the games you own on the console you own.

- **Piracy is not condoned.**
- **No games, BIOS files, console firmware or decryption keys are included, and they never will be.**
- **Use only legally obtained backups of games you own**, made yourself from your own discs, cartridges or digital purchases.
- **Use only BIOS and firmware files dumped from hardware you own.**
- **Requests for, or links to, games, BIOS files, firmware or keys are not welcome** in this project's issues or discussions.

---

> [!WARNING]                      (only where it applies)
> **PlayStation 3 (RPCS3) is not in this release, and it will not be in any future
> release.** ... compile it yourself from source ... don't share the binaries.

## What's new        (or "Highlights")
- **<Change in the player's words.>** What they saw, why, what changed, the number
  that proves it.

## Tested for this release
<What ran on the console, with the build that is in the ZIP: a table by core or
system with frame rate after boot and after reload, audio, crashes. No game titles.
What was not tested, and why (no BIOS on my console, not re-run for an asset-only
change).>

## Known issues
<Everything known not to work, carried over items marked as not re-checked.>

## Install or update
1. Copy the complete `<TITLE_ID>` folder to your homebrew title location ...
2. What updating keeps (configs, saves, content).
3. Your own legally obtained backups in `content/`, BIOS dumped from your own
   hardware in `system/`. None included, none ever will be.
4. Read `LEGAL.txt`.

## Reporting problems
<What to include; copy the logs before relaunching; never attach game files,
BIOS, firmware or keys.>

## Source
This ZIP was built from <Project> commit `<full sha>`, which is this release's tag.
The source of every part is attached ... no RPCS3 file ... non-commercial parts may
not be sold.

## Build
Build identity `<identity>`
| Component | Version |      (every pin: frontend, RADV and Mesa revision, SDK fork, each core or library)

SHA-256 of `<zip>`: `<sha256>`
```

## Rules for the words

- **No game titles**, anywhere in the notes, the README or the release title. Write
  "the tested game", "a GameCube game I test with", "a 30 fps game". The same goes
  for title ids of games and for file names in evidence that is published.
- **Measured, not hoped.** Every performance or fix claim has the run behind it. A
  feature not proven on the console is not claimed. If the code exists but the run
  showed it does not work, say so, and say what works instead.
- **One line per paragraph and per list item.** The release page turns every line
  break in the body into a `<br>`, so a wrapped paragraph shows ragged there.
- **Players' words.** Describe what someone saw ("frame drops with crackling audio")
  before the cause.
- **First person**: "I tested", "my console". Never "the owner", "the user", or "the
  maintainer".
- **Carry over honestly.** Known issues not re-checked in this release are marked as
  carried over.
- **Editing published notes** is fine for corrections (for example removing game
  titles). The ZIP, its tag and its sha256 never change.

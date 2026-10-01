---
name: ps5-release
description: Publishing a release of one of my PS5 homebrew titles (PS5_RetroArch or a new title) - the legal checks, licence notices and source archives, what must never be in a package, the build order, the checks before tagging, the GitHub release itself, reading it back, and the release notes. Use it whenever a ZIP, a tag or a release page is about to leave the machine.
---

# Releasing a PS5 title

A release is a ZIP of the title folder plus the complete source of everything in
it, on a GitHub release whose tag is the exact commit the ZIP was built from. It is
published, so **legal safety comes before convenience**: when a choice carries legal
uncertainty, take the option with no legal exposure, and say what it rules out.

## Never in a package, a repository or an upload

- games, ROMs, ISOs, game backups or game files of any kind; saves;
- BIOS files, console firmware (`PS3UPDAT.PUP`, PS2 BIOS dumps), anything from the
  PS5 firmware dump, decryption keys, licence files (`.rap`), IRDs;
- **RPCS3 binaries**: RPCS3 is GPL-2.0-only and cannot be distributed with my
  GPL-3.0-or-later code. It is source-only, built by each person for their own
  console. PS5_RetroArch's release build leaves it out by itself, and the release
  check fails on any RPCS3 file;
- art, sound or text of unrecorded origin: replace it with something generated or
  owned (PS5_RetroArch's launcher art is drawn by `tools/make-title-art.py`);
- the console's address, `.env`, klog captures, screenshots of games.

## The order

PS5_RetroArch's procedure (its local `docs/RELEASING.md`), which any title follows:

1. **Everything linked in is committed and on GitHub**: the title, every pinned
   fork, the SDK fork pin, PS5_Vulkan and the PS5_Mesa revision its RADV archive
   records. Check each pin resolves (`gh api repos/mihawk-99/<repo>/commits/<sha>`).
   The trees must be clean: the notices record `dirty`, and the release check
   refuses it. Stash an unrelated local change in a sibling for the build, and
   restore it afterwards.
2. **Gates, then the release build, then the release check, in that order**:
   ```bash
   bash tools/verify.sh                                     # its build gate makes a development build
   PS5_RELEASE_TAG=<tag> bash tools/build-title.sh          # the release build, last
   python3 tools/check-notices.py --release                 # every part's licence and published source; no RPCS3 file
   ```
3. **Regression run on the console** of every core or title the release changes,
   unless the change is literally an asset (a background image): `ps5-console`
   skill, "Regression runs". The notes report what it measured.
4. **The source archives and the ZIP**:
   ```bash
   python3 tools/source-bundle.py dist/<TITLE_ID> dist/source-<tag>
   ```
   Pack `dist/<TITLE_ID>/` into `PS5_<Project>-<tag>.zip` the same way every time:
   directory entries included, deflate, Unix modes kept, every timestamp set to the
   release date (the host has no `zip`: Python's `zipfile`). Test it
   (`testzip()`), unpack a copy, run the release check on it, and record its sha256.
5. **Tag the built commit** (annotated), push the tag, then create the release with
   `--verify-tag --prerelease`, the ZIP and every source archive with `SHA256SUMS` and
   `SOURCES.txt`, and the notes as `--notes-file`.
6. **Read it back**: download the ZIP from the release, compare its sha256, unpack
   it, run the release check on the download, and check that the release body names
   no game.

Never change a published ZIP in place: its notes name its commit and its sha256.
Publish a new version instead (an asset-only change is a small version, built the
same way).

## Release notes

`references/release-notes.md` has the skeleton. The fixed parts:
- a piracy notice at the top: no games, BIOS, firmware or keys, now or ever; legally
  obtained backups of games you own only;
- the RPCS3 notice: not in any release, compile it yourself, don't share the binaries;
- **no game titles anywhere**: say what was measured on which core or system ("a
  GameCube game I test with slows to 76-85% at some transitions");
- honest testing: what ran on the console, with numbers, and what did not run;
- the source commit (the tag), the build identity, the component pins, and the
  ZIP's sha256;
- first person ("I"), and the trademark line.

## Licensing

`references/licensing.md`: which licences can ship together, the non-commercial
cores, what the notices in the title folder must carry, and how to add a new part.

## Publishing rules

- Pushing my repositories' finished work is fine. A public **release** happens when I
  ask for it.
- Keep released artifacts in `dist/` until the read-back passes. Don't delete or
  rename my own copies there.
- After publishing, update the README's "latest release" line and its tested-status
  claims to what the release measured.

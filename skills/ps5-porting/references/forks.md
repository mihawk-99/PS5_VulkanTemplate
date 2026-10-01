# Forks

Every third-party project I change lives as a fork on GitHub (`mihawk-99/PS5_<Name>`),
never as a patch series applied at setup time. Existing forks: PS5_PayloadSDK,
PS5_Mesa, PS5_LLVM, PS5_LRPS2, PS5_BeetlePSX, PS5_Mupen64Plus, PS5_BeetleSaturn,
PS5_VICE, PS5_MAME, PS5_DeSmuME, PS5_Azahar, PS5_Dynarmic, PS5_RPCS3,
PS5_ProsperoEden.

## Making one

1. Check the licence (`ps5-release` skill, `licensing.md`) and note what it allows.
2. `gh repo fork <upstream> --fork-name PS5_<Name> --clone=false --default-branch-only`.
   Keep it a real GitHub fork, so it stays connected to upstream.
3. Clone it beside the other projects (`../PS5_<Name>`), add
   `upstream` as a remote with its push URL disabled
   (`git remote set-url --push upstream DISABLED`).
4. Make `main` the branch, even when upstream calls its branch `master` or
   `develop`. Set it as the fork's default, and delete the copied upstream branch.
5. Add a short "About this fork" section at the top of its README: what the fork is
   for, that upstream is the project and gets the credit, and how I keep it in step.
   Leave upstream's own README text below it unchanged, so merges stay clean.

## Working in it

- **All work on `main`.** No long-lived side branches. A pull request to upstream
  goes from a short-lived branch cut from `upstream/<default>`, deleted once merged.
  Open a pull request to someone else's repository only when the person you work
  for asks for it.
- **Upstream is merged in, never rebased onto.** `main` is published: never rewrite
  it or force-push.
- **PS5 changes are marked:** behind `__PROSPERO__`, or in files of their own, with
  comments that say what the console needed and how it was measured.
- **Gaps that are the console's go into the platform layer**, not into the fork.
  The fork calls `ps5platform/*` instead of carrying its own copy.

## Pinning it

The consumer's build script pins the fork by full revision. PS5_RetroArch's cores
all do it through `tools/core-fork.sh`:

```bash
revision=4598458e115f108a6e2211eb0763eb22ab383d4c  # ../PS5_Azahar main   (tools/build-azahar.sh)
core_fork_checkout PS5_Azahar "$revision" submodules
```

`core_fork_checkout` clones from the sibling checkout `../PS5_<Name>` when there is
one, else from `github.com/mihawk-99/PS5_<Name>`, checks out the pin into
`.deps/<core>-src`, cleans it, and refuses a tree at any other revision. A fork's own
submodule forks are named by relative URL (`../PS5_Dynarmic`), so they resolve the
same way. Copy that shape for any consumer that builds a fork.

A pinned revision must be pushed before a release names it: a release's source
archives are built from the pins, and anyone building from source fetches them from
GitHub. Before a release, check every pin resolves on GitHub
(`gh api repos/mihawk-99/<fork>/commits/<sha>`).

## Keeping up with upstream

- Merge upstream regularly, test, push, then bump the pin in the consumer and run
  the consumer's regression battery (`ps5-console` skill).
- Where the fork is a libretro or older derivative of a living project (LRPS2 from
  PCSX2), port the living project's renderer fixes and features into it, so it
  resembles upstream instead of falling further behind.
- A fork may depart from upstream's design where the console needs a different one
  (RPCS3's fork redesigns whole areas for the PS5's hardware). Then write down why,
  in the fork's README section.

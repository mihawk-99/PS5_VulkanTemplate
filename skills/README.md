# PS5 agent skills

Agent skills for building PlayStation 5 homebrew on this stack: native titles that
render through RADV (Mesa's Vulkan driver, ported to the PS5 by PS5_Mesa and
PS5_Vulkan) on the PS5_PayloadSDK fork's platform layer. They hold what is true of
every title and expensive to rediscover, and point at the projects that own
everything else. They live in PS5_VulkanTemplate, beside the foundation they describe.

| Skill | Use it to |
| --- | --- |
| [`ps5-homebrew/`](ps5-homebrew/SKILL.md) | start and build a title: the stack, new titles on this repository's foundation, Vulkan on RADV, the platform layer, packaging, the toolchain, driver work |
| [`ps5-console/`](ps5-console/SKILL.md) | run, test and debug on the console: ps5vkctl, deploying, klog, clean stops, crash reading, measuring, regression runs |
| [`ps5-porting/`](ps5-porting/SKILL.md) | bring existing software over: forks and pins, memory and JIT, files, loading code without `dlopen` |
| [`ps5-release/`](ps5-release/SKILL.md) | publish a release: what never ships, licences and notices, the build order, the release notes |

## Installing

Each skill is a folder with a `SKILL.md`. Link the folders into your agent's skill
folder, so they stay in this repository and follow its updates. From the repository's
folder:

```bash
mkdir -p ~/.claude/skills
for skill in ps5-homebrew ps5-console ps5-porting ps5-release; do
    ln -sfn "$PWD/skills/$skill" ~/.claude/skills/$skill
done
```

## A new title in three commands

```bash
python3 ps5/tools/new-title.py ../PS5_MyTitle --title-id PPSA99121 --name "My Title"
cd ../PS5_MyTitle
ps5/tools/build.sh && ps5/tools/deploy.sh && ps5/tools/run.sh
```

The title gets this repository's foundation: Sascha Willems' Vulkan example base
class with its PS5 hooks, the PS5 layer (the launch, the pad, klog, test runs, the
build, the console tools), and the starter sample as its program, a textured, lit glTF
model with a camera on the sticks and a settings window on the pad. The other fifteen
samples are classes on the same base, so their techniques copy into a title as they
stand. On my console (2026-10-01) a title made this way ran its test run at 119.9 fps
on a 119.88 Hz display, matched its picture from the PC's driver within 0.6 levels in
255, and ended through the shell. `ps5/tools/bootstrap.sh` sets up what it builds
against first (PS5_Vulkan, PS5_Mesa and PS5_PayloadSDK beside this repository).

## Keeping them true

- A skill states what stays true across driver rounds and titles. Numbers that
  change (CTS counts, frame rates, open gaps) stay in their owners' documents, and
  the skills point at them.
- Anything a skill tells an agent to do must have been done on the console.

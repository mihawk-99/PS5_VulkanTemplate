# The UI kit on Vulkan

A title's interface, as the console's own software would draw it: BlackBearReloaded's
ps5-homebrew-ui kit (an instanced signed-distance-field renderer, springs, a mixer
with recorded cues, thirty themes, about a hundred components and twenty-one complete
designs), drawn through RADV by my fork PS5_VKHomebrewUI's `gfx::VkRenderer`.
PS5_VulkanTemplate carries it as an opt-in module, `ps5/ui/` (its README is the
manual); a title made with `new-title.py --ui <design>` has it, one made without has
none of it.

## Licence first

The kit is GPL-3.0-or-later. In the template that is a module, not the template:
`ps5/ui/`, the programs that draw with it (`examples/uikit`, `uioverlay`, `uiscreen`,
and a `--ui` title's program and `kit/screen.cpp`) and the kit's sounds carry it; its
fonts are OFL-1.1 and Bitstream Vera. Never move kit code into the MIT starter or the
MIT foundation files, and keep every kit file's SPDX header. A built title is
GPL-3.0 as a whole anyway (the platform layer), but a title without `--ui` keeps an
MIT source tree.

## Where things are

| What | Where |
| --- | --- |
| the kit's source, at the pinned revision | `.deps/hui/src/` (exported by `ps5/ui/setup-kit.sh`; the pin is in that file) |
| the kit's guides | PS5_VKHomebrewUI's `docs/`: CRAFT.md (the quality bar), COMPONENTS.md and COMPONENT_INDEX.md, THEMES.md, KIT.md, VULKAN.md (the backend) |
| the glue on the base class | `ps5/ui/kit.hpp`: `ps5ui::Kit`, `ps5ui::KitExample`, `ps5ui::make_screen` |
| the platform pieces it uses | `ps5/src/platform.h`: `pad_readings`, `pad_vibrate`, `pad_light_bar`, `audio_start` (MIT, for any title) |
| worked programs | `examples/uikit` (the gallery, every design), `examples/uioverlay` (HUD and pause menu over a 3D scene, dynamic rendering), `examples/uiscreen` (one design full screen: what `--ui` titles grow from) |

## The shape of a program

Derive from `ps5ui::KitExample`, set `settings.overlay = false` and
`ps5.ownOverlay = true` (the kit draws the screen and owns the pad), call
`prepareKit()` after `VulkanExampleBase::prepare()`. Each frame: `prepareFrame()`;
read `kit.input()`; update your screen or components with `dt` (clamp it, 0.05 s);
`kit.play(feedback, soundSet)`, `kit.tick(dt)`; queue the layers on `kit.renderer`
(`begin`, `backdrop`, `draw(list)`, `glass`); `buildKitCommandBuffer()`;
`submitFrame()`.

- **Layers** go back to front: backdrop, scene, `glass()`, overlay, post. Lists queued
  after a `glass()` sample its blurred copy through `kit.renderer.glass_texture()`
  (a component's `canvas.glass`); with none, frosted styles fall back to solid panels.
- **A 3D scene under the UI**: render it off-screen in the `offscreen` hook of
  `buildKitCommandBuffer`, transition it to `SHADER_READ_ONLY_OPTIMAL`, hand its view
  to `kit.renderer.import_texture` once, and draw it as the first list's image
  (`gfx::kFullUv`). The glass copy then blurs the scene itself (`uioverlay`).
- **Under a scene drawn in the main pass** instead, pass a `scene` lambda; the UI
  draws over it, but the glass cannot see it.
- **Textures you make**: `create_texture` (top row first, `kFullUv`),
  `render_to_texture` (bottom row first, `kCanvasUv`, as every target the renderer
  draws into). Uploads wait for the queue: at start-up or between frames.

## Pad, sound and leaving

- `kit.input()` folds every pad reading of the frame (a tap shorter than a frame
  counts) into the kit's `InputFrame`. In a test run the pad is ignored: give the
  screen an idle frame (`connected = true`) or a scripted one.
- OPTIONS is the designs' (pause menus, drawers). The programs leave when OPTIONS is
  **held** for a second: back to the menu, or out of a title of one program
  (`quit = true`, never `exit`). `ps5ui::HoldToLeave` does it and draws the hold as
  a plate with a filling ring; draw it last, so people can see the way out.
- **Hints** are glyphs, not words: `ps5ui::draw_hints` draws the kit's DualSense
  buttons with their labels in a theme's colours.
- **The title's theme** (`ps5ui::active_theme()`, picked in the samples title's
  Themes) also styles the samples' ImGui windows through `ps5StyleOverlay`; a test
  run always has the default, `tiles`.
- Sound starts with `prepareKit()` (mixer, cues, music on `audio_start`'s thread);
  `{ .music = false }` leaves the songs out. The host reference build has no audio
  output and says so.

## Tests

- A test run must draw the same frame every time: `benchmark.active` seeds the music
  shuffle with 0, the gallery takes a fresh settings folder, telemetry shows
  stand-in numbers, and time steps 1/60 s.
- The gallery's variants (`uikit.aurora` ... `uikit.toolbox`, `samples.cpp`'s
  `VARIANTS`) play the kit's own tour of one design; `all` runs them.
- References: `ps5/tools/host-reference.sh --save <ids>`, after looking at the pictures.

## Changing the kit

A fix to the kit or its backend goes into PS5_VKHomebrewUI (commit, check both
backends on the PC with `tools/host-snapshots.sh`, `tools/host-snapshots-vk.sh` and
`tools/compare-backends.py`, push), then a new pin in `ps5/ui/setup-kit.sh`, then the
UI samples on the console. Never patch `.deps/hui`: it is re-exported from the pin.

## Pitfalls already paid for

- `VkRenderer::draw(cmd, w, h)` hid `Renderer::draw(list)` until a using-declaration
  brought it back (4451c26): call `kit.renderer.draw(list)` freely.
- A design file copied into a program must stay outside the program's namespace:
  `new-title.py --ui` puts it in `kit/`, which `ps5/ui/ui.cmake` compiles unwrapped,
  and renames its namespace (`hui::screen`) so it cannot meet the kit's copy.
- A sample with no shaders of its own (the kit carries its SPIR-V) needs nothing in
  `shaders/glsl/`; `link-title.sh` skips it.

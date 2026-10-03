# The UI module

Console-grade interfaces for a title, drawn with Vulkan: the kit of
BlackBearReloaded's [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui)
(an instanced signed-distance-field renderer, springs, a 32-voice mixer with two
sets of recorded cues, thirty themes, a component library and twenty-one complete
designs), through my fork
[PS5_VKHomebrewUI](https://github.com/mihawk-99/PS5_VKHomebrewUI), which adds the
Vulkan backend (`gfx::VkRenderer`) beside upstream's OpenGL one. The designs, the
themes, the components and the sounds are BlackBearReloaded's work.

## Licence: GPL-3.0-or-later, and only where it is used

The kit is GPL-3.0-or-later, so this module is too: everything in `ps5/ui/`, the
programs that draw with it (`examples/uikit/`), and the kit's assets
(`ps5/assets.json` lists each with its licence: OFL-1.1 and Bitstream Vera for the
fonts, GPL-3.0-or-later for the sounds and music). The rest of the template stays
MIT. A title made without the module (`new-title.py` without `--ui`) has none of
these files and builds none of the kit, so its source stays MIT; a built title is
GPL-3.0-or-later as a whole either way, since every title links the platform
layer (`skills/ps5-release/references/licensing.md`).

## What is here

| File | What it does |
| --- | --- |
| `setup-kit.sh` | exports the pinned PS5_VKHomebrewUI revision into `.deps/hui` (from `../PS5_VKHomebrewUI`, or GitHub) |
| `ui.cmake` | included by `ps5/CMakeLists.txt` when this folder exists: compiles the kit (not its OpenGL backend or its own console layer) into the title, defines `PS5_UI`, and tells `build-assets.py` to lay the kit's assets into `/app0/assets/hui/` |
| `kit.hpp`, `kit.cpp` | `ps5ui::Kit` (the renderer, the six fonts, the mixer on the console's audio output, the cues, the music, the pad as the kit's input frames, the sample covers) and `ps5ui::KitExample`, the base class a program that draws with the kit derives from |
| `hui_platform.cpp` | the kit's five system calls (`hui::sys::log`, the clock...) on the template's `platform.h` |

The template's platform layer gives the module what the kit's own console layer
gave it: every pad reading of the frame (`pad_readings`, so a tap shorter than a
frame counts), rumble (`pad_vibrate`), the light bar (`pad_light_bar`) and a 48 kHz
output thread (`audio_start`). They are MIT and any title may use them.

## A program that draws with the kit

```cpp
class VulkanExample : public ps5ui::KitExample
{
	std::unique_ptr<hui::app::Shell> shell; // or a design, or your own draw lists
public:
	VulkanExample() { settings.overlay = false; ps5.ownOverlay = true; }
	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit(); // fonts, covers, sound
		shell = std::make_unique<hui::app::Shell>(kit.fonts, kit.catalog, PS5_APP_ROOT "/hui",
			kit.renderer.glass_texture());
		prepared = true;
	}
	void render() override
	{
		prepareFrame();
		shell->update(kit.input(), std::min(frameTimer, 0.05f));
		kit.play(shell->feedback(), shell->sound_set());
		kit.tick(frameTimer);
		shell->compose(kit.renderer);  // queue the layers
		buildKitCommandBuffer();       // off-screen work, then the pass; a lambda adds a 3D scene under the UI
		submitFrame();
	}
};
```

`examples/uikit/uikit.cpp` is the whole gallery in about 150 lines. The kit's own
guides say how to build a screen from its components and themes:
`.deps/hui/` has the source, and the fork's `docs/` (COMPONENTS.md, THEMES.md,
KIT.md, CRAFT.md) the rest; `docs/VULKAN.md` there describes the backend.

## Moving the pin

1. In PS5_VKHomebrewUI: merge upstream, then check both backends on the PC
   (`tools/host-snapshots.sh`, `tools/host-snapshots-vk.sh`,
   `tools/compare-backends.py`), commit and push.
2. Here: the new revision in `setup-kit.sh`, `ps5/tools/build.sh`, then
   `ps5/tools/host-reference.sh uikit $(python3 ps5/tools/check-run.py --resolve all | grep '^uikit\.')`
   and the same on the console with `ps5/tools/run.sh`.
3. New pictures on purpose get new references (`host-reference.sh --save`), named in
   the commit.

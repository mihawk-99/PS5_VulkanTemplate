# Vulkan on the console: RADV, linked

## What a title gets

RADV on the PS5 is upstream RADV with a console winsys, so a title writes ordinary
Vulkan. It reports **Vulkan 1.4** (device name "PlayStation 5 GPU (RADV NAVI21)"),
with RADV's usual feature and extension set, minus what the hardware or the
console lacks. Query features and extensions at run time as on any desktop GPU;
`PS5_Vulkan/docs/CTS_GAPS.md` is the live record of what is absent and why.

What differs from a desktop, and stays true across driver rounds:

| | On the console |
| --- | --- |
| Loader | none. RADV is linked into the title; every command comes from `vk_icdGetInstanceProcAddr` |
| Surface | `VK_KHR_surface` + `VK_KHR_display`: one display, a plane surface; no window system |
| Extent | 3840x2160, fixed |
| Refresh | the display's modes; 119.88 Hz only when `param.json` sets the high-frame-rate bits, else 59.94 Hz |
| Present | FIFO, 3 to 5 swapchain images, paced by the display's flips |
| Queues | graphics, compute and transfer families, all served from the graphics ring: no asynchronous compute (yet) |
| Conformance | `conformanceVersion` is 0.0.0.0: the CTS runs on the console, and no conformance is claimed |
| Shader cache | on by default, in the title folder: `/app0/radv-shader-cache/` (Mesa's database form) |

Features the hardware lacks are reported absent rather than emulated, among them
fragment shading rate and fragment shader barycentrics. Check before relying on one.

## Getting the commands

```c
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance, const char *);

vkCreateInstance = (PFN_vkCreateInstance)vk_icdGetInstanceProcAddr(NULL, "vkCreateInstance");
/* after the instance: instance commands from vk_icdGetInstanceProcAddr(instance, ...)
 * after the device:   device commands from vkGetDeviceProcAddr(device, ...) */
```

A program written against a loader (`vkGetInstanceProcAddr`, volk, SDL's
`SDL_Vulkan_GetVkGetInstanceProcAddr`) needs one function that forwards to the ICD
entry point. PS5_vkQuake's `src/radv_icd_ps5.c` is the whole of it:

```c
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance i, const char *n)
{ return vk_icdGetInstanceProcAddr(i, n); }
```

Titles made on the PS5_VulkanTemplate foundation use volk without that forwarder:
every unit includes volk first (`-include volk.h`, with `VK_NO_PROTOTYPES`), the
launch calls `volkInitializeCustom(vk_icdGetInstanceProcAddr)` (`ps5/src/main.cpp`),
and the base class calls `volkLoadInstance` and `volkLoadDevice` once each exists, so
the rest of the code reads as ordinary Vulkan.

Headers: PS5_Mesa's `include/vulkan/` matches the driver's version. The PS5_VulkanTemplate
foundation builds with the headers upstream bundles (`external/vulkan/`, a
release or so newer); what the driver reports is what counts, not the header.

## The display

1. Enable `VK_KHR_surface` and `VK_KHR_display` on the instance.
2. `vkGetPhysicalDeviceDisplayPropertiesKHR`: one display, 3840x2160.
3. `vkGetDisplayModePropertiesKHR`: take the fastest mode (`refreshRate` is in
   millihertz). With the high-frame-rate bits it is 119.88 Hz. The WSI then measures
   the vblank period, and **falls back to 59.94 Hz by itself** when the display stays
   at 60, so a 60 Hz TV gets correct pacing without the title knowing.
4. `vkCreateDisplayPlaneSurfaceKHR` with the mode's `visibleRegion` as the extent,
   identity transform, opaque alpha.
5. Pick a queue family with graphics and `vkGetPhysicalDeviceSurfaceSupportKHR`.
6. Swapchain: `minImageCount` 3, `B8G8R8A8_UNORM` (or what the surface reports),
   `COLOR_ATTACHMENT` usage (add `TRANSFER_SRC` to read frames back), FIFO.

The title owns the console's one display through this swapchain. It must not open
VideoOut itself as well (that is the CPU-written path PS5_RetroArch and PS5_vkQuake
keep in `display.cpp` for bring-up only).

## Linking

Use `PS5_Vulkan/tools/radv-link.sh` exactly: `source` it, call
`radv_link_recipe "$vulkan" "$sdk" "$archive"`, then link with
`radv_linker_script`, `radv_link_flags` and `radv_link_inputs`, as a title's
`ps5/tools/link-title.sh` does. What it takes care of, and why each was found by a failure:

- **`--whole-archive`.** Mesa's dispatch tables name entry points through weak
  references, and a weak reference pulls no archive member in. Without it, an entry
  point is NULL (the first console run called one inside `wsi_device_init`).
- **The heap wrap** (`--wrap=malloc` and its family): every allocation goes to the
  platform layer's heap in direct memory. libc's own heap ran out under the CTS's
  first shader build.
- **The thread wrap** (`pthread_create/join/detach`): threads that ask for no stack
  get 2 MiB in direct memory.
- **`--defsym name=ps5_name`** for the libc functions the console lacks, refuses or
  faults in, kept local by a version script. A title that defined those names itself
  would export them, and the packaging tool refuses exports.
- **libc++, libc++abi, libunwind** from the SDK (ACO is C++) and clang's builtins
  (`__emutls_get_address`).

RADV calls AGC, for which the SDK has no link stubs: build PS5_Vulkan's two stub
sources (`vendor/ps5/sdk/stubs/agc_*canary_link_stub.c`) into `libSceAgc.so` and
`libSceAgcDriver.so` and pass them to the link and to the native tool's `--stub`.

**Release or debug archive.** Titles link the release archive (assertions off). The
debug build (`tools/build-radv.sh` without `release`) compiles shaders 5 to 6 times
slower, because of Mesa's assertions and NIR validation. Use it to chase a driver
bug, never to measure.

## Environment

The driver reads Mesa's and RADV's usual variables (`RADV_DEBUG`, `ACO_DEBUG`,
`MESA_SHADER_CACHE_DIR`...). Set them with `setenv` in `main` before the instance is
made: a title has no shell to inherit them from. A test-only switch belongs in a
test file the launch consumes (the `ps5-console` skill, "Test hooks"), never in a
default.

PS5-specific switches:
- `RADV_THREADED_RECORDING=1`: a worker thread records the application's `vkCmd*`
  calls. It passes the same CTS groups as without it, and stays off by default until
  it measures faster in a real program.
- `RADV_PS5_*`: probe-only switches for features still being brought up. They are
  not for titles.

## Pipelines and shader compiles

ACO compiles on the CPU, and a cold compile of a big pipeline costs frames. A title
must hold full speed while shaders compile:
- compile on background threads (the heap has an arena per thread, so threads
  compiling at once do not wait on each other);
- prefer pipeline libraries, or an ubershader path, for anything that appears
  mid-game;
- let RADV's on-disk cache carry the second launch, but never depend on a warmed
  cache for the first.

## When RADV is the problem

Capture what it said: its messages reach klog through the stderr capture. Reduce
the case to the smallest program. If it is a driver bug, the fix goes into PS5_Mesa
for every application (`driver-work.md`). A title-side workaround is acceptable only
when the title chooses different but valid Vulkan usage, and the code says so.

# Upstream contributions

One entry per problem: where it belongs upstream, what was checked, and what was
done (`skills/ps5-porting/references/upstream-contributions.md`). Newest entries
go last.

## 2026-10-10: the host reference on a driver without headless surfaces

- **Upstream:** SaschaWillems/Vulkan, branch `master` (checked at 4ee2501c on
  2026-10-10).
- **Problem:** the host reference build (`PS5_HOST_REFERENCE`, which defines
  `VK_USE_PLATFORM_HEADLESS_EXT`) asked for `VK_EXT_headless_surface`. NVIDIA's
  driver (615, RTX 3070) does not offer it. With only that driver, instance
  creation failed with `VK_ERROR_EXTENSION_NOT_PRESENT`. With other drivers also
  loaded, the device could not present to the surface.
- **Local change:** `base/VulkanSwapChain.{h,cpp}` and `createInstance` in
  `base/vulkanexamplebase.cpp`, marked `PS5`. The change renders into a ring of
  offscreen images and takes over `vkAcquireNextImageKHR` and
  `vkQueuePresentKHR` in volk's table. Validation:
  - all 39 linked samples pass on the RTX 3070;
  - their pictures are within 2.21 levels in 255 of `ps5/reference/` (median
    0.20);
  - the validation layer, with synchronization checks, reports nothing (four
    samples, 60 frames each);
  - llvmpipe still uses its headless swapchain.
- **Searched:** issues and PRs for "headless surface", "VK_EXT_headless_surface",
  "headless NVIDIA" and "USE_HEADLESS".
  - #832 (merged) added the headless mode.
  - #1051 (closed) was a crash in a third-party WSI layer; the maintainer's
    answer was that the layer is at fault.
  - No issue or PR asks for a fallback.
- **Disposition: outside upstream's scope; not submitted.**
  - Upstream's headless mode is an opt-in build for drivers that offer the
    extension; a driver without it can gain it through a WSI layer (#1051).
  - The fallback exists for this port's host reference, a testing tool upstream
    does not have.
  - It works only because every Vulkan command goes through volk, which
    upstream does not use.

## 2026-10-10: a model never loaded crashed in its destructor

- **Upstream:** SaschaWillems/Vulkan, `master`.
- **Problem:** `vkglTF::Model::~Model` dereferenced `device` with no null check,
  and `device` started uninitialized. A title never exits (`AGENTS.md`), so a
  sample whose start failed was destroyed with its models never loaded, and
  crashed there.
- **Local change:** `base/VulkanglTFModel.{h,cpp}`, marked `PS5`. `device`
  starts as `nullptr`, and the destructor returns while it is null.
- **Disposition: not reachable upstream; not submitted.** Upstream's
  `vks::tools::exitFatal` calls `exit()` on desktop platforms, so a failed start
  never reaches the destructor. The crash comes from the port's rule that a title
  never exits.

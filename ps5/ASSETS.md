# Assets

The assets the title ships, written from `ps5/assets.json` by
`ps5/tools/build-assets.py --notices`. Only assets with a clear licence are
shipped: in PS5_VulkanTemplate, the asset pack's other files (the `assets`
submodule) stay out, and the samples that use them get the replacements here.

| Path under `/app0/assets/` | Asset | Author | Licence | Samples |
| --- | --- | --- | --- | --- |
| `Roboto-Medium.ttf` | [Roboto Medium](https://fonts.google.com/specimen/Roboto) | Christian Robertson (Google Fonts) | Apache-2.0 | all |
| `models/FlightHelmet` | [Flight Helmet (glTF sample model)](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/FlightHelmet) | Microsoft, donated for glTF testing | CC0-1.0 | gltfloading |
| `models/armor` | [Knight](https://opengameart.org/content/knight-2) (converted to glTF and KTX by Sascha Willems) | piacenti (OpenGameArt) | CC-BY-3.0 | deferred |
| `models/retroufo.gltf` | [Retro UFO](https://github.com/SaschaWillems/Vulkan-Assets) | Sascha Willems | CC-BY-3.0 | bloom |
| `models/retroufo_glow.gltf` | [Retro UFO, glowing parts](https://github.com/SaschaWillems/Vulkan-Assets) | Sascha Willems | CC-BY-3.0 | bloom |
| `models/ps5/cube.gltf` | [Skybox cube](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | bloom, pbrtexture |
| `textures/ps5/cubemap_space.ktx` | [Starfield cube map](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | bloom |
| `models/ps5/tunnel.gltf` | [Tunnel](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | texturemipmapgen |
| `textures/ps5/metalplate_nomips_rgba.ktx` | [Metal Plate](https://polyhaven.com/a/metal_plate) (1024x1024, one mip level, KTX) | Rob Tuytel (Poly Haven) | CC0-1.0 | texturemipmapgen |
| `models/ps5/videocamera` | [Vintage Video Camera](https://polyhaven.com/a/vintage_video_camera) (centred and scaled, tangents added, maps at 1024x1024 as separate KTX files with mipmaps) | James Ray Cock, Jenelle van Heerden (Poly Haven) | CC0-1.0 | pbrtexture |
| `textures/ps5/hdr/kloofendal_cube.ktx` | [Kloofendal 48d Partly Cloudy](https://polyhaven.com/a/kloofendal_48d_partly_cloudy) (2k equirectangular HDR made into a 512x512 RGBA16F cube map with mipmaps) | Greg Zaal (Poly Haven) | CC0-1.0 | pbrtexture |
| `models/ps5/shadowscene_columns.gltf` | [Columns and a knot](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | shadowmapping, rayquery |
| `models/ps5/shadowscene_shapes.gltf` | [Shapes on a floor](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | shadowmapping |
| `models/ps5/floor.gltf` | [Floor](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | deferred |
| `textures/ps5/cobblestone_color_rgba.ktx` | [Cobblestone Floor 01, colour](https://polyhaven.com/a/cobblestone_floor_01) (KTX with mipmaps) | Rob Tuytel (Poly Haven) | CC0-1.0 | deferred |
| `textures/ps5/cobblestone_normal_rgba.ktx` | [Cobblestone Floor 01, normals](https://polyhaven.com/a/cobblestone_floor_01) (KTX with mipmaps) | Rob Tuytel (Poly Haven) | CC0-1.0 | deferred |
| `models/ps5/lantern` | [Lantern (glTF sample model)](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Lantern) (parent nodes centre it and scale it to 1.6, 5.5 and 8 tall, one .gltf for each sample) | Microsoft (sbtron) | CC0-1.0 | starter, multisampling, dynamicrendering |
| `models/ps5/rock.gltf` | [Rock](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | instancing |
| `models/ps5/planet.gltf` | [Planet](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | instancing |
| `textures/ps5/lavaplanet_rgba.ktx` | [Lava planet surface](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | instancing |
| `textures/ps5/texturearray_rocks_rgba.ktx` | [Rock Boulder Dry, Rock Face, Aerial Rocks 02, Dry Riverbed Rock](https://polyhaven.com/textures) (four colour maps at 512x512 in one KTX array with mipmaps) | Dimitrios Savva, Rico Cilliers, Greg Zaal, Dario Barresi, Rob Tuytel, Amal Kumar (Poly Haven) | CC0-1.0 | instancing |
| `models/ps5/plants.gltf` | [Plants](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | indirectdraw |
| `models/ps5/ground_disc.gltf` | [Ground disc](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | indirectdraw |
| `models/ps5/skysphere.gltf` | [Sky sphere](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | indirectdraw |
| `textures/ps5/texturearray_plants_rgba.ktx` | [Plant pictures](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | indirectdraw |
| `textures/ps5/ground_dry_rgba.ktx` | [Dry Ground 01](https://polyhaven.com/a/dry_ground_01) (512x512 KTX with mipmaps) | Rob Tuytel (Poly Haven) | CC0-1.0 | indirectdraw |
| `textures/ps5/particle01_rgba.ktx` | [Particle sprite](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | computeparticles |
| `textures/ps5/particle_gradient_rgba.ktx` | [Particle colour ramp](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | computeparticles |
| `models/ps5/imgui_models.gltf` | [Shapes](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | imgui |
| `models/ps5/imgui_background.gltf` | [Room](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | imgui |
| `models/ps5/imgui_ring.gltf` | [Ring of lights](ps5/tools/generate_assets.py) | made for this fork (ps5/tools/generate_assets.py) | MIT | imgui |

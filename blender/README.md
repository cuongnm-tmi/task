# ICON MODE Blender Cycles Render

Use this path when the target is a still image that looks close to the reference render. The C++ OpenGL app remains useful for real-time walkthrough, but the photorealistic output should be rendered with Blender Cycles.

## Requirements

- Blender 4.x recommended
- Existing assets in `Bai1.trenlop/`
- HDRI Map (Must be manually downloaded from [Polyhaven](https://polyhaven.com/hdris) and saved as `Bai1.trenlop/studio.hdr`)

## Render

From the repository root:

```bash
blender --background --python blender/icon_mode_cycles_scene.py -- --output renders/icon_mode_cycles.png --resolution 1920 1080 --samples 512
```

Windows example:

```bat
"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 1920 1080 --samples 512
```

The script also saves `renders/icon_mode_cycles.blend`, so the scene can be opened and tuned manually if needed.

## What This Builds

- Dark storefront facade with illuminated `ICON MODE` sign
- Environment mapping using HDRI for realistic ambient lighting
- Warm track lighting, Area lights replacing point lights, and hidden strip lights
- Concrete walls, wood floor (`wood.jpg`), rear checkout counter
- Symmetric wall shelving, folded apparel, hanging shirts, shoes (`shoe_store.obj`)
- Front mannequins, posters, plants, central display tables
- Cycles renderer with denoising, Filmic tone mapping, Depth of Field (DOF), and PBR materials

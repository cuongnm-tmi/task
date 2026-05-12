# ICON MODE Blender Cycles Render

Use this path when the target is a still image that looks close to the reference render. The C++ OpenGL app remains useful for real-time walkthrough, but the photorealistic output should be rendered with Blender Cycles.

## Requirements

- Blender 4.x recommended
- Existing assets in `Bai1.trenlop/`

## Render

From the repository root:

```bash
blender --background --python blender/icon_mode_cycles_scene.py -- --output renders/icon_mode_cycles.png --resolution 1920 1080 --samples 256
```

Windows example:

```bat
"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 1920 1080 --samples 256
```

The script also saves `renders/icon_mode_cycles.blend`, so the scene can be opened and tuned manually if needed.

## What This Builds

- Dark storefront facade with illuminated `ICON MODE` sign
- Warm track lighting and hidden strip lights
- Concrete walls, stone floor, rear checkout counter
- Symmetric wall shelving, folded apparel, hanging shirts, shoes
- Front mannequins, posters, plants, central display tables
- Cycles renderer with denoising, Filmic tone mapping, and PBR-style materials

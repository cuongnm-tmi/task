# task - ICON MODE Store

Visual Studio C++ OpenGL walkthrough migrated from GLUT to GLFW + GLAD.

For a still image that looks close to the provided photorealistic storefront reference, use the Blender Cycles pipeline in `blender/`. Real-time OpenGL cannot reach that render quality without a much larger deferred/PBR lighting pipeline.

## Dependencies

The project uses vcpkg manifest mode via `vcpkg.json`:

- glfw3
- glad
- glm
- assimp

If `tools/vcpkg` is not present, install vcpkg there before opening the solution:

```bat
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Then open `Bai1.trenlop.slnx` in Visual Studio and build `x64` or `Win32`.

## Photorealistic Render

Install Blender 4.x, then run:

```bat
blender\render_icon_mode.bat
```

Or run Blender directly:

```bat
"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 1920 1080 --samples 256
```

The script builds a Cycles scene with the ICON MODE facade, warm lighting, wall shelves, display tables, mannequins, apparel, shoes, posters, plants, and PBR-style materials.

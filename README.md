# ICON MODE Store

ICON MODE Store is now a C++ Filament walkthrough project. The old raw OpenGL
Visual Studio path has been retired because the reference images require a
higher-level real-time PBR renderer with built-in lighting, shadows, glTF, and
post-processing.

## Current Stack

- C++20
- Google Filament for real-time PBR rendering
- SDL2 for the desktop window and input
- Blender 4.2+ EEVEE Next for still previews and GLB export
- CMake as the active build system

## Reference Direction

The images in `image/` define the target store:

- dark charcoal concrete walls and ceiling
- large grey floor tiles
- black steel shelf frames with dark walnut boards
- warm amber LED strips under shelves and reception counter ribs
- white `ICON MODE / MEN'S WEAR` backlit signage
- symmetrical menswear racks, folded stacks, mannequins, campaign posters,
  plants, shoes, and a central reception counter

## Project Layout

```text
.
|-- CMakeLists.txt
|-- cmake/
|   `-- FilamentSdk.cmake
|-- src/
|   |-- main.cpp
|   |-- StoreApp.h/.cpp
|   |-- StoreScene.h/.cpp
|   |-- CameraController.h/.cpp
|   `-- LightManager.h/.cpp
|-- assets/
|   |-- source/      Original OBJ/JPG source assets
|   |-- models/      Generated icon_mode_store.glb
|   |-- textures/    Optional KTX2 textures
|   `-- ibl/         Optional cmgen output
|-- blender/
|   |-- icon_mode_scene.py
|   |-- export_gltf.py
|   `-- render_eevee.bat
`-- image/           Reference photos, ignored by git
```

## Generate The Store GLB

```bat
blender.exe --background --python blender\icon_mode_scene.py -- ^
  --skip-render ^
  --glb-output assets\models\icon_mode_store.glb
```

Or render a still preview and export the GLB in one pass:

```bat
blender\render_eevee.bat
```

## Build Filament Viewer

Build Filament separately first, or download an official Filament SDK. Then:

```bat
cmake -G Ninja -B build -S . ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DFILAMENT_ROOT=C:\dev\filament\out\cmake-release

cmake --build build
build\icon_mode_store.exe --model assets\models\icon_mode_store.glb
```

If your Filament libraries are not under `FILAMENT_ROOT`, pass
`-DFILAMENT_LIB_DIR=<path-to-libs>`.

## Controls

- Hold right mouse button: look around
- `WASD`: move
- `Q/E`: move down/up
- `Esc`: quit

## Notes

- `vcpkg.json` only keeps SDL2. Filament is supplied by `FILAMENT_ROOT`.
- `assets/models/icon_mode_store.glb` is generated output and ignored by git.
- Blender remains the authoring path. Filament is the interactive walkthrough
  runtime.

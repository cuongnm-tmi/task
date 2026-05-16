# ICON MODE Filament Visual Guide

The project is now centered on Filament for real-time walkthrough and Blender
EEVEE Next for fast still previews.

## Filament Runtime Goals

- Load a single Blender-exported `assets/models/icon_mode_store.glb`.
- Let glTF carry PBR material assignments instead of custom GLSL.
- Use Filament lights for warm ceiling spots, shelf strip glows, and low
  ambient fill.
- Keep camera movement at human eye level for walkthrough presentation.

## Reference Match Checklist

- Back wall: dark charcoal with white `ICON MODE / MEN'S WEAR` sign.
- Reception: black counter, vertical ribs, amber glow, POS monitors.
- Walls: concrete texture feel, black framed shelves, dark walnut boards.
- Lighting: low warm ambient, sharp overhead spots, amber shelf/counter LEDs.
- Merchandising: navy/cream/grey menswear, folded stacks, shoes, mannequins,
  posters, and plants.

## Pipeline

```text
Blender icon_mode_scene.py
  -> renders/icon_mode_eevee.png for still preview
  -> assets/models/icon_mode_store.glb for runtime

C++ Filament viewer
  -> loads GLB
  -> adds runtime lights and walkthrough camera
```

## Commands

```bat
blender.exe --background --python blender\icon_mode_scene.py -- --skip-render --glb-output assets\models\icon_mode_store.glb
cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DFILAMENT_ROOT=C:\dev\filament\out\cmake-release
cmake --build build
build\icon_mode_store.exe --model assets\models\icon_mode_store.glb
```

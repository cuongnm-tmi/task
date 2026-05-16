# ICON MODE Blender Pipeline

Blender is the authoring and still-render path. It now builds the reference
store scene and exports one GLB for the C++ Filament viewer.

## Generate GLB Only

```bat
blender.exe --background --python blender\icon_mode_scene.py -- ^
  --skip-render ^
  --glb-output assets\models\icon_mode_store.glb
```

## Render EEVEE Next Preview And Export GLB

```bat
blender\render_eevee.bat
```

Equivalent direct command:

```bat
blender.exe --background --python blender\icon_mode_scene.py -- ^
  --output renders\icon_mode_eevee.png ^
  --glb-output assets\models\icon_mode_store.glb ^
  --resolution 1920 1080 ^
  --samples 64 ^
  --save-blend
```

## What The Script Builds

- dark concrete store shell
- grey tile floor and black ceiling tracks
- warm spotlights and amber LED strips
- rear and front `ICON MODE / MEN'S WEAR` signs
- reception counter with ribbed amber backlight
- side wall racks and folded menswear stacks
- central display tables, shoes, posters, plants, and mannequins

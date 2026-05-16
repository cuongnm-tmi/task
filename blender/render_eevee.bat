@echo off
setlocal

where blender.exe >nul 2>nul
if errorlevel 1 (
    set "BLENDER_EXE=C:\Program Files\Blender Foundation\Blender 4.3\blender.exe"
) else (
    set "BLENDER_EXE=blender.exe"
)

"%BLENDER_EXE%" --background --python blender\icon_mode_scene.py -- ^
  --output renders\icon_mode_eevee.png ^
  --glb-output assets\models\icon_mode_store.glb ^
  --resolution 1920 1080 ^
  --samples 64 ^
  --save-blend

endlocal


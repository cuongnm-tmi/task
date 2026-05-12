@echo off
setlocal

set BLENDER_EXE=blender
if exist "C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" set BLENDER_EXE=C:\Program Files\Blender Foundation\Blender 4.3\blender.exe
if exist "C:\Program Files\Blender Foundation\Blender 4.2\blender.exe" set BLENDER_EXE=C:\Program Files\Blender Foundation\Blender 4.2\blender.exe

"%BLENDER_EXE%" --background --python "%~dp0icon_mode_cycles_scene.py" -- --output "%~dp0..\renders\icon_mode_cycles.png" --resolution 1920 1080 --samples 256

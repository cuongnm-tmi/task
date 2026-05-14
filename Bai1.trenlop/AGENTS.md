# BAI1.TRENLOP MODULE KNOWLEDGE

**Scope:** `Bai1.trenlop/`
**Role:** Real-time OpenGL walkthrough application (Windows/Visual Studio path)

## OVERVIEW
`Main.cpp` is the active runtime entrypoint and currently contains most first-party rendering/game-loop logic for the OpenGL path, which has recently been upgraded to include Shadow Mapping (via custom FBO) and PBR vertex shader integrations.

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Runtime bootstrap | `Main.cpp` (`main`) | GLFW init, GLAD load, callbacks, render loop |
| Frame orchestration | `Main.cpp` (`display`, `renderScene`) | Controls present path vs fallback path, runs shadow depth passes |
| Shader configuration | `shaders/` | Contains `pbr.vert` GLSL shader definitions |
| Build settings | `Bai1.trenlop.vcxproj` | C++20, warning level, vcpkg integration |
| Inactive reference source | `Bai1.cpp` | Outside active build target (`.vcxproj` compiles `Main.cpp`) |
| Shared assets | `floor.jpg`, `wood.jpg`, `*.obj`, `studio.hdr` | Also consumed by Blender pipeline |

## HIGH-RISK HOTSPOTS
- `Main.cpp` is a 2k+ line monolith; surgical edits were made using dummy structs (`DummyShader`) to safely embed modernized shadow/PBR loop sequences without breaking legacy code.
- GL state changes are frequent (`glEnable/glDisable/glBlendFunc/glDepthMask/framebuffer bind`); keep state transitions balanced.
- Present pipeline has dual path (FBO+shader vs fallback); validate both paths after any render/state edits.
- Model loading has fallback behavior (imported mesh vs procedural path); avoid breaking fallback-only behavior.

## CONVENTIONS
- Active source of truth for app runtime is `Main.cpp`, not `Bai1.cpp`.
- Build via Visual Studio/MSBuild (`.slnx` + `.vcxproj`), not CMake/Make.
- Compiler setup in project file: `stdcpp20`, `Level3`, `SDLCheck=true`, `ConformanceMode=true`.
- Treat `stb_image.h`, `tiny_obj_loader.h`, local `glm*` folders as vendor/third-party scope.

## ANTI-PATTERNS
- Do not refactor large render regions in one sweep; split changes to isolated concerns.
- Do not assume CI/test safety net exists for visual regressions; manual validation is required.
- Do not infer architecture from `Debug/` artifacts or vendor folders.
- Do not switch app entrypoint to `Bai1.cpp` unless build graph is intentionally changed.

## COMMANDS
```bash
# Build/Run (Windows)
# Open Bai1.trenlop.slnx in Visual Studio
# Select Debug/Release + x64
# Build and run Bai1.trenlop project

# Optional dependency install via vcpkg
tools\vcpkg\vcpkg.exe install --triplet x64-windows

# Triplet follows selected platform:
# Win32 -> x86-windows
# x64   -> x64-windows
```

## NOTES
- If Visual Studio lacks toolset `v145`, retarget project toolset before build.
- Prioritize edits that preserve visual output stability over broad structural cleanup.

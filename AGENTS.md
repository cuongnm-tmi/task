# PROJECT KNOWLEDGE BASE

**Generated:** 2026-05-14 (Asia/Saigon)
**Commit:** 4a668cb
**Branch:** main
**Mode:** update

## OVERVIEW
ICON MODE Store has two production paths: a real-time C++ OpenGL walkthrough and an offline Blender Cycles render pipeline. Most editable app logic is in `Bai1.trenlop/Main.cpp` including newly introduced Shadow Mapping and PBR shader setup; render automation lives in `blender/icon_mode_cycles_scene.py`.

## STRUCTURE
```text
./
├── Bai1.trenlop.slnx            # Visual Studio solution entry
├── vcpkg.json                   # Manifest dependencies (assimp/glad/glfw3/glm)
├── Bai1.trenlop/                # C++ runtime app + assets + VS project
│   ├── shaders/                 # GLSL shaders (e.g., pbr.vert)
│   └── Main.cpp                 # OpenGL runtime with PBR + Shadow map passes
├── blender/                     # Offline photorealistic render pipeline
├── tools/                       # vendored/build tooling cache (ignore for product logic)
└── vcpkg_installed/             # installed dependencies (ignore for product logic)
```

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| Main app runtime flow | `Bai1.trenlop/Main.cpp` | Single active C++ entrypoint in build graph |
| Shaders | `Bai1.trenlop/shaders/` | Contains PBR vertex and shadow passes |
| Solution/project build config | `Bai1.trenlop.slnx`, `Bai1.trenlop/Bai1.trenlop.vcxproj` | VS/MSBuild + vcpkg manifest mode |
| Offline high-quality rendering | `blender/icon_mode_cycles_scene.py`, `blender/render_icon_mode.bat` | Blender background CLI path |
| Dependency source of truth | `vcpkg.json` | Do not infer deps from vendored folders |
| Developer runbook | `README.md`, `blender/README.md` | Windows build + Blender render commands |

## CODE MAP
| Symbol/Flow | Type | Location | Role |
|-------------|------|----------|------|
| `main(int argc, char** argv)` | Function | `Bai1.trenlop/Main.cpp` | App bootstrap (`glfwInit`, `gladLoadGLLoader`, render loop) |
| `display()` | Function | `Bai1.trenlop/Main.cpp` | Frame orchestration + present/fallback path + Shadow Map pass |
| `renderScene()` | Function | `Bai1.trenlop/Main.cpp` | Scene graph drawing orchestration |
| `if __name__ == "__main__"` | Python entrypoint | `blender/icon_mode_cycles_scene.py` | Blender batch render invocation path |

## CONVENTIONS
- Build system is Visual Studio/MSBuild (`.slnx` + `.vcxproj`), not CMake/Make/NPM.
- C++ standard is `stdcpp20`; warning level `Level3`; `SDLCheck=true`; `ConformanceMode=true`.
- vcpkg is manifest-first (`VcpkgEnableManifest=true`); triplet follows platform (`x86-windows` / `x64-windows`).
- No first-party lint/test workflow is documented in the reviewed project entrypoints (`README.md`, `.slnx`, `.vcxproj`).

## ANTI-PATTERNS (THIS PROJECT)
- Do not treat `Bai1.trenlop/Bai1.cpp` as active app entry; build graph compiles `Main.cpp`.
- Do not use `tools/` or `vcpkg_installed/` as source architecture signals; they are vendored/operational artifacts.
- Do not infer CI for this app from `tools/vcpkg/.github/workflows/*` (that is vendor CI, not project CI).
- Do not expect an existing first-party automated test suite; no unit/integration/e2e runner is currently defined.

## UNIQUE STYLES
- Two-track delivery model is intentional:
  - C++ OpenGL path for interactive walkthrough (recently enhanced with custom shadow FBO and `pbr.vert` shader code).
  - Blender Cycles path for photorealistic still output (fully configured with HDRI, Area lights, and node-based PBR).
- Rendering quality tuning in C++ relies on hand-tuned GL state transitions, shadow maps, and fallback rendering branches.

## COMMANDS
```bash
# OpenGL app (Windows/Visual Studio)
# 1) Open Bai1.trenlop.slnx
# 2) Select Debug/Release + x64
# 3) Build and Run Bai1.trenlop project

# vcpkg bootstrap (Windows)
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics

# Optional manual dependency install
tools\vcpkg\vcpkg.exe install --triplet x64-windows

# Blender render (cross-platform CLI)
blender --background --python blender/icon_mode_cycles_scene.py -- --output renders/icon_mode_cycles.png --resolution 1920 1080 --samples 512
```

## NOTES
- `PlatformToolset` in project is `v145`; retarget in Visual Studio if local installation differs.
- `Main.cpp` is a high-complexity monolith (2k+ lines); surgical edits implemented Dummy Shader constructs to integrate new shadow pass securely.
- Treat `Bai1.trenlop/stb_image.h`, `tiny_obj_loader.h`, local `glm*` content as third-party/vendor scope.
- `studio.hdr` needs to be placed into `Bai1.trenlop/` for Blender photorealism to achieve 100% reference parity.

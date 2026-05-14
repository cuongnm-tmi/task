# BLENDER MODULE KNOWLEDGE

**Scope:** `blender/`
**Role:** Offline photorealistic render pipeline (Cycles)

## OVERVIEW
`icon_mode_cycles_scene.py` is the pipeline entrypoint for scripted scene construction and final still rendering.

## WHERE TO LOOK
| Task | Location | Notes |
|------|----------|-------|
| CLI args and outputs | `icon_mode_cycles_scene.py` (`parse_args`) | Handles `--output`, `--resolution`, `--samples`, `--save-blend` |
| Scene/material assembly | `icon_mode_cycles_scene.py` (`build_*`, `create_materials`) | Procedural scene generation |
| Render settings | `icon_mode_cycles_scene.py` (`configure_render`) | Cycles, sampling, denoise/tone mapping settings |
| Asset bridge | `icon_mode_cycles_scene.py` (`ASSET_DIR`) | Reads assets from `../Bai1.trenlop` |
| Windows quick run | `render_icon_mode.bat` | Wrapper around Blender background command |
| Module runbook | `README.md` | Canonical render command examples |

## CONVENTIONS
- Treat this module as offline render path; do not mix assumptions from real-time OpenGL runtime.
- Keep asset path contract stable (`Bai1.trenlop` as shared asset source).
- Prefer idempotent scene-building behavior (clear scene first, then deterministic rebuild).
- Keep render defaults practical for quality/runtime balance (default samples currently 256).

## ANTI-PATTERNS
- Do not hardcode machine-specific Blender paths in Python logic; keep OS-specific wrappers in `.bat` or docs.
- Do not break `--` argument parsing contract used by Blender background invocations.
- Do not silently rename shared assets without updating both pipelines.
- Do not add fragile randomness to procedural scene generation unless reproducibility is preserved.

## COMMANDS
```bash
# Cross-platform blender render from repo root
blender --background --python blender/icon_mode_cycles_scene.py -- --output renders/icon_mode_cycles.png --resolution 1920 1080 --samples 256

# Windows helper script
blender\render_icon_mode.bat
```

## NOTES
- Script saves both image output and a `.blend` file for manual tuning.
- If Blender CLI is not on PATH, use absolute executable path as documented in `blender/README.md`.

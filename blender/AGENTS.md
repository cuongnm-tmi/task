# Blender Pipeline Notes

The active Blender entrypoint is `icon_mode_scene.py`.

- Use it to generate `assets/models/icon_mode_store.glb`.
- Use EEVEE Next for fast reference previews.
- Keep geometry/material changes aligned with the `image/` reference folder.
- `icon_mode_cycles_scene.py` is retired and should not be brought back as the
  main pipeline unless the user explicitly asks for Cycles.

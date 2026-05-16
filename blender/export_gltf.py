import os
import runpy
import sys


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    output = os.path.join("assets", "models", "icon_mode_store.glb")

    for index, arg in enumerate(argv):
        if arg in ("--output", "--glb-output") and index + 1 < len(argv):
            output = argv[index + 1]

    script = os.path.join(os.path.dirname(__file__), "icon_mode_scene.py")
    sys.argv = [script, "--", "--skip-render", "--glb-output", output]
    runpy.run_path(script, run_name="__main__")


if __name__ == "__main__":
    main()


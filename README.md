# task - ICON MODE Store

Visual Studio C++ OpenGL walkthrough migrated from GLUT to GLFW + GLAD.

## Dependencies

The project uses vcpkg manifest mode via `vcpkg.json`:

- glfw3
- glad
- glm
- assimp

If `tools/vcpkg` is not present, install vcpkg there before opening the solution:

```bat
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Then open `Bai1.trenlop.slnx` in Visual Studio and build `x64` or `Win32`.

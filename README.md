# ICON MODE Store

Du an dung de dung canh cua hang thoi trang nam `ICON MODE` theo hai huong:

- `C++ OpenGL`: ung dung walkthrough real-time, da migrate tu GLUT sang GLFW + GLAD.
- `Blender Cycles`: render anh tinh photorealistic gan reference hon, phu hop khi muc tieu la anh dep tren 90%.

## Trang Thai Hien Tai

- Code chinh cua app OpenGL nam o `Bai1.trenlop/Main.cpp`.
- Visual Studio solution: `Bai1.trenlop.slnx`.
- Project C++: `Bai1.trenlop/Bai1.trenlop.vcxproj`.
- Quan ly thu vien bang vcpkg manifest: `vcpkg.json`.
- Blender render pipeline nam trong thu muc `blender/`.
- Thu muc output render mac dinh la `renders/` va dang duoc ignore khoi git.

## Cong Nghe

- C++20
- OpenGL
- GLFW 3
- GLAD
- GLM
- Assimp
- stb_image
- Blender Cycles cho render photorealistic

## Cau Truc Thu Muc

```text
.
+-- Bai1.trenlop.slnx
+-- vcpkg.json
+-- Bai1.trenlop/
|   +-- Main.cpp
|   +-- Bai1.trenlop.vcxproj
|   +-- floor.jpg
|   +-- wood.jpg
|   +-- mannequin_store.obj
|   +-- shoe_store.obj
+-- blender/
    +-- icon_mode_cycles_scene.py
    +-- render_icon_mode.bat
    +-- README.md
```

## Yeu Cau Cai Dat

### De chay OpenGL app

- Windows 10/11
- Visual Studio co workload `Desktop development with C++`
- Git
- vcpkg dat tai `tools/vcpkg`

Project dang khai bao `PlatformToolset` la `v145`. Neu Visual Studio tren may khong co toolset nay, hay retarget project trong Visual Studio sang toolset dang co san tren may.

### De render anh giong reference

- Blender 4.x
- Cac asset san co trong `Bai1.trenlop/`

## Cai Dependency Bang vcpkg

Tu thu muc goc cua repo, chay tren Windows Command Prompt hoac PowerShell:

```bat
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Dependency duoc khai bao trong `vcpkg.json`:

```json
{
  "dependencies": [
    "assimp",
    "glad",
    "glfw3",
    "glm"
  ]
}
```

Khi build bang Visual Studio, project se doc vcpkg manifest va cai cac dependency can thiet. Neu muon cai thu cong:

```bat
tools\vcpkg\vcpkg.exe install --triplet x64-windows
```

## Cach Chay OpenGL App

1. Mo `Bai1.trenlop.slnx` bang Visual Studio.
2. Chon configuration `Debug` hoac `Release`.
3. Chon platform `x64`.
4. Build solution.
5. Run project `Bai1.trenlop`.

Neu build `Win32`, vcpkg se dung triplet `x86-windows`. Neu build `x64`, vcpkg se dung triplet `x64-windows`.

## Cach Render Anh Photorealistic Bang Blender

Day la huong khuyen nghi neu muc tieu la anh tinh giong reference nhat.

Chay nhanh tren Windows:

```bat
blender\render_icon_mode.bat
```

Hoac chay truc tiep bang Blender:

```bat
"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 1920 1080 --samples 256
```

Output mac dinh:

- Anh render: `renders/icon_mode_cycles.png`
- File scene Blender: `renders/icon_mode_cycles.blend`

Co the tang chat luong bang cach tang `--samples`, vi du:

```bat
blender --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 2560 1440 --samples 512
```

## Ghi Chu Ve Do Giong Anh Mau

OpenGL real-time hien tai phu hop de di chuyen va xem scene truc tiep, nhung khong the dat do photorealistic nhu anh ray-traced neu khong viet them pipeline lon gom PBR, shadow mapping, SSAO, HDR, bloom va tone mapping.

Blender Cycles la huong duoc dung trong repo de dat chat luong anh cao hon: vat lieu PBR, den am, den track, ke quan ao, ban trung bay, mannequin, poster, cay trang tri va camera storefront duoc tao tu script `blender/icon_mode_cycles_scene.py`.

## Loi Thuong Gap

### Khong tim thay `glfw3`, `glad`, `glm` hoac `assimp`

Kiem tra lai `tools/vcpkg` da ton tai va da bootstrap:

```bat
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

Sau do build lai project trong Visual Studio.

### Visual Studio bao loi toolset

Mo project properties va retarget `Platform Toolset` sang toolset co tren may, vi du `v143` neu dang dung Visual Studio 2022.

### Lenh Blender khong chay

Kiem tra Blender da duoc cai va `blender.exe` co trong PATH. Neu khong, dung duong dan day du toi `blender.exe` nhu vi du o tren.

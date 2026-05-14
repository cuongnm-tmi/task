# ICON MODE Store

Dự án dựng cảnh 3D cửa hàng thời trang nam `ICON MODE` theo hai hướng:

- `C++ OpenGL`: Ứng dụng walkthrough real-time, đã nâng cấp shader PBR và Shadow Mapping.
- `Blender Cycles`: Render ảnh tĩnh photorealistic với chất lượng cao (ưu tiên khi cần ảnh đẹp trên 90% giống reference).

## Trạng Thái Hiện Tại

- Mã nguồn chính của app OpenGL nằm ở `Bai1.trenlop/Main.cpp`.
- Các Shader mới nằm trong `Bai1.trenlop/shaders/` (ví dụ: `pbr.vert`).
- Visual Studio solution: `Bai1.trenlop.slnx`.
- Project C++: `Bai1.trenlop/Bai1.trenlop.vcxproj`.
- Quản lý thư viện bằng `vcpkg manifest`: `vcpkg.json`.
- Blender render pipeline (PBR, HDRI, Area lights) nằm trong thư mục `blender/`.
- Thư mục output render mặc định là `renders/`.

## Công Nghệ

- C++20, OpenGL 4.6 (hoặc tương thích ngược với Shader)
- GLFW 3, GLAD, GLM, Assimp, stb_image
- Shadow Mapping, PBR Shader
- Blender Cycles 4.x (Photorealistic Rendering)

## Cấu Trúc Thư Mục

```text
.
+-- Bai1.trenlop.slnx
+-- vcpkg.json
+-- Bai1.trenlop/
|   +-- Main.cpp
|   +-- Bai1.trenlop.vcxproj
|   +-- shaders/
|       +-- pbr.vert
|   +-- floor.jpg
|   +-- wood.jpg
|   +-- mannequin_store.obj
|   +-- shoe_store.obj
|   +-- studio.hdr (Tải thủ công từ polyhaven.com)
+-- blender/
    +-- icon_mode_cycles_scene.py
    +-- render_icon_mode.bat
    +-- README.md
```

## Hướng Dẫn Setup Thư Viện

Dự án này sử dụng `vcpkg` ở chế độ manifest để tự động cài đặt thư viện (`assimp`, `glad`, `glfw3`, `glm`).

**Bước 1: Tải và cài đặt vcpkg**
Mở Windows Command Prompt hoặc PowerShell tại thư mục gốc của repository:
```bat
git clone https://github.com/microsoft/vcpkg.git tools\vcpkg
tools\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

**Bước 2: Cài đặt thư viện C++ (Tùy chọn, Visual Studio sẽ tự động làm)**
```bat
tools\vcpkg\vcpkg.exe install --triplet x64-windows
```

**Bước 3: Chuẩn bị tài nguyên HDR cho Blender (Bắt buộc để ảnh đẹp)**
- Truy cập https://polyhaven.com/hdris
- Tải một file HDRI (Gợi ý: `studio_small_09_4k.hdr`)
- Đổi tên file vừa tải thành `studio.hdr` và đặt vào thư mục `Bai1.trenlop/`

## Hướng Dẫn Khởi Chạy Dự Án

### Khởi chạy ứng dụng C++ OpenGL
1. Đảm bảo máy tính có Visual Studio (với workload `Desktop development with C++`).
2. Mở file `Bai1.trenlop.slnx` bằng Visual Studio.
3. Chọn configuration `Debug` hoặc `Release` và platform `x64`.
4. (Lưu ý: Nếu bị lỗi Toolset, hãy mở Project Properties và Retarget `Platform Toolset` sang bản hiện có trên máy như `v143`).
5. Chọn Build Solution.
6. Nhấn F5 (hoặc Ctrl+F5) để chạy project `Bai1.trenlop`.

### Khởi chạy Render bằng Blender
Đây là hướng được dùng để đạt chất lượng ảnh PBR + Raytracing cao nhất (giống reference).

**Cách 1: Dùng script có sẵn (Windows)**
```bat
blender\render_icon_mode.bat
```

**Cách 2: Chạy trực tiếp qua command line**
```bat
"C:\Program Files\Blender Foundation\Blender 4.3\blender.exe" --background --python blender\icon_mode_cycles_scene.py -- --output renders\icon_mode_cycles.png --resolution 1920 1080 --samples 512
```

Output mặc định:
- Ảnh render tĩnh: `renders/icon_mode_cycles.png`
- File scene Blender có thể mở lại: `renders/icon_mode_cycles.blend`

## Ghi Chú Cập Nhật Photorealism

- **Blender**: Hệ thống đèn đã được cấu hình với Area lights, DOF (Depth of Field), PBR Materials và HDRI Environment mapping để cho ra kết quả xuất sắc.
- **OpenGL**: Mã nguồn C++ đã được nâng cấp với Shadow mapping (sử dụng Framebuffer Object 4096x4096) và PBR Shader chuẩn bị cho quá trình kết xuất ánh sáng thực tế.

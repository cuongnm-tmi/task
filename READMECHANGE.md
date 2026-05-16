# ICON MODE Store — Agent.md (Revised)

> **Lý do cập nhật:** Stack cũ (raw OpenGL 4.6 + GLFW + GLAD + vcpkg) quá thấp cấp để đạt chất lượng hình ảnh giống reference. Tài liệu này đề xuất lộ trình hiện đại hơn, vẫn dùng C++.

---

## Vấn Đề Với Stack Cũ

| Thành phần cũ | Vấn đề |
|---|---|
| Raw OpenGL 4.6 | Phải tự viết toàn bộ PBR shader, Shadow mapping, IBL — rất dễ sai, khó debug |
| GLFW + GLAD + GLM + Assimp | Setup phức tạp, không có abstraction layer, viết nhiều boilerplate |
| vcpkg manifest | Thêm dependency management layer, dễ conflict khi upgrade |
| Blender Cycles | Render tĩnh, không interactive, mỗi frame mất vài phút |
| Tự viết PBR shader | Kết quả thường còn cách reference ~40–60% về chất lượng visual |

**Kết luận:** Raw OpenGL không thể đạt được chất lượng ảnh trong reference (ảnh reference trông như Blender Cycles render với 500+ samples, IBL, và area lights) bằng real-time rendering thông thường.

---

## Ba Lộ Trình Thay Thế (Vẫn Dùng C++)

### Lộ Trình 1 — Google Filament ⭐ KHUYẾN NGHỊ

**Phù hợp:** Muốn walkthrough real-time, code C++, không cần GPU RTX riêng.

**Chất lượng đạt được:** ~80–85% so với reference (tương đương Blender EEVEE Next)

**Tại sao Filament thắng raw OpenGL:**
- PBR, IBL, Shadow mapping **đã được built-in**, không cần tự viết shader
- Load model **glTF 2.0 trực tiếp từ Blender** — không cần chuyển định dạng
- Google dùng trong production: Android Studio, Google Maps, Sketchfab
- Backend linh hoạt: OpenGL ES 3.1, Vulkan, Metal — **cùng code C++ chạy được trên Windows/Mac/Linux**
- CMake-based, **không cần vcpkg**

```
Tech Stack Mới:
  C++20
  + Google Filament (thay toàn bộ: GLFW, GLAD, raw shaders, GLM)
  + SDL2 (window management)
  + Blender → export glTF 2.0 → load vào Filament
  Build: CMake
```

**Cấu trúc thư mục mới:**
```
icon_mode/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── StoreScene.h / .cpp      ← thay thế Main.cpp
│   ├── CameraController.h/.cpp  ← WASD walkthrough
│   └── LightingSetup.h/.cpp     ← Area lights, IBL
├── assets/
│   ├── models/
│   │   ├── store_room.glb       ← export từ Blender
│   │   ├── reception_desk.glb
│   │   ├── shelving_unit.glb
│   │   └── mannequin.glb
│   ├── textures/
│   │   ├── concrete_wall.ktx2   ← Filament dùng KTX2
│   │   ├── wood_oak.ktx2
│   │   └── metal_black.ktx2
│   └── ibl/
│       └── studio_warm.ktx      ← HDR baked thành IBL cubemap
└── tools/
    └── cmgen.exe                ← Filament tool: convert HDRI → IBL
```

**Setup nhanh (Windows):**
```bat
REM Clone Filament
git clone https://github.com/google/filament.git
cd filament

REM Build Filament (lần đầu ~10-20 phút)
mkdir out/cmake-release
cd out/cmake-release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
ninja

REM Convert HDRI sang IBL cho store
tools\cmgen.exe -x assets\ibl\ --format=ktx studio.hdr
```

**Ví dụ code tối thiểu (main.cpp):**
```cpp
#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/IndirectLight.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <SDL2/SDL.h>

using namespace filament;
using namespace filament::math;

int main() {
    // 1. Tạo engine (OpenGL backend)
    Engine* engine = Engine::create(Engine::Backend::OPENGL);
    
    // 2. Load toàn bộ cửa hàng từ glTF
    auto* assetLoader = gltfio::AssetLoader::create({engine});
    auto* storeAsset = assetLoader->createAssetFromFile("assets/models/icon_mode_store.glb");
    storeAsset->loadAllResources();
    
    // 3. IBL lighting (thay thế Area lights trong Blender)
    // KTX file được tạo bởi cmgen từ HDRI
    auto* ibl = IndirectLight::Builder()
        .reflections(loadKtxTexture("assets/ibl/studio_warm_ibl.ktx"))
        .intensity(30000.0f)
        .build(*engine);
    scene->setIndirectLight(ibl);
    
    // 4. Thêm directional light (ambient warm)
    auto sunLight = EntityManager::get().create();
    LightManager::Builder(LightManager::Type::SUN)
        .color({1.0f, 0.95f, 0.85f})   // warm amber tone
        .intensity(100000.0f)
        .direction({0.0f, -1.0f, -0.8f})
        .build(*engine, sunLight);
    
    // Camera walkthrough
    Camera* camera = engine->createCamera(EntityManager::get().create());
    camera->setProjection(60.0, 16.0/9.0, 0.1, 50.0);
    camera->lookAt({0,1.7,5}, {0,1.5,0}); // eye-level view vào reception
    
    // Render loop...
}
```

---

### Lộ Trình 2 — NVIDIA Falcor (RTX Path Tracing)

**Phù hợp:** Có GPU NVIDIA RTX, muốn chất lượng gần 100% như reference.

**Chất lượng đạt được:** ~93–97% so với reference (real-time path tracing)

**Điều kiện:** GPU NVIDIA RTX 2070 trở lên, Windows + DirectX 12.

```
Tech Stack:
  C++17
  + NVIDIA Falcor 7.x (DX12 + RTX path tracing)
  + Python scripting layer (builtin)
  Build: CMake + Visual Studio 2022
```

Falcor render real-time path tracing — kết quả gần như không phân biệt được với Blender Cycles sau vài giây accumulate. Phù hợp nếu mục tiêu là showcase ảnh chất lượng cao mà vẫn interactive.

**Setup:**
```bat
git clone https://github.com/NVIDIAGameWorks/Falcor.git
cd Falcor
python tools\build.py --config Release
```

---

### Lộ Trình 3 — Unreal Engine 5 C++ (Lumen)

**Phù hợp:** Muốn chất lượng cao nhất, sẵn sàng học UE5, viết gameplay logic bằng C++.

**Chất lượng đạt được:** ~90–95% (Lumen GI + Nanite + Ray Tracing)

**Trade-off:** Cài UE5 ~80GB, nhưng kết quả là walkthrough real-time đẹp nhất có thể.

```
Tech Stack:
  C++ (UE5 C++ Actor/Component system)
  + Unreal Engine 5 (Lumen GI, Nanite, TSR)
  + Blender → FBX/glTF → UE5 import
  Build: UnrealBuildTool (tự động)
```

UE5 cho phép viết C++ thuần, logic game/walkthrough bằng C++, nhưng toàn bộ rendering được UE5 xử lý. Đây là lựa chọn nếu muốn kết quả thương mại.

---

## So Sánh 3 Lộ Trình

| Tiêu chí | Filament | Falcor (RTX) | UE5 Lumen |
|---|---|---|---|
| **Ngôn ngữ** | C++ thuần | C++ thuần | C++ (UE pattern) |
| **Chất lượng visual** | ★★★★☆ | ★★★★★ | ★★★★★ |
| **Setup thời gian** | 1–2 ngày | 1–2 ngày | 3–5 ngày |
| **Yêu cầu GPU** | Bất kỳ | RTX 2070+ | RTX 2060+ |
| **Dung lượng install** | ~500MB | ~2GB | ~80GB |
| **Độ phức tạp code** | Trung bình | Cao | Trung bình |
| **Tốc độ render** | 60fps | 30–60fps | 60fps |
| **glTF từ Blender** | ✅ Native | ✅ Có thể | ✅ Native |
| **Phù hợp dự án này** | ✅ Tốt nhất | ✅ Nếu có RTX | ⚠️ Nếu dài hạn |

---

## Pipeline Tổng Thể (Khuyến Nghị: Filament)

```
[Blender]                     [C++ Filament App]
  Model cửa hàng    →  .glb  →  Load + Scene
  PBR Materials     →  .glb  →  Filament PBR
  HDRI studio.hdr   →  cmgen →  IBL .ktx
  Animation         →  .glb  →  Filament Animation
                               ↓
                          Walkthrough
                          WASD + Mouse
                          60fps real-time
```

---

## Blender — Giữ Nguyên Cho Render Tĩnh (Nâng Cấp)

Thay **Cycles** bằng **EEVEE Next** (Blender 4.2+):

- **Nhanh hơn Cycles 8–15 lần** (giây thay vì phút mỗi frame)
- Chất lượng tương đương Cycles cho studio interior lighting
- Giữ nguyên PBR materials, area lights, HDRI
- Screen-Space Reflections + Real-time AO

**Cập nhật lệnh render:**
```bat
REM Thay --engine=CYCLES bằng --engine=BLENDER_EEVEE_NEXT
blender.exe --background --python blender\icon_mode_scene.py ^
  -- --engine BLENDER_EEVEE_NEXT ^
  --output renders\icon_mode_eevee.png ^
  --resolution 1920 1080 ^
  --samples 64
```

---

## Cấu Trúc Thư Mục Mới (Filament Stack)

```
icon_mode_store/
├── CMakeLists.txt
├── vcpkg.json                    ← Chỉ SDL2 (không còn GLFW/GLAD/Assimp)
├── src/
│   ├── main.cpp
│   ├── StoreApp.h / .cpp         ← Application lifecycle
│   ├── StoreScene.h / .cpp       ← Scene setup, load glTF
│   ├── CameraController.h/.cpp   ← FPS walkthrough WASD
│   ├── LightManager.h/.cpp       ← Area lights, IBL setup
│   └── UIOverlay.h/.cpp          ← ImGui debug panel (optional)
├── assets/
│   ├── models/
│   │   └── icon_mode_store.glb   ← Toàn bộ cửa hàng, 1 file
│   ├── textures/                 ← KTX2 format (dùng Filament's texconv)
│   └── ibl/
│       └── studio_warm.ktx       ← Baked từ HDRI bằng cmgen
├── blender/
│   ├── icon_mode_scene.blend     ← Scene file
│   ├── export_gltf.py            ← Script export .glb cho Filament
│   └── render_eevee.bat          ← Render tĩnh (thay Cycles bằng EEVEE)
├── renders/
│   └── icon_mode_eevee.png
└── tools/
    ├── cmgen.exe                 ← Filament: HDRI → IBL
    └── texconv.exe               ← Filament: PNG → KTX2
```

---

## Hướng Dẫn Setup (Filament, Windows)

### Bước 1: Cài đặt dependencies
```bat
REM Python 3.10+, CMake 3.25+, Ninja, Visual Studio 2022
winget install Ninja-build.Ninja
winget install Kitware.CMake
```

### Bước 2: Build Filament
```bat
git clone --depth=1 https://github.com/google/filament.git
cd filament

set CC=cl
set CXX=cl
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DFILAMENT_ENABLE_JAVA=OFF ^
      -B out\cmake-release -S .
cmake --build out\cmake-release --target filament gltfio_core sdl2

REM Tools (cmgen, texconv)
cmake --build out\cmake-release --target cmgen texconv
```

### Bước 3: Chuẩn bị assets từ Blender
```bat
REM Export scene từ Blender sang glTF
blender.exe --background blender\icon_mode_scene.blend ^
  --python blender\export_gltf.py ^
  -- --output assets\models\icon_mode_store.glb

REM Convert HDRI sang IBL (chạy 1 lần)
tools\cmgen.exe -x assets\ibl\ --format=ktx --size=256 studio.hdr

REM Convert textures sang KTX2 (compressed, GPU-native)
tools\texconv.exe -f etc2-rgb8-srgb -o assets\textures\ concrete_wall.png
tools\texconv.exe -f etc2-rgb8-srgb -o assets\textures\ wood_oak.png
```

### Bước 4: Build project
```bat
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DFILAMENT_DIR=filament/out/cmake-release ^
      -B build -S .
cmake --build build
.\build\icon_mode_store.exe
```

---

## Điều Chỉnh Ánh Sáng Để Giống Reference

Ảnh reference có đặc điểm lighting:
- **Nền tối** (~2700K ambient, rất thấp)
- **Accent warm amber** từ LED strip dưới kệ và desk (3000–3500K)
- **Spotlight trần** chiếu thẳng xuống sản phẩm (4000K, sharp shadow)
- **IBL rất thấp** (chủ yếu là artificial lighting)

```cpp
// Trong LightManager.cpp — setup giống reference
void LightManager::setupIconModeStore(Engine* engine, Scene* scene) {

    // IBL rất thấp (chủ yếu artificial)
    indirectLight->setIntensity(500.0f); // thấp hơn nhiều so với outdoor

    // Ceiling spotlights (Sharp, warm white)
    for (int i = 0; i < 8; i++) {
        auto spot = em.create();
        LightManager::Builder(LightManager::Type::SPOT)
            .color(Color::toLinear<ACCURATE>({1.0f, 0.96f, 0.88f})) // 3800K
            .intensity(50000.0f)
            .position({-3.0f + i*0.9f, 3.0f, 2.0f})
            .direction({0.0f, -1.0f, -0.1f})
            .spotLightCone(math::f::PI / 8.0f, math::f::PI / 6.0f)
            .castShadows(true)
            .build(*engine, spot);
        scene->addEntity(spot);
    }

    // LED strip dưới kệ gỗ (warm amber)
    auto ledStrip = em.create();
    LightManager::Builder(LightManager::Type::POINT)
        .color(Color::toLinear<ACCURATE>({1.0f, 0.75f, 0.35f})) // 2800K amber
        .intensity(3000.0f)
        .position({-2.0f, 1.2f, -2.5f})
        .falloff(1.5f)
        .build(*engine, ledStrip);
    scene->addEntity(ledStrip);

    // Reception desk backlit glow
    auto deskGlow = em.create();
    LightManager::Builder(LightManager::Type::POINT)
        .color(Color::toLinear<ACCURATE>({1.0f, 0.72f, 0.30f})) // deep amber
        .intensity(5000.0f)
        .position({0.0f, 0.5f, 0.0f})
        .falloff(2.0f)
        .build(*engine, deskGlow);
    scene->addEntity(deskGlow);
}
```

---

## Ghi Chú

- **Blender giữ vai trò modeling và render tĩnh** — không bỏ, chỉ chuyển Cycles → EEVEE Next
- **Filament là real-time viewer** — walkthrough, demo, interactive
- Hai công cụ bổ sung cho nhau: Blender cho ảnh marketing, Filament cho presentation interactive
- Nếu sau này muốn nâng cấp lên RTX path tracing: migration từ Filament sang Falcor không quá khó vì đều dùng glTF

---

*Cập nhật: Thay thế raw OpenGL 4.6 + vcpkg stack bằng Google Filament + CMake. Thay Blender Cycles bằng EEVEE Next cho render tĩnh nhanh hơn.*
# ICON MODE Store — Hướng dẫn nâng cấp Photorealism

## Chẩn đoán: Tại sao nhìn giả?

| Vấn đề | Nguyên nhân | Fix |
|--------|------------|-----|
| Vật liệu phẳng | Dùng Phong/Lambert đơn giản | PBR (Metallic/Roughness workflow) |
| Ánh sáng cứng | Point/Dir light duy nhất | HDRI + Area lights |
| Không có bóng | Shadow mapping chưa có / yếu | Shadow maps hoặc ray-traced shadows |
| Sàn nhà "bay" | Thiếu Ambient Occlusion | SSAO (real-time) / Baked AO (offline) |
| Màu sắc flat | Không có tone mapping | ACES/Filmic tone map + gamma |
| Vật liệu giống nhau | Texture chất lượng thấp | PBR texture sets (albedo/normal/rough/metal) |

---

## Path A: Blender Cycles — Photorealistic Render (Ưu tiên cho showcase)

### Upgrade `blender/icon_mode_cycles_scene.py`

```python
import bpy
import math
import os
import sys
import argparse

# ════════════════════════════════════════════════════
#  PARSE ARGS
# ════════════════════════════════════════════════════
def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output",     default="renders/icon_mode.png")
    parser.add_argument("--resolution", nargs=2, type=int, default=[1920, 1080])
    parser.add_argument("--samples",    type=int, default=512)   # ≥512 để đẹp
    parser.add_argument("--save-blend", action="store_true")
    return parser.parse_args(argv)

ARGS     = parse_args()
ASSET_DIR = os.path.join(os.path.dirname(__file__), "..", "Bai1.trenlop")


# ════════════════════════════════════════════════════
#  RESET SCENE
# ════════════════════════════════════════════════════
def reset_scene():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for col in bpy.data.collections:
        bpy.data.collections.remove(col)


# ════════════════════════════════════════════════════
#  RENDER SETTINGS — Cycles Photorealistic
# ════════════════════════════════════════════════════
def configure_render():
    scene = bpy.context.scene
    scene.render.engine            = 'CYCLES'
    scene.cycles.device            = 'GPU'          # đổi sang 'CPU' nếu không có GPU
    scene.cycles.samples           = ARGS.samples
    scene.cycles.use_denoising     = True
    scene.cycles.denoiser          = 'OPENIMAGEDENOISE'

    # Caustics (phản xạ thủy tinh/kim loại)
    scene.cycles.caustics_reflective = True
    scene.cycles.caustics_refractive = True

    # Độ phân giải + output
    scene.render.resolution_x      = ARGS.resolution[0]
    scene.render.resolution_y      = ARGS.resolution[1]
    scene.render.resolution_percentage = 100
    scene.render.filepath           = ARGS.output
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_depth = '16'  # 16-bit PNG

    # Filmic tone mapping (giống thật nhất)
    scene.view_settings.view_transform = 'Filmic'
    scene.view_settings.look           = 'High Contrast'
    scene.view_settings.exposure       = 0.0
    scene.view_settings.gamma          = 1.0

    # Color management
    scene.sequencer_colorspace_settings.name = 'sRGB'


# ════════════════════════════════════════════════════
#  HDRI WORLD LIGHTING — Quan trọng nhất để trông thật
# ════════════════════════════════════════════════════
def setup_hdri_world(hdri_path: str = None):
    """
    Dùng HDRI map cho ambient lighting thực tế.
    Download HDRI miễn phí: https://polyhaven.com/hdris
    Gợi ý cho store: 'studio_small_09_4k.hdr' hoặc 'kloppenheim_02_4k.hdr'
    """
    world = bpy.context.scene.world
    world.use_nodes = True
    nt = world.node_tree
    nt.nodes.clear()

    out   = nt.nodes.new('ShaderNodeOutputWorld')
    bg    = nt.nodes.new('ShaderNodeBackground')
    env   = nt.nodes.new('ShaderNodeTexEnvironment')
    coord = nt.nodes.new('ShaderNodeTexCoord')
    rot   = nt.nodes.new('ShaderNodeVectorRotate')

    # Xoay HDRI
    rot.rotation_type = 'EULER_XYZ'
    rot.inputs['Rotation'].default_value = (0, 0, math.radians(45))

    if hdri_path and os.path.exists(hdri_path):
        env.image = bpy.data.images.load(hdri_path)
    else:
        # Fallback: gradient sky nếu không có HDRI
        bg.inputs['Color'].default_value = (0.9, 0.9, 1.0, 1.0)
        bg.inputs['Strength'].default_value = 1.0
        nt.links.new(bg.outputs['Background'], out.inputs['Surface'])
        return

    bg.inputs['Strength'].default_value = 1.5   # cường độ ambient

    nt.links.new(coord.outputs['Generated'], rot.inputs['Vector'])
    nt.links.new(rot.outputs['Vector'],      env.inputs['Vector'])
    nt.links.new(env.outputs['Color'],       bg.inputs['Color'])
    nt.links.new(bg.outputs['Background'],   out.inputs['Surface'])


# ════════════════════════════════════════════════════
#  MATERIALS — PBR Physically Based
# ════════════════════════════════════════════════════
def make_pbr_material(name, albedo, metallic=0.0, roughness=0.5,
                       albedo_tex=None, normal_tex=None,
                       roughness_tex=None, ao_tex=None):
    """Tạo PBR material chuẩn Cycles."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()

    out    = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf   = nt.nodes.new('ShaderNodeBsdfPrincipled')

    # Values mặc định
    bsdf.inputs['Base Color'].default_value    = (*albedo, 1.0)
    bsdf.inputs['Metallic'].default_value      = metallic
    bsdf.inputs['Roughness'].default_value     = roughness
    bsdf.inputs['Specular IOR Level'].default_value = 0.5

    # Albedo texture
    if albedo_tex and os.path.exists(albedo_tex):
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(albedo_tex)
        tex.image.colorspace_settings.name = 'sRGB'
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])

    # Normal map texture
    if normal_tex and os.path.exists(normal_tex):
        tex  = nt.nodes.new('ShaderNodeTexImage')
        nmap = nt.nodes.new('ShaderNodeNormalMap')
        tex.image = bpy.data.images.load(normal_tex)
        tex.image.colorspace_settings.name = 'Non-Color'
        nmap.inputs['Strength'].default_value = 1.0
        nt.links.new(tex.outputs['Color'],  nmap.inputs['Color'])
        nt.links.new(nmap.outputs['Normal'], bsdf.inputs['Normal'])

    # Roughness texture
    if roughness_tex and os.path.exists(roughness_tex):
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = bpy.data.images.load(roughness_tex)
        tex.image.colorspace_settings.name = 'Non-Color'
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Roughness'])

    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return mat


def create_materials():
    ASSET = ASSET_DIR
    mats = {}

    # ── Sàn nhà (gỗ)
    mats['floor'] = make_pbr_material(
        'Floor_Wood',
        albedo=(0.45, 0.28, 0.15),
        metallic=0.0, roughness=0.7,
        albedo_tex=os.path.join(ASSET, 'wood.jpg'),
    )

    # ── Tường sơn trắng
    mats['wall'] = make_pbr_material(
        'Wall_Paint',
        albedo=(0.92, 0.92, 0.90),
        metallic=0.0, roughness=0.85,
    )

    # ── Kệ kim loại (chrome/inox)
    mats['shelf_metal'] = make_pbr_material(
        'Shelf_Metal',
        albedo=(0.7, 0.7, 0.72),
        metallic=1.0, roughness=0.15,
    )

    # ── Kệ gỗ
    mats['shelf_wood'] = make_pbr_material(
        'Shelf_Wood',
        albedo=(0.55, 0.35, 0.20),
        metallic=0.0, roughness=0.6,
    )

    # ── Kính tủ (Glass)
    mats['glass'] = make_pbr_material(
        'Glass_Cabinet',
        albedo=(0.9, 0.95, 1.0),
        metallic=0.0, roughness=0.02,
    )
    # Thêm transmission cho kính trong suốt
    mats['glass'].node_tree.nodes['Principled BSDF'].inputs['Transmission Weight'].default_value = 0.95
    mats['glass'].node_tree.nodes['Principled BSDF'].inputs['IOR'].default_value = 1.52

    # ── Giầy/Sản phẩm: da (leather)
    mats['shoe_leather'] = make_pbr_material(
        'Shoe_Leather',
        albedo=(0.05, 0.03, 0.02),
        metallic=0.0, roughness=0.35,
    )

    # ── Đế giầy rubber
    mats['shoe_sole'] = make_pbr_material(
        'Shoe_Sole',
        albedo=(0.15, 0.15, 0.15),
        metallic=0.0, roughness=0.8,
    )

    # ── Vải áo quần
    mats['fabric'] = make_pbr_material(
        'Fabric_Cloth',
        albedo=(0.6, 0.15, 0.10),  # màu đỏ đậm, đổi theo sản phẩm
        metallic=0.0, roughness=0.95,
    )
    # Subsurface scattering nhẹ cho vải
    bsdf = mats['fabric'].node_tree.nodes['Principled BSDF']
    bsdf.inputs['Sheen Weight'].default_value = 0.3
    bsdf.inputs['Sheen Roughness'].default_value = 0.5

    return mats


# ════════════════════════════════════════════════════
#  STORE GEOMETRY
# ════════════════════════════════════════════════════
def build_room(mats):
    """Tạo phòng store: sàn, tường, trần."""
    W, D, H = 12.0, 10.0, 3.5   # rộng, sâu, cao (mét)

    # Sàn
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, 0, 0))
    floor = bpy.context.active_object
    floor.name = 'Floor'
    floor.scale = (W/2, D/2, 1)
    bpy.ops.object.transform_apply(scale=True)
    floor.data.materials.append(mats['floor'])

    # Tường sau
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, -D/2, H/2))
    wall_back = bpy.context.active_object
    wall_back.name = 'Wall_Back'
    wall_back.scale = (W/2, H/2, 1)
    wall_back.rotation_euler = (math.radians(90), 0, 0)
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    wall_back.data.materials.append(mats['wall'])

    # Tường trái
    bpy.ops.mesh.primitive_plane_add(size=1, location=(-W/2, 0, H/2))
    wl = bpy.context.active_object
    wl.name = 'Wall_Left'
    wl.scale = (D/2, H/2, 1)
    wl.rotation_euler = (math.radians(90), 0, math.radians(90))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    wl.data.materials.append(mats['wall'])

    # Tường phải
    bpy.ops.mesh.primitive_plane_add(size=1, location=(W/2, 0, H/2))
    wr = bpy.context.active_object
    wr.name = 'Wall_Right'
    wr.scale = (D/2, H/2, 1)
    wr.rotation_euler = (math.radians(90), 0, math.radians(-90))
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    wr.data.materials.append(mats['wall'])

    # Trần
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, 0, H))
    ceiling = bpy.context.active_object
    ceiling.name = 'Ceiling'
    ceiling.scale = (W/2, D/2, 1)
    ceiling.rotation_euler = (math.radians(180), 0, 0)
    bpy.ops.object.transform_apply(scale=True, rotation=True)
    ceiling.data.materials.append(mats['wall'])


def build_shelf(mats, location, scale=(1, 1, 1), num_levels=4):
    """Tạo kệ trưng bày sản phẩm."""
    x, y, z = location
    sw, sd, sh = 1.2 * scale[0], 0.4 * scale[1], 1.8 * scale[2]
    level_h = sh / num_levels

    # Khung kệ dọc
    for dx in [-sw/2 + 0.03, sw/2 - 0.03]:
        bpy.ops.mesh.primitive_cube_add(location=(x + dx, y, z + sh/2))
        post = bpy.context.active_object
        post.scale = (0.03, sd/2, sh/2)
        bpy.ops.object.transform_apply(scale=True)
        post.data.materials.append(mats['shelf_metal'])

    # Tấm kệ ngang
    for i in range(num_levels + 1):
        lz = z + i * level_h
        bpy.ops.mesh.primitive_cube_add(location=(x, y, lz))
        plank = bpy.context.active_object
        plank.scale = (sw/2, sd/2, 0.02)
        bpy.ops.object.transform_apply(scale=True)
        plank.data.materials.append(mats['shelf_wood'])


def build_display_table(mats, location):
    """Bàn trưng bày sản phẩm trung tâm."""
    x, y, z = location
    # Mặt bàn
    bpy.ops.mesh.primitive_cube_add(location=(x, y, z + 0.9))
    top = bpy.context.active_object
    top.name = 'Display_Table_Top'
    top.scale = (0.8, 0.5, 0.03)
    bpy.ops.object.transform_apply(scale=True)
    top.data.materials.append(mats['shelf_wood'])

    # Chân bàn x4
    for dx, dy in [(-0.7, -0.4), (0.7, -0.4), (-0.7, 0.4), (0.7, 0.4)]:
        bpy.ops.mesh.primitive_cube_add(location=(x + dx, y + dy, z + 0.45))
        leg = bpy.context.active_object
        leg.scale = (0.04, 0.04, 0.45)
        bpy.ops.object.transform_apply(scale=True)
        leg.data.materials.append(mats['shelf_metal'])


def load_product_model(filepath, mat, location, rotation=(0,0,0), scale=1.0):
    """Import OBJ model sản phẩm."""
    if not os.path.exists(filepath):
        print(f"[WARN] Model không tồn tại: {filepath}")
        return None

    bpy.ops.wm.obj_import(filepath=filepath)
    obj = bpy.context.selected_objects[0] if bpy.context.selected_objects else None
    if not obj:
        return None

    obj.location = location
    obj.rotation_euler = rotation
    obj.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(scale=True)

    # Gán material
    if obj.data.materials:
        obj.data.materials[0] = mat
    else:
        obj.data.materials.append(mat)
    return obj


# ════════════════════════════════════════════════════
#  LIGHTING — Studio Store Lighting
# ════════════════════════════════════════════════════
def setup_lighting():
    """
    Hệ thống đèn store thực tế:
    - Đèn LED âm trần (Area lights dọc theo trần)
    - Spotlight chiếu sản phẩm
    - Fill light nhẹ cho ambient
    """

    def add_area_light(name, location, rotation, energy, size=(2.0, 0.2), color=(1,1,1)):
        bpy.ops.object.light_add(type='AREA', location=location)
        light = bpy.context.active_object
        light.name = name
        light.rotation_euler = rotation
        light.data.energy = energy
        light.data.size    = size[0]
        light.data.size_y  = size[1]
        light.data.color   = color
        light.data.use_shadow = True
        return light

    def add_spot_light(name, location, direction_point, energy, angle_deg=25, color=(1,0.95,0.85)):
        bpy.ops.object.light_add(type='SPOT', location=location)
        light = bpy.context.active_object
        light.name = name
        # Hướng spot về phía sản phẩm
        dx = direction_point[0] - location[0]
        dy = direction_point[1] - location[1]
        dz = direction_point[2] - location[2]
        import mathutils
        v = mathutils.Vector((dx, dy, dz)).normalized()
        light.rotation_euler = v.to_track_quat('-Z', 'Y').to_euler()
        light.data.energy           = energy
        light.data.spot_size        = math.radians(angle_deg)
        light.data.spot_blend       = 0.3
        light.data.color            = color
        light.data.use_shadow       = True
        light.data.shadow_soft_size = 0.1
        return light

    # Đèn LED trần (dải đèn)
    add_area_light('LED_Ceiling_1', (0, -3, 3.3),  (0,0,0), energy=800, size=(8, 0.15), color=(1, 0.97, 0.9))
    add_area_light('LED_Ceiling_2', (0,  0, 3.3),  (0,0,0), energy=800, size=(8, 0.15), color=(1, 0.97, 0.9))
    add_area_light('LED_Ceiling_3', (0,  3, 3.3),  (0,0,0), energy=800, size=(8, 0.15), color=(1, 0.97, 0.9))

    # Spotlight chiếu sản phẩm (warm white 3000K)
    product_positions = [(-3, -2, 0), (0, -2, 0), (3, -2, 0),
                         (-3,  0, 0), (0,  0, 0), (3,  0, 0)]
    for i, pos in enumerate(product_positions):
        add_spot_light(
            f'Spot_Product_{i}',
            location=(pos[0], pos[1], 3.2),
            direction_point=(pos[0], pos[1], 0),
            energy=500,
            angle_deg=30,
            color=(1.0, 0.92, 0.8)
        )

    # Window light từ cửa sổ (soft blue daylight)
    add_area_light('Window_Light', (-5.9, 0, 1.8),
                   (0, math.radians(90), 0),
                   energy=1500, size=(1.5, 2.0),
                   color=(0.8, 0.9, 1.0))

    # Fill light ngược lại (tránh bóng quá tối)
    add_area_light('Fill_Light', (5, 0, 2.0),
                   (0, math.radians(-90), 0),
                   energy=200, size=(3.0, 2.0),
                   color=(1.0, 0.98, 0.95))


# ════════════════════════════════════════════════════
#  CAMERA
# ════════════════════════════════════════════════════
def setup_camera():
    """Camera góc nhìn người mua hàng (eye level ~1.6m)."""
    bpy.ops.object.camera_add(
        location=(0, 6.5, 1.6),
        rotation=(math.radians(85), 0, math.radians(180))
    )
    cam = bpy.context.active_object
    cam.name = 'Store_Camera'
    cam.data.lens         = 35    # focal length 35mm
    cam.data.dof.use_dof  = True
    cam.data.dof.focus_distance = 6.0
    cam.data.dof.aperture_fstop = 5.6  # depth of field nhẹ

    # Thêm camera product closeup
    bpy.ops.object.camera_add(
        location=(0, 2.5, 1.2),
        rotation=(math.radians(75), 0, math.radians(180))
    )
    cam2 = bpy.context.active_object
    cam2.name = 'Product_Camera'
    cam2.data.lens = 85    # telephoto cho product shot
    cam2.data.dof.use_dof  = True
    cam2.data.dof.focus_distance = 2.0
    cam2.data.dof.aperture_fstop = 2.8  # bokeh rõ

    bpy.context.scene.camera = bpy.data.objects['Store_Camera']


# ════════════════════════════════════════════════════
#  MAIN
# ════════════════════════════════════════════════════
def main():
    reset_scene()
    configure_render()

    # HDRI: tải file HDRI từ https://polyhaven.com/hdris
    hdri_path = os.path.join(ASSET_DIR, "studio.hdr")  # đặt file vào Bai1.trenlop/
    setup_hdri_world(hdri_path)

    mats = create_materials()

    # Phòng store
    build_room(mats)

    # Kệ dọc 2 bên tường
    for i, x in enumerate([-5, -3.5, 3.5, 5]):
        build_shelf(mats, location=(x, -3.5, 0), scale=(1,1,1), num_levels=4)

    # Bàn trưng bày trung tâm
    for x in [-2, 0, 2]:
        build_display_table(mats, location=(x, 0, 0))

    # Load model sản phẩm (từ thư mục Bai1.trenlop)
    shoe_model = os.path.join(ASSET_DIR, "shoe.obj")
    load_product_model(shoe_model, mats['shoe_leather'],
                       location=(0, 0, 0.95), scale=0.3)

    setup_lighting()
    setup_camera()

    # Render
    os.makedirs(os.path.dirname(ARGS.output) if os.path.dirname(ARGS.output) else '.', exist_ok=True)
    bpy.ops.render.render(write_still=True)

    if ARGS.save_blend:
        blend_path = ARGS.output.replace('.png', '.blend')
        bpy.ops.wm.save_as_mainfile(filepath=blend_path)
        print(f"[INFO] Saved blend: {blend_path}")

    print(f"[INFO] Render xong: {ARGS.output}")


if __name__ == "__main__":
    main()
```

---

## Path B: OpenGL Real-time — Nâng cấp `Main.cpp`

### Shader PBR + Shadow + Post-processing

**Thêm vào `shaders/pbr.vert`:**
```glsl
#version 460 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoords;
layout(location=3) in vec3 aTangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;  // cho shadow mapping
    mat3 TBN;                // tangent space → normal map
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos         = worldPos.xyz;
    vs_out.TexCoords       = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    vs_out.TBN    = mat3(T, B, N);
    vs_out.Normal = N;

    gl_Position = projection * view * worldPos;
}
```

**Thêm Shadow Map setup vào `Main.cpp`:**
```cpp
// Trong hàm init():
unsigned int shadowFBO, shadowMap;
const int SHADOW_W = 4096, SHADOW_H = 4096;  // resolution cao = bóng đẹp

glGenFramebuffers(1, &shadowFBO);
glGenTextures(1, &shadowMap);
glBindTexture(GL_TEXTURE_2D, shadowMap);
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
             SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// PCF soften edges
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
glDrawBuffer(GL_NONE);
glReadBuffer(GL_NONE);
glBindFramebuffer(GL_FRAMEBUFFER, 0);

// Ma trận ánh sáng
glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 50.0f);
glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
glm::mat4 lightSpace = lightProjection * lightView;

// Trong render loop:
// Pass 1: render depth từ góc đèn
glViewport(0, 0, SHADOW_W, SHADOW_H);
glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
glClear(GL_DEPTH_BUFFER_BIT);
shadowShader.use();
shadowShader.setMat4("lightSpaceMatrix", lightSpace);
renderScene(shadowShader);
glBindFramebuffer(GL_FRAMEBUFFER, 0);

// Pass 2: render thật với shadow
glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
pbrShader.use();
glActiveTexture(GL_TEXTURE5);
glBindTexture(GL_TEXTURE_2D, shadowMap);
pbrShader.setInt("shadowMap", 5);
pbrShader.setMat4("lightSpaceMatrix", lightSpace);
renderScene(pbrShader);
```

---

## Thứ tự ưu tiên (làm theo thứ tự này)

```
1. [BLENDER] Thêm HDRI lighting          → +50% trông thật
2. [BLENDER] PBR materials đúng          → +20% trông thật  
3. [BLENDER] Area lights thay Point light → +15% trông thật
4. [OPENGL]  Shadow mapping              → +30% trông thật
5. [OPENGL]  PBR shader                  → +25% trông thật
6. [BOTH]    Normal maps cho vật liệu    → +10% chi tiết
7. [BOTH]    Depth of field camera       → cinematic feel
```

## Download PBR Assets miễn phí

| Tài nguyên | Link | Dùng cho |
|-----------|------|---------|
| HDRI maps | polyhaven.com/hdris | World lighting |
| PBR textures | polyhaven.com/textures | Sàn, tường, vật liệu |
| 3D Models | sketchfab.com | Giầy, quần áo, tủ kệ |
| PBR textures | ambientcg.com | Gỗ, kim loại, vải |
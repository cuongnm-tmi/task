import argparse
import math
import os
import sys

import bpy
import mathutils


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SOURCE_DIR = os.path.join(ROOT_DIR, "assets", "source")


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default=os.path.join("renders", "icon_mode_eevee.png"))
    parser.add_argument("--glb-output", default=os.path.join("assets", "models", "icon_mode_store.glb"))
    parser.add_argument("--resolution", nargs=2, type=int, default=[1920, 1080])
    parser.add_argument("--samples", type=int, default=64)
    parser.add_argument("--engine", default="BLENDER_EEVEE_NEXT")
    parser.add_argument("--camera", default="entrance")
    parser.add_argument("--skip-render", action="store_true")
    parser.add_argument("--skip-export", action="store_true")
    parser.add_argument("--save-blend", action="store_true")
    return parser.parse_args(argv)


ARGS = parse_args()


def project_path(path):
    if os.path.isabs(path):
        return path
    return os.path.join(ROOT_DIR, path)


def ensure_parent(path):
    parent = os.path.dirname(os.path.abspath(project_path(path)))
    if parent:
        os.makedirs(parent, exist_ok=True)


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    for image in list(bpy.data.images):
        if not image.users:
            bpy.data.images.remove(image)


def set_principled_input(node, names, value):
    for name in names:
        if name in node.inputs:
            node.inputs[name].default_value = value
            return


def make_material(name, color, roughness=0.65, metallic=0.0, emission=None, emission_strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        set_principled_input(bsdf, ["Base Color"], (*color, 1.0))
        set_principled_input(bsdf, ["Roughness"], roughness)
        set_principled_input(bsdf, ["Metallic"], metallic)
        if emission:
            set_principled_input(bsdf, ["Emission Color", "Emission"], (*emission, 1.0))
            set_principled_input(bsdf, ["Emission Strength"], emission_strength)
    return mat


def create_materials():
    return {
        "concrete": make_material("charcoal concrete wall", (0.13, 0.12, 0.105), 0.94),
        "floor": make_material("large grey stone tile", (0.30, 0.275, 0.235), 0.78),
        "black": make_material("matte black powder coated steel", (0.005, 0.005, 0.004), 0.5, 0.75),
        "wood": make_material("dark walnut shelf", (0.20, 0.105, 0.045), 0.62),
        "amber": make_material("warm amber LED diffuser", (1.0, 0.54, 0.18), 0.35, 0.0, (1.0, 0.48, 0.16), 8.0),
        "logo": make_material("white backlit logo", (0.95, 0.92, 0.86), 0.25, 0.0, (1.0, 0.92, 0.78), 7.5),
        "glass": make_material("smoked glass", (0.04, 0.05, 0.05), 0.08, 0.0),
        "navy": make_material("deep navy fabric", (0.015, 0.025, 0.055), 0.92),
        "cream": make_material("warm cream fabric", (0.72, 0.66, 0.56), 0.9),
        "white": make_material("soft white cotton", (0.84, 0.82, 0.76), 0.88),
        "grey": make_material("mid grey fabric", (0.28, 0.27, 0.25), 0.9),
        "brown": make_material("brown leather", (0.28, 0.12, 0.045), 0.45),
        "plant": make_material("deep green plant", (0.05, 0.22, 0.08), 0.7),
        "skin": make_material("matte mannequin white", (0.78, 0.76, 0.70), 0.7),
        "poster": make_material("monochrome campaign print", (0.70, 0.68, 0.63), 0.82),
    }


def cube(name, loc, scale, mat):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if mat:
        obj.data.materials.append(mat)
    return obj


def bevel(obj, amount=0.02, segments=1):
    mod = obj.modifiers.new("small bevel", "BEVEL")
    mod.width = amount
    mod.segments = segments
    obj.modifiers.new("weighted normals", "WEIGHTED_NORMAL")
    return obj


def cylinder(name, loc, radius, depth, mat, vertices=32, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    if mat:
        obj.data.materials.append(mat)
    return obj


def point_object_at(obj, target):
    direction = mathutils.Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def add_room(mats):
    w, d, h = 9.2, 12.0, 3.6
    bevel(cube("stone tile floor", (0, 0, -0.03), (w, d, 0.06), mats["floor"]), 0.005)
    cube("rear charcoal logo wall", (0, -d / 2, h / 2), (w, 0.12, h), mats["concrete"])
    cube("left concrete wall", (-w / 2, 0, h / 2), (0.12, d, h), mats["concrete"])
    cube("right concrete wall", (w / 2, 0, h / 2), (0.12, d, h), mats["concrete"])
    cube("raw concrete ceiling", (0, 0, h), (w, d, 0.12), mats["concrete"])
    cube("black rear feature wall", (0, -5.91, 1.86), (4.75, 0.045, 2.75), mats["black"])

    for x in [-2.3, 2.3]:
        cube("rear concrete column", (x, -5.95, 1.8), (0.32, 0.20, 3.6), mats["concrete"])

    for x in [-3.2, -1.6, 0.0, 1.6, 3.2]:
        cube("floor grout line long", (x, 0, 0.006), (0.012, d, 0.004), mats["black"])
    for y in [-4.0, -2.0, 0.0, 2.0, 4.0]:
        cube("floor grout line wide", (0, y, 0.007), (w, 0.012, 0.004), mats["black"])

    cube("front black fascia", (0, 6.05, 3.15), (9.2, 0.16, 0.72), mats["black"])
    cube("front fascia warm cove", (0, 5.96, 3.48), (8.6, 0.04, 0.055), mats["amber"])


def add_logo(mats, y=-5.86, z=2.26, scale=1.0, prefix="rear", face="front"):
    rotation_x = math.radians(90 if face == "front" else -90)

    def text_obj(name, body, size, loc):
        bpy.ops.object.text_add(location=loc, rotation=(rotation_x, 0, 0))
        obj = bpy.context.object
        obj.name = name
        obj.data.body = body
        obj.data.align_x = "CENTER"
        obj.data.align_y = "CENTER"
        obj.data.size = size
        obj.data.extrude = 0.018
        obj.data.materials.append(mats["logo"])
        if face == "front":
            obj.scale.x = -1.0
        bpy.ops.object.convert(target="MESH")
        return bpy.context.object

    cube(prefix + " black sign backing", (0, y + (-0.025 if face == "front" else 0.025), z - 0.12 * scale), (3.2 * scale, 0.035, 0.88 * scale), mats["black"])
    text_obj(prefix + " ICON MODE sign", "ICON MODE", 0.48 * scale, (0, y, z))
    text_obj(prefix + " mens wear sign", "MEN'S WEAR", 0.13 * scale, (0, y, z - 0.39 * scale))


def add_reception(mats):
    bevel(cube("black reception counter body", (0, -5.25, 0.55), (3.6, 0.58, 1.1), mats["black"]), 0.025)
    cube("counter black stone top", (0, -5.0, 1.14), (3.8, 0.78, 0.10), mats["black"])
    cube("counter amber backlight", (0, -4.92, 0.93), (3.36, 0.035, 0.08), mats["amber"])
    for i in range(18):
        x = -1.63 + i * 0.192
        cube("counter vertical rib", (x, -4.89, 0.52), (0.045, 0.07, 0.78), mats["wood"])
        cube("counter rib glow", (x, -4.84, 0.50), (0.018, 0.018, 0.70), mats["amber"])
    for x in [-0.55, 0.55]:
        cube("pos monitor", (x, -4.84, 1.43), (0.42, 0.08, 0.28), mats["black"])
        cube("pos stand", (x, -4.91, 1.25), (0.07, 0.07, 0.22), mats["black"])


def add_track_lights(mats):
    for x in [-2.8, 2.8]:
        cube("ceiling track rail", (x, 0.0, 3.42), (0.06, 10.2, 0.05), mats["black"])
        for y in [-4.8, -3.2, -1.6, 0.0, 1.6, 3.2, 4.8]:
            cylinder("black track spotlight", (x, y, 3.18), 0.105, 0.22, mats["black"], 24, (math.radians(90), 0, 0))
            bpy.ops.object.light_add(type="SPOT", location=(x, y, 3.03))
            light = bpy.context.object
            light.name = "warm narrow product spot"
            point_object_at(light, (x * 0.55, y - 0.35, 0.82))
            light.data.energy = 900
            light.data.spot_size = math.radians(28)
            light.data.spot_blend = 0.22
            light.data.color = (1.0, 0.92, 0.78)
            light.data.use_shadow = True

    for x in [-3.6, 3.6]:
        for y in [-3.9, -2.2, -0.5, 1.2, 2.9]:
            bpy.ops.object.light_add(type="AREA", location=(x, y, 2.45))
            light = bpy.context.object
            light.name = "warm shelf wash"
            point_object_at(light, (x, y, 1.35))
            light.data.energy = 95
            light.data.size = 1.1
            light.data.color = (1.0, 0.74, 0.42)

    bpy.ops.object.light_add(type="AREA", location=(0, -4.2, 2.65))
    back = bpy.context.object
    back.name = "rear logo wall softbox"
    point_object_at(back, (0, -5.95, 2.0))
    back.data.energy = 180
    back.data.size = 1.8
    back.data.color = (1.0, 0.90, 0.78)

    bpy.ops.object.light_add(type="AREA", location=(0, 3.8, 2.7))
    fill = bpy.context.object
    fill.name = "soft entrance fill"
    point_object_at(fill, (0, 0.0, 1.1))
    fill.data.energy = 120
    fill.data.size = 4.6
    fill.data.color = (0.75, 0.78, 0.84)


def add_folded_stack(mats, loc, colors):
    x, y, z = loc
    for i, mat in enumerate(colors):
        obj = bevel(cube("folded garment stack", (x, y, z + i * 0.055), (0.48, 0.34, 0.045), mat), 0.025, 2)
        obj.rotation_euler.z = math.radians((i % 2) * 1.8 - 0.9)


def add_shoe_pair(mats, loc, color_mat):
    x, y, z = loc
    for dx in [-0.11, 0.11]:
        shoe = bevel(cube("minimal shoe upper", (x + dx, y, z), (0.18, 0.42, 0.10), color_mat), 0.05, 3)
        toe = bevel(cube("minimal shoe toe", (x + dx, y + 0.16, z + 0.01), (0.20, 0.16, 0.09), color_mat), 0.06, 3)
        sole = bevel(cube("minimal shoe sole", (x + dx, y, z - 0.055), (0.21, 0.45, 0.035), mats["white"]), 0.025, 2)
        shoe.rotation_euler.z = math.radians(dx * 18)
        toe.rotation_euler.z = shoe.rotation_euler.z
        sole.rotation_euler.z = shoe.rotation_euler.z


def add_wall_shelf(mats, side, y, width=1.42):
    x = side * 4.18
    inward = -side
    cube("wall shelf back panel", (x + inward * 0.055, y, 1.48), (0.08, width, 2.45), mats["wood"])
    cube("wall shelf led strip", (x + inward * 0.13, y, 2.72), (0.035, width * 0.90, 0.04), mats["amber"])

    for z in [0.35, 1.28, 2.15, 2.75]:
        cube("side shelf board", (x + inward * 0.35, y, z), (0.62, width, 0.055), mats["wood"])
    for yy in [y - width / 2, y + width / 2]:
        cube("side shelf front post", (x + inward * 0.64, yy, 1.48), (0.045, 0.045, 2.55), mats["black"])
        cube("side shelf rear post", (x + inward * 0.08, yy, 1.48), (0.045, 0.045, 2.55), mats["black"])
    cube("side hanging rail", (x + inward * 0.47, y, 1.78), (0.05, width * 0.82, 0.05), mats["black"])

    palette = [mats["navy"], mats["cream"], mats["grey"], mats["white"], mats["navy"], mats["black"]]
    for i in range(8):
        yy = y - width * 0.34 + i * width * 0.095
        cloth = bevel(cube("hanging menswear garment", (x + inward * 0.49, yy, 1.28), (0.12, width * 0.055, 0.88), palette[(i + int(y * 10)) % len(palette)]), 0.018)
        cloth.rotation_euler.y = math.radians(side * 2.5)

    for i in range(5):
        yy = y - width * 0.27 + i * width * 0.13
        mat = [mats["white"], mats["cream"], mats["navy"], mats["grey"], mats["white"]][(i + int(abs(y) * 10)) % 5]
        jacket = bevel(cube("upper hanging jacket", (x + inward * 0.47, yy, 2.27), (0.15, width * 0.08, 0.54), mat), 0.018)
        jacket.rotation_euler.y = math.radians(side * 2.5)

    add_folded_stack(mats, (x + inward * 0.40, y - width * 0.26, 2.37), [mats["white"], mats["grey"], mats["navy"]])
    add_folded_stack(mats, (x + inward * 0.40, y + width * 0.24, 0.58), [mats["cream"], mats["navy"], mats["black"]])


def add_display_table(mats, loc, size=(1.65, 1.05)):
    x, y = loc
    w, d = size
    cube("display table top black", (x, y, 0.82), (w, d, 0.075), mats["black"])
    cube("display table lower shelf", (x, y, 0.34), (w, d, 0.055), mats["wood"])
    for dx in [-w / 2 + 0.04, w / 2 - 0.04]:
        for dy in [-d / 2 + 0.04, d / 2 - 0.04]:
            cube("display table leg", (x + dx, y + dy, 0.42), (0.055, 0.055, 0.78), mats["black"])
    for dx in [-0.42, 0.0, 0.42]:
        add_folded_stack(mats, (x + dx, y - 0.18, 0.91), [mats["white"], mats["grey"], mats["navy"]])
    add_shoe_pair(mats, (x - 0.45, y + 0.27, 0.44), mats["brown"])
    add_shoe_pair(mats, (x + 0.38, y + 0.27, 0.44), mats["black"])


def add_campaign_poster(mats, side, y):
    x = side * 4.51
    rot = (math.radians(90), 0, math.radians(90 * side))
    cube("poster black frame", (x, y, 2.02), (0.05, 0.90, 1.34), mats["black"])
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x - side * 0.026, y, 2.02), rotation=rot)
    poster = bpy.context.object
    poster.name = "monochrome framed campaign poster"
    poster.dimensions = (0.028, 0.80, 1.18)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    poster.data.materials.append(mats["poster"])
    cube("poster suit silhouette", (x - side * 0.055, y, 1.92), (0.03, 0.22, 0.62), mats["black"])
    cube("poster shirt highlight", (x - side * 0.06, y, 1.98), (0.032, 0.075, 0.36), mats["white"])


def add_plant(mats, loc):
    x, y, z = loc
    cylinder("matte black plant pot", (x, y, z + 0.16), 0.18, 0.32, mats["black"], 28)
    for i in range(12):
        angle = i / 12.0 * math.tau
        length = 0.38 + 0.12 * (i % 3)
        leaf = bevel(cube("long narrow plant leaf", (x + math.cos(angle) * 0.08, y + math.sin(angle) * 0.08, z + 0.44), (0.045, length, 0.018), mats["plant"]), 0.02, 2)
        leaf.rotation_euler = (math.radians(58), 0, angle)


def add_mannequin(mats, loc, facing=0.0):
    x, y, z = loc
    yaw = facing
    group = []
    head = bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=0.16, location=(x, y, z + 1.62))
    obj = bpy.context.object
    obj.name = "faceless mannequin head"
    obj.data.materials.append(mats["skin"])
    group.append(obj)
    group.append(bevel(cube("mannequin torso jacket", (x, y, z + 1.18), (0.48, 0.24, 0.74), mats["navy"]), 0.055, 2))
    group.append(bevel(cube("white inner shirt", (x, y - 0.13, z + 1.20), (0.18, 0.035, 0.60), mats["white"]), 0.018))
    group.append(cube("left trouser leg", (x - 0.12, y, z + 0.46), (0.16, 0.18, 0.82), mats["black"]))
    group.append(cube("right trouser leg", (x + 0.12, y, z + 0.46), (0.16, 0.18, 0.82), mats["black"]))
    group.append(cube("left mannequin arm", (x - 0.35, y, z + 1.08), (0.11, 0.12, 0.70), mats["navy"]))
    group.append(cube("right mannequin arm", (x + 0.35, y, z + 1.08), (0.11, 0.12, 0.70), mats["navy"]))
    group.append(bevel(cube("left white sneaker", (x - 0.12, y - 0.02, z + 0.05), (0.22, 0.36, 0.10), mats["white"]), 0.04, 3))
    group.append(bevel(cube("right white sneaker", (x + 0.12, y - 0.02, z + 0.05), (0.22, 0.36, 0.10), mats["white"]), 0.04, 3))
    for obj in group:
        obj.rotation_euler.z = yaw


def add_reference_store():
    mats = create_materials()
    add_room(mats)
    add_logo(mats, face="front")
    add_logo(mats, y=6.16, z=3.20, scale=0.92, prefix="front", face="front")
    add_reception(mats)
    add_track_lights(mats)

    for side in [-1, 1]:
        for y in [-3.9, -2.2, -0.5, 1.2, 2.9]:
            add_wall_shelf(mats, side, y)
        add_campaign_poster(mats, side, -1.55)
        add_campaign_poster(mats, side, 3.95)

    add_display_table(mats, (-1.45, 1.55), (1.7, 1.15))
    add_display_table(mats, (1.45, 1.10), (1.7, 1.15))
    add_display_table(mats, (0.0, -0.75), (1.8, 1.05))
    add_display_table(mats, (-2.25, -2.55), (1.35, 0.85))
    add_display_table(mats, (2.25, -2.55), (1.35, 0.85))

    add_mannequin(mats, (-3.35, 5.42, 0.0), facing=math.radians(180))
    add_mannequin(mats, (3.35, 5.42, 0.0), facing=math.radians(180))
    add_mannequin(mats, (-3.45, -4.72, 0.0), facing=0.0)
    add_mannequin(mats, (3.45, -4.72, 0.0), facing=0.0)

    add_plant(mats, (-1.35, -4.55, 0.0))
    add_plant(mats, (0.0, 0.65, 0.82))
    add_plant(mats, (3.55, 2.05, 0.0))

    for x in [-3.35, 3.35]:
        bpy.ops.object.light_add(type="SPOT", location=(x, 5.10, 2.65))
        light = bpy.context.object
        light.name = "front mannequin spot"
        point_object_at(light, (x, 5.42, 1.05))
        light.data.energy = 520
        light.data.spot_size = math.radians(34)
        light.data.spot_blend = 0.45
        light.data.color = (1.0, 0.92, 0.80)


def configure_render():
    scene = bpy.context.scene
    try:
        scene.render.engine = ARGS.engine
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in scene.render.bl_rna.properties["engine"].enum_items else "CYCLES"

    scene.render.resolution_x = ARGS.resolution[0]
    scene.render.resolution_y = ARGS.resolution[1]
    scene.render.resolution_percentage = 100
    scene.render.filepath = project_path(ARGS.output)
    scene.render.image_settings.file_format = "PNG"

    if hasattr(scene, "eevee"):
        if hasattr(scene.eevee, "taa_render_samples"):
            scene.eevee.taa_render_samples = ARGS.samples
        if hasattr(scene.eevee, "use_gtao"):
            scene.eevee.use_gtao = True
        if hasattr(scene.eevee, "gtao_distance"):
            scene.eevee.gtao_distance = 3
        if hasattr(scene.eevee, "gtao_factor"):
            scene.eevee.gtao_factor = 1.4

    scene.view_settings.view_transform = "Filmic"
    scene.view_settings.look = "Medium High Contrast"
    scene.view_settings.exposure = -0.45
    scene.view_settings.gamma = 1.0

    world = scene.world or bpy.data.worlds.new("World")
    scene.world = world
    world.color = (0.010, 0.008, 0.006)


def add_camera():
    presets = {
        "storefront": ((0, 10.2, 1.92), (0, -0.65, 1.66), 24),
        "entrance": ((0, 7.1, 1.68), (0, -2.55, 1.58), 25),
        "reception": ((0, 2.85, 1.55), (0, -5.45, 1.70), 32),
        "right_wall": ((-2.75, 4.25, 1.55), (4.25, 0.35, 1.62), 26),
        "left_wall": ((2.75, 4.25, 1.55), (-4.25, 0.35, 1.62), 26),
        "tables": ((0, 3.9, 1.18), (0, -1.0, 0.82), 35),
    }
    location, target, lens = presets.get(ARGS.camera, presets["entrance"])

    bpy.ops.object.camera_add(location=location)
    cam = bpy.context.object
    cam.name = "reference " + ARGS.camera + " camera"
    point_object_at(cam, target)
    cam.data.lens = lens
    cam.data.dof.use_dof = True
    cam.data.dof.focus_distance = (mathutils.Vector(target) - mathutils.Vector(location)).length
    cam.data.dof.aperture_fstop = 7.0
    bpy.context.scene.camera = cam


def export_glb(path):
    path = project_path(path)
    ensure_parent(path)
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLB",
        export_apply=True,
        export_lights=False,
        export_cameras=True,
        export_materials="EXPORT",
    )
    print(f"[icon_mode_scene] exported GLB: {path}")


def render_still(path):
    path = project_path(path)
    ensure_parent(path)
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f"[icon_mode_scene] rendered still: {path}")


def main():
    reset_scene()
    configure_render()
    add_reference_store()
    add_camera()

    if not ARGS.skip_export:
        export_glb(ARGS.glb_output)
    if not ARGS.skip_render:
        render_still(ARGS.output)
    if ARGS.save_blend:
        blend_path = os.path.splitext(project_path(ARGS.output))[0] + ".blend"
        ensure_parent(blend_path)
        bpy.ops.wm.save_as_mainfile(filepath=blend_path)
        print(f"[icon_mode_scene] saved blend: {blend_path}")


if __name__ == "__main__":
    main()

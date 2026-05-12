import argparse
import math
import os
import sys

import bpy
from mathutils import Vector


ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ASSET_DIR = os.path.join(ROOT_DIR, "Bai1.trenlop")


def parse_args():
    parser = argparse.ArgumentParser(description="Render the ICON MODE storefront with Blender Cycles.")
    parser.add_argument("--output", default=os.path.join(ROOT_DIR, "renders", "icon_mode_cycles.png"))
    parser.add_argument("--resolution", nargs=2, type=int, default=(1920, 1080), metavar=("WIDTH", "HEIGHT"))
    parser.add_argument("--samples", type=int, default=256)
    parser.add_argument("--save-blend", default=os.path.join(ROOT_DIR, "renders", "icon_mode_cycles.blend"))

    if "--" in sys.argv:
        return parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    return parser.parse_args([])


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()


def ensure_dir(path):
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)


def set_origin_name(obj, name):
    obj.name = name
    obj.data.name = name + "Mesh" if hasattr(obj.data, "name") else obj.name
    return obj


def set_node_input(node, names, value):
    if not node:
        return
    for name in names:
        if name in node.inputs:
            node.inputs[name].default_value = value
            return


def mat_principled(name, color, roughness=0.55, metallic=0.0, alpha=1.0, emission=None, emission_strength=0.0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    set_node_input(bsdf, ("Base Color",), color)
    set_node_input(bsdf, ("Roughness",), roughness)
    set_node_input(bsdf, ("Metallic",), metallic)
    set_node_input(bsdf, ("Alpha",), alpha)
    if emission:
        set_node_input(bsdf, ("Emission Color", "Emission"), emission)
        set_node_input(bsdf, ("Emission Strength",), emission_strength)
    if alpha < 1.0:
        mat.blend_method = "BLEND"
        mat.use_screen_refraction = True
        mat.show_transparent_back = True
    return mat


def mat_image(name, image_path, fallback_color, roughness=0.62):
    mat = mat_principled(name, fallback_color, roughness)
    if not os.path.exists(image_path):
        return mat

    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    if not bsdf:
        return mat
    tex = nodes.new(type="ShaderNodeTexImage")
    tex.image = bpy.data.images.load(image_path)
    mapping = nodes.new(type="ShaderNodeMapping")
    coord = nodes.new(type="ShaderNodeTexCoord")
    mapping.inputs["Scale"].default_value = (4.0, 4.0, 1.0)
    links.new(coord.outputs["Generated"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], tex.inputs["Vector"])
    if "Base Color" in bsdf.inputs:
        links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


MAT = {}


def create_materials():
    MAT["black_metal"] = mat_principled("powder coated black metal", (0.015, 0.014, 0.013, 1), 0.32, 0.75)
    MAT["dark_panel"] = mat_principled("charcoal facade panels", (0.025, 0.024, 0.023, 1), 0.72)
    MAT["concrete_wall"] = mat_principled("warm concrete plaster", (0.31, 0.28, 0.24, 1), 0.88)
    MAT["black_wall"] = mat_principled("matte black rear wall", (0.012, 0.012, 0.012, 1), 0.8)
    MAT["floor"] = mat_image("large gray stone floor", os.path.join(ASSET_DIR, "floor.jpg"), (0.45, 0.42, 0.36, 1), 0.68)
    MAT["wood"] = mat_image("dark walnut laminate", os.path.join(ASSET_DIR, "wood.jpg"), (0.18, 0.105, 0.055, 1), 0.48)
    MAT["glass"] = mat_principled("soft reflective glass", (0.55, 0.62, 0.70, 0.26), 0.08, 0.0, 0.26)
    MAT["white"] = mat_principled("warm white fabric", (0.86, 0.84, 0.78, 1), 0.78)
    MAT["cream"] = mat_principled("cream folded cotton", (0.65, 0.59, 0.48, 1), 0.82)
    MAT["navy"] = mat_principled("deep navy fabric", (0.03, 0.05, 0.095, 1), 0.83)
    MAT["tan"] = mat_principled("camel wool fabric", (0.38, 0.27, 0.16, 1), 0.84)
    MAT["charcoal"] = mat_principled("charcoal fabric", (0.055, 0.055, 0.052, 1), 0.86)
    MAT["skin"] = mat_principled("matte mannequin resin", (0.78, 0.76, 0.70, 1), 0.62)
    MAT["shoe_black"] = mat_principled("black leather", (0.01, 0.01, 0.01, 1), 0.38, 0.0)
    MAT["shoe_white"] = mat_principled("white leather", (0.84, 0.82, 0.76, 1), 0.36, 0.0)
    MAT["sign_light"] = mat_principled(
        "lit white sign letters",
        (0.92, 0.90, 0.84, 1),
        0.28,
        emission=(1.0, 0.93, 0.78, 1),
        emission_strength=1.8,
    )
    MAT["warm_emit"] = mat_principled(
        "warm led strip",
        (1.0, 0.76, 0.42, 1),
        0.3,
        emission=(1.0, 0.72, 0.38, 1),
        emission_strength=3.0,
    )


def cube(name, loc, scale, mat, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    obj = set_origin_name(bpy.context.object, name)
    obj.dimensions = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if mat:
        obj.data.materials.append(mat)
    if bevel > 0.0:
        mod = obj.modifiers.new("small bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 3
        mod.affect = "EDGES"
        obj.modifiers.new("weighted normals", "WEIGHTED_NORMAL")
    return obj


def cylinder(name, loc, radius, depth, mat, vertices=48, rotation=(0, 0, 0), bevel=False):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc, rotation=rotation)
    obj = set_origin_name(bpy.context.object, name)
    if mat:
        obj.data.materials.append(mat)
    obj.modifiers.new("weighted normals", "WEIGHTED_NORMAL")
    if bevel:
        mod = obj.modifiers.new("edge bevel", "BEVEL")
        mod.width = radius * 0.08
        mod.segments = 2
    return obj


def plane(name, loc, scale, mat, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=loc, rotation=rotation)
    obj = set_origin_name(bpy.context.object, name)
    obj.scale = scale
    if mat:
        obj.data.materials.append(mat)
    return obj


def add_text(name, text, loc, size, mat, align="CENTER", rotation=(math.radians(90), 0, 0), extrude=0.015):
    bpy.ops.object.text_add(location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.name = name + "Curve"
    obj.data.body = text
    obj.data.align_x = align
    obj.data.align_y = "CENTER"
    obj.data.size = size
    obj.data.extrude = extrude
    obj.data.bevel_depth = extrude * 0.18
    obj.data.resolution_u = 16
    if mat:
        obj.data.materials.append(mat)
    return obj


def add_area_light(name, loc, rotation, size, power, color=(1.0, 0.82, 0.56)):
    bpy.ops.object.light_add(type="AREA", location=loc, rotation=rotation)
    light = bpy.context.object
    light.name = name
    light.data.name = name + "Data"
    light.data.size = size
    light.data.energy = power
    light.data.color = color
    return light


def add_spot(name, loc, target, power=450, angle=0.5, blend=0.65):
    bpy.ops.object.light_add(type="SPOT", location=loc)
    light = bpy.context.object
    light.name = name
    light.data.energy = power
    light.data.spot_size = angle
    light.data.spot_blend = blend
    light.data.color = (1.0, 0.84, 0.58)
    direction = Vector(target) - Vector(loc)
    light.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return light


def add_camera():
    bpy.ops.object.camera_add(location=(0.0, -10.8, 2.15), rotation=(math.radians(79.5), 0.0, 0.0))
    camera = bpy.context.object
    camera.name = "centered storefront camera"
    camera.data.lens = 22
    camera.data.sensor_width = 32
    camera.data.dof.use_dof = True
    camera.data.dof.focus_distance = 10.5
    camera.data.dof.aperture_fstop = 8.0
    bpy.context.scene.camera = camera


def build_architecture():
    cube("wide stone floor", (0, 0.2, 0), (13.0, 15.4, 0.08), MAT["floor"], 0.01)
    cube("left concrete wall", (-5.35, 0.6, 2.1), (0.24, 12.4, 4.2), MAT["concrete_wall"], 0.02)
    cube("right concrete wall", (5.35, 0.6, 2.1), (0.24, 12.4, 4.2), MAT["concrete_wall"], 0.02)
    cube("rear matte wall", (0, 5.95, 2.1), (10.7, 0.22, 4.2), MAT["black_wall"], 0.01)
    cube("dark ceiling slab", (0, 0.1, 4.18), (10.9, 12.5, 0.22), MAT["dark_panel"], 0.01)

    cube("front left pier", (-4.95, -5.55, 2.15), (1.0, 0.36, 4.3), MAT["dark_panel"], 0.015)
    cube("front right pier", (4.95, -5.55, 2.15), (1.0, 0.36, 4.3), MAT["dark_panel"], 0.015)
    cube("front overhead sign fascia", (0, -5.62, 4.45), (11.8, 0.45, 1.0), MAT["dark_panel"], 0.015)
    cube("front threshold strip", (0, -5.56, 0.07), (10.8, 0.35, 0.14), MAT["black_metal"], 0.01)
    cube("front warm top light", (0, -5.86, 4.95), (10.7, 0.04, 0.06), MAT["warm_emit"], 0.01)
    add_area_light("long hidden facade light", (0, -5.7, 4.86), (math.radians(90), 0, 0), 8.5, 650)

    add_text("main exterior logo", "ICON MODE", (0, -5.89, 4.55), 0.58, MAT["sign_light"], extrude=0.028)
    add_text("main exterior subtitle", "MEN'S WEAR", (0, -5.90, 4.18), 0.22, MAT["sign_light"], extrude=0.012)

    add_text("rear logo", "ICON MODE", (0, 5.81, 2.98), 0.27, MAT["sign_light"], rotation=(math.radians(90), 0, math.radians(180)), extrude=0.012)
    add_text("rear subtitle", "MEN'S WEAR", (0, 5.80, 2.72), 0.095, MAT["sign_light"], rotation=(math.radians(90), 0, math.radians(180)), extrude=0.006)

    for x in [i * 1.25 for i in range(-5, 6)]:
        cube("subtle vertical tile grout", (x, 0.2, 0.055), (0.012, 12.4, 0.012), MAT["black_metal"], 0.0)
    for y in [i * 1.05 - 5.2 for i in range(12)]:
        cube("subtle horizontal tile grout", (0, y, 0.058), (12.2, 0.012, 0.012), MAT["black_metal"], 0.0)


def build_lighting():
    rail_x = [-3.25, -1.05, 1.05, 3.25]
    rail_y_positions = [-4.3, -3.2, -2.1, -1.0, 0.1, 1.2, 2.3, 3.4]
    for x in rail_x:
        cube("black ceiling track", (x, -0.35, 4.02), (0.055, 9.7, 0.055), MAT["black_metal"], 0.005)
        for y in rail_y_positions:
            cube("track light stem", (x, y, 3.87), (0.035, 0.035, 0.16), MAT["black_metal"], 0.004)
            bpy.ops.mesh.primitive_cone_add(vertices=28, radius1=0.13, radius2=0.09, depth=0.24, location=(x, y, 3.68), rotation=(0, 0, 0))
            head = set_origin_name(bpy.context.object, "angled black spotlight head")
            head.data.materials.append(MAT["black_metal"])
            head.rotation_euler[1] = math.radians(12 if x < 0 else -12)
            add_spot("warm track spotlight", (x, y, 3.55), (x * 0.38, y + 0.4, 0.45), 240, 0.42)

    add_area_light("soft store fill", (0, -0.3, 3.8), (0, 0, 0), 6.0, 260, (1.0, 0.82, 0.64))
    add_spot("counter focused light", (0, 3.7, 3.85), (0, 4.8, 0.9), 420, 0.55)


def build_shelf_unit(side=-1, y=0.0, mode=0):
    x = side * 4.7
    cube("wall bay backing", (x, y, 2.0), (0.12, 1.95, 3.55), MAT["dark_panel"], 0.01)
    cube("walnut low cabinet", (x - side * 0.27, y, 0.42), (0.62, 1.82, 0.75), MAT["wood"], 0.015)
    cube("top shelf walnut", (x - side * 0.30, y, 3.02), (0.58, 1.68, 0.06), MAT["wood"], 0.01)
    cube("mid shelf walnut", (x - side * 0.30, y, 1.02), (0.58, 1.58, 0.055), MAT["wood"], 0.01)
    for dy in [-0.82, 0.82]:
        cube("thin shelf upright", (x - side * 0.62, y + dy, 1.9), (0.035, 0.035, 3.3), MAT["black_metal"], 0.004)
        cube("glass side panel", (x - side * 0.50, y + dy, 1.95), (0.035, 0.02, 2.95), MAT["glass"], 0.002)

    cylinder(
        "hanging rail",
        (x - side * 0.55, y - 0.67, 2.35),
        0.018,
        1.34,
        MAT["black_metal"],
        24,
        rotation=(math.radians(90), 0, 0),
    )

    colors = [MAT["cream"], MAT["white"], MAT["navy"], MAT["tan"], MAT["charcoal"], MAT["white"], MAT["cream"], MAT["navy"]]
    for i in range(8):
        yy = y - 0.58 + i * 0.165
        jacket = cube("hanging shirt jacket", (x - side * 0.45, yy, 1.78), (0.10, 0.12, 0.76), colors[(i + mode) % len(colors)], 0.03)
        jacket.rotation_euler[2] = math.radians(side * 2.0)
        cube("hanger shoulder", (x - side * 0.45, yy, 2.18), (0.035, 0.34, 0.035), MAT["black_metal"], 0.004)

    for level, z in enumerate([1.15, 1.78, 2.42]):
        for i, mat in enumerate([MAT["white"], MAT["navy"], MAT["charcoal"]]):
            cube("folded clothing stack", (x - side * 0.34, y - 0.48 + i * 0.46, z), (0.34, 0.28, 0.075), mat, 0.025)
            cube("folded clothing upper layer", (x - side * 0.33, y - 0.48 + i * 0.46, z + 0.075), (0.30, 0.23, 0.045), mat, 0.025)


def build_center_table(y, variant=0):
    cube("black center table frame", (0, y, 0.43), (2.75, 1.15, 0.08), MAT["black_metal"], 0.01)
    cube("dark center table top", (0, y, 0.86), (2.88, 1.25, 0.08), MAT["wood"], 0.015)
    cube("lower center display shelf", (0, y, 0.23), (2.55, 1.02, 0.055), MAT["black_metal"], 0.008)
    for x in [-1.28, 1.28]:
        for yy in [-0.48, 0.48]:
            cylinder("thin table leg", (x, y + yy, 0.44), 0.025, 0.8, MAT["black_metal"], 18)

    mats = [MAT["white"], MAT["navy"], MAT["charcoal"], MAT["cream"], MAT["tan"]]
    for i in range(5):
        cube("folded front stack", (-0.95 + i * 0.48, y - 0.22, 0.96), (0.36, 0.32, 0.09), mats[(i + variant) % len(mats)], 0.03)
        cube("folded front stack layer", (-0.95 + i * 0.48, y - 0.22, 1.06), (0.31, 0.26, 0.055), mats[(i + variant) % len(mats)], 0.025)
    for i, mat in enumerate([MAT["shoe_white"], MAT["shoe_black"], MAT["tan"], MAT["shoe_black"]]):
        add_shoe_pair((-0.78 + i * 0.52, y + 0.35, 0.36), mat)


def import_obj(path, name, loc, scale, rotation=(0, 0, 0), material=None):
    if not os.path.exists(path):
        return None
    before = set(bpy.context.scene.objects)
    if hasattr(bpy.ops.wm, "obj_import"):
        bpy.ops.wm.obj_import(filepath=path)
    else:
        bpy.ops.import_scene.obj(filepath=path)
    imported = [obj for obj in bpy.context.scene.objects if obj not in before]
    if not imported:
        return None
    root = imported[0]
    for obj in imported:
        obj.name = name if obj == root else f"{name}_{obj.name}"
        obj.location = loc
        obj.scale = scale
        obj.rotation_euler = rotation
        if material and hasattr(obj.data, "materials"):
            obj.data.materials.clear()
            obj.data.materials.append(material)
        obj.modifiers.new("weighted normals", "WEIGHTED_NORMAL")
    return imported


def add_shoe_pair(loc, mat):
    shoe_path = os.path.join(ASSET_DIR, "shoe_store.obj")
    imported = import_obj(shoe_path, "imported shoe pair", loc, (0.22, 0.22, 0.22), rotation=(0, 0, math.radians(90)), material=mat)
    if imported:
        return imported
    for dx in [-0.12, 0.12]:
        cube("procedural shoe sole", (loc[0] + dx, loc[1], loc[2]), (0.18, 0.42, 0.055), mat, 0.035)
        cube("procedural shoe upper", (loc[0] + dx, loc[1] - 0.02, loc[2] + 0.055), (0.15, 0.28, 0.08), mat, 0.045)


def add_mannequin(loc, side=1, jacket_mat=None):
    mannequin_path = os.path.join(ASSET_DIR, "mannequin_store.obj")
    imported = import_obj(mannequin_path, "front mannequin", loc, (1.0, 1.0, 1.0), rotation=(0, 0, math.radians(side * 2.0)))
    if imported:
        return imported

    jacket_mat = jacket_mat or MAT["charcoal"]
    cylinder("mannequin base", (loc[0], loc[1], 0.04), 0.34, 0.08, MAT["black_metal"], 48)
    cylinder("left leg", (loc[0] - 0.12, loc[1], 0.55), 0.055, 0.95, MAT["charcoal"], 22)
    cylinder("right leg", (loc[0] + 0.12, loc[1], 0.55), 0.055, 0.95, MAT["charcoal"], 22)
    cube("white shirt torso", (loc[0], loc[1], 1.42), (0.34, 0.16, 0.56), MAT["white"], 0.08)
    cube("dark jacket torso", (loc[0], loc[1] - 0.015, 1.42), (0.46, 0.14, 0.64), jacket_mat, 0.09)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=0.16, location=(loc[0], loc[1], 1.86))
    head = set_origin_name(bpy.context.object, "mannequin head")
    head.scale.z = 1.18
    head.data.materials.append(MAT["skin"])
    add_shoe_pair((loc[0], loc[1] - 0.02, 0.11), MAT["shoe_white"])


def build_posters_and_plants():
    for side in [-1, 1]:
        x = side * 4.72
        cube("black framed poster", (x, -3.65, 2.15), (0.08, 0.82, 1.35), MAT["black_metal"], 0.015)
        cube("poster paper", (x - side * 0.045, -3.65, 2.15), (0.025, 0.68, 1.16), MAT["white"], 0.008)
        cube("poster silhouette body", (x - side * 0.06, -3.65, 2.03), (0.018, 0.20, 0.54), MAT["charcoal"], 0.01)
        bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=0.11, location=(x - side * 0.06, -3.65, 2.43))
        bpy.context.object.data.materials.append(MAT["charcoal"])

    for x, y, size in [(-1.45, -0.95, 0.7), (4.55, -3.1, 0.42), (-0.75, 4.6, 0.45)]:
        cube("square planter", (x, y, 0.17), (0.32 * size, 0.32 * size, 0.34 * size), MAT["black_metal"], 0.02)
        cylinder("thin plant stem", (x, y, 0.53 * size), 0.018 * size, 0.62 * size, MAT["wood"], 12)
        for i in range(9):
            angle = i * 2.399
            leaf_x = x + math.cos(angle) * 0.16 * size
            leaf_y = y + math.sin(angle) * 0.16 * size
            bpy.ops.mesh.primitive_uv_sphere_add(segments=18, ring_count=8, radius=0.09 * size, location=(leaf_x, leaf_y, 0.70 * size + 0.08 * (i % 3)))
            leaf = bpy.context.object
            leaf.name = "soft green plant leaf"
            leaf.scale = (0.42, 0.15, 1.0)
            leaf.rotation_euler = (math.radians(60), 0, angle)
            leaf.data.materials.append(mat_principled("plant leaf green" + str(i), (0.12, 0.36 + 0.03 * (i % 3), 0.08, 1), 0.7))


def build_scene():
    build_architecture()
    build_lighting()
    for y, mode in [(-3.7, 0), (-1.45, 1), (0.85, 0), (3.15, 1)]:
        build_shelf_unit(-1, y, mode)
        build_shelf_unit(1, y, mode + 1)
    build_center_table(-2.15, 0)
    build_center_table(-0.25, 1)
    build_center_table(1.55, 2)
    cube("black ottoman left", (-0.62, 3.05, 0.36), (0.82, 0.62, 0.28), MAT["charcoal"], 0.07)
    cube("black ottoman right", (0.62, 3.05, 0.36), (0.82, 0.62, 0.28), MAT["charcoal"], 0.07)
    cube("rear checkout counter", (0, 4.95, 0.58), (3.6, 0.78, 1.0), MAT["black_metal"], 0.02)
    cube("counter wood slats panel", (0, 4.53, 0.55), (3.35, 0.08, 0.72), MAT["wood"], 0.01)
    for x in [i * 0.16 - 1.55 for i in range(20)]:
        cube("counter vertical black groove", (x, 4.48, 0.55), (0.022, 0.04, 0.72), MAT["black_metal"], 0.002)
    add_mannequin((-4.05, -4.2, 0.08), -1, MAT["charcoal"])
    add_mannequin((4.05, -4.2, 0.08), 1, MAT["navy"])
    build_posters_and_plants()


def configure_render(args):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.cycles.max_bounces = 8
    scene.cycles.diffuse_bounces = 4
    scene.cycles.glossy_bounces = 4
    scene.cycles.transparent_max_bounces = 6

    scene.render.resolution_x = args.resolution[0]
    scene.render.resolution_y = args.resolution[1]
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new("dark warm studio world") if not scene.world else scene.world
    scene.world.color = (0.015, 0.013, 0.012)

    try:
        scene.view_settings.view_transform = "Filmic"
        scene.view_settings.look = "Medium High Contrast"
    except TypeError:
        pass
    scene.view_settings.exposure = -0.25
    scene.view_settings.gamma = 1.0

    scene.render.filepath = args.output


def main():
    args = parse_args()
    clear_scene()
    create_materials()
    build_scene()
    add_camera()
    configure_render(args)

    ensure_dir(args.output)
    ensure_dir(args.save_blend)
    if args.save_blend:
        bpy.ops.wm.save_as_mainfile(filepath=args.save_blend)
    bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main()

"""P8: NASA star map + moon phase sprites -> /Game/Ironwake/Sky, M_IW_NightSky (unlit sky material, evaluated analytically per pixel).
Run headless: IWTEST_SCRIPT=iw_p8_sky UnrealEditor IronwakeTest.uproject -log. Idempotent (new asset names are replaced via delete+create)."""
import unreal, os
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SRC = os.path.join(proj, "build/assets/p8/sky")
DEST = "/Game/Ironwake/Sky"
MEL = unreal.MaterialEditingLibrary; AT = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
L = lambda m: unreal.log("IWP8SKY " + str(m))

def imp(fn, srgb=True, mx=8192):
    name = os.path.splitext(os.path.basename(fn))[0]
    t = unreal.AssetImportTask(); t.filename = fn; t.destination_path = DEST; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset("%s/%s" % (DEST, name))
    tex.set_editor_property("srgb", srgb); tex.set_editor_property("max_texture_size", mx)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP if False else unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP); tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    EAL.save_asset("%s/%s" % (DEST, name)); L("tex %s %s" % (name, tex.blueprint_get_size_x()))
    return tex
star = imp(os.path.join(SRC, "T_IW_StarMap8k.png"))
def imp_glow():
    fn = os.path.join(SRC, "T_IW_StarGlow.exr"); name = "T_IW_StarGlow"
    t = unreal.AssetImportTask(); t.filename = fn; t.destination_path = DEST; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    AT.import_asset_tasks([t])
    tex = unreal.load_asset("%s/%s" % (DEST, name))
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP); tex.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    EAL.save_asset("%s/%s" % (DEST, name)); L("glow tex %s %s" % (tex.get_class().get_name(), tex.blueprint_get_size_x()))
    return tex
glowtex = imp_glow()
moons = [imp(os.path.join(SRC, "T_IW_MoonPhase%d.png" % i), True, 512) for i in range(8)]

path = DEST + "/M_IW_NightSky"
if EAL.does_asset_exist(path): EAL.delete_asset(path)
m = AT.create_asset("M_IW_NightSky", DEST, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("translucency_sort_priority", -100) if False else None
m.set_editor_property("two_sided", True)
E = lambda c, x, y: MEL.create_material_expression(m, c, x, y)
def V(name, y, d=(0, 0, 0, 0)):
    e = E(unreal.MaterialExpressionVectorParameter, -600, y); e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*d)); return e
def S(name, y, d):
    e = E(unreal.MaterialExpressionScalarParameter, -600, y); e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", d); return e
wp = E(unreal.MaterialExpressionWorldPosition, -600, 0)
cam = E(unreal.MaterialExpressionCameraPositionWS, -600, 60)
r0 = V("R0", 120, (1, 0, 0, 0)); r1 = V("R1", 180, (0, 1, 0, 0)); r2 = V("R2", 240, (0, 0, 1, 0)); md = V("MoonDir", 300, (1, 0, 0, 0))
sg = S("StarGain", 360, 8.0); fd = S("StarFade", 420, 1.0); mc = S("MoonCos", 480, 0.9999); mg = S("MoonGain", 540, 30.0); mf = S("MoonFade", 600, 1.0); mip = S("Mip", 660, 0.5); pth = S("PtThresh", 880, 0.12); ptg = S("PtGain", 940, 1.6); glg = S("GlowGain", 1000, 0.03); glm = S("GlowMip", 1060, 4.0)
st = E(unreal.MaterialExpressionTextureObjectParameter, -600, 720); st.set_editor_property("parameter_name", "StarTex"); st.set_editor_property("texture", star)
gt = E(unreal.MaterialExpressionTextureObjectParameter, -600, 860); gt.set_editor_property("parameter_name", "GlowTex"); gt.set_editor_property("texture", glowtex)
mt = E(unreal.MaterialExpressionTextureObjectParameter, -600, 800); mt.set_editor_property("parameter_name", "MoonTex"); mt.set_editor_property("texture", moons[4])
cu = E(unreal.MaterialExpressionCustom, -200, 300)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property("description", "IW_NightSky")
names = ["WP", "Cam", "R0", "R1", "R2", "MoonDir", "StarGain", "StarFade", "MoonCos", "MoonGain", "MoonFade", "Mip", "PtThresh", "PtGain", "GlowGain", "GlowMip", "StarTex", "GlowTex", "MoonTex"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", r"""
float3 d = normalize(WP - Cam);
float3 q = float3(dot(R0.xyz, d), dot(R1.xyz, d), dot(R2.xyz, d));
float al = atan2(q.y, q.x);
float de = asin(clamp(q.z, -1.0, 1.0));
float2 uv = float2(0.5 - al / 6.2831853, 0.5 - de / 3.14159265);
float3 s0 = Texture2DSampleLevel(StarTex, StarTexSampler, uv, 0).rgb;
float3 gl = Texture2DSampleLevel(GlowTex, GlowTexSampler, uv, 0).rgb;
// naked-eye look: only the brighter stars as points (threshold), the unresolved ones as the soft Milky Way glow
float3 pts = max(s0 - PtThresh, 0.0) * PtGain;
float3 s = pts + gl * GlowGain;
float ext = saturate(d.z / 0.12);
ext = ext * ext * (3.0 - 2.0 * ext);
float3 col = s * StarGain * StarFade * ext;
float3 M = normalize(MoonDir.xyz);
float c = dot(d, M);
if (c > MoonCos)
{
    float3 Rt = normalize(cross(float3(0, 0, 1), M));
    float3 Up = cross(M, Rt);
    float r = sqrt(max(1.0 - MoonCos * MoonCos, 1e-9));
    float2 p = float2(dot(d, Rt), dot(d, Up)) / r;
    float2 muv = float2(p.x * 0.5 + 0.5, 0.5 - p.y * 0.5);
    float4 mo = Texture2DSampleLevel(MoonTex, MoonTexSampler, muv, 0);
    float disc = saturate((1.0 - length(p)) * 20.0);
    float mex = saturate((M.z + 0.02) / 0.1);
    col = col * (1.0 - disc * mo.a) + mo.rgb * mo.a * disc * MoonGain * MoonFade * mex;
}
return col;
""")
srcs = [wp, cam, r0, r1, r2, md, sg, fd, mc, mg, mf, mip, pth, ptg, glg, glm, st, gt, mt]
for n, s in zip(names, srcs):
    if not MEL.connect_material_expressions(s, "", cu, n): L("connect %s failed" % n)
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m); EAL.save_asset(path); L("material saved")
unreal.SystemLibrary.quit_editor()

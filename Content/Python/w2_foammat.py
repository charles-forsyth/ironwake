"""W2: whitecaps + wind streaks. Duplicate the Water plugin material chain into /Game/Ironwake/Water and blend a white foam
layer (crest height from WPO, crest slope, wind-aligned streaks, noise breakup) into the material attributes.
MI_Ocean_Storm is reparented onto the copy. Idempotent: rebuilds the copies each run."""
import unreal
L = lambda m: unreal.log("IWFM " + str(m))
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
D = "/Game/Ironwake/Water/"
def dup(src, dst):
    if EAL.does_asset_exist(dst): EAL.delete_asset(dst)
    a = EAL.duplicate_loaded_asset(unreal.load_asset(src), dst); L(f"dup {src} -> {dst}: {a}"); return a
root = dup("/Water/Materials/WaterSurface/Water_Material", D + "M_IW_WaterRoot")
inst = dup("/Water/Materials/WaterSurface/Water_Material_Inst", D + "MI_IW_WaterInst")
ocn = dup("/Water/Materials/WaterSurface/Water_Material_Ocean", D + "MI_IW_WaterOcean")
inst.set_editor_property("parent", root); MEL.update_material_instance(inst)
ocn.set_editor_property("parent", inst); MEL.update_material_instance(ocn)
m = root
P = unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES
orig = MEL.get_material_property_input_node(m, P)
L(f"orig attrs node {orig.get_name()}")
X0, Y0 = -2600, 1200
def E(cls, x, y):
    return MEL.create_material_expression(m, cls, X0 + x, Y0 + y)
wp = E(unreal.MaterialExpressionWorldPosition, 0, 300)
wp0 = E(unreal.MaterialExpressionWorldPosition, 0, 400)
wp0.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
cam = E(unreal.MaterialExpressionCameraPositionWS, 0, 500)
tm = E(unreal.MaterialExpressionTime, 0, 600)
tex = E(unreal.MaterialExpressionTextureObject, 0, 700)
tex.set_editor_property("texture", unreal.load_asset("/Water/Textures/Foam/T_WaterFlow_03_Foam_Tiled"))
def SP(name, v, y):
    e = E(unreal.MaterialExpressionScalarParameter, 200, y); e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); e.set_editor_property("group", "IW Foam"); return e
hs = SP("IW_Hs", 2.0, 800); wc = SP("IW_Whitecap", 0.3, 900); st = SP("IW_Streak", 0.0, 1000); dbg = SP("IW_Debug", 0.0, 1300); plz = SP("IW_PlaneZ", -512.0, 1500);
camabs = E(unreal.MaterialExpressionVectorParameter, 200, 1600); camabs.set_editor_property("parameter_name", "IW_CamPos"); camabs.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 0)); camabs.set_editor_property("group", "IW Foam")
ck = SP("IW_CrestK", 0.25, 1100)
wt = SP("IW_WaterTime", 0.0, 1700)
wv = []
for i in range(8):
    e = E(unreal.MaterialExpressionVectorParameter, 200, 1800 + i * 60); e.set_editor_property("parameter_name", f"IW_Wave{i}"); e.set_editor_property("default_value", unreal.LinearColor(1, 0, 10000, 0)); e.set_editor_property("group", "IW Foam"); wv.append(e)
wind = E(unreal.MaterialExpressionVectorParameter, 200, 1200); wind.set_editor_property("parameter_name", "IW_Wind"); wind.set_editor_property("default_value", unreal.LinearColor(1, 0, 0, 0)); wind.set_editor_property("group", "IW Foam")
cu = E(unreal.MaterialExpressionCustom, 600, 600)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("description", "IW_Foam")
names = ["WP", "WP0", "Cam", "Time", "Tex", "Hs", "Whitecap", "Streak", "CrestK", "Wind", "Dbg", "PlaneZ", "CamAbs", "WT", "W0", "W1", "W2", "W3", "W4", "W5", "W6", "W7"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", r"""
// The water vertex factory bakes the Gerstner displacement into the position (WP0 == WP), so crest height is measured
// against the ocean plane (IW_PlaneZ, set from C++). Hs in metres.
float hsc = max(Hs * 100.0, 40.0);
// WorldPosition/CameraPosition reach a Custom node camera-translated (LWC); rebuild absolute position from the camera
// position sent each frame by C++ (IW_CamPos).
float3 AP = (WP - Cam) + CamAbs.xyz;
// The water mesh's Gerstner displacement is NOT in the pixel shader's world position (measured: flat). Evaluate the 8
// dominant waves (dir.xy, wavelength cm, amplitude cm) analytically at this pixel, same phase as the plugin/FastZ.
float h = 0;
float4 Wv[8] = { W0, W1, W2, W3, W4, W5, W6, W7 };
[unroll] for (int i = 0; i < 8; ++i) { float k = 6.2831853 / max(Wv[i].z, 1.0); h += Wv[i].w * cos(dot(AP.xy, Wv[i].xy) * k - sqrt(k * 980.0) * WT); }
float crest = saturate((h - CrestK * hsc) / (0.22 * hsc));
float slope = 0.2;
#if PIXELSHADER
float3 n = normalize(cross(ddx(WP), ddy(WP)));
slope = 1.0 - abs(n.z);
#endif
float2 xy = AP.xy;
float2 wd = normalize(Wind.xy + float2(1e-4, 0));
float2 pd = float2(-wd.y, wd.x);
float t1 = Texture2DSampleLevel(Tex, TexSampler, xy / 1800.0 + Time * 0.012 * wd, 0).r;
float t2 = Texture2DSampleLevel(Tex, TexSampler, xy / 640.0 - Time * 0.017 * pd, 0).r;
float br = saturate(t1 * 0.6 + t2 * 0.6);
// whitecaps: on and just downwind of crests, steeper = more, broken up by foam texture; solid cores on the highest crests
float w = crest * (0.6 + 2.5 * saturate(slope * 2.0)) * Whitecap;
float caps = saturate((w - (1.0 - br) * 0.9) * 2.5);
caps = max(caps, saturate((crest - 0.7) * 3.0) * Whitecap * br * 1.5);
// wind streaks (GALE/STORM): sparse long thin lanes along the wind, mostly on the backs/troughs between caps
float2 suv = float2(dot(xy, wd) / 14000.0, dot(xy, pd) / 300.0);
float s1 = Texture2DSampleLevel(Tex, TexSampler, suv + float2(-Time * 0.004, 0), 0).r;
float s2 = Texture2DSampleLevel(Tex, TexSampler, float2(dot(xy, wd) / 5200.0, dot(xy, pd) / 2400.0) + 0.37, 0).r;
float streak = saturate((s1 - 0.5) * 3.0) * saturate((s2 - 0.25) * 2.5) * Streak;
float d = length(WP - Cam);
float fade = 1.0 - saturate((d - 80000.0) / 120000.0);
float foam = saturate(caps + streak * 0.8) * fade;
if (Dbg > 0.5 && Dbg < 1.5) foam = 1;
if (Dbg > 1.5 && Dbg < 2.5) foam = crest;
if (Dbg > 2.5 && Dbg < 3.5) foam = saturate(slope * 3);
if (Dbg > 3.5 && Dbg < 4.5) foam = frac(abs(h) / 300.0);
if (Dbg > 4.5 && Dbg < 5.5) foam = saturate((h + 1500.0) / 3000.0);
if (Dbg > 5.5 && Dbg < 6.5) foam = frac(AP.x / 5000.0);
if (Dbg > 6.5 && Dbg < 7.5) foam = h > 0 ? 1 : 0;
if (Dbg > 7.5 && Dbg < 8.5) foam = saturate(Hs / 20.0);
if (Dbg > 8.5 && Dbg < 9.5) foam = h > 300 ? 1 : 0;
if (Dbg > 9.5 && Dbg < 10.5) foam = saturate(-WP.z / 20000.0);
if (Dbg > 10.5 && Dbg < 11.5) foam = saturate(Cam.z / 20000.0);
if (Dbg > 11.5 && Dbg < 12.5) foam = saturate(CamAbs.z / 20000.0);
if (Dbg > 12.5) foam = saturate((h + 3000.0) / 6000.0);
return foam;
""")


srcs = [wp, wp0, cam, tm, tex, hs, wc, st, ck, wind, dbg, plz, camabs, wt] + wv
for n, s in zip(names, srcs):
    ok = MEL.connect_material_expressions(s, "", cu, n); 
    if not ok: L(f"connect {n} failed")
# foam layer B
mk = E(unreal.MaterialExpressionMakeMaterialAttributes, 1000, 300)
white = E(unreal.MaterialExpressionConstant3Vector, 800, 200); white.set_editor_property("constant", unreal.LinearColor(0.9, 0.93, 0.95, 1))
one = E(unreal.MaterialExpressionConstant, 800, 260); one.set_editor_property("r", 1.0)
rough = E(unreal.MaterialExpressionConstant, 800, 320); rough.set_editor_property("r", 0.75)
spec = E(unreal.MaterialExpressionConstant, 800, 380); spec.set_editor_property("r", 0.3)
for s, pin in [(white, "BaseColor"), (one, "Opacity"), (rough, "Roughness"), (spec, "Specular")]:
    if not MEL.connect_material_expressions(s, "", mk, pin): L(f"mk {pin} failed")
# Storm light is dark: matte white foam alone renders darker than the specular water. Foam gets a small emissive lift
# (IW_FoamGlow, set per sea state from C++) so it reads as the brightest thing on the sea, as in real storm footage.
glow = SP("IW_FoamGlow", 0.4, 1400)
gm = E(unreal.MaterialExpressionMultiply, 900, 440)
MEL.connect_material_expressions(white, "", gm, "A"); MEL.connect_material_expressions(glow, "", gm, "B")
MEL.connect_material_expressions(gm, "", mk, "EmissiveColor")
bl = E(unreal.MaterialExpressionBlendMaterialAttributes, 1300, 0)
try:
    bl.set_editor_property("vertex_attribute_blend_type", unreal.MaterialAttributeBlend.USE_A)
    bl.set_editor_property("pixel_attribute_blend_type", unreal.MaterialAttributeBlend.BLEND)
except Exception as e: L(f"blend types {e}")
for a_, b_ in [(orig, "A"), (mk, "B"), (cu, "Alpha")]:
    L(f"bl {b_} {MEL.connect_material_expressions(a_, '', bl, b_)}")
L(f"bl types {bl.get_editor_property('pixel_attribute_blend_type')} {bl.get_editor_property('vertex_attribute_blend_type')}")
MEL.connect_material_property(bl, "", P)
MEL.recompile_material(m)
for a in [root, inst, ocn]: EAL.save_loaded_asset(a)
storm = unreal.load_asset(D + "MI_Ocean_Storm")
storm.set_editor_property("parent", ocn)
MEL.update_material_instance(storm)
EAL.save_loaded_asset(storm)
L(f"storm parent now {storm.get_editor_property('parent').get_path_name()}")
L("done")

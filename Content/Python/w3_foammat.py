"""W3: upgrade the IW_Foam Custom node in M_IW_WaterRoot IN PLACE (no deletes: deleting referenced water assets crashed the
editor before). 16 analytic Gerstner waves (dir*(1+steepness), wavelength, amplitude), Jacobian whitecaps with a C++-calibrated
threshold (IW_JT -> target coverage), lace = the same Jacobian at two past times (foam left behind moving crests), wind streaks
x3 density. Run in the live editor: python3 build/tools/uepy.py Content/Python/w3_foammat.py, or headless IWTEST_SCRIPT=w3_foammat."""
import unreal, os
L = lambda m: unreal.log("IWW3M " + str(m))
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
m = unreal.load_asset("/Game/Ironwake/Water/M_IW_WaterRoot")
exprs = MEL.get_material_expressions(m)
cu = None; params = {}; others = {}
for e in exprs:
    if isinstance(e, unreal.MaterialExpressionCustom) and e.get_editor_property("description") == "IW_Foam": cu = e
    elif isinstance(e, (unreal.MaterialExpressionScalarParameter, unreal.MaterialExpressionVectorParameter)):
        params[str(e.get_editor_property("parameter_name"))] = e
    elif isinstance(e, unreal.MaterialExpressionWorldPosition): others.setdefault("wp", e)
    elif isinstance(e, unreal.MaterialExpressionCameraPositionWS): others["cam"] = e
    elif isinstance(e, unreal.MaterialExpressionTime): others["time"] = e
    elif isinstance(e, unreal.MaterialExpressionTextureObject): others["tex"] = e
assert cu, "IW_Foam custom node not found"
L(f"found custom node; params={sorted(params)}")
X0, Y0 = int(cu.get_editor_property("material_expression_editor_x")), int(cu.get_editor_property("material_expression_editor_y"))
def SP(name, v):
    if name in params: return params[name]
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, X0 - 400, Y0 + 1400 + len(params) * 60)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); e.set_editor_property("group", "IW Foam")
    params[name] = e; return e
def VP(name, v):
    if name in params: return params[name]
    e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, X0 - 400, Y0 + 1400 + len(params) * 60)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); e.set_editor_property("group", "IW Foam")
    params[name] = e; return e
for i in range(16): VP(f"IW_Wave{i}", unreal.LinearColor(1, 0, 10000, 0))
SP("IW_JT", 0.75); SP("IW_Lace", 0.6); SP("IW_LaceT", 2.0); SP("IW_FoamCov", 0.2)
srcs = [("WP", others["wp"]), ("Cam", others["cam"]), ("Time", others["time"]), ("Tex", others["tex"]),
        ("Hs", params["IW_Hs"]), ("Whitecap", params["IW_Whitecap"]), ("Streak", params["IW_Streak"]), ("JT", params["IW_JT"]),
        ("Wind", params["IW_Wind"]), ("Dbg", params["IW_Debug"]), ("CamAbs", params["IW_CamPos"]), ("WT", params["IW_WaterTime"]),
        ("Lace", params["IW_Lace"]), ("LaceT", params["IW_LaceT"])] + [(f"W{i}", params[f"IW_Wave{i}"]) for i in range(16)]
ins = []
for n, _ in srcs:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", r"""
// W3 whitecaps. Pixel WorldPosition carries no Gerstner displacement and arrives camera-relative (LWC): rebuild the absolute
// position from IW_CamPos and evaluate 16 waves analytically (same phase as the plugin / FastZ). Wn = (dir*(1+steep), wl cm, amp cm).
float3 AP = (WP - Cam) + CamAbs.xyz;
float4 Wv[16] = { W0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W10, W11, W12, W13, W14, W15 };
float h = 0, J = 1, J1 = 1, J2 = 1;
[unroll] for (int i = 0; i < 16; ++i)
{
    float ln = max(length(Wv[i].xy), 1e-3);
    float q = max(ln - 1.0, 0.0);
    float2 d = Wv[i].xy / ln;
    float k = 6.2831853 / max(Wv[i].z, 1.0);
    float w = sqrt(k * 980.0);
    float ph = dot(AP.xy, d) * k - w * WT;
    float c = cos(ph);
    h += Wv[i].w * c;
    J -= q * c;
    J1 -= q * cos(ph + w * LaceT);          // the surface LaceT s ago: foam left behind by a crest that has moved on
    J2 -= q * cos(ph + w * LaceT * 2.2);
}
float2 xy = AP.xy;
float2 wd = normalize(Wind.xy + float2(1e-4, 0));
float2 pd = float2(-wd.y, wd.x);
float al = dot(xy, wd), ac = dot(xy, pd);
// bubbly detail (fine, two scales, drifting) so foam is lace, not flat paint
float d1 = Texture2DSampleLevel(Tex, TexSampler, xy / 380.0 + Time * 0.02 * wd, 0).r;
float d2 = Texture2DSampleLevel(Tex, TexSampler, xy / 1100.0 - Time * 0.011 * pd, 0).r;
float det = saturate(d1 * 0.65 + d2 * 0.65);
// breakup stretched ALONG the crest (across the wind): caps become crest-line segments, not round blobs
float b1 = Texture2DSampleLevel(Tex, TexSampler, float2(ac / 5200.0, al / 1300.0) + Time * 0.01 * float2(0, 1), 0).r;
float cap = saturate((JT - J) / 0.12);
float core = saturate((JT - 0.08 - J) / 0.08);
float caps = saturate(cap * (0.35 + 1.2 * b1)) * (0.45 + 0.55 * det);
caps = max(caps, core * (0.7 + 0.3 * det));
caps *= Whitecap;
// lace: decaying foam left behind where crests broke 2-4 s ago, as a net (detail-thresholded)
float l1 = saturate((JT + 0.04 - J1) / 0.16), l2 = saturate((JT + 0.08 - J2) / 0.2);
float lace = saturate(max(l1 * 0.8, l2 * 0.5) * saturate((det - 0.45) * 3.0)) * Lace * Whitecap;
// wind streaks (Beaufort 8+: dense white streaks along the wind): 3-8 m wide, 60-200 m long, gated in lanes
float s1 = Texture2DSampleLevel(Tex, TexSampler, float2(al / 16000.0 - Time * 0.003, ac / 520.0), 0).r;
float s2 = Texture2DSampleLevel(Tex, TexSampler, float2(al / 7000.0, ac / 2600.0) + 0.37, 0).r;
float streak = saturate((s1 - 0.42) * 2.6) * saturate((s2 - 0.3) * 2.5) * (0.5 + 0.5 * det) * Streak;
float dist = length(WP - Cam);
float fade = 1.0 - saturate((dist - 90000.0) / 140000.0);
float foam = saturate(caps + lace + streak * 0.85) * fade;
if (Dbg > 0.5 && Dbg < 1.5) foam = 1;
if (Dbg > 1.5 && Dbg < 2.5) foam = cap;
if (Dbg > 2.5 && Dbg < 3.5) foam = saturate(1.0 - J);
if (Dbg > 3.5 && Dbg < 4.5) foam = frac(abs(h) / 300.0);
if (Dbg > 4.5 && Dbg < 5.5) foam = lace;
if (Dbg > 5.5 && Dbg < 6.5) foam = streak;
if (Dbg > 12.5) foam = saturate((h + 3000.0) / 6000.0);
return foam;
""")
for n, s in srcs:
    pin = "RGBA" if (n.startswith("W") and n[1:].isdigit()) else ""
    if not MEL.connect_material_expressions(s, pin, cu, n):
        L(f"connect {n} pin {pin} FAILED")
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
L("W3 foam node done")
if os.environ.get("IWTEST_SCRIPT") == "w3_foammat": unreal.SystemLibrary.quit_editor()

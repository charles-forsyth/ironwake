"""W4: IW_Foam Custom node in M_IW_WaterRoot, edited IN PLACE (no deletes: deleting referenced water assets crashed the editor).
Fixes over W3:
 - combed parallel stripes: every texture sample is now mip-filtered (Texture2DSampleGrad with the pixel's world-space
   derivatives) instead of Level 0, wind-streak lanes are 24-38 m wide instead of 5.2 m, and each wave's Jacobian term is
   faded out once its wavelength drops under ~6 pixel footprints (sub-pixel short waves aliased into moire lines).
 - coverage runaway: lace and streak gates tightened (they were adding ~40 % on top of the calibrated caps).
 - debug mode 7 = binary full foam mask (foam > 0.3), used by w2run.sh w4cov to measure coverage numerically.
Headless: IWTEST_SCRIPT=w4_foammat UnrealEditor IronwakeTest.uproject -log"""
import unreal, os
L = lambda m: unreal.log("IWW4M " + str(m))
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
need = ["IW_Hs", "IW_Whitecap", "IW_Streak", "IW_JT", "IW_Wind", "IW_Debug", "IW_CamPos", "IW_WaterTime", "IW_Lace", "IW_LaceT"] + [f"IW_Wave{i}" for i in range(16)]
missing = [n for n in need if n not in params]
assert not missing, f"missing params {missing}"
srcs = [("WP", others["wp"]), ("Cam", others["cam"]), ("Time", others["time"]), ("Tex", others["tex"]),
        ("Hs", params["IW_Hs"]), ("Whitecap", params["IW_Whitecap"]), ("Streak", params["IW_Streak"]), ("JT", params["IW_JT"]),
        ("Wind", params["IW_Wind"]), ("Dbg", params["IW_Debug"]), ("CamAbs", params["IW_CamPos"]), ("WT", params["IW_WaterTime"]),
        ("Lace", params["IW_Lace"]), ("LaceT", params["IW_LaceT"])] + [(f"W{i}", params[f"IW_Wave{i}"]) for i in range(16)]
ins = []
for n, _ in srcs:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
cu.set_editor_property("code", r"""
// W4 whitecaps. Pixel WorldPosition carries no Gerstner displacement and arrives camera-relative (LWC): rebuild the absolute
// position from IW_CamPos and evaluate 16 waves analytically (same phase as the plugin / FastZ). Wn = (dir*(1+steep), wl cm, amp cm).
float3 AP = (WP - Cam) + CamAbs.xyz;
float2 xy = AP.xy;
float2 dX = float2(0, 0), dY = float2(0, 0);
#if PIXELSHADER
dX = ddx(xy); dY = ddy(xy);
#endif
float fp = max(max(length(dX), length(dY)), 1.0);   // pixel footprint on the sea (cm)
float4 Wv[16] = { W0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W10, W11, W12, W13, W14, W15 };
float h = 0, J = 1, J1 = 1, J2 = 1;
[unroll] for (int i = 0; i < 16; ++i)
{
    float ln = max(length(Wv[i].xy), 1e-3);
    float q = max(ln - 1.0, 0.0);
    // band-limit: a wave shorter than ~6 pixel footprints cannot be resolved; its Jacobian term only aliases (moire stripes)
    q *= saturate(Wv[i].z / (fp * 6.0) - 1.0);
    float2 d = Wv[i].xy / ln;
    float k = 6.2831853 / max(Wv[i].z, 1.0);
    float w = sqrt(k * 980.0);
    float ph = dot(xy, d) * k - w * WT;
    float c = cos(ph);
    h += Wv[i].w * c;
    J -= q * c;
    J1 -= q * cos(ph + w * LaceT);          // the surface LaceT s ago: foam left behind by a crest that has moved on
    J2 -= q * cos(ph + w * LaceT * 2.2);
}
float2 wd = normalize(Wind.xy + float2(1e-4, 0));
float2 pd = float2(-wd.y, wd.x);
float al = dot(xy, wd), ac = dot(xy, pd);
float2 dAX = float2(dot(dX, wd), dot(dX, pd)), dAY = float2(dot(dY, wd), dot(dY, pd));   // derivatives in (along, across)
// mip-filtered sample helpers: uv built from world xy with scales (cm per tile)
#define IWS_XY(s, off) Texture2DSampleGrad(Tex, TexSampler, xy / (s) + (off), dX / (s), dY / (s)).r
#define IWS_AC(sa, sb, off) Texture2DSampleGrad(Tex, TexSampler, float2(al / (sa), ac / (sb)) + (off), float2(dAX.x / (sa), dAX.y / (sb)), float2(dAY.x / (sa), dAY.y / (sb))).r
// bubbly detail (two scales, drifting) so foam is lace, not flat paint
float d1 = IWS_XY(420.0, Time * 0.02 * wd);
float d2 = IWS_XY(1300.0, -Time * 0.011 * pd);
float det = saturate(d1 * 0.65 + d2 * 0.65);
// breakup stretched ALONG the crest (across the wind) but short enough (W4: 26 m, plus an isotropic octave) that distant caps
// break into segments instead of long parallel bars
float b1 = saturate(IWS_AC(1400.0, 2600.0, float2(Time * 0.01, 0)) * 0.65 + IWS_XY(2300.0, float2(0.21, 0.53)) * 0.6);
float cap = saturate((JT - J) / 0.12);
float core = saturate((JT - 0.08 - J) / 0.08);
float caps = saturate(cap * (0.35 + 1.2 * b1)) * (0.45 + 0.55 * det);
caps = max(caps, core * (0.7 + 0.3 * det));
caps *= Whitecap;
// lace: decaying foam left behind where crests broke 2-4 s ago, as a sparse net (detail-thresholded)
float l1 = saturate((JT - J1) / 0.14), l2 = saturate((JT + 0.03 - J2) / 0.18);
float lace = saturate(max(l1 * 0.7, l2 * 0.4) * saturate((det - 0.55) * 3.0)) * Lace * Whitecap;
// wind streaks (Beaufort 8+): sparse soft lanes along the wind, 15-40 m apart, 60-200 m long, semi-transparent
float s1 = IWS_AC(18000.0, 3800.0, float2(-Time * 0.003, 0));
float s2 = IWS_AC(9000.0, 2400.0, float2(0.37, 0.61));
float streak = saturate((s1 - 0.58) * 3.0) * saturate((s2 - 0.45) * 2.5) * (0.4 + 0.6 * det) * Streak;
float dist = length(WP - Cam);
float fade = 1.0 - saturate((dist - 90000.0) / 140000.0);
float foam = saturate(caps + lace + streak * 0.5) * fade;
if (Dbg > 0.5 && Dbg < 1.5) foam = 1;
if (Dbg > 1.5 && Dbg < 2.5) foam = cap;
if (Dbg > 2.5 && Dbg < 3.5) foam = saturate(1.0 - J);
if (Dbg > 3.5 && Dbg < 4.5) foam = frac(abs(h) / 300.0);
if (Dbg > 4.5 && Dbg < 5.5) foam = lace;
if (Dbg > 5.5 && Dbg < 6.5) foam = streak;
if (Dbg > 6.5 && Dbg < 7.5) foam = foam > 0.3 ? 1 : 0;
if (Dbg > 7.5 && Dbg < 8.5) foam = 0;
if (Dbg > 12.5) foam = saturate((h + 3000.0) / 6000.0);
return foam;
""")
for n, s in srcs:
    pin = "RGBA" if (n.startswith("W") and n[1:].isdigit()) else ""
    if not MEL.connect_material_expressions(s, pin, cu, n):
        L(f"connect {n} pin {pin} FAILED")
MEL.recompile_material(m)
EAL.save_loaded_asset(m)
L("W4 foam node done")
if os.environ.get("IWTEST_SCRIPT") == "w4_foammat": unreal.SystemLibrary.quit_editor()

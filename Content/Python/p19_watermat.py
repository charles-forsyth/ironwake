"""P19: ship-wave field in the ocean material (M_IW_WaterRoot, edited IN PLACE, nothing deleted).
HLSL twin of Source/IronwakeTest/IronwakeShipWave.cpp (ShipWaveEval). Up to 4 sources (IW_SA0..3, IW_SB, IW_SC, IW_SD per source)
displace the water vertices (WPO += (0,0,h)), tilt the pixel normal and add foam to the blend alpha.
Headless: IWTEST_SCRIPT=p19_watermat (build/tools/pyrun.sh p19_watermat 500). Idempotent: skips if node 'IW_ShipWaveV' exists."""
import unreal
L = lambda m: unreal.log("IWP19M " + str(m))
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

BODY = r"""
float3 AP = (WP - Cam) + CamAbs.xyz;
float fp = 100.0;
#if PIXELSHADER
fp = max(max(length(ddx(AP.xy)), length(ddy(AP.xy))), 1.0);
#endif
float4 SA[4] = { SA0, SA1, SA2, SA3 };
float4 SB[4] = { SB0, SB1, SB2, SB3 };
float4 SC[4] = { SC0, SC1, SC2, SC3 };
float4 SD[4] = { SD0, SD1, SD2, SD3 };
float hh[3] = { 0, 0, 0 };
float foam = 0;
float eps = 150.0;
const float G = 9.81;
[unroll] for (int s = 0; s < NS; ++s)
{
  float2 Pq = AP.xy + (s == 1 ? float2(eps, 0) : (s == 2 ? float2(0, eps) : float2(0, 0)));
  float h = 0;
  [loop] for (int i = 0; i < 4; ++i)
  {
    float V = SB[i].x; float L = max(SB[i].y, 1.0); float X1 = SB[i].z; float HB = SB[i].w;
    float Dr = SC[i].x; float Fb = SC[i].y; float CK = SC[i].z; float TK = SC[i].w;
    float QK = SD[i].x; float WK = SD[i].y; float Bl = SD[i].z;
    float2 Dm = (Pq - SA[i].xy) * 0.01;
    if (dot(Dm, Dm) > 49.0 * L * L) continue;
    float Fn = V / sqrt(G * L);
    float Gk = pow(saturate(Fn / 0.32), 1.5);
    if (Gk < 0.002) continue;
    float Xl = Dm.x * SA[i].z + Dm.y * SA[i].w;
    float Yl = -Dm.x * SA[i].w + Dm.y * SA[i].z;
    float U = X1 - Xl; float Ay = abs(Yl);
    float HbL = 0;
    if (U >= 0 && U <= L) HbL = HB * pow(saturate(U / (0.2 * L)), 0.65) * (1.0 - 0.5 * smoothstep(0.85 * L, L, U));
    float Lat = max(Ay - HbL, 0.0);
    float Hb = CK * Fb * Gk * (0.8 + 0.4 * Bl);
    float SU = U < 0 ? U / (0.05 * L) : U / (0.09 * L);
    float Lt = Lat / (0.07 * L + 1.0);
    float B = Hb * exp(-(SU * SU + Lt * Lt));
    float LatW = Lat / (0.5 * HB + 1.0); LatW = LatW * LatW;
    float Tq = (U - 0.36 * L) / (0.13 * L);
    float TrW = exp(-Tq * Tq) * exp(-LatW);
    float T = -TK * Dr * Gk * TrW;
    float Qh = QK * Hb;
    float Qq = (U - 0.64 * L) / (0.07 * L);
    float QrW = exp(-Qq * Qq) * exp(-LatW);
    float Q = Qh * QrW;
    float Wd = 0; float FoamW = 0;
    float Aw = WK * Hb;
    if (U > 0 && Aw > 1e-4)
    {
      float K0 = G / max(V * V, 0.01);
      float Rc = tan(radians(lerp(19.47, 12.0, saturate((Fn - 0.25) / 0.25))));
      float Fade = smoothstep(0.15 * L, 0.9 * L, U);
      float REff = (Ay / U) * (0.353553 / Rc);
      float Xx = REff / 0.353553;
      float Rcl = min(REff, 0.353553);
      float Sqd = sqrt(max(1.0 - 8.0 * Rcl * Rcl, 0.0));
      float TT = Rcl > 1e-4 ? (1.0 - Sqd) / (4.0 * Rcl) : 0.0;
      float TD = Rcl > 1e-4 ? min((1.0 + Sqd) / (4.0 * Rcl), 6.0) : 6.0;
      float PhT = K0 * U * pow(1.0 + TT * TT, 1.5) / (1.0 + 2.0 * TT * TT);
      float PhD = K0 * U * pow(1.0 + TD * TD, 1.5) / (1.0 + 2.0 * TD * TD);
      float Rho = sqrt(U * U + Ay * Ay);
      float Decay = sqrt(0.6 * L / (0.6 * L + Rho));
      float Out = max(Xx - 1.0, 0.0) / 0.12; Out = exp(-Out * Out);
      float Lam0 = 6.2831853 / K0;
      float BlT = saturate(Lam0 / (1.0 + TT * TT) / (fp * 0.01 * 6.0) - 1.0);
      float BlD = saturate(Lam0 / (1.0 + TD * TD) / (fp * 0.01 * 6.0) - 1.0);
      float Xd = (Xx - 1.0) / 0.18;
      float AT = Aw * Decay * 0.7 * Fade * Out * BlT;
      float AD = Aw * Decay * (0.35 + 1.1 * exp(-Xd * Xd)) * smoothstep(0.3, 0.6, Xx) * Fade * Out * BlD;
      float CD = cos(PhD);
      Wd = AT * cos(PhT) + AD * CD;
      FoamW = Gk * Gk * saturate(AD * CD / (0.5 * Aw + 1e-3) - 0.35) * 1.0 * (Decay * 2.0);
    }
    h += (B + T + Q + Wd) * 100.0;
    if (s == 0)
    {
      float BowF = Hb > 0.01 ? saturate(B / Hb * 1.3) * smoothstep(0.0, 0.03 * L, U) : 0.0;
      float Sx = (U < L ? 1.0 : exp(-(U - L) / (0.25 * L)));
      float SideF = Gk * 0.55 * exp(-(Lat / 1.3) * (Lat / 1.3)) * smoothstep(0.03 * L, 0.12 * L, U) * (1.0 - 0.85 * TrW) * Sx;
      float QF = Qh > 0.01 ? saturate(Q / Qh) * 0.9 : 0.0;
      foam = max(foam, saturate(max(max(BowF, SideF), QF) + 0.85 * FoamW));
    }
  }
  hh[s] = h;
}
"""

PIXEL_TAIL = r"""
float2 g = float2(hh[1] - hh[0], hh[2] - hh[0]) / eps;
float tex = 0.5;
#if PIXELSHADER
float2 uv = AP.xy / 420.0 + Time * float2(0.01, 0.017);
float2 dx = ddx(AP.xy) / 420.0; float2 dy = ddy(AP.xy) / 420.0;
tex = Texture2DSampleGrad(Tex, TexSampler, uv, dx, dy).r;
#endif
float m = saturate((foam - (1.0 - tex) * 0.55) * 3.0) * saturate(foam * 3.0) * FoamK;
return float4(g.x, g.y, m, hh[0]);
"""

VERTEX_TAIL = r"""
return float3(0, 0, hh[0]);
"""


def fix(m, exprs):
    """Re-wire the IW_S* vector parameters into both ship-wave Custom nodes via the RGBA pin (idempotent)."""
    pars = {str(e.get_editor_property("parameter_name")): e for e in exprs if e.get_class().get_name() == "MaterialExpressionVectorParameter" and str(e.get_editor_property("parameter_name")).startswith("IW_S")}
    n = 0
    for e in exprs:
        if e.get_class().get_name() == "MaterialExpressionCustom" and str(e.get_editor_property("description")) in ("IW_ShipWaveP", "IW_ShipWaveV"):
            for k in ("SA", "SB", "SC", "SD"):
                for i in range(4):
                    if MEL.connect_material_expressions(pars[f"IW_{k}{i}"], "RGBA", e, f"{k}{i}"): n += 1
    L(f"fix: rewired {n} inputs via RGBA")
    # refresh code + defaults on the existing nodes (so HLSL edits apply without a rebuild)
    pbody = ("const int NS = 3;\n" + BODY + PIXEL_TAIL).replace("[unroll] for (int s = 0; s < NS", "[unroll] for (int s = 0; s < 3")
    vbody = BODY.replace("s < NS", "s < 1") + VERTEX_TAIL
    for e in exprs:
        c = e.get_class().get_name()
        if c == "MaterialExpressionCustom":
            d = str(e.get_editor_property("description"))
            if d == "IW_ShipWaveP": e.set_editor_property("code", pbody)
            if d == "IW_ShipWaveV": e.set_editor_property("code", vbody)
        if c == "MaterialExpressionScalarParameter":
            pn = str(e.get_editor_property("parameter_name"))
            if pn == "IW_ShipNormalK": e.set_editor_property("default_value", 12.0)
    L("compile errors: " + str(MEL.recompile_material(m)))
    EAL.save_loaded_asset(m)


def build():
    m = unreal.load_asset("/Game/Ironwake/Water/M_IW_WaterRoot")
    exprs = MEL.get_material_expressions(m)
    for e in exprs:
        if e.get_editor_property("desc") == "IW_ShipWaveV":
            fix(m, exprs); return
    cu = [e for e in exprs if e.get_class().get_name() == "MaterialExpressionCustom" and e.get_editor_property("description") == "IW_Foam"][0]
    bl = [e for e in exprs if e.get_class().get_name() == "MaterialExpressionBlendMaterialAttributes"][0]
    top = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    L("top=" + top.get_name() + " bl=" + bl.get_name() + " cu=" + cu.get_name())
    assert top.get_name() == bl.get_name()
    # reuse existing inputs (WP, Cam, CamAbs, Tex, Time) from the foam node
    ins = MEL.get_inputs_for_material_expression(m, cu)
    names = [str(n) for n in MEL.get_material_expression_input_names(cu)]
    src = dict(zip(names, ins))
    X0, Y0 = 1500, 3200

    def E(cls, x, y):
        return MEL.create_material_expression(m, cls, X0 + x, Y0 + y)

    def VP(name, default, y):
        e = E(unreal.MaterialExpressionVectorParameter, 0, y)
        e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", default); e.set_editor_property("group", "IW Ship")
        return e
    pv = {}
    row = 0
    for i in range(4):
        for k, d in (("SA", unreal.LinearColor(0, 0, 1, 0)), ("SB", unreal.LinearColor(0, 0, 0, 0)), ("SC", unreal.LinearColor(0, 0, 0, 0)), ("SD", unreal.LinearColor(0, 0, 0, 0))):
            pv[f"{k}{i}"] = VP(f"IW_{k}{i}", d, row * 60); row += 1
    fk = E(unreal.MaterialExpressionScalarParameter, 0, row * 60); fk.set_editor_property("parameter_name", "IW_ShipFoamK"); fk.set_editor_property("default_value", 1.0); fk.set_editor_property("group", "IW Ship")

    wp0 = E(unreal.MaterialExpressionWorldPosition, 0, -100)
    wp0.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)

    def custom(desc, out_type, body, extra_inputs, x, y, wpn=None):
        c = E(unreal.MaterialExpressionCustom, x, y)
        c.set_editor_property("output_type", out_type)
        c.set_editor_property("description", desc); c.set_editor_property("desc", desc)
        nm = ["WP", "Cam", "CamAbs"] + extra_inputs + [f"{k}{i}" for i in range(4) for k in ("SA", "SB", "SC", "SD")]
        lst = []
        for n in nm:
            ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); lst.append(ci)
        c.set_editor_property("inputs", lst)
        c.set_editor_property("code", body)
        MEL.connect_material_expressions(wpn if wpn is not None else src["WP"], "", c, "WP")
        MEL.connect_material_expressions(src["Cam"], "", c, "Cam")
        MEL.connect_material_expressions(src["CamAbs"], "", c, "CamAbs")
        for n, s in pv.items():
            MEL.connect_material_expressions(s, "RGBA", c, n)   # 'RGBA' pin: default '' passes float3 -> 'too few elements in vector initialization'
        return c
    # pixel node: float4 (gx, gy, foam, h)
    pbody = "const int NS = 3;\n" + BODY + PIXEL_TAIL
    pbody = pbody.replace("[unroll] for (int s = 0; s < NS", "[unroll] for (int s = 0; s < 3")
    px = custom("IW_ShipWaveP", unreal.CustomMaterialOutputType.CMOT_FLOAT4, pbody, ["Time", "Tex", "FoamK"], 2200, 0)
    MEL.connect_material_expressions(src["Time"], "", px, "Time")
    MEL.connect_material_expressions(src["Tex"], "", px, "Tex")
    MEL.connect_material_expressions(fk, "", px, "FoamK")
    vbody = BODY.replace("s < NS", "s < 1") + VERTEX_TAIL
    vx = custom("IW_ShipWaveV", unreal.CustomMaterialOutputType.CMOT_FLOAT3, vbody, [], 2200, 600, wp0)
    vx.set_editor_property("desc", "IW_ShipWaveV")

    # foam: alpha = max(old, ship foam)
    cm = E(unreal.MaterialExpressionComponentMask, 2600, 0)
    cm.set_editor_property("r", False); cm.set_editor_property("g", False); cm.set_editor_property("b", True); cm.set_editor_property("a", False)
    MEL.connect_material_expressions(px, "", cm, "")
    mx = E(unreal.MaterialExpressionMax, 2800, 0)
    MEL.connect_material_expressions(cu, "", mx, "A"); MEL.connect_material_expressions(cm, "", mx, "B")
    L("alpha " + str(MEL.connect_material_expressions(mx, "", bl, "Alpha")))

    # Break the blended attributes, add to WPO and Normal, Make again (names matched 1:1, read back below)
    br = E(unreal.MaterialExpressionBreakMaterialAttributes, 3000, 300)
    mk = E(unreal.MaterialExpressionMakeMaterialAttributes, 3600, 300)
    MEL.connect_material_expressions(bl, "", br, "")
    outs = [str(n) for n in MEL.get_material_expression_output_names(br)]
    mins = [str(n) for n in MEL.get_material_expression_input_names(mk)]
    L("break outs %d make ins %d" % (len(outs), len(mins)))
    add = E(unreal.MaterialExpressionAdd, 3300, 520)
    MEL.connect_material_expressions(br, "WorldPositionOffset", add, "A"); MEL.connect_material_expressions(vx, "", add, "B")
    nbody = "float2 g = G.xy * 1.0; return normalize(N + float3(-g.x, -g.y, 0.0) * K);"
    nc = E(unreal.MaterialExpressionCustom, 3300, 700)
    nc.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    nc.set_editor_property("description", "IW_ShipNormal")
    lst = []
    for n in ("N", "G", "K"):
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); lst.append(ci)
    nc.set_editor_property("inputs", lst); nc.set_editor_property("code", nbody)
    nk = E(unreal.MaterialExpressionScalarParameter, 3100, 800); nk.set_editor_property("parameter_name", "IW_ShipNormalK"); nk.set_editor_property("default_value", 12.0); nk.set_editor_property("group", "IW Ship")
    MEL.connect_material_expressions(br, "Normal", nc, "N"); MEL.connect_material_expressions(px, "", nc, "G"); MEL.connect_material_expressions(nk, "", nc, "K")
    bad = []
    for o in mins:
        if o in ("WorldPositionOffset",):
            ok = MEL.connect_material_expressions(add, "", mk, o)
        elif o == "Normal":
            ok = MEL.connect_material_expressions(nc, "", mk, o)
        else:
            on = o.replace("CustomizedUVs_", "CustomizedUV")
            if on not in outs: bad.append(o); continue
            ok = MEL.connect_material_expressions(br, on, mk, o)
        if not ok: bad.append(o)
    L("unconnected/failed: " + str(bad))
    # readback
    mins_in = MEL.get_inputs_for_material_expression(m, mk)
    for n, node in zip(mins, mins_in):
        if node is None: L(f"readback {n}: NONE"); continue
        on = MEL.get_input_node_output_name_for_material_expression(mk, node)
        L(f"readback {n} <- {node.get_class().get_name()}[{on}]")
    L("connect top " + str(MEL.connect_material_property(mk, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)))
    errs = MEL.recompile_material(m)
    L("compile errors: " + str(errs))
    EAL.save_loaded_asset(m)
    L("saved")


try:
    build()
except Exception as ex:
    import traceback
    L("EXC " + traceback.format_exc().replace("\n", " | "))
finally:
    L("done")
    unreal.SystemLibrary.quit_editor()

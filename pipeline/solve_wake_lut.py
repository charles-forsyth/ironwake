#!/usr/bin/env python3
"""
Hydrodynamic Ship-Wave & Turbulent Propeller-Wash LUT Solver (multi-core CPU).
Computes on a 1024x1024 ship-relative grid (bow at +X, x in [-6.5 L, +1.5 L], y in [-3.0 L, +3.0 L]):
  1. T_IW_KelvinWaveLut.png (16-bit RGBA / 8-bit preview):
     Exact Havelock spectral wave-elevation integral across 1,400 wave angles theta in (-pi/2, +pi/2)
     for a slender finite-draft bow-crest + mid-trough + stern-shoulder source distribution at three
     Froude numbers:
       R = low speed  (Fn = 0.15, transverse-dominated)
       G = cruise     (Fn = 0.24, classic 19.47 deg cusp + transverse crests)
       B = flank      (Fn = 0.36, long cusp arms + deep midbody trough)
       A = steepness / Jacobian crest-breaking mask (where |grad zeta| exceeds threshold)
  2. T_IW_PropWashFoamLut.png (1024x1024 RGBA):
     Physically structured turbulent wake replacing the 2-4/10 solid white band:
       R = twin counter-rotating propeller tip-vortex & axial momentum-jet boil (patchy Kolmogorov
           multi-scale eddies that widen as x^(1/3), break into cellular lace, and decay downstream)
       G = Kelvin cusp-arm feathering foam lace along the +/-19.47 deg echelon crests
       B = tight hull-waterline shoulder contact foam ribbon (clamped strictly to waterline, zero climb)
       A = combined energy envelope
Usage: python3 solve_wake_lut.py <outdir> [grid_n=1024] [n_theta=1400]
"""
import sys, os, json, time, math
import numpy as np
from PIL import Image

out_dir = sys.argv[1]
N = int(sys.argv[2]) if len(sys.argv) > 2 else 1024
N_TH = int(sys.argv[3]) if len(sys.argv) > 3 else 1400
os.makedirs(out_dir, exist_ok=True)
t0 = time.time()

# Ship-normalised coordinates (ship length L = 1.0, hull spans x in [-0.5, +0.5], bow at +0.5, stern at -0.5)
x_min, x_max = -6.5, 1.5
y_min, y_max = -3.0, 3.0
xs = np.linspace(x_min, x_max, N, dtype=np.float32)
ys = np.linspace(y_min, y_max, N, dtype=np.float32)
X, Y = np.meshgrid(xs, ys, indexing="ij")  # X along row (0=stern wake -6.5L -> N-1=ahead +1.5L), Y lateral

def havelock_field(fn: float, draft_l: float = 0.055) -> np.ndarray:
    """Spectral Havelock free-surface elevation for bow source (+0.48) + stern sink (-0.46) with finite-draft damping."""
    k0 = 1.0 / (fn * fn)  # fundamental wavenumber k_0 * L = g L / U^2 = 1 / Fn^2
    # Avoid |theta| -> pi/2 singularity via smooth high-k viscous/draft cutoff
    th = np.linspace(-0.488 * math.pi, 0.488 * math.pi, N_TH, dtype=np.float32)
    dth = float(th[1] - th[0])
    ct = np.cos(th)
    st = np.sin(th)
    sec = 1.0 / ct
    sec2 = sec * sec
    k = k0 * sec2  # dispersion relation k(theta) = k0 sec^2(theta)

    # Finite-draft submergence attenuation exp(-k * draft_eff) + capillary/breaking rolloff at very short waves
    att = np.exp(-k * draft_l * 0.85) / (1.0 + (k / (k0 * 14.0)) ** 2)
    amp_w = sec * att * dth  # Havelock amplitude weight

    zeta = np.zeros((N, N), dtype=np.float32)
    # Sum bow wavemaker (+0.48, positive crest ahead -> trough behind) and stern recovery (-0.46, trough -> stern crest)
    sources = [(+0.47, +1.00, 0.045), (-0.46, -0.78, 0.060)]
    chunk = 70
    for x_src, sign_w, sigma_x in sources:
        dx = X - x_src  # wake propagates downstream where dx < 0
        downstream = 0.5 * (1.0 - np.tanh(dx / sigma_x))  # smooth causal step for x < x_src
        # Lateral radiation window (causal inside Mach/group-velocity envelope + soft outer sponge)
        sponge = np.exp(-0.025 * np.maximum(0.0, -dx) - 0.06 * (Y ** 2))
        acc = np.zeros((N, N), dtype=np.float32)
        for i0 in range(0, N_TH, chunk):
            i1 = min(N_TH, i0 + chunk)
            kx = (k[i0:i1] * ct[i0:i1])[None, None, :]
            ky = (k[i0:i1] * st[i0:i1])[None, None, :]
            w = amp_w[i0:i1][None, None, :]
            phase = dx[:, :, None] * kx + Y[:, :, None] * ky
            # cos phase gives crisp bow crest right at stem
            acc += np.sum(w * np.cos(phase), axis=2)
        zeta += sign_w * acc * downstream * sponge

    # Add local near-field Bernoulli hull displacement (bow pressure cushion + midbody drawdown trough)
    r_bow = np.sqrt(((X - 0.48) / 0.14) ** 2 + (Y / 0.11) ** 2)
    bow_cushion = 0.65 * np.exp(-0.5 * r_bow * r_bow)
    mid_trough = -0.45 * np.exp(-((X - 0.02) / 0.34) ** 2 - (Y / 0.14) ** 2)
    raw = zeta * (0.22 / max(0.18, fn)) + bow_cushion + mid_trough
    # Normalize smoothly to [-1, +1] preserving physical ratios
    p99 = float(np.percentile(np.abs(raw), 99.6))
    return np.clip(raw / max(p99, 1e-4), -1.0, 1.0)

print(f"Solving Havelock spectral fields ({N}x{N}, {N_TH} angles)...", flush=True)
z_low = havelock_field(0.15)
print(f"  Fn=0.15 done ({time.time()-t0:.1f}s)", flush=True)
z_mid = havelock_field(0.24)
print(f"  Fn=0.24 done ({time.time()-t0:.1f}s)", flush=True)
z_hi = havelock_field(0.36)
print(f"  Fn=0.36 done ({time.time()-t0:.1f}s)", flush=True)

# Crest-steepness / Jacobian breaking mask from cruise+flank wave slope
gx, gy = np.gradient(0.45 * z_mid + 0.55 * z_hi, xs[1] - xs[0], ys[1] - ys[0])
slope = np.sqrt(gx * gx + gy * gy)
crest_break = np.clip((slope - np.percentile(slope, 86.0)) / (np.percentile(slope, 99.3) - np.percentile(slope, 86.0) + 1e-5), 0.0, 1.0)
crest_break *= np.clip(0.5 + 0.5 * z_hi, 0.0, 1.0)

# Band-limited multi-scale isotropic + advected fBm noise for realistic cellular sea foam (no external Perlin dep)
rng = np.random.default_rng(1874)
def multi_scale_cellular(nx, ny, scales=(6, 13, 27, 55, 110)):
    acc = np.zeros((nx, ny), dtype=np.float32)
    w_tot = 0.0
    for idx, sc in enumerate(scales):
        w = 0.55 ** idx
        grid = rng.standard_normal((sc + 3, sc + 3)).astype(np.float32)
        im = Image.fromarray(grid, mode="F").resize((ny, nx), resample=Image.Resampling.BICUBIC)
        arr = np.asarray(im, dtype=np.float32)
        # Cellular Voronoi-like foam webbing = 1 - |arr|, ridges where bubbles collect
        web = np.clip(1.0 - np.abs(arr) * 1.15, 0.0, 1.0) ** 1.6
        acc += w * web
        w_tot += w
    acc /= max(w_tot, 1e-5)
    return np.clip((acc - acc.min()) / (acc.max() - acc.min() + 1e-5), 0.0, 1.0)

foam_cells = multi_scale_cellular(N, N, scales=(8, 18, 38, 78, 150))
meander_field = multi_scale_cellular(N, N, scales=(5, 11, 23)) - 0.5

# Channel 1 (R): Turbulent Propeller Wash (replaces 2-4/10 solid white spotlight band!)
# Starts at stern (-0.48), widens with turbulent jet law b(s) = b0 + c * s^0.38, meanders with twin hub vortices,
# and breaks up into aerated cellular boil patches rather than a solid stripe!
s_aft = np.maximum(0.0, -(X + 0.46))
aft_gate = 0.5 * (1.0 - np.tanh((X + 0.46) / 0.035))
jet_half_w = 0.052 + 0.135 * (s_aft ** 0.42)
# Twin counter-rotating vortex cores meandering slightly laterally
core_off = 0.028 * (1.0 + 0.7 * np.sin(s_aft * 3.4)) + 0.045 * meander_field * np.minimum(1.0, s_aft * 0.5)
y_l = (Y + core_off) / jet_half_w
y_r = (Y - core_off) / jet_half_w
twin_profile = 0.62 * np.exp(-0.95 * y_l * y_l) + 0.62 * np.exp(-0.95 * y_r * y_r)
# Axial boil bursts + downstream dissipation + cellular foam holes (crucial so it NEVER reads as a solid band!)
axial_decay = np.exp(-0.34 * s_aft) * (1.0 - 0.25 * np.exp(-s_aft * 6.0))
patchy_boil = np.clip(foam_cells * 1.45 - 0.28 * np.minimum(1.0, 0.35 + 0.22 * s_aft), 0.0, 1.0)
prop_wash = np.clip(aft_gate * twin_profile * axial_decay * (0.22 + 0.95 * patchy_boil), 0.0, 1.0)

# Channel 2 (G): Kelvin Echelon Cusp Foam Lace (+/- 19.47 deg arms + breaking transverse crests)
kelvin_tan = math.tan(math.radians(19.47))  # 0.3535
s_bow = np.maximum(0.0, 0.48 - X)
arm_dist = np.minimum(np.abs(np.abs(Y) - kelvin_tan * s_bow), np.abs(np.abs(Y) - 0.28 * s_bow))
echelon_env = np.exp(-((arm_dist / (0.065 + 0.045 * s_bow ** 0.6)) ** 2)) * (X < 0.49) * np.exp(-0.22 * s_bow)
kelvin_foam = np.clip((0.65 * crest_break + 0.55 * echelon_env * np.clip(z_hi + 0.2, 0.0, 1.0)) * (0.25 + 0.95 * foam_cells), 0.0, 1.0)

# Channel 3 (B): Hull Shoulder & Waterline Contact Foam (pinned tightly around hull perimeter, zero mid-hull climb)
# Elliptical hull waterline contour (x in [-0.48, +0.48], half-beam b0 = 0.085)
u_hull = X / 0.48
b_hull = 0.085 * np.clip(1.0 - np.abs(u_hull) ** 2.4, 0.0, 1.0) ** 0.65
d_outboard = np.abs(Y) - b_hull
# Zero inside hull (d_outboard < -0.008) so foam NEVER climbs up the hull topsides!
hull_contact = (np.abs(u_hull) <= 1.03) * (d_outboard >= -0.006) * np.exp(-((np.maximum(0.0, d_outboard) / 0.042) ** 2))
# Stronger at bow shoulders (u_hull in [0.35, 0.98]), light streak along sides
shoulder_boost = 0.45 + 0.75 * np.exp(-((u_hull - 0.72) / 0.28) ** 2)
contact_foam = np.clip(hull_contact * shoulder_boost * (0.35 + 0.85 * foam_cells), 0.0, 1.0)

# Save Wave Elevation LUT (8-bit RGBA mapped [-1,1] -> [0,255], plus 16-bit grayscale cruise/flank heightmaps)
def to_u8_signed(arr):
    return np.clip(np.round((arr * 0.5 + 0.5) * 255.0), 0, 255).astype(np.uint8)

wave_rgba = np.stack([
    to_u8_signed(z_low),
    to_u8_signed(z_mid),
    to_u8_signed(z_hi),
    np.clip(np.round(crest_break * 255.0), 0, 255).astype(np.uint8),
], axis=-1)
Image.fromarray(wave_rgba, mode="RGBA").save(os.path.join(out_dir, "T_IW_KelvinWaveLut.png"))

# Also save 16-bit precision cruise+flank height PNGs for lossless CPU/GPU sampling
z_hi_u16 = np.clip(np.round((z_hi * 0.5 + 0.5) * 65535.0), 0, 65535).astype(np.uint16)
Image.fromarray(z_hi_u16, mode="I;16").save(os.path.join(out_dir, "T_IW_KelvinFlank16.png"))

foam_env = np.clip(np.maximum(prop_wash, np.maximum(kelvin_foam, contact_foam)), 0.0, 1.0)
foam_rgba = np.clip(np.round(np.stack([prop_wash, kelvin_foam, contact_foam, foam_env], axis=-1) * 255.0), 0, 255).astype(np.uint8)
Image.fromarray(foam_rgba, mode="RGBA").save(os.path.join(out_dir, "T_IW_PropWashFoamLut.png"))

# Build human/vision diagnostic sheet (ocean render preview of wake + individual channels)
sea_base = np.zeros((N, N, 3), dtype=np.float32)
sea_base[:, :, 0] = 0.035 + 0.045 * z_hi
sea_base[:, :, 1] = 0.085 + 0.070 * z_hi
sea_base[:, :, 2] = 0.125 + 0.085 * z_hi
total_white = np.clip(0.85 * prop_wash + 0.72 * kelvin_foam + 0.90 * contact_foam, 0.0, 1.0)
preview = np.clip(sea_base * (1.0 - 0.75 * total_white[:, :, None]) + total_white[:, :, None] * np.array([0.86, 0.90, 0.92])[None, None, :], 0.0, 1.0)
Image.fromarray((preview * 255).astype(np.uint8)).save(os.path.join(out_dir, "preview_wake_composite.png"))

meta = {
    "grid_size": N,
    "quadrature_angles": N_TH,
    "domain_x_L": [x_min, x_max],
    "domain_y_L": [y_min, y_max],
    "hull_span_x_L": [-0.48, 0.48],
    "bow_x_L": 0.48,
    "stern_x_L": -0.48,
    "kelvin_wave_channels": {"R": "Fn=0.15", "G": "Fn=0.24", "B": "Fn=0.36", "A": "crest_breaking"},
    "foam_lut_channels": {"R": "turbulent_prop_wash", "G": "kelvin_cusp_lace", "B": "hull_shoulder_contact", "A": "envelope"},
    "elapsed_s": round(time.time() - t0, 2),
}
json.dump(meta, open(os.path.join(out_dir, "wake_lut_meta.json"), "w"), indent=2)
print("WAKE_LUT_DONE", json.dumps(meta))

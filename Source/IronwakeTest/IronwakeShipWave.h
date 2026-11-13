// P19 + P27: hydrodynamic per-ship Havelock wave field (bow crest, midbody trough, quarter crest, Kelvin wedge)
// synchronized between CPU (Content/Data/kelvin_lut_256.bin) and GPU ocean shader (Content/Python/iw_p27_water_lut.py:
// T_IW_KelvinWaveLut + T_IW_PropWashFoamLut across 7 simultaneous ship slots: player + nearest 6 moving NPCs).
#pragma once
#include "CoreMinimal.h"
#include "Math/Vector2D.h"

class UWorld;
class UMaterialInstanceDynamic;

struct FIwShipWaveSrc
{
	FVector2D PosCm = FVector2D::ZeroVector;   // actor origin
	float CosH = 1.f, SinH = 0.f;              // heading
	float V = 0.f;                             // world speed through the water, m/s
	float L = 100.f;                           // waterline length, m
	float X1 = 50.f;                           // stem position in hull-local X (m) from the actor origin
	float HalfBeam = 8.f, Draft = 5.f, Freeboard = 3.f, Bluff = 0.5f;
	float CrestK = 0.40f, TroughK = 0.10f, QuarterK = 0.60f, WedgeK = 0.45f;   // hull-class data (ShipDef)
	float ThrustSteam = 0.f;                   // P27: [0..1.2] screw thrust * steam factor (0 on pure sail)
	float RollOutM = 0.f;                      // P27: dynamic outboard waterline margin from roll/heel (m)
	bool bPlayer = false;                      // P27: guarantees player ship occupies shader slot 0 when in view
	bool bTwinScrew = false;                   // P27: twin-screw split behind sternpost (ironclad)
	double Stamp = 0.0;
	const void* Key = nullptr;
};

namespace IronwakeSea
{
	/** Register / refresh a moving ship as a wave source (call every tick; entries expire after 0.6 s). */
	IRONWAKETEST_API void RegisterShipWave(const void* Key, const FIwShipWaveSrc& S);
	IRONWAKETEST_API void UnregisterShipWave(const void* Key);
	/** Height (cm) of one source at a world point; optional foam 0..1 and the four parts in metres (bow, trough, quarter, wedge). */
	IRONWAKETEST_API float ShipWaveEval(const FIwShipWaveSrc& S, const FVector2D& PCm, float FpCm, float* OutFoam = nullptr, float* OutParts = nullptr);
	/** Sum over all live sources except Skip (cm). */
	IRONWAKETEST_API float ShipWaveZ(const FVector& P, const void* Skip = nullptr);
	IRONWAKETEST_API bool ShipWaveEnabled();
	IRONWAKETEST_API int32 ShipWaveCount();
	/** P27: number of active moving ship slots pushed to M_IW_WaterRoot last frame (0..7). */
	IRONWAKETEST_API int32 ActiveShipWaveSlots();
	/** Push up to 7 sources nearest the camera (player + 6 moving NPCs) to M_IW_WaterRoot (IW_SA0..6 .. IW_SD0..6). */
	IRONWAKETEST_API void PushShipWaveParams(UMaterialInstanceDynamic* MID, const FVector& Cam);
	/** Half angle (deg) of the wedge for a Froude number (19.47 at low Fn, narrowing to ~12 at high Fn). */
	IRONWAKETEST_API float WedgeHalfAngleDeg(float Fn);
}

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GerstnerWaterWaves.h"
#include "IronwakeHydro.generated.h"

class UMaterialInterface;

/**
 * W1: hull waterline data + sea state + pushed-water bow wave.
 *
 * FIronwakeHullProfile: max half-breadth of each library hull between 0.5 m below and 2.5 m above the boot-top
 * (measured in Blender from the FBX, build/w1/clear.py), used for (a) waterplane-weighted wave averaging in the
 * hydrostatics and (b) keeping every bow-wave vertex OUTSIDE the hull.
 */
struct FIronwakeHullProfile
{
	float X0 = -60.f, X1 = 60.f;        // m, stern..stem in hull-local X (boot-top at z=0, bow +X)
	TArray<float> HalfB;                 // m, evenly spaced from X0 to X1
	float BowDeckM = 3.2f;               // forecastle deck height over the boot-top near the stem (m)
	float KeelM = 7.f;                   // keel depth under the boot-top (m), for the log
	float HalfBreadthAt(float XM) const;
	float Length() const { return X1 - X0; }
	static FIronwakeHullProfile Devastation();
	static FIronwakeHullProfile Kracken();
	static FIronwakeHullProfile Goliath();
	static FIronwakeHullProfile Aglae();
	static FIronwakeHullProfile Frigate(); // S2 (IronwakeFleet.cpp)
	static FIronwakeHullProfile TallBrig(); // P26 (IronwakeFleet.cpp)
	static FIronwakeHullProfile ChapmanSloop(); // P32 (IronwakeFleet.cpp)
	static FIronwakeHullProfile AmphionSchooner(); // P32 (IronwakeFleet.cpp)
};

/** W2: our own Gerstner generator: a fixed set of components (primary swell, crossing swell, wind sea, chop) whose
 *  wavelengths and directions never change; only amplitudes/steepness follow the sea state, so blends never jump. */
UCLASS()
class IRONWAKETEST_API UIronwakeWaveGen : public UGerstnerWaterWaveGeneratorBase
{
	GENERATED_BODY()
public:
	TArray<FGerstnerWave> Waves;
	virtual void GenerateGerstnerWaves_Implementation(TArray<FGerstnerWave>& OutWaves) const override { OutWaves = Waves; }
};

namespace IronwakeSea
{
	/** W2 sea states: 0 CALM, 1 MODERATE, 2 ROUGH, 3 GALE, 4 STORM (continuous; blends amplitude only). */
	IRONWAKETEST_API void SetSeaState(UWorld* World, float State);
	IRONWAKETEST_API float GetSeaState();
	/** P13: 0..1 blue body glow for moonlit nights. */
	IRONWAKETEST_API void SetNightWater(UWorld* World, float K);
	IRONWAKETEST_API const TCHAR* SeaStateName(float State);
	/** Mean wind for a sea state (kn): 8 / 16 / 24 / 36 / 48. */
	IRONWAKETEST_API float WindForState(float State);
	/** Significant wave height of the current spectrum (m) and the measured-at-generation max crest (m). */
	IRONWAKETEST_API float SigWaveHeightM();
	/** Direction the wind sea travels (UE yaw, deg). */
	IRONWAKETEST_API float WaveTravelYaw();
	/** Fast analytic surface (no water-body query): height (cm, rendered frame) and a Jacobian-like crest sharpness
	 *  (1 = flat, < 0.6 = sharp/breaking crest). Calibrated against RenderedZ once per frame. */
	IRONWAKETEST_API float FastZ(UWorld* World, const FVector& P, float* OutJ = nullptr);
	/** P16 measurement: pure analytic Gerstner sum (cm over the flat plane), NO per-frame calibration to the query. */
	IRONWAKETEST_API float AnalyticZ(UWorld* World, const FVector& P);
	/** Water material foam needs the absolute camera position (the shader sees camera-translated positions). Call per frame. */
	IRONWAKETEST_API void SetFoamCamera(UWorld* World, const FVector& Cam);
	/** Legacy W1 API (tests): scale 1 = MODERATE, 1.6 = ROUGH, 0.65 = CALM-ish, 0 = flat. */
	IRONWAKETEST_API void SetSeaScale(UWorld* World, float Scale);
	/** Wind-equivalent scale (mean wind / 16 kn), kept for older callers. */
	IRONWAKETEST_API float GetSeaScale();
	/** Test-only: replace the spectrum with one band (wavelengths/amplitudes in cm). SetSeaScale restores the normal sea. */
	IRONWAKETEST_API void SetSeaBand(UWorld* World, int32 NumWaves, float MinWL, float MaxWL, float MinA, float MaxA);
	/** Rendered ocean surface height (wave query + the measured render offset). */
	IRONWAKETEST_API float RenderedZ(UWorld* World, const FVector& P);
	/** W4 test: force the water foam debug mode every frame (-1 = off). 7 = binary full foam mask, 8 = no foam, 1 = all foam. */
	IRONWAKETEST_API void SetFoamDebug(float Mode);
}

/** Procedural foam ribbons: stem mound, hull-hugging waterline foam, two diverging Kelvin-angle crests. World-space. */
UCLASS()
class IRONWAKETEST_API UIronwakeBowWave : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UIronwakeBowWave(const FObjectInitializer& OI);
	void Init(const FIronwakeHullProfile& InHull, float InScale);
	/** HullXf: hull frame (yaw used for placement). SpeedFrac 0..1 (FULL ~0.8-1). */
	void UpdateWave(const FTransform& HullXf, float SpeedFrac, float SpeedMS, float DeckWash = 0.f, float BowDeckZ = 0.f);
	float LastStemCrestM = 0.f;
private:
	FIronwakeHullProfile Hull;
	float Scale = 1.f; // small ships: ribbon sizes scale with length
	bool bBuilt = false;
	TArray<FVector> V; TArray<int32> T; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> C; TArray<FProcMeshTangent> Tan;
	struct FSt { FVector2D In; FVector2D Out; float W; float A; float H; };
	void AddRibbon(UWorld* W, const FTransform& Xf, const FVector& Origin, const TArray<FSt>& S, const FTransform* DeckXf = nullptr, float DeckZ = 0.f, const float* ProfA = nullptr, const float* ProfH = nullptr);
};

/** W3: long Kelvin wake that rides the waves. A history of stern/bow positions (every 0.5 s, 60 s) drives three ribbons:
 *  the turbulent centre wake (prop boil on steamers) and two diverging 19.47 deg Kelvin arms whose lateral spread at age a is
 *  V * a * tan(19.47). Vertices sit on the analytic sea (FastZ) each frame; foam material M_IW_Surf2 (unlit, no depth fade). */
UCLASS()
class IRONWAKETEST_API UIronwakeWake : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UIronwakeWake(const FObjectInitializer& OI);
	void Init(float InLengthM, float InHalfBeamM, bool bInSteamer);
	/** P19/P27: prop wash only while screws turn (steamers, or sailing ship under auxiliary steam); twin screws for ironclad; ThrustFrac [0..1.2]. */
	void SetPropWash(bool bOn, bool bTwin, float InThrust = -1.f)
	{
		bSteamer = bOn;
		bTwin_ = bTwin;
		ThrustSteam_ = bOn ? (InThrust >= 0.f ? InThrust : 0.8f) : 0.f;
	}
	void UpdateWake(float Dt, const FTransform& HullXf, float SternXM, float BowXM, float SpeedMS, float SpeedFrac, float SeaState);
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> MID;
private:
	struct FS { FVector2D Stern, Bow, Fwd; float T; float V; float F; float Thrust; };
	TArray<FS> Hist;
	float Acc = 0.f, Age = 0.f, LengthM = 100.f, HalfBeamM = 8.f, ThrustSteam_ = 0.f;
	bool bSteamer = false, bBuilt = false, bTwin_ = false;
	TArray<FVector> V; TArray<int32> T; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> C; TArray<FProcMeshTangent> Tan;
};

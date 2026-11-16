#include "IronwakeHydro.h"
#include "IronwakeShell.h"
#include "IronwakeShipWave.h"
#include "IronwakeTest.h"
#include "EngineUtils.h"
#include "GerstnerWaterWaves.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyManager.h"
#include "WaterSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// ---------------------------------------------------------------------------- hull profiles (build/w1/clear.py)
static FIronwakeHullProfile MakeProf(float X0, float X1, std::initializer_list<float> B, float Deck, float Keel)
{
	FIronwakeHullProfile P; P.X0 = X0; P.X1 = X1; P.HalfB = B; P.BowDeckM = Deck; P.KeelM = Keel; return P;
}
FIronwakeHullProfile FIronwakeHullProfile::Devastation()
{
	return MakeProf(-62.5f, 59.2f, { 3.49f, 7.22f, 9.33f, 10.55f, 11.17f, 11.95f, 12.41f, 12.63f, 12.82f, 12.82f, 12.82f, 12.89f, 12.89f, 12.65f, 12.65f, 12.62f, 12.62f, 12.54f, 12.34f, 11.88f, 11.61f, 11.42f, 9.21f, 6.69f, 3.55f }, 3.2f, 10.8f);
}
FIronwakeHullProfile FIronwakeHullProfile::Kracken()
{
	return MakeProf(-56.6f, 60.f, { 1.40f, 3.86f, 4.91f, 6.80f, 7.85f, 8.27f, 8.72f, 8.75f, 10.01f, 10.01f, 10.01f, 10.01f, 8.45f, 8.45f, 8.75f, 8.75f, 8.75f, 8.75f, 8.70f, 8.23f, 7.81f, 6.80f, 4.87f, 3.82f, 1.39f }, 5.0f, 7.5f);
}
FIronwakeHullProfile FIronwakeHullProfile::Goliath()
{
	return MakeProf(-25.f, 24.9f, { 2.75f, 3.3f, 3.5f, 3.75f, 4.02f, 4.23f, 4.32f, 4.36f, 4.4f, 4.42f, 4.44f, 4.46f, 4.46f, 4.46f, 4.44f, 4.4f, 4.4f, 4.32f, 4.27f, 4.19f, 4.07f, 3.73f, 3.37f, 2.18f, 1.31f }, 2.5f, 5.0f);
}
FIronwakeHullProfile FIronwakeHullProfile::Aglae()
{
	return MakeProf(-14.94f, 15.f, { 0.03f, 0.99f, 1.52f, 1.72f, 1.78f, 1.81f, 1.83f, 1.85f, 1.86f, 1.87f, 1.87f, 1.89f, 1.9f, 1.91f, 1.92f, 1.92f, 1.91f, 1.89f, 1.86f, 1.8f, 1.7f, 1.55f, 1.35f, 1.05f, 0.63f }, 1.2f, 1.6f);
}
float FIronwakeHullProfile::HalfBreadthAt(float XM) const
{
	if (HalfB.Num() < 2 || XM < X0 - 0.5f || XM > X1 + 0.5f) return 0.f;
	const float U = FMath::Clamp((XM - X0) / FMath::Max(X1 - X0, 1.f), 0.f, 1.f) * (HalfB.Num() - 1);
	const int32 I = FMath::Min((int32)U, HalfB.Num() - 2);
	return FMath::Lerp(HalfB[I], HalfB[I + 1], U - I);
}

// ---------------------------------------------------------------------------- sea state (W2)
namespace IronwakeSea
{
	static float GScale = -1.f;      // legacy
	static float GNightWater = 0.f;
	static float GState = -1.f;      // W2 sea state 0..4
	static float GHs = 2.f;
	static TWeakObjectPtr<UIronwakeWaveGen> GGen;
	static TWeakObjectPtr<UGerstnerWaterWaves> GWaves;
	static TWeakObjectPtr<UWorld> GSeaWorld;
	static TWeakObjectPtr<UWaterBodyComponent> GOceanC;
	static float GFastOff = 0.f; static uint64 GFastOffFrame = 0;
	constexpr float WindYaw = 120.f; // wind sea TRAVELS toward ~ESE (wind from WNW, IronwakeFleet prevailing 300)

	// Fixed components: wavelength (m), direction offset from the wind (deg), group.
	struct FComp { float WL; float Dir; int32 G; };
	static const FComp Comps[] = {
		{ 240.f, -8.f, 0 }, { 205.f, 6.f, 0 }, { 172.f, -3.f, 0 }, { 150.f, 11.f, 0 },     // A primary swell (150-250 m)
		{ 185.f, 62.f, 1 }, { 128.f, -55.f, 1 },                                          // B crossing swell (2nd direction)
		{ 128.f, 18.f, 2 }, { 104.f, -22.f, 2 }, { 84.f, 9.f, 2 }, { 66.f, -14.f, 2 },     // C wind sea
		{ 31.f, 35.f, 3 }, { 23.f, -30.f, 3 }, { 16.5f, 12.f, 3 }, { 12.f, -41.f, 3 }, { 9.f, 25.f, 3 }, { 7.f, -8.f, 3 }, // D chop
		{ 5.f, 48.f, 4 }, { 3.8f, -36.f, 4 },                                             // E ripples
		{ 58.f, -28.f, 5 }, { 44.f, 37.f, 5 }, { 33.f, -48.f, 5 }, { 24.f, 14.f, 5 },     // F W3 short steep wind sea (GALE/STORM breakers); W4: wider spread so crest lines cross (no parallel foam bars)
	};
	constexpr int32 NComp = UE_ARRAY_COUNT(Comps);
	static const int32 GroupN[6] = { 4, 2, 4, 6, 2, 4 };
	// Group significant heights (m) per sea state CALM, MODERATE, ROUGH, GALE, STORM.
	static const float GroupHs[6][5] = {
		{ 0.85f, 1.5f, 2.6f, 4.8f, 8.4f },  // A
		{ 0.30f, 0.8f, 1.3f, 2.0f, 3.0f },  // B
		{ 0.18f, 1.0f, 2.4f, 3.8f, 5.0f },  // C
		{ 0.12f, 0.6f, 1.1f, 1.6f, 2.2f },  // D
		{ 0.05f, 0.15f, 0.25f, 0.3f, 0.35f }, // E
		{ 0.04f, 0.25f, 0.75f, 2.0f, 3.0f }  // F (W3)
	};
	// Total Gerstner steepness per state (sum over all waves; ~1 = crests pinch to a point) and its share per group.
	static const float SteepTotal[5] = { 0.16f, 0.45f, 0.72f, 0.92f, 1.05f };
	static const float SteepShare[6] = { 0.17f, 0.06f, 0.30f, 0.16f, 0.03f, 0.28f };
	static const float FoamBoost[5] = { 1.2f, 2.6f, 5.0f, 8.0f, 11.f };
	static const float FoamHeightBias[5] = { -0.7f, -0.45f, -0.15f, 0.1f, 0.3f };
	static const float WindKnS[5] = { 8.f, 16.f, 24.f, 36.f, 48.f };
	static const TCHAR* Names[5] = { TEXT("CALM"), TEXT("MODERATE"), TEXT("ROUGH"), TEXT("GALE"), TEXT("STORM") };

	static float Lerp5(const float* T, float S) { S = FMath::Clamp(S, 0.f, 4.f); const int32 I = FMath::Min((int32)S, 3); return FMath::Lerp(T[I], T[I + 1], S - I); }

	float GetSeaState() { return GState < 0.f ? 1.f : GState; }
	const TCHAR* SeaStateName(float S) { return Names[FMath::Clamp(FMath::RoundToInt(S), 0, 4)]; }
	float WindForState(float S) { return Lerp5(WindKnS, S); }
	float SigWaveHeightM() { return GHs; }
	float WaveTravelYaw() { return WindYaw; }
	float GetSeaScale() { return WindForState(GetSeaState()) / 16.f; }

	float RenderedZ(UWorld* World, const FVector& P) { return AIronwakeShell::WaterZAtWaves(World, P) + AIronwakeShell::RenderedSeaOffsetCm; }

	static void ShaderWaves(const UIronwakeWaveGen* Gen, TArray<int32>& Idx)
	{
		Idx.Reset(); for (int32 i = 0; i < Gen->Waves.Num(); ++i) Idx.Add(i);
		Idx.Sort([&](int32 A, int32 B) { const FGerstnerWave& a = Gen->Waves[A]; const FGerstnerWave& b = Gen->Waves[B];
			return FMath::Abs(a.Steepness - b.Steepness) > 1e-4f ? a.Steepness > b.Steepness : a.Amplitude > b.Amplitude; });
		if (Idx.Num() > 16) Idx.SetNum(16);
	}

	// W3: whitecap threshold on the analytic Jacobian (same 16 waves as the shader), chosen so the J < JT area is the target
	// coverage for the sea state (CALM 0, MODERATE 1.5 %, ROUGH 7 %, GALE 15 %, STORM 24 %). Deterministic grid sample.
	static float CalibrateJT(const UIronwakeWaveGen* Gen, float State, float& OutCov)
	{
		// W4: caps are the main foam (lace/streak are small add-ons); -iwcovk= scales the target for tuning runs.
		// W4: target = J<JT area; measured full-mask coverage (w4cov.py) at these values: ROUGH 5.6-8 %, STORM 12-24 %.
		static const float Cov[5] = { 0.0f, 0.023f, 0.095f, 0.19f, 0.30f };
		static const float CovK = [] { float V = 1.f; FParse::Value(FCommandLine::Get(), TEXT("iwcovk="), V); return V; }();
		OutCov = Lerp5(Cov, State) * CovK;
		if (OutCov < 0.002f) return -10.f;
		TArray<int32> Idx; ShaderWaves(Gen, Idx);
		TArray<float> Js; Js.Reserve(4096);
		for (int32 a = 0; a < 64; ++a) for (int32 b = 0; b < 64; ++b)
		{
			const float X = a * 1731.f + b * 97.f, Y = b * 1913.f - a * 53.f;
			float J = 1.f;
			for (int32 i : Idx) { const FGerstnerWave& W = Gen->Waves[i]; const float K = 2.f * PI / W.WaveLength; J -= W.Steepness * FMath::Cos((X * W.Direction.X + Y * W.Direction.Y) * K + 0.37f * i); }
			Js.Add(J);
		}
		Js.Sort();
		return Js[FMath::Clamp(FMath::RoundToInt(OutCov * Js.Num()), 0, Js.Num() - 1)];
	}

	static AWaterBody* FindOcean(UWorld* World)
	{
		for (TActorIterator<AWaterBody> It(World); It; ++It) if (It->GetWaterBodyType() == EWaterBodyType::Ocean) return *It;
		return nullptr;
	}

	static bool Apply(UWorld* World, float State, float AmpMul)
	{
		AWaterBody* Ocean = FindOcean(World);
		if (!Ocean) return false;
		UGerstnerWaterWaves* Waves = Ocean->GetWaterWaves() ? Cast<UGerstnerWaterWaves>(Ocean->GetWaterWaves()->GetWaterWaves()) : nullptr;
		if (!Waves) { UE_LOG(LogIronwake, Warning, TEXT("IWW2 ocean has no Gerstner waves")); return false; }
		UIronwakeWaveGen* Gen = (GSeaWorld.Get() == World) ? GGen.Get() : nullptr;
		if (!Gen)
		{
			Gen = NewObject<UIronwakeWaveGen>(Waves);
			Waves->GerstnerWaveGenerator = Gen;
			GGen = Gen; GSeaWorld = World; GWaves = Waves; GOceanC = Ocean->GetWaterBodyComponent();
		}
		Gen->Waves.Reset();
		double SumA2 = 0.0;
		const float StT = Lerp5(SteepTotal, State);
		for (int32 i = 0; i < NComp; ++i)
		{
			const FComp& C = Comps[i];
			const float Hs = Lerp5(GroupHs[C.G], State) * AmpMul;
			const float A = Hs / (2.83f * FMath::Sqrt((float)GroupN[C.G]));   // m, per component
			FGerstnerWave& W = Gen->Waves.AddDefaulted_GetRef();
			W.WaveLength = C.WL * 100.f;
			W.Amplitude = FMath::Max(A * 100.f, 0.05f);
			W.Steepness = AmpMul > 0.01f ? StT * SteepShare[C.G] / GroupN[C.G] : 0.f;
			const float Yaw = FMath::DegreesToRadians(WindYaw + C.Dir);
			W.Direction = FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);
			SumA2 += A * A;
		}
		GHs = 2.83f * FMath::Sqrt((float)SumA2);
		Waves->RecomputeWaves(false);
		if (FWaterBodyManager* Mgr = UWaterSubsystem::GetWaterBodyManager(World)) Mgr->RequestWaveDataRebuild();
		if (UWaterBodyComponent* C = Ocean->GetWaterBodyComponent())
			if (UMaterialInstanceDynamic* MID = C->GetWaterMaterialInstance())
			{
				// Whitecaps: the water material's crest foam (wave height/steepness driven) rises with the sea state.
				MID->SetScalarParameterValue(TEXT("Foam Boost"), Lerp5(FoamBoost, State) * FMath::Clamp(AmpMul, 0.3f, 1.f));
				MID->SetScalarParameterValue(TEXT("Height Bias"), Lerp5(FoamHeightBias, State));
				MID->SetScalarParameterValue(TEXT("FoamContrast"), FMath::Lerp(1.f, 1.6f, FMath::Clamp(State / 4.f, 0.f, 1.f)));
				// W2 round 2: own whitecap layer (M_IW_WaterRoot custom node): crest height over the Gerstner WPO x slope x wind,
				// broken up by a foam texture, plus wind-aligned streaks (wind lanes) in GALE/STORM.
				static const float Whitecap[5] = { 0.0f, 0.6f, 1.0f, 1.0f, 1.0f };
				static const float Streak[5] = { 0.0f, 0.0f, 0.25f, 0.75f, 1.0f };
				static const float CrestK[5] = { 0.6f, 0.3f, 0.12f, 0.06f, 0.02f };
				MID->SetScalarParameterValue(TEXT("IW_Hs"), FMath::Max(GHs, 0.3f));
				static const float FoamDbg = [] { float V = 1.f; FParse::Value(FCommandLine::Get(), TEXT("iwfoamdbg="), V); return V; }();
				MID->SetScalarParameterValue(TEXT("IW_Whitecap"), Lerp5(Whitecap, State) * FMath::Clamp(AmpMul, 0.f, 1.f) * FoamDbg);
				static const float StreakK = [] { float V = 1.f; FParse::Value(FCommandLine::Get(), TEXT("iwstreakk="), V); return V; }();
				MID->SetScalarParameterValue(TEXT("IW_Streak"), Lerp5(Streak, State) * FMath::Clamp(AmpMul, 0.f, 1.f) * StreakK);
				MID->SetScalarParameterValue(TEXT("IW_CrestK"), Lerp5(CrestK, State));
				static const float FoamMode = [] { float V = 0.f; FParse::Value(FCommandLine::Get(), TEXT("iwfoammode="), V); return V; }();
				MID->SetScalarParameterValue(TEXT("IW_Debug"), FoamMode);
				// W3: Jacobian whitecaps at a calibrated coverage + lace that lingers ~2-4 s behind the crests.
				{ float Cov = 0.f; const float JT = CalibrateJT(Gen, State, Cov); MID->SetScalarParameterValue(TEXT("IW_JT"), JT);
				  static const float LaceK = [] { float V = 1.f; FParse::Value(FCommandLine::Get(), TEXT("iwlacek="), V); return V; }();
				  MID->SetScalarParameterValue(TEXT("IW_Lace"), FMath::Clamp((State - 0.8f) / 2.f, 0.f, 1.f) * 0.9f * LaceK); MID->SetScalarParameterValue(TEXT("IW_LaceT"), 2.2f);
				  if (FMath::Abs(State - GState) > 0.2f || GState < 0.f) UE_LOG(LogIronwake, Display, TEXT("IWW3 whitecap JT=%.3f target coverage=%.1f pct"), JT, Cov * 100.f); }
				// Foam is emissive-weighted (SingleLayerWater shading darkens the blended base colour, worst at night/storm light).
				static const float FoamGlow[5] = { 0.25f, 0.35f, 0.5f, 0.6f, 0.65f }; // W4: STORM 0.9 blew the caps out to flat white
				MID->SetScalarParameterValue(TEXT("IW_FoamGlow"), Lerp5(FoamGlow, State));
				const float PlaneZ = AIronwakeShell::WaterZAt(World, FVector(60000.f, 40000.f, 0.f));
				static const float PlaneBias = [] { float V = 0.f; FParse::Value(FCommandLine::Get(), TEXT("iwplanebias="), V); return V; }();
				MID->SetScalarParameterValue(TEXT("IW_PlaneZ"), 0.f); (void)PlaneZ; (void)PlaneBias; // h is now analytic (relative to the mean surface)
				static bool bPZ = false; if (!bPZ) { bPZ = true; UE_LOG(LogIronwake, Display, TEXT("IWW2 foam plane z=%.0f (ocean actor z=%.0f)"), PlaneZ, Ocean->GetActorLocation().Z); }
				// Kill the green rim: deep-ocean blue-grey scattering, absorption up with storminess (less light through crests).
				const float Sk = FMath::Clamp((State - 1.f) / 3.f, 0.f, 1.f);
				MID->SetVectorParameterValue(TEXT("Scattering"), FMath::Lerp(FMath::Lerp(FLinearColor(0.012f, 0.016f, 0.018f), FLinearColor(0.006f, 0.007f, 0.009f), Sk), FLinearColor(0.35f, 0.55f, 1.0f), GNightWater));   // P13: moonlit night gets a blue body glow so the sea reads
				MID->SetVectorParameterValue(TEXT("Absorption"), FMath::Lerp(FLinearColor(0.35f, 0.08f, 0.06f), FLinearColor(0.45f, 0.2f, 0.12f), Sk));
				static bool bLogged = false;
				if (!bLogged) { bLogged = true; float Dv = -1.f; const bool bHas = MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("IW_Debug")), Dv);
					UE_LOG(LogIronwake, Display, TEXT("IWW2 water MID parent=%s base=%s IW_Debug has=%d v=%.1f"), MID->Parent ? *MID->Parent->GetPathName() : TEXT("none"), MID->GetMaterial() ? *MID->GetMaterial()->GetPathName() : TEXT("none"), bHas, Dv); }
				// P13: wind ripple normals strengthen with the wind (captured defaults x 1 .. 1.9 from CALM to STORM)
				{
					static const TCHAR* NN[] = { TEXT("Default Near Normal Strength"), TEXT("Default Distant Normal Strength"), TEXT("Default Distant Normal StrengthB"), TEXT("Far Normal Strength") };
					static float Def[4] = { -1.f, -1.f, -1.f, -1.f };
					const float Wk = 1.f + 1.8f * FMath::Clamp(State / 4.f, 0.f, 1.f);   // P14: 0.9 -> 1.8 so swell faces read
					for (int32 i = 0; i < 4; ++i)
					{
						if (Def[i] < 0.f) { float V = 0.f; Def[i] = MID->GetScalarParameterValue(FHashedMaterialParameterInfo(NN[i]), V) ? V : 0.f; UE_LOG(LogIronwake, Display, TEXT("IWP13 normal param '%s' default %.3f"), NN[i], Def[i]); }
						if (Def[i] > 0.f) MID->SetScalarParameterValue(NN[i], Def[i] * Wk);
					}
				}
				const float Wy = FMath::DegreesToRadians(WindYaw);
				MID->SetVectorParameterValue(TEXT("IW_Wind"), FLinearColor(FMath::Cos(Wy), FMath::Sin(Wy), 0.f, 0.f));
			}
		return true;
	}

	void SetSeaState(UWorld* World, float State)
	{
		if (!World) return;
		State = FMath::Clamp(State, 0.f, 4.f);
		if (GSeaWorld.Get() == World && GGen.IsValid() && FMath::Abs(State - GState) < 0.01f) return;
		if (Apply(World, State, 1.f))
		{
			if (FMath::Abs(State - GState) > 0.2f || GState < 0.f)
				UE_LOG(LogIronwake, Display, TEXT("IWW2 sea state %.2f (%s) Hs=%.1f m maxH=%.1f m wind=%.0f kn"), State, SeaStateName(State), GHs, GWaves.IsValid() ? GWaves->GetMaxWaveHeight() / 100.f : 0.f, WindForState(State));
			GState = State; GScale = -1.f;
		}
	}

	void SetSeaBand(UWorld* World, int32 NumWaves, float MinWL, float MaxWL, float MinA, float MaxA)
	{
		// Test-only (W1 band tests): replace the spectrum with an evenly spread single band.
		if (!World) return;
		SetSeaState(World, GetSeaState());
		if (UIronwakeWaveGen* Gen = GGen.Get())
		{
			Gen->Waves.Reset();
			for (int32 i = 0; i < NumWaves; ++i)
			{
				const float U = NumWaves > 1 ? (float)i / (NumWaves - 1) : 0.f;
				FGerstnerWave& W = Gen->Waves.AddDefaulted_GetRef();
				W.WaveLength = FMath::Lerp(MaxWL, MinWL, U); W.Amplitude = FMath::Lerp(MaxA, MinA, U); W.Steepness = 0.2f / NumWaves;
				const float Yaw = FMath::DegreesToRadians(WindYaw + (i % 2 ? 1.f : -1.f) * 25.f * U);
				W.Direction = FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f);
			}
			if (GWaves.IsValid()) GWaves->RecomputeWaves(false);
			if (FWaterBodyManager* Mgr = UWaterSubsystem::GetWaterBodyManager(World)) Mgr->RequestWaveDataRebuild();
			GState = -2.f; // forces the next SetSeaState
		}
	}

	void SetSeaScale(UWorld* World, float Scale)
	{
		// Legacy (W1/A1/S2 tests): 0 = flat, 0.25 calm, 0.65 calm+, 1 = MODERATE, 1.6 = ROUGH.
		if (!World) return;
		if (Scale <= 0.01f) { if (Apply(World, 0.f, 0.f)) { GState = -2.f; GScale = 0.f; } return; }
		const float S = Scale <= 1.f ? FMath::Lerp(0.f, 1.f, FMath::Clamp((Scale - 0.25f) / 0.75f, 0.f, 1.f)) : FMath::Lerp(1.f, 2.f, FMath::Clamp((Scale - 1.f) / 0.6f, 0.f, 1.5f));
		SetSeaState(World, S);
	}

	static float GFoamDbg = -1.f;
	void SetFoamDebug(float Mode) { GFoamDbg = Mode; }
	void SetNightWater(UWorld* World, float K)
	{
		K = FMath::Clamp(K, 0.f, 1.f);
		if (FMath::Abs(K - GNightWater) < 0.01f || GSeaWorld.Get() != World) return;
		GNightWater = K;
		if (UWaterBodyComponent* C = GOceanC.Get())
			if (UMaterialInstanceDynamic* MID = C->GetWaterMaterialInstance())
			{
				const float Sk = FMath::Clamp((GState - 1.f) / 3.f, 0.f, 1.f);
				MID->SetVectorParameterValue(TEXT("Scattering"), FMath::Lerp(FMath::Lerp(FLinearColor(0.012f, 0.016f, 0.018f), FLinearColor(0.006f, 0.007f, 0.009f), Sk), FLinearColor(0.35f, 0.55f, 1.0f), GNightWater));
			}
	}

	void SetFoamCamera(UWorld* World, const FVector& Cam)
	{
		if (GSeaWorld.Get() != World) return;
		if (UWaterBodyComponent* C = GOceanC.Get())
			if (UMaterialInstanceDynamic* MID = C->GetWaterMaterialInstance())
			{
				if (GFoamDbg >= 0.f) MID->SetScalarParameterValue(TEXT("IW_Debug"), GFoamDbg);
				MID->SetVectorParameterValue(TEXT("IW_CamPos"), FLinearColor(Cam.X, Cam.Y, Cam.Z, 0.f));
				PushShipWaveParams(MID, Cam);   // P19: ship-wave sources for the water material
				if (const UWaterSubsystem* WS = UWaterSubsystem::GetWaterSubsystem(World)) MID->SetScalarParameterValue(TEXT("IW_WaterTime"), WS->GetWaterTimeSeconds());
				// W3: 16 waves for the shader (largest steepness first: the Jacobian is what makes whitecaps); xy = dir*(1+steepness).
				if (const UIronwakeWaveGen* Gen = GGen.Get())
				{
					TArray<int32> Idx; ShaderWaves(Gen, Idx);
					for (int32 i = 0; i < 16; ++i)
					{
						FLinearColor V(1.f, 0.f, 10000.f, 0.f);
						if (i < Idx.Num()) { const FGerstnerWave& Wv = Gen->Waves[Idx[i]]; V = FLinearColor(Wv.Direction.X * (1.f + Wv.Steepness), Wv.Direction.Y * (1.f + Wv.Steepness), Wv.WaveLength, Wv.Amplitude); }
						MID->SetVectorParameterValue(*FString::Printf(TEXT("IW_Wave%d"), i), V);
					}
				}
			}
	}

	float AnalyticZ(UWorld* World, const FVector& P)
	{
		const UIronwakeWaveGen* Gen = GGen.Get();
		const UWaterSubsystem* WS = UWaterSubsystem::GetWaterSubsystem(World);
		if (!Gen || !WS) return 0.f;
		const float T = WS->GetWaterTimeSeconds();
		float Z = 0.f;
		for (const FGerstnerWave& W : Gen->Waves)
		{
			const float K = 2.f * PI / W.WaveLength;
			Z += W.Amplitude * FMath::Cos((P.X * W.Direction.X + P.Y * W.Direction.Y) * K - FMath::Sqrt(K * 980.f) * T);
		}
		return Z + AIronwakeShell::WaterZAt(World, P);
	}

	float FastZ(UWorld* World, const FVector& P, float* OutJ)
	{
		const UIronwakeWaveGen* Gen = GGen.Get();
		const UWaterSubsystem* WS = UWaterSubsystem::GetWaterSubsystem(World);
		if (!Gen || !WS) { if (OutJ) *OutJ = 1.f; return RenderedZ(World, P); }
		const float T = WS->GetWaterTimeSeconds();
		auto Raw = [&](const FVector& Q, float* J) {
			float Z = 0.f, Jac = 1.f;
			for (const FGerstnerWave& W : Gen->Waves)
			{
				const float K = 2.f * PI / W.WaveLength;
				const float Ph = (Q.X * W.Direction.X + Q.Y * W.Direction.Y) * K - FMath::Sqrt(K * 980.f) * T;
				const float C = FMath::Cos(Ph);
				Z += W.Amplitude * C;
				Jac -= W.Steepness * C;
			}
			if (J) *J = Jac;
			return Z;
		};
		if (GFastOffFrame != GFrameCounter && GOceanC.IsValid())
		{
			// Calibrate the analytic surface to the plugin's rendered height once per frame (plane z, render offset).
			GFastOffFrame = GFrameCounter;
			GFastOff = RenderedZ(World, P) - Raw(P, nullptr);
		}
		return Raw(P, OutJ) + GFastOff;
	}
}

// ---------------------------------------------------------------------------- bow wave ribbons
UIronwakeBowWave::UIronwakeBowWave(const FObjectInitializer& OI) : Super(OI)
{
	SetUsingAbsoluteLocation(true);
	SetUsingAbsoluteRotation(true);
	SetUsingAbsoluteScale(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	bReceivesDecals = false;
	bAffectDynamicIndirectLighting = false;
	SetGenerateOverlapEvents(false);
	TranslucencySortPriority = 12;
	bUseAsyncCooking = true;
}

void UIronwakeBowWave::Init(const FIronwakeHullProfile& InHull, float InScale)
{
	Hull = InHull; Scale = InScale; bBuilt = false;
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/FX/M_IW_BowFoam.M_IW_BowFoam"))) SetMaterial(0, M);
}

namespace
{
	constexpr int32 NCrest = 22, NStrip = 40, NMound = 13, NAcross = 4;
	// P27: zero alpha at inner edge (j=0) so ribbons feather smoothly BEFORE reaching Clear(X) and never smear onto dark hull topsides.
	const float AcrossT[NAcross] = { 0.f, 0.28f, 0.62f, 1.f };
	const float AcrossA[NAcross] = { 0.f, 0.82f, 0.52f, 0.f };
	const float AcrossH[NAcross] = { 0.25f, 1.f, 0.55f, 0.f };
}

void UIronwakeBowWave::AddRibbon(UWorld* W, const FTransform& Xf, const FVector& Origin, const TArray<FSt>& S, const FTransform* DeckXf, float DeckZ, const float* ProfA, const float* ProfH)
{
	const int32 Base = V.Num();
	const float* PA = ProfA ? ProfA : AcrossA;
	const float* PH = ProfH ? ProfH : AcrossH;
	for (int32 i = 0; i < S.Num(); ++i)
	{
		for (int32 j = 0; j < NAcross; ++j)
		{
			const FVector2D P2 = FMath::Lerp(S[i].In, S[i].Out, AcrossT[j]);
			FVector Wp;
			if (DeckXf) Wp = DeckXf->TransformPosition(FVector(P2.X * 100.f, P2.Y * 100.f, DeckZ + 30.f + S[i].H * PH[j] * 100.f)); // green water on the forecastle (follows the hull)
			else
			{
				Wp = Xf.TransformPosition(FVector(P2.X * 100.f, P2.Y * 100.f, 0.f));
				Wp.Z = IronwakeSea::RenderedZ(W, Wp) + IronwakeSea::ShipWaveZ(Wp) + 6.f + S[i].H * PH[j] * 100.f;   // P27: strictly clamped to WaterZAtWaves + ShipWaveZ + 6 cm (no vertical foam wall!)
			}
			V.Add(Wp - Origin);
			C.Add(FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(S[i].A * PA[j], 0.f, 1.f)));
			UV.Add(FVector2D(AcrossT[j], (float)i / FMath::Max(S.Num() - 1, 1)));
			N.Add(FVector::UpVector);
			Tan.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}
	}
	if (!bBuilt)
	{
		for (int32 i = 0; i + 1 < S.Num(); ++i)
			for (int32 j = 0; j + 1 < NAcross; ++j)
			{
				const int32 A = Base + i * NAcross + j, B = A + 1, Cc = A + NAcross, D = Cc + 1;
				T.Append({ A, Cc, B, B, Cc, D });
			}
	}
}

void UIronwakeBowWave::UpdateWave(const FTransform& HullXf, float SpeedFrac, float SpeedMS, float DeckWash, float BowDeckZ)
{
	UWorld* W = GetWorld();
	if (!W) return;
	const bool bOn = SpeedFrac > 0.08f;
	SetVisibility(bOn);
	if (!bOn && bBuilt) return;
	const float F = FMath::Clamp(SpeedFrac, 0.f, 1.2f);
	// Level hull frame (yaw only) at the waterline: foam lies on the sea, not on the pitching deck.
	const FVector Loc = HullXf.GetLocation();
	const FTransform Xf(FRotator(0.f, HullXf.Rotator().Yaw, 0.f), FVector(Loc.X, Loc.Y, 0.f));
	const FVector Origin(Loc.X, Loc.Y, Loc.Z);
	SetWorldLocation(Origin);
	V.Reset(); C.Reset(); UV.Reset(); N.Reset(); Tan.Reset();
	if (!bBuilt) T.Reset();
	// P34: tight waterline contact clearance (12 cm + roll margin) so subtle bow-peel & quarter lace meet the hull cleanly without floating outboard as detached slabs!
	const float RollRad = FMath::DegreesToRadians(FMath::Abs(HullXf.Rotator().Roll));
	const float Margin = 0.12f + FMath::Sin(RollRad) * FMath::Max(Hull.BowDeckM, 3.0f) * 0.65f;
	auto Clear = [&](float X) { return Hull.HalfBreadthAt(X) + Margin; };
	const float Stem = Hull.X1;
	const float L = Hull.Length();

	// 1-3) P34: M_IW_WaterRoot (SingleLayerWater ocean shader) renders 100% of sea-level bow, shoulder, and hull-waterline waves & foam directly IN the water surface so translucent ribbons NEVER create dark/sawtooth overlay seams!
	LastStemCrestM = (0.2f + 1.4f * F * F) * Scale;
	// 4) Green water on the forecastle ONLY when the stem buries in heavy storm seas (DeckWash > 0.02).
	if (DeckWash <= 0.02f)
	{
		SetVisibility(false);
		return;
	}
	{
		TArray<FSt> S;
		const float A = FMath::Clamp(DeckWash, 0.f, 1.f);
		for (int32 i = 0; i < 8; ++i)
		{
			const float U = i / 7.f;
			const float X = Stem - 1.f - U * 0.16f * L;
			const float HB = FMath::Max(Hull.HalfBreadthAt(X) - 0.4f, 0.3f);
			S.Add({ FVector2D(X, -HB), FVector2D(X, HB), 0.f, A * (1.f - 0.7f * U), 0.25f * A });
		}
		AddRibbon(W, Xf, Origin, S, &HullXf, BowDeckZ);
	}
	if (!bBuilt)
	{
		CreateMeshSection_LinearColor(0, V, T, N, UV, C, Tan, false, false);
		bBuilt = true;
	}
	else
	{
		UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
	}
}

// ---------------------------------------------------------------------------- W3 Kelvin wake trail
namespace { constexpr int32 WakeN = 160; constexpr float WakeStep = 0.5f; constexpr int32 WakeAcross = 6; }

UIronwakeWake::UIronwakeWake(const FObjectInitializer& OI) : Super(OI)
{
	SetUsingAbsoluteLocation(true); SetUsingAbsoluteRotation(true); SetUsingAbsoluteScale(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision); SetCastShadow(false); SetGenerateOverlapEvents(false);
	bReceivesDecals = false; bAffectDynamicIndirectLighting = false;
	TranslucencySortPriority = 11;
	bUseAsyncCooking = true;
}

void UIronwakeWake::Init(float InLengthM, float InHalfBeamM, bool bInSteamer)
{
	LengthM = InLengthM; HalfBeamM = InHalfBeamM; bSteamer = bInSteamer;
	UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Water/M_IW_WakeLut.M_IW_WakeLut"));
	if (!M) M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Islands/M_IW_Surf2.M_IW_Surf2"));
	if (M)
	{
		MID = UMaterialInstanceDynamic::Create(M, this);
		MID->SetScalarParameterValue(TEXT("Thresh"), 1.35f);
		MID->SetScalarParameterValue(TEXT("Contrast"), 2.4f);
		MID->SetVectorParameterValue(TEXT("FoamColor"), FLinearColor(0.85f, 0.88f, 0.9f));
		SetMaterial(0, MID);
	}
}

void UIronwakeWake::UpdateWake(float Dt, const FTransform& HullXf, float SternXM, float BowXM, float SpeedMS, float SpeedFrac, float SeaState)
{
	UWorld* W = GetWorld();
	if (!W) return;
	Age += Dt; Acc += Dt;
	const FVector Fwd3 = FRotator(0.f, HullXf.Rotator().Yaw, 0.f).Vector();
	const FVector Loc = HullXf.GetLocation();
	const FVector2D F2(Fwd3.X, Fwd3.Y), L2(Loc.X, Loc.Y);
	if (Hist.Num() > 0 && FVector2D::Distance(Hist[0].Stern, L2 + F2 * SternXM * 100.f) > FMath::Max(6000.f, SpeedMS * 100.f * 1.5f + 3000.f)) { Hist.Reset(); Acc = WakeStep; }
	float StepNow = WakeStep;
	if (Hist.Num() > 0) { const float Dot = FMath::Clamp((float)FVector2D::DotProduct(F2, Hist[0].Fwd), -1.f, 1.f); const float TurnDeg = FMath::RadiansToDegrees(FMath::Acos(Dot)); StepNow = FMath::Clamp(WakeStep - TurnDeg * 0.12f, 0.12f, WakeStep); }
	if (Acc >= StepNow || Hist.Num() == 0)
	{
		Acc = 0.f;
		Hist.Insert({ L2 + F2 * SternXM * 100.f, L2 + F2 * BowXM * 100.f, F2, Age, SpeedMS, SpeedFrac, ThrustSteam_ }, 0);
		if (Hist.Num() > WakeN) Hist.SetNum(WakeN);
	}
	Hist[0] = { L2 + F2 * SternXM * 100.f, L2 + F2 * BowXM * 100.f, F2, Age, SpeedMS, SpeedFrac, ThrustSteam_ };
	const FVector Origin(Loc.X, Loc.Y, Loc.Z);
	SetWorldLocation(Origin);
	V.Reset(); C.Reset(); UV.Reset(); N.Reset(); Tan.Reset();
	if (!bBuilt) T.Reset();
	const float SeaFoam = FMath::Clamp(1.f - SeaState * 0.12f, 0.5f, 1.f);
	const float LwlM = FMath::Max(BowXM - SternXM + 4.f, 10.f);
	const float WkT[WakeAcross] = { 0.f, 0.2f, 0.45f, 0.65f, 0.85f, 1.f };
	const float WkA[WakeAcross] = { 0.f, 0.45f, 1.f, 0.85f, 0.35f, 0.f };
	float RowZ = 0.f, RowD = 0.f;
	auto Emit = [&](const FVector2D& P, const FVector2D& Uv, const FLinearColor& Col, float Lift)
	{
		const float Z = IronwakeSea::FastZ(W, FVector(P.X, P.Y, 0.f)) + IronwakeSea::ShipWaveZ(FVector(P.X, P.Y, 0.f)) + RowD + Lift;
		V.Add(FVector(P.X, P.Y, Z) - Origin);
		C.Add(Col);
		UV.Add(Uv);
		N.Add(FVector::UpVector); Tan.Add(FProcMeshTangent(1.f, 0.f, 0.f));
	};
	// ribbon 0: turbulent centre prop-wash jet; ribbons 1/2: curved Kelvin echelon arms (smooth handoff past 1.4 L from ocean-shader LUT)
	for (int32 Rb = 0; Rb < 3; ++Rb)
	{
		const int32 Base = V.Num();
		const float Side = Rb == 1 ? -1.f : 1.f;
		for (int32 i = 0; i < WakeN; ++i)
		{
			const bool bHave = i < Hist.Num();
			const FS& S = bHave ? Hist[i] : Hist.Last();
			const float A0 = Age - S.T;
			const FVector2D Rt(-S.Fwd.Y, S.Fwd.X);
			const float Kel = FMath::Tan(FMath::DegreesToRadians(IronwakeSea::WedgeHalfAngleDeg(S.V / FMath::Sqrt(9.81f * LwlM))));
			const float SpeedOK = FMath::Clamp(FMath::Pow(FMath::Clamp(S.F, 0.f, 1.2f), 1.5f), 0.f, 1.f);
			const float DistM = FMath::Max(S.V * A0, 0.f);
			const float sL = DistM / LwlM;
			// Lateral deviation from current straight ship axis: on straight courses 0..4.2 L is owned 100% by the crisp 1024x1024 ocean-shader LUT
			// (zero low-poly ribbon dithering!); during turns or past 4.2 L astern, UIronwakeWake smoothly carries the curved trail!
			const float LatStraightCm = FMath::Abs((float)FVector2D::DotProduct(S.Stern - (L2 + F2 * SternXM * 100.f), FVector2D(-F2.Y, F2.X)));
			const float TurnFade = FMath::SmoothStep(HalfBeamM * 45.f, HalfBeamM * 160.f, LatStraightCm);
			{
				FVector2D Pc;
				if (Rb == 0) Pc = S.Stern; else Pc = S.Bow + Rt * Side * FMath::Max(DistM * Kel * 100.f, HalfBeamM * 100.f);
				RowZ = (bHave && (i % 2 == 0 || i < 12)) ? IronwakeSea::RenderedZ(W, FVector(Pc.X, Pc.Y, 0.f)) : RowZ;
				if (bHave && (i % 2 == 0 || i < 12)) RowD = FMath::Clamp(RowZ - IronwakeSea::FastZ(W, FVector(Pc.X, Pc.Y, 0.f)), -150.f, 150.f);
			}
			for (int32 j = 0; j < WakeAcross; ++j)
			{
				FVector2D P; float Al = 0.f, Drive = 0.f;
				if (Rb == 0)
				{
					const float Half = (HalfBeamM * 0.34f + LwlM * 0.155f * FMath::Pow(sL, 0.45f)) * 100.f;
					P = S.Stern + Rt * FMath::Lerp(-Half, Half, WkT[j]);
					const float Life = FMath::Lerp(10.f, 75.f, FMath::Min(S.F, 1.f)) * SeaFoam;
					static const float TwinA[WakeAcross] = { 0.f, 0.78f, 0.46f, 0.46f, 0.78f, 0.f };
					const float Ac = (bSteamer && bTwin_ ? TwinA[j] : WkA[j]);
					// P34: M_IW_WaterRoot handles straight-line wake out to 14.5 L! Ribbon only fills curved turning trails (>1.2 L astern) and ultra-far (>9 L) foam!
					const float NearHandoff = FMath::Max(TurnFade * FMath::SmoothStep(1.2f, 2.5f, sL), FMath::SmoothStep(8.5f, 11.5f, sL));
					Drive = bSteamer ? FMath::Clamp(FMath::Max(S.Thrust, 0.12f * SpeedOK), 0.f, 1.15f) : (0.06f * SpeedOK);
					Al = Ac * NearHandoff * FMath::Pow(FMath::Clamp(1.f - A0 / Life, 0.f, 1.f), 1.45f) * 0.62f;
				}
				else
				{
					const float Spread = DistM * Kel * 100.f;
					const float LenK = FMath::Clamp(LengthM / 90.f, 0.4f, 1.3f);
					const float Wd = (2.2f + 0.095f * DistM) * 100.f * LenK;
					const float Off = FMath::Max(Spread, (HalfBeamM + 0.4f) * 100.f) + Wd * (WkT[j] - 0.18f);
					P = S.Bow + Rt * Side * Off;
					const float Life = FMath::Lerp(8.f, 62.f, FMath::Min(S.F, 1.f)) * SeaFoam;
					const float NearHandoff = FMath::Max(TurnFade * FMath::SmoothStep(1.2f, 2.5f, sL), FMath::SmoothStep(8.5f, 11.5f, sL));
					Drive = 0.38f * SpeedOK;
					Al = WkA[j] * NearHandoff * FMath::Pow(FMath::Clamp(1.f - A0 / Life, 0.f, 1.f), 1.40f) * 0.55f;
				}
				if (!bHave) Al = 0.f;
				// P27: clamped strictly to local wave surface + 6 cm (was +70 cm!) so wake ribbons never float over sterns or climb hulls!
				Emit(P, FVector2D(WkT[j], sL), FLinearColor(Drive, Rb == 0 ? 1.f : 0.f, 0.f, FMath::Clamp(Al, 0.f, 1.f)), 6.f);
			}
		}
		if (!bBuilt)
			for (int32 i = 0; i + 1 < WakeN; ++i)
				for (int32 j = 0; j + 1 < WakeAcross; ++j)
				{
					const int32 A = Base + i * WakeAcross + j, B = A + 1, Cc = A + WakeAcross, D = Cc + 1;
					T.Append({ A, Cc, B, B, Cc, D });
				}
	}
	if (!bBuilt) { CreateMeshSection_LinearColor(0, V, T, N, UV, C, Tan, false, false); bBuilt = true; }
	else UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
	static const bool bDbg = FParse::Param(FCommandLine::Get(), TEXT("iwwakedbg"));
	if (bDbg && FMath::Fmod(Age, 5.f) < Dt + 1e-4f && GetOwner() && GetOwner()->GetInstigatorController() && GetOwner()->GetInstigatorController()->IsPlayerController())
	{
		int32 NA = 0; double Gap = 0.0; int32 NG = 0;
		for (int32 k = 0; k < V.Num(); ++k) if (C[k].A > 0.2f) { ++NA; if ((k % 7) == 0) { const FVector Wp = V[k] + Origin; Gap += Wp.Z - IronwakeSea::RenderedZ(W, Wp); ++NG; } }
		UE_LOG(LogIronwake, Display, TEXT("IWW4 wake hist=%d verts=%d alpha>0.2=%d z-minus-sea=%.0f cm bounds=%s vis=%d kn-ish=%.1f"), Hist.Num(), V.Num(), NA, NG ? Gap / NG : 0.0, *Bounds.GetBox().GetSize().ToString(), IsVisible(), SpeedMS / 0.5144f / 1.5f);
	}
}

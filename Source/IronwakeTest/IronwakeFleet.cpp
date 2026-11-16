// S2: fleet roster, progression, wind, and the frigate's moving sail rig.
#include "IronwakeFleet.h"
#include "IronwakeTest.h"
#include "IronwakeHydro.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"

// ============================================================================ roster
namespace
{
	const TCHAR* Section = TEXT("Ironwake");

	TArray<FIronwakeShipDef> MakeRoster()
	{
		TArray<FIronwakeShipDef> R;
		{
			// (c) steam-sail frigate: Svjatoi Nikolai hull (sails/yards stripped) scaled to 88 m overall, 67 m waterline.
			FIronwakeShipDef D;
			D.Id = TEXT("frigate"); D.Name = TEXT("Albatross"); D.Class = TEXT("Steam-sail frigate  |  88 m  |  2,200 t");
			D.Blurb = TEXT("Three masts, square sails. Fastest on a broad reach, slow close-hauled, can't sail into the wind. Auxiliary steam (B).");
			D.UnlockText = TEXT("Start ship");
			D.HullMesh = TEXT("/Game/Ironwake/Ships/Frigate/SM_Frigate_Hull.SM_Frigate_Hull");
			D.LengthM = 88.f; D.BeamM = 16.4f; D.MassKg = 2.2e6f; D.bSail = true;
			D.TelegraphKnots = { -5.f, -2.5f, 0.f, 4.f, 6.f, 8.f, 9.f };   // auxiliary steam only; sails add their own
			D.MaxAccelMS2 = 0.35f; D.SpeedTimeConstant = 6.f; D.MinTurnRadiusM = 185.f; D.YawResponse = 2.2f; D.LateralDrag = 2.6f; D.RudderSlew = 18.f;
			D.HeavePeriod = 5.5f; D.HeaveZeta = 0.5f; D.PitchPeriod = 5.5f; D.PitchZeta = 0.55f; D.RollPeriod = 8.f; D.RollZeta = 0.4f; D.SeaFollowHeave = 1.f; D.SeaFollowPitch = 1.f; D.SeaFollowRoll = 0.85f; D.FitSpan = 0.7f;
			D.DraftBelowBootTopCm = -40.f; // W3 fix: boot-top 0.4 m above the true sea (the old +100 was tuned against a 4.7 m-too-low sea reference)
			D.WaveCrestK = 0.30f; D.WaveTroughK = 0.08f; D.WaveQuarterK = 0.50f; D.WaveWedgeK = 1.0f; D.WaveBluff = 0.3f; D.FreeboardM = 4.f; D.DraftM = 4.2f;
			D.Profile = &FIronwakeHullProfile::Frigate;
			R.Add(D);
		}
		{
			// P26: 42 m two-masted merchant brig (rigged SM_TallBrig + 17 moving spar/sail parts incl. gaff spanker).
			FIronwakeShipDef D;
			D.Id = TEXT("brig"); D.Name = TEXT("Curlew"); D.Class = TEXT("Merchant brig  |  42 m  |  480 t");
			D.Blurb = TEXT("Two masts, square sails and gaff spanker. Handier in stays than the frigate, lively on a reach. Auxiliary steam (B).");
			D.UnlockText = TEXT("Merchant service, always available");
			D.HullMesh = TEXT("/Game/Ironwake/World/TallBrig/SM_TallBrig.SM_TallBrig");
			D.LengthM = 42.f; D.BeamM = 9.5f; D.MassKg = 4.8e5f; D.bSail = true;
			D.TelegraphKnots = { -5.f, -2.5f, 0.f, 4.f, 6.f, 8.f, 9.5f };
			D.MaxAccelMS2 = 0.55f; D.SpeedTimeConstant = 3.8f; D.MinTurnRadiusM = 95.f; D.YawResponse = 3.1f; D.LateralDrag = 3.2f; D.RudderSlew = 26.f;
			D.HeavePeriod = 4.1f; D.HeaveZeta = 0.52f; D.PitchPeriod = 4.0f; D.PitchZeta = 0.55f; D.RollPeriod = 6.2f; D.RollZeta = 0.42f;
			D.SeaFollowHeave = 1.f; D.SeaFollowPitch = 1.f; D.SeaFollowRoll = 0.92f; D.FitSpan = 0.78f;
			D.DraftBelowBootTopCm = 310.f; // SM_TallBrig z=0 is at the KEEL -> +310 cm keeps Nelson gunport band well above sea with ~0.40 m dark boot-top below it
			D.WaveCrestK = 0.28f; D.WaveTroughK = 0.06f; D.WaveQuarterK = 0.52f; D.WaveWedgeK = 1.05f; D.WaveBluff = 0.28f; D.FreeboardM = 3.20f; D.DraftM = 3.1f;
			D.Profile = &FIronwakeHullProfile::TallBrig;
			R.Add(D);
		}
		{
			// (a) start ship: Aglae-class steam launch, 30 m. Quick to answer the telegraph, turns on a sixpence, lively
			// in a seaway (short hull follows the chop; heave/pitch periods ~3 s).
			FIronwakeShipDef D;
			D.Id = TEXT("cutter"); D.Name = TEXT("Kittiwake"); D.Class = TEXT("Steam cutter  |  30 m  |  60 t");
			D.Blurb = TEXT("Quick and nimble steam launch for harbour work.");
			D.UnlockText = TEXT("Harbour tender, always available");
			D.HullMesh = TEXT("/Game/Ironwake/Ships/Aglae/SM_Aglae_Hull.SM_Aglae_Hull");
			D.LengthM = 30.f; D.BeamM = 3.9f; D.MassKg = 6.0e4f; D.bFunnelSmoke = true;
			D.TelegraphKnots = { -6.f, -3.f, 0.f, 6.f, 10.f, 14.f, 17.f };
			D.MaxAccelMS2 = 2.2f; D.SpeedTimeConstant = 1.2f; D.MinTurnRadiusM = 55.f; D.YawResponse = 5.f; D.LateralDrag = 4.5f; D.RudderSlew = 45.f;
			D.HeavePeriod = 3.2f; D.HeaveZeta = 0.5f; D.PitchPeriod = 3.0f; D.PitchZeta = 0.5f; D.RollPeriod = 4.5f; D.RollZeta = 0.4f; D.SeaFollowHeave = 1.f; D.SeaFollowPitch = 1.f; D.SeaFollowRoll = 1.f; D.FitSpan = 1.f;
			D.DraftBelowBootTopCm = -25.f; // W3 fix: true sea reference (old +300 compensated the bogus -4.7 m offset)
			D.FunnelTop = FVector(60.f, 0.f, 965.f); D.SmokeScale = 0.2f;
			D.WaveCrestK = 0.22f; D.WaveTroughK = 0.f; D.WaveQuarterK = 0.55f; D.WaveWedgeK = 1.1f; D.WaveBluff = 0.15f; D.FreeboardM = 1.3f; D.DraftM = 1.4f;
			D.Profile = &FIronwakeHullProfile::Aglae;
			R.Add(D);
		}
		{
			// P32: 26 m Chapman gun sloop (historical Chapman Architectura Navalis Mercatoria sloop, nimble fore-and-aft + topsail coastal runner).
			FIronwakeShipDef D;
			D.Id = TEXT("sloop"); D.Name = TEXT("Petrel"); D.Class = TEXT("Chapman gun sloop  |  26 m  |  165 t");
			D.Blurb = TEXT("Fast single-masted historical Chapman sloop with woven flax sails. Points close to the wind and slips into tight coves. Auxiliary steam (B).");
			D.UnlockText = TEXT("Dispatch & packet sloop, always available");
			D.HullMesh = TEXT("/Game/Ironwake/World/ChapmanSloop/SM_ChapmanSloop.SM_ChapmanSloop");
			D.LengthM = 26.f; D.BeamM = 6.2f; D.MassKg = 1.65e5f; D.bSail = true;
			D.TelegraphKnots = { -5.f, -2.5f, 0.f, 4.5f, 7.f, 9.5f, 11.5f };
			D.MaxAccelMS2 = 0.95f; D.SpeedTimeConstant = 2.4f; D.MinTurnRadiusM = 58.f; D.YawResponse = 4.2f; D.LateralDrag = 3.8f; D.RudderSlew = 34.f;
			D.HeavePeriod = 3.4f; D.HeaveZeta = 0.50f; D.PitchPeriod = 3.3f; D.PitchZeta = 0.52f; D.RollPeriod = 5.0f; D.RollZeta = 0.42f;
			D.SeaFollowHeave = 1.f; D.SeaFollowPitch = 1.f; D.SeaFollowRoll = 0.96f; D.FitSpan = 0.85f;
			D.DraftBelowBootTopCm = 195.f; // SM_ChapmanSloop z=0 is at the KEEL -> +195 cm sets calm waterline cleanly on lower hull
			D.WaveCrestK = 0.24f; D.WaveTroughK = 0.04f; D.WaveQuarterK = 0.54f; D.WaveWedgeK = 1.08f; D.WaveBluff = 0.20f; D.FreeboardM = 2.1f; D.DraftM = 1.95f;
			D.Profile = &FIronwakeHullProfile::ChapmanSloop;
			R.Add(D);
		}
		{
			// P32: 38 m Amphion two-masted topsail schooner (sleek clipper-bowed archipelago packet schooner).
			FIronwakeShipDef D;
			D.Id = TEXT("schooner"); D.Name = TEXT("Amphion"); D.Class = TEXT("Topsail schooner  |  38 m  |  320 t");
			D.Blurb = TEXT("Two-masted topsail packet schooner. High close-hauled speed and generous 85-ton hold for long archipelago trade runs. Auxiliary steam (B).");
			D.UnlockText = TEXT("Archipelago packet schooner, always available");
			D.HullMesh = TEXT("/Game/Ironwake/World/AmphionSchooner/SM_AmphionSchooner.SM_AmphionSchooner");
			D.LengthM = 38.f; D.BeamM = 8.4f; D.MassKg = 3.2e5f; D.bSail = true;
			D.TelegraphKnots = { -5.f, -2.5f, 0.f, 4.5f, 6.8f, 9.0f, 11.0f };
			D.MaxAccelMS2 = 0.72f; D.SpeedTimeConstant = 3.1f; D.MinTurnRadiusM = 78.f; D.YawResponse = 3.6f; D.LateralDrag = 3.5f; D.RudderSlew = 30.f;
			D.HeavePeriod = 3.8f; D.HeaveZeta = 0.51f; D.PitchPeriod = 3.7f; D.PitchZeta = 0.54f; D.RollPeriod = 5.8f; D.RollZeta = 0.42f;
			D.SeaFollowHeave = 1.f; D.SeaFollowPitch = 1.f; D.SeaFollowRoll = 0.94f; D.FitSpan = 0.82f;
			D.DraftBelowBootTopCm = 265.f; // SM_AmphionSchooner z=0 is at the KEEL -> +265 cm places hull cleanly in the water
			D.WaveCrestK = 0.26f; D.WaveTroughK = 0.05f; D.WaveQuarterK = 0.53f; D.WaveWedgeK = 1.06f; D.WaveBluff = 0.24f; D.FreeboardM = 2.7f; D.DraftM = 2.65f;
			D.Profile = &FIronwakeHullProfile::AmphionSchooner;
			R.Add(D);
		}
		{
			// (b) the heavy ironclad: unchanged H1/W1 tuning (values come from BP_PlayerShip / the pawn defaults).
			FIronwakeShipDef D;
			D.Id = TEXT("ironclad"); D.Name = TEXT("Devastation"); D.Class = TEXT("Turret ironclad  |  125 m  |  8,000 t");
			D.Blurb = TEXT("Heavy steam ironclad, twin turrets. Slow to answer, unstoppable once moving.");
			D.UnlockText = FString::Printf(TEXT("Complete %d voyages (point-of-interest arrivals)"), IronwakeFleet::ArrivalsForIronclad);
			D.HullMesh = TEXT("/Game/Ironwake/Ships/Devastation/SM_Devastation_Hull.SM_Devastation_Hull");
			D.LengthM = 125.f; D.BeamM = 25.8f; D.MassKg = 8.0e6f; D.bTurrets = true; D.bProps = true;
			D.DraftBelowBootTopCm = -50.f; D.MinTurnRadiusM = 330.f; /*also the pawn default, IronwakeShipPawn.h*/  // P8 (Chuck OK 21:25): 280 -> 330, circle ~3.5 min; P3: circle 4.0 -> ~3.5 min; W3 fix: true sea reference
			D.WaveCrestK = 0.45f; D.WaveTroughK = 0.10f; D.WaveQuarterK = 0.60f; D.WaveWedgeK = 0.9f; D.WaveBluff = 0.8f; D.FreeboardM = 3.2f; D.DraftM = 8.f;
			D.Profile = &FIronwakeHullProfile::Devastation;
			R.Add(D);
		}
		return R;
	}

	int32 ReadInt(const TCHAR* Key, int32 Def) { int32 V = Def; if (GConfig) GConfig->GetInt(Section, Key, V, GGameUserSettingsIni); return V; }
	void WriteInt(const TCHAR* Key, int32 V) { if (!GConfig) return; GConfig->SetInt(Section, Key, V, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }

	bool bWindFixed = false;
	float FixedFrom = 300.f, FixedKn = 16.f;
}

namespace IronwakeFleet
{
	const TArray<FIronwakeShipDef>& Roster() { static TArray<FIronwakeShipDef> R = MakeRoster(); return R; }

	const FIronwakeShipDef* Find(FName Id)
	{
		for (const FIronwakeShipDef& D : Roster()) if (D.Id == Id) return &D;
		return nullptr;
	}

	FName GetSelected()
	{
		FString Cmd;
		if (FParse::Value(FCommandLine::Get(), TEXT("iwship="), Cmd) && Find(FName(*Cmd))) return FName(*Cmd);
		FString S = TEXT("frigate");
		if (GConfig) GConfig->GetString(Section, TEXT("Ship"), S, GGameUserSettingsIni);
		const FName N(*S);
		return (Find(N) && IsUnlocked(N)) ? N : FName(TEXT("frigate"));
	}

	void SetSelected(FName Id)
	{
		if (!Find(Id) || !GConfig) return;
		GConfig->SetString(Section, TEXT("Ship"), *Id.ToString(), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		UE_LOG(LogIronwake, Display, TEXT("IWS2 selected ship %s (saved)"), *Id.ToString());
	}

	int32 GetArrivals() { return ReadInt(TEXT("Arrivals"), 0); }
	int32 GetVisitedMask() { return ReadInt(TEXT("PoiMask"), 0); }

	bool IsUnlocked(FName Id)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("iwunlockall"))) return true;
		// Chuck (22:35): the sailing frigate is the start ship; the merchant brig and cutter are always available; the ironclad is earned by voyages or 1,200 Silver Crowns.
		if (Id == TEXT("frigate") || Id == TEXT("brig") || Id == TEXT("cutter") || Id == TEXT("sloop") || Id == TEXT("schooner")) return true;
		if (Id == TEXT("ironclad")) return GetArrivals() >= ArrivalsForIronclad || ReadInt(TEXT("Crowns"), 0) >= 1200;
		return false;
	}

	TArray<FString> RecordArrival(int32 PoiIndex, int32 NumPois)
	{
		TArray<bool> Before;
		for (const FIronwakeShipDef& D : Roster()) Before.Add(IsUnlocked(D.Id));
		if (PoiIndex >= 0 && PoiIndex < 31) WriteInt(TEXT("PoiMask"), GetVisitedMask() | (1 << PoiIndex));
		WriteInt(TEXT("Arrivals"), GetArrivals() + 1);
		TArray<FString> New;
		for (int32 i = 0; i < Roster().Num(); ++i)
			if (!Before[i] && IsUnlocked(Roster()[i].Id)) New.Add(Roster()[i].Name);
		UE_LOG(LogIronwake, Display, TEXT("IWS2 arrival poi=%d mask=0x%X arrivals=%d unlocked_now=[%s]"), PoiIndex, GetVisitedMask(), GetArrivals(), *FString::Join(New, TEXT(",")));
		return New;
	}

	void ResetProgress()
	{
		WriteInt(TEXT("PoiMask"), 0); WriteInt(TEXT("Arrivals"), 0);
		if (GConfig) { GConfig->SetString(Section, TEXT("Ship"), TEXT("frigate"), GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
		UE_LOG(LogIronwake, Display, TEXT("IWS2 progress reset"));
	}

	FString ProgressLine()
	{
		int32 Vis = 0;
		for (int32 i = 0; i < 4; ++i) if (GetVisitedMask() & (1 << i)) ++Vis;
		return FString::Printf(TEXT("Points of interest visited %d/4     Voyages %d/%d (ironclad)"), Vis, FMath::Min(GetArrivals(), ArrivalsForIronclad), ArrivalsForIronclad);
	}

	// ---- wind: prevailing WNW (from 300), slow +/-15 deg veer over ~11 min, gusts +/-20 % (deterministic, no RNG state)
	float WindFromYaw(UWorld* W)
	{
		if (bWindFixed) return FixedFrom;
		const float T = W ? W->GetTimeSeconds() : 0.f;
		return FMath::Fmod(300.f + 15.f * FMath::Sin(T * 2.f * PI / 660.f) + 4.f * FMath::Sin(T * 0.071f) + 360.f, 360.f);
	}
	float WindKn(UWorld* W)
	{
		if (bWindFixed) return FixedKn;
		const float T = W ? W->GetTimeSeconds() : 0.f;
		// Weather drives it: the sea state (dusk 0.65, day 1, storm 1.6) sets the mean wind (10 / 16 / 26 kn).
		// W2: the sea state sets the mean wind (CALM 8, MODERATE 16, ROUGH 24, GALE 36, STORM 48 kn); stronger gusts in a blow.
		const float S = IronwakeSea::GetSeaState();
		const float G = 0.12f + 0.04f * FMath::Clamp(S - 2.f, 0.f, 2.f);
		return IronwakeSea::WindForState(S) * (1.f + G * FMath::Sin(T * 0.21f) + 0.08f * FMath::Sin(T * 0.53f + 1.3f));
	}
	void SetFixedWind(float FromYaw, float Kn) { bWindFixed = true; FixedFrom = FMath::Fmod(FromYaw + 360.f, 360.f); FixedKn = Kn; }

	float Polar(float TWA)
	{
		// Square-rigged steam frigate: no drive inside 40 deg, best on a broad reach (~115-125 deg), a little slower running
		// dead downwind (the after sails blanket the fore sails).
		static const float A[] = { 0.f, 35.f, 50.f, 60.f, 75.f, 90.f, 105.f, 120.f, 135.f, 150.f, 165.f, 180.f };
		static const float P[] = { 0.f, 0.f, 0.40f, 0.62f, 0.80f, 0.92f, 0.99f, 1.0f, 0.96f, 0.90f, 0.84f, 0.80f };
		TWA = FMath::Clamp(FMath::Abs(TWA), 0.f, 180.f);
		for (int32 i = 1; i < UE_ARRAY_COUNT(A); ++i)
			if (TWA <= A[i]) return FMath::Lerp(P[i - 1], P[i], (TWA - A[i - 1]) / (A[i] - A[i - 1]));
		return P[UE_ARRAY_COUNT(P) - 1];
	}

	const TCHAR* PointOfSail(float TWA)
	{
		TWA = FMath::Abs(TWA);
		if (TWA < 35.f) return TEXT("IN IRONS");
		if (TWA < 60.f) return TEXT("CLOSE-HAULED");
		if (TWA < 80.f) return TEXT("CLOSE REACH");
		if (TWA < 110.f) return TEXT("BEAM REACH");
		if (TWA < 155.f) return TEXT("BROAD REACH");
		return TEXT("RUNNING");
	}

	const TCHAR* CompassPoint(float Yaw)
	{
		static const TCHAR* N[] = { TEXT("N"), TEXT("NNE"), TEXT("NE"), TEXT("ENE"), TEXT("E"), TEXT("ESE"), TEXT("SE"), TEXT("SSE"),
			TEXT("S"), TEXT("SSW"), TEXT("SW"), TEXT("WSW"), TEXT("W"), TEXT("WNW"), TEXT("NW"), TEXT("NNW") };
		return N[FMath::RoundToInt(FMath::Fmod(Yaw + 360.f, 360.f) / 22.5f) % 16];
	}
}

FIronwakeHullProfile FIronwakeHullProfile::Frigate()
{
	// build/s2/clear_frigate.py (max half-breadth between -0.5 and +2.5 m of the boot-top, 88 m game scale)
	FIronwakeHullProfile P; P.X0 = -40.51f; P.X1 = 26.47f;
	P.HalfB = { 0.56f, 3.9f, 6.04f, 6.25f, 6.91f, 7.39f, 7.55f, 7.79f, 7.92f, 7.92f, 7.99f, 8.08f, 8.2f, 8.21f, 8.18f, 8.06f, 7.99f, 7.91f, 7.91f, 7.8f, 7.57f, 7.13f, 5.93f, 3.77f, 0.76f };
	P.BowDeckM = 6.0f; P.KeelM = 4.2f;
	return P;
}

FIronwakeHullProfile FIronwakeHullProfile::TallBrig()
{
	// P26: measured from b2_brig_hull_rigged.glb Material_0 around calm waterline Z=2.5..3.9 m (42 m overall incl. jibboom; LWL=27.74 m, Xc=-5.29 m)
	FIronwakeHullProfile P; P.X0 = -19.16f; P.X1 = 8.58f;
	P.HalfB = { 0.65f, 1.85f, 2.75f, 3.15f, 3.46f, 3.65f, 3.85f, 4.18f, 4.40f, 4.52f, 4.62f, 4.70f, 4.76f, 4.82f, 4.84f, 4.81f, 4.72f, 4.55f, 4.15f, 3.82f, 3.55f, 3.22f, 2.65f, 1.65f, 0.55f };
	P.BowDeckM = 6.20f;   // forecastle deck Z in mesh metres above z=0 keel (~3.10 m above 3.10 m calm waterline)
	P.KeelM = 3.10f;
	return P;
}

FIronwakeHullProfile FIronwakeHullProfile::ChapmanSloop()
{
	FIronwakeHullProfile P; P.X0 = -11.2f; P.X1 = 9.4f;
	P.HalfB = { 0.45f, 1.35f, 2.05f, 2.48f, 2.75f, 2.92f, 3.04f, 3.10f, 3.12f, 3.12f, 3.10f, 3.05f, 2.98f, 2.88f, 2.75f, 2.58f, 2.38f, 2.15f, 1.88f, 1.58f, 1.25f, 0.92f, 0.62f, 0.35f, 0.18f };
	P.BowDeckM = 4.1f; P.KeelM = 1.95f;
	return P;
}

FIronwakeHullProfile FIronwakeHullProfile::AmphionSchooner()
{
	FIronwakeHullProfile P; P.X0 = -16.8f; P.X1 = 12.6f;
	P.HalfB = { 0.52f, 1.55f, 2.35f, 2.90f, 3.32f, 3.65f, 3.90f, 4.08f, 4.18f, 4.22f, 4.22f, 4.18f, 4.12f, 4.02f, 3.86f, 3.65f, 3.38f, 3.05f, 2.68f, 2.25f, 1.80f, 1.35f, 0.92f, 0.52f, 0.22f };
	P.BowDeckM = 5.35f; P.KeelM = 2.65f;
	return P;
}

// ============================================================================ sail rig (frigate)
namespace
{
	struct FYardDef { int32 Mast; FVector Pivot; float Half; };
	struct FSquareSailDef { int32 Yard; float HeadHalf; float FootHalf; float Height; int32 Level; };
	struct FForeAftSailDef { FVector P[4]; int32 Level; };
	// generated by build/s2/riggen.py -- frigate rig in hull-mesh cm (+X bow, boot-top z=0)
	static const FYardDef GYards[] = {
		{ 0, FVector(1631.9f, 36.8f, 2123.5f), 1022.0f },
		{ 0, FVector(1645.6f, 36.8f, 3584.6f), 936.9f },
		{ 0, FVector(1639.2f, 36.8f, 4624.1f), 423.9f },
		{ 1, FVector(-976.8f, 36.8f, 2210.6f), 1174.7f },
		{ 1, FVector(-924.1f, 36.8f, 3680.0f), 932.1f },
		{ 1, FVector(-921.8f, 36.8f, 4721.1f), 468.5f },
		{ 2, FVector(-2939.5f, 36.8f, 2008.2f), 834.3f },
		{ 2, FVector(-2916.5f, 36.8f, 3443.0f), 447.0f },
		{ 2, FVector(-2899.0f, 36.8f, 4287.7f), 369.8f },
	};
	// level: 3 topsails, 4 courses + spanker, 5 topgallants + jibs, 6 staysails
	static const FSquareSailDef GSquare[] = {
		{ 7, 447.0f, 1083.2f, 1429.2f, 3 },
		{ 8, 369.8f, 805.7f, 834.3f, 5 },
		{ 4, 932.1f, 1403.0f, 1446.7f, 3 },
		{ 3, 1174.7f, 1382.3f, 1235.5f, 4 },
		{ 5, 468.5f, 1113.5f, 1022.8f, 5 },
		{ 2, 423.9f, 1054.6f, 1040.3f, 5 },
		{ 1, 936.9f, 1302.8f, 1456.3f, 3 },
		{ 0, 1022.0f, 1299.6f, 1080.1f, 4 },
	};
	static const FForeAftSailDef GForeAft[] = {  // head-luff, head-leech, tack, clew
		{ { FVector(-3117.7f, 36.8f, 2253.9f), FVector(-4350.5f, 36.8f, 2958.7f), FVector(-3117.7f, 36.8f, 1103.9f), FVector(-4350.5f, 36.8f, 1178.1f) }, 4 },
		{ { FVector(-2492.4f, 36.8f, 3443.8f), FVector(-2492.4f, 36.8f, 3443.8f), FVector(-1356.8f, 36.8f, 2366.7f), FVector(-2591.2f, 36.8f, 2273.1f) }, 6 },
		{ { FVector(-2446.0f, 36.8f, 1916.8f), FVector(-2446.0f, 36.8f, 1916.8f), FVector(-1453.8f, 36.8f, 1075.3f), FVector(-2532.3f, 36.8f, 1002.1f) }, 6 },
		{ { FVector(-385.3f, 36.8f, 3647.4f), FVector(-385.3f, 36.8f, 3647.4f), FVector(964.0f, 36.8f, 2135.7f), FVector(-502.6f, 36.8f, 2004.2f) }, 6 },
		{ { FVector(-309.8f, 36.8f, 2164.9f), FVector(-309.8f, 36.8f, 2164.9f), FVector(975.1f, 36.8f, 1168.3f), FVector(-421.5f, 36.8f, 1081.7f) }, 6 },
		{ { FVector(2134.5f, 36.8f, 2136.3f), FVector(2134.5f, 36.8f, 2136.3f), FVector(3138.4f, 36.8f, 1059.2f), FVector(2047.2f, 36.8f, 965.5f) }, 5 },
		{ { FVector(2445.3f, 36.8f, 2771.0f), FVector(2445.3f, 36.8f, 2771.0f), FVector(3456.6f, 36.8f, 1172.9f), FVector(2357.4f, 36.8f, 1033.9f) }, 5 },
		{ { FVector(2957.4f, 36.8f, 2734.4f), FVector(2957.4f, 36.8f, 2734.4f), FVector(4186.7f, 36.8f, 1519.7f), FVector(2850.5f, 36.8f, 1414.1f) }, 5 },
	};
	static const FVector GMastTops[] = { FVector(1625.7f, 36.8f, 5448.1f), FVector(-951.2f, 36.8f, 5686.7f), FVector(-2907.7f, 36.8f, 5098.1f) };
	static const FVector GEnsignHoist = FVector(-4188.2f, 36.8f, 1805.4f);

	constexpr int32 SqU = 14, SqV = 10;   // square sail grid (quads)
	constexpr int32 FaU = 10, FaV = 10;
	constexpr int32 EnU = 14, EnV = 6;    // ensign
	constexpr int32 PnU = 18, PnV = 1;    // pennant

	const FLinearColor Canvas(0.50f, 0.47f, 0.40f);       // linear, weathered canvas
	const FLinearColor CanvasFoot(0.40f, 0.37f, 0.31f);
}

UIronwakeSailRig::UIronwakeSailRig()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIronwakeSailRig::BuildGrid(UProceduralMeshComponent* M, int32 NU, int32 NV)
{
	V.SetNum((NU + 1) * (NV + 1)); N.SetNum(V.Num()); UV.SetNum(V.Num()); C.SetNum(V.Num()); Tan.SetNum(V.Num());
	TArray<int32> Tri;
	for (int32 j = 0; j < NV; ++j)
		for (int32 i = 0; i < NU; ++i)
		{
			const int32 A = j * (NU + 1) + i, B = A + 1, Cc = A + (NU + 1), D = Cc + 1;
			Tri.Append({ A, Cc, B, B, Cc, D });
		}
	for (int32 j = 0; j <= NV; ++j)
		for (int32 i = 0; i <= NU; ++i)
		{
			const int32 k = j * (NU + 1) + i;
			V[k] = FVector(0.f, i * 10.f, -j * 10.f); N[k] = FVector(1, 0, 0); UV[k] = FVector2D((float)i / NU, (float)j / NV);
			C[k] = FLinearColor::White; Tan[k] = FProcMeshTangent(0, 1, 0);
		}
	M->CreateMeshSection_LinearColor(0, V, Tri, N, UV, C, Tan, false, false);
	if (ClothMat) M->SetMaterial(0, ClothMat);
	M->SetCastShadow(true);
	M->bUseAsyncCooking = true;
}

void UIronwakeSailRig::Prep(int32 NU, int32 NV)
{
	const int32 Num = (NU + 1) * (NV + 1);
	V.SetNum(Num); N.SetNum(Num); C.SetNum(Num); Tan.SetNum(Num); UV.SetNum(Num);
	for (int32 j = 0; j <= NV; ++j)
		for (int32 i = 0; i <= NU; ++i) UV[j * (NU + 1) + i] = FVector2D((float)i / NU, (float)j / NV);
}

void UIronwakeSailRig::Normals(int32 NU, int32 NV)
{
	for (int32 j = 0; j <= NV; ++j)
		for (int32 i = 0; i <= NU; ++i)
		{
			const int32 k = j * (NU + 1) + i;
			const FVector& Pu0 = V[j * (NU + 1) + FMath::Max(i - 1, 0)];
			const FVector& Pu1 = V[j * (NU + 1) + FMath::Min(i + 1, NU)];
			const FVector& Pv0 = V[FMath::Max(j - 1, 0) * (NU + 1) + i];
			const FVector& Pv1 = V[FMath::Min(j + 1, NV) * (NU + 1) + i];
			const FVector Du = Pu1 - Pu0, Dv = Pv1 - Pv0;
			FVector Nn = FVector::CrossProduct(Dv, Du).GetSafeNormal();
			if (Nn.IsNearlyZero()) Nn = FVector(1, 0, 0);
			N[k] = Nn;
			Tan[k] = FProcMeshTangent(Du.GetSafeNormal(), false);
		}
}

void UIronwakeSailRig::BuildTallBrig()
{
	struct FBrigPartSpec { const TCHAR* Asset; FVector PivotCm; uint8 Kind; int32 Mast; int32 Tier; int32 MinStep; int32 ParentYardIdx; float FurlHalfCm; FVector StayHeadRel; };
	// Exact pivots in SM_TallBrig hull-mesh cm (from TallBrig/parts.json: X_ue=+100*X_b, Y_ue=-100*Y_b=-4.35, Z_ue=+100*Z_b):
	static const FBrigPartSpec GBrigSpecs[17] = {
		{ TEXT("TallBrig_yard_m0_0"),     FVector( 180.10f, -4.35f, 1439.70f), 0,  0,  0, 0, -1,   0.f, FVector::ZeroVector }, // 0 Fore course yard
		{ TEXT("TallBrig_sail_m0_0"),     FVector::ZeroVector,                 1,  0,  0, 4,  0, 504.f, FVector::ZeroVector }, // 1 Fore course sail
		{ TEXT("TallBrig_yard_m0_1"),     FVector( 207.37f, -4.35f, 2004.68f), 0,  0,  1, 0, -1,   0.f, FVector::ZeroVector }, // 2 Fore topsail yard
		{ TEXT("TallBrig_sail_m0_1"),     FVector::ZeroVector,                 1,  0,  1, 3,  2, 412.f, FVector::ZeroVector }, // 3 Fore topsail
		{ TEXT("TallBrig_yard_m0_2"),     FVector( 226.85f, -4.35f, 2413.80f), 0,  0,  2, 0, -1,   0.f, FVector::ZeroVector }, // 4 Fore topgallant yard
		{ TEXT("TallBrig_sail_m0_2"),     FVector::ZeroVector,                 1,  0,  2, 5,  4, 266.f, FVector::ZeroVector }, // 5 Fore topgallant sail
		{ TEXT("TallBrig_yard_m1_0"),     FVector(-768.46f, -4.35f, 1595.56f), 0,  1,  0, 0, -1,   0.f, FVector::ZeroVector }, // 6 Main course yard
		{ TEXT("TallBrig_sail_m1_0"),     FVector::ZeroVector,                 1,  1,  0, 4,  6, 513.f, FVector::ZeroVector }, // 7 Main course sail
		{ TEXT("TallBrig_yard_m1_1"),     FVector(-741.18f, -4.35f, 2121.57f), 0,  1,  1, 0, -1,   0.f, FVector::ZeroVector }, // 8 Main topsail yard
		{ TEXT("TallBrig_sail_m1_1"),     FVector::ZeroVector,                 1,  1,  1, 3,  8, 430.f, FVector::ZeroVector }, // 9 Main topsail
		{ TEXT("TallBrig_yard_m1_2"),     FVector(-721.70f, -4.35f, 2530.69f), 0,  1,  2, 0, -1,   0.f, FVector::ZeroVector }, // 10 Main topgallant yard
		{ TEXT("TallBrig_sail_m1_2"),     FVector::ZeroVector,                 1,  1,  2, 5, 10, 275.f, FVector::ZeroVector }, // 11 Main topgallant sail
		{ TEXT("TallBrig_boom_spanker"),  FVector(-823.01f, -4.35f, 1118.25f), 2,  1, -1, 0, -1,   0.f, FVector::ZeroVector }, // 12 Gaff spanker boom
		{ TEXT("TallBrig_gaff_spanker"),  FVector(-823.01f, -4.35f, 1624.78f), 3,  1, -1, 0, -1,   0.f, FVector::ZeroVector }, // 13 Gaff spanker gaff
		{ TEXT("TallBrig_sail_spanker"),  FVector(-823.01f, -4.35f, 1118.25f), 4,  1, -1, 4, -1,   0.f, FVector::ZeroVector }, // 14 Gaff spanker sail (Step>=4)
		{ TEXT("TallBrig_fore_staysail"), FVector( 982.75f, -4.35f,  806.54f), 5, -1, -1, 4, -1,   0.f, FVector(-789.0f, 0.f,  934.4f) }, // 15 Fore staysail (Step>=4)
		{ TEXT("TallBrig_jib"),           FVector(2067.99f, -4.35f,  952.66f), 6, -1, -1, 3, -1,   0.f, FVector(-1899.6f, 0.f, 1246.8f) }, // 16 Outer jib (Step>=3: harbour topsails+jib)
	};

	AActor* Owner = GetOwner();
	if (!ClothMat) ClothMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Ships/Materials/M_IW_Cloth.M_IW_Cloth"));
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* PaintMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Ships/Materials/M_IW_BrigPaint.M_IW_BrigPaint"));
	if (!PaintMat) PaintMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Ships/Materials/M_IW_ShipPaint.M_IW_ShipPaint"));
	if (!PaintMat) PaintMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto MakePaint = [&](const FLinearColor& Col, float Rough, float Metal = 0.f, float Spec = 0.20f) -> UMaterialInstanceDynamic*
	{
		if (!PaintMat) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(PaintMat, this);
		M->SetVectorParameterValue(TEXT("Color"), Col);
		M->SetScalarParameterValue(TEXT("Roughness"), Rough);
		M->SetScalarParameterValue(TEXT("Metallic"), Metal);
		M->SetScalarParameterValue(TEXT("Specular"), Spec);
		return M;
	};
	UMaterialInstanceDynamic* FurlMat = MakePaint(FLinearColor(0.48f, 0.43f, 0.34f), 0.90f, 0.f, 0.15f);
	UMaterialInstanceDynamic* WoodMat = MakePaint(FLinearColor(0.18f, 0.11f, 0.058f), 0.85f, 0.f, 0.22f);
	UMaterialInstanceDynamic* RopeMat = MakePaint(FLinearColor(0.048f, 0.040f, 0.032f), 0.92f, 0.f, 0.12f);
	UMaterialInstanceDynamic* IronMat = MakePaint(FLinearColor(0.045f, 0.045f, 0.050f), 0.58f, 0.0f, 0.24f);

	auto AddLine = [&](USceneComponent* Parent, const FVector& P0, const FVector& P1, float RadiusCm, UMaterialInterface* Mat) -> UStaticMeshComponent*
	{
		if (!Cyl) return nullptr;
		const FVector D = P1 - P0;
		const float Len = D.Size();
		if (Len < 1.f) return nullptr;
		UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
		Comp->SetupAttachment(Parent);
		Comp->SetStaticMesh(Cyl);
		Comp->SetRelativeLocation((P0 + P1) * 0.5f);
		Comp->SetRelativeRotation(FRotationMatrix::MakeFromZ(D / Len).Rotator());
		Comp->SetRelativeScale3D(FVector(RadiusCm * 2.f / 100.f, RadiusCm * 2.f / 100.f, Len / 100.f));
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCastShadow(false);
		if (Mat) Comp->SetMaterial(0, Mat);
		Comp->RegisterComponent();
		BrigComps.Add(Comp);
		return Comp;
	};

	// Standing shrouds & backstays on Foremast (X=150) and Mainmast (X=-815): grounds both masts cleanly to the hull channels
	for (int32 m = 0; m < 2; ++m)
	{
		const float Mx = (m == 0) ? 152.f : -818.f;
		const float TopZ = (m == 0) ? 1385.f : 1465.f;
		const float TmTopZ = (m == 0) ? 2020.f : 2130.f;
		for (int32 s = -1; s <= 1; s += 2)
		{
			for (int32 k = 0; k < 3; ++k)
			{
				const FVector Chain(Mx - 25.f - 78.f * k, -4.35f + s * 432.f, 595.f);
				const FVector Hound(Mx + 12.f, -4.35f + s * 34.f, TopZ);
				AddLine(this, Chain, Hound, 2.8f, RopeMat);
			}
			// Topmast shrouds from crosstrees to topmast head + royal backstay to aft channel
			AddLine(this, FVector(Mx + 22.f, -4.35f + s * 84.f, TopZ), FVector(Mx + 38.f, -4.35f + s * 16.f, TmTopZ), 2.0f, RopeMat);
			AddLine(this, FVector(Mx - 275.f, -4.35f + s * 420.f, 595.f), FVector(Mx + 42.f, -4.35f + s * 10.f, TmTopZ + 320.f), 2.0f, RopeMat);
		}
	}

	BrigParts.Reserve(17);
	for (int32 i = 0; i < 17; ++i)
	{
		const FBrigPartSpec& S = GBrigSpecs[i];
		USceneComponent* ParentComp = (S.ParentYardIdx >= 0 && BrigParts.IsValidIndex(S.ParentYardIdx)) ? BrigParts[S.ParentYardIdx].Pivot.Get() : this;
		USceneComponent* Piv = NewObject<USceneComponent>(Owner, *FString::Printf(TEXT("BrigPivot_%s"), S.Asset));
		Piv->SetupAttachment(ParentComp);
		Piv->SetRelativeLocation(S.PivotCm);
		Piv->RegisterComponent();
		BrigComps.Add(Piv);

		UStaticMesh* Sm = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/World/%s/SM_%s.SM_%s"), S.Asset, S.Asset, S.Asset));
		UStaticMeshComponent* Mc = NewObject<UStaticMeshComponent>(Owner, *FString::Printf(TEXT("BrigMesh_%s"), S.Asset));
		Mc->SetupAttachment(Piv);
		if (Sm) Mc->SetStaticMesh(Sm);
		Mc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mc->SetGenerateOverlapEvents(false);
		Mc->SetCastShadow(true);
		Mc->RegisterComponent();
		BrigComps.Add(Mc);

		FBrigPartRuntime Rt;
		Rt.Pivot = Piv;
		Rt.Mesh = Mc;
		Rt.Kind = S.Kind;
		Rt.Mast = S.Mast;
		Rt.Tier = S.Tier;
		Rt.MinStep = S.MinStep;
		Rt.SetFrac = 0.f;
		if (!S.StayHeadRel.IsNearlyZero()) Rt.StayAxis = S.StayHeadRel.GetSafeNormal();

		// Connect spanker boom & gaff physically to the mainmast with wooden jaws, iron gooseneck collar, and peak/topping-lift halyards
		if (S.Kind == 2) // Spanker boom: extend jaws forward to mainmast trunk + topping lift
		{
			AddLine(Piv, FVector(12.f, 0.f, 0.f), FVector(-52.f, 0.f, 2.f), 14.f, WoodMat);
			AddLine(Piv, FVector(4.f, 0.f, -8.f), FVector(4.f, 0.f, 8.f), 22.f, IronMat);
			AddLine(Piv, FVector(10.f, 0.f, 480.f), FVector(-1360.f, 0.f, 155.f), 1.5f, RopeMat);
		}
		else if (S.Kind == 3) // Spanker gaff: extend jaws forward to main topmast trunk + throat & peak halyards
		{
			AddLine(Piv, FVector(38.f, 0.f, 0.f), FVector(-48.f, 0.f, 8.f), 12.f, WoodMat);
			AddLine(Piv, FVector(32.f, 0.f, -7.f), FVector(32.f, 0.f, 7.f), 19.f, IronMat);
			AddLine(Piv, FVector(36.f, 0.f, 495.f), FVector(-1080.f, 0.f, 442.f), 1.6f, RopeMat);
			AddLine(Piv, FVector(36.f, 0.f, 495.f), FVector(-560.f, 0.f, 228.f), 1.5f, RopeMat);
		}

		if (S.Kind == 1 || S.Kind >= 4)
		{
			Rt.SailMID = Mc->CreateAndSetMaterialInstanceDynamic(0);
			if (Rt.SailMID) Rt.SailMID->SetScalarParameterValue(TEXT("Phase"), 0.73f * i);
		}
		if (S.Kind == 1 && Cyl)
		{
			UStaticMeshComponent* Fr = NewObject<UStaticMeshComponent>(Owner, *FString::Printf(TEXT("BrigFurl_%d"), i));
			Fr->SetupAttachment(ParentComp);
			Fr->SetStaticMesh(Cyl);
			Fr->SetRelativeLocation(FVector(12.f, 0.f, 14.f));
			Fr->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
			Fr->SetRelativeScale3D(FVector(0.32f, 0.32f, S.FurlHalfCm * 1.88f / 100.f));
			Fr->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Fr->SetCastShadow(false);
			if (FurlMat) Fr->SetMaterial(0, FurlMat);
			Fr->RegisterComponent();
			BrigComps.Add(Fr);
			Rt.FurlRoll = Fr;
		}
		BrigParts.Add(Rt);
	}

	// Both player and NPC brigs get sleek streaming masthead pennants at both masthead trucks (no stern pole or floating sky rectangle!)
	if (Owner)
	{
		const FVector BrigFlags[2] = {
			FVector(  232.f, -4.35f, 2555.f),  // foremast truck pennant
			FVector( -718.f, -4.35f, 2685.f),  // mainmast truck pennant
		};
		for (int32 f = 0; f < 2; ++f)
		{
			USceneComponent* P = NewObject<USceneComponent>(Owner, *FString::Printf(TEXT("BrigFlagPivot%d"), f));
			P->SetupAttachment(this);
			P->SetRelativeLocation(BrigFlags[f]);
			P->RegisterComponent();
			FlagPivots.Add(P);
			UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(Owner, *FString::Printf(TEXT("BrigFlag%d"), f));
			M->SetupAttachment(P);
			M->RegisterComponent();
			BuildGrid(M, PnU, PnV);
			M->SetCastShadow(false);
			Flags.Add(M);
		}
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP26 TallBrig rig built: %d moving rig parts, %d flags"), BrigParts.Num(), Flags.Num());
}

void UIronwakeSailRig::TickTallBrig(float Dt, float RelWind, float AppWind, int32 SetLevel, float Fill, float WindKnots)
{
	T += Dt;
	const float TWA = FMath::Abs(RelWind);
	const float UseWind = (FMath::Abs(AppWind) <= 180.f) ? AppWind : RelWind;
	const float AWA = FMath::Abs(UseWind);
	const float Side = UseWind >= 0.f ? 1.f : -1.f;     // wind from starboard (+Y) = +1
	const float Ls = -Side;                              // leeward in hull Y (-Y port when wind from stbd)
	const float Wf = FMath::Clamp(WindKnots / 16.f, 0.25f, 1.5f);
	const bool bLuff = (TWA < 35.f || Fill < 0.05f);
	const bool bFurledAll = (SetLevel <= 2);

	// 1. Square yard bracing around Z (+/- 32 deg from apparent wind; centred when furled at dock)
	const float Brace = bFurledAll ? 0.f : (32.f * FMath::Clamp((180.f - AWA) / 125.f, 0.f, 1.f));
	const float WantYard = Side * Brace;
	YardYaw += (WantYard - YardYaw) * (1.f - FMath::Exp(-Dt / 2.6f));

	// 2. Gaff spanker sheeting around Z (0..38 deg out to leeward: +Yaw swings aft-pointing -X boom toward -Y port when Side=+1)
	const float WantSpanker = bFurledAll ? 0.f : (bLuff
		? (4.5f * FMath::Sin(T * 2.9f))
		: (Side * FMath::Lerp(8.f, 38.f, FMath::Clamp((AWA - 35.f) / 120.f, 0.f, 1.f))));
	SpankerYaw += (WantSpanker - SpankerYaw) * (1.f - FMath::Exp(-Dt / 2.2f));

	// 3. Headsail sheeting around each inclined stay axis (0..18 deg out to leeward; tack & head stay 100% locked to stay!)
	const float WantHead = bFurledAll ? 0.f : (bLuff
		? (3.5f * FMath::Sin(T * 3.4f + 1.1f))
		: (Ls * FMath::Lerp(5.f, 18.f, FMath::Clamp((AWA - 35.f) / 120.f, 0.f, 1.f))));
	HeadsailYaw += (WantHead - HeadsailYaw) * (1.f - FMath::Exp(-Dt / 2.0f));

	const FVector HullRight = GetComponentTransform().GetUnitAxis(EAxis::Y);
	// Smooth taut canvas when drawing (zero sine-wave dent on broad reach!), lively shiver when pinched/luffing
	const float LuffCm = (bLuff ? 15.f : FMath::Clamp((0.45f - Fill) * 14.f, 0.f, 4.5f)) * Wf;
	const float BellyCm = bLuff ? 1.0f : (Fill * 8.5f * FMath::Clamp(Wf, 0.5f, 1.25f));
	LastLuffCm = bFurledAll ? 0.f : LuffCm;
	LastBellyM = bFurledAll ? 0.f : ((0.85f * FMath::Lerp(0.55f, 1.35f, Fill)) + BellyCm / 100.f);

	for (int32 i = 0; i < BrigParts.Num(); ++i)
	{
		FBrigPartRuntime& P = BrigParts[i];
		if (!P.Pivot || !P.Mesh) continue;

		// Target sail set fraction by SailPlan step (2=Furled, 3=Reefed/Harbour topsails+jib [courses furled!], 4=Half +courses+spanker+staysail, 5/6=Full +topgallants)
		float Want = 0.f;
		if (SetLevel >= P.MinStep)
		{
			if (SetLevel == 3)
			{
				if (P.Kind == 1 && P.Tier == 1) Want = 0.68f;      // reefed fore & main topsails (courses m0_0 & m1_0 furled!)
				else if (P.Kind == 6) Want = 0.88f;                // jib set for balanced harbour steerage
				else Want = 1.0f;
			}
			else if (P.Kind == 1 && P.Tier == 1 && WindKnots > 40.f) Want = 0.58f;
			else Want = 1.0f;
		}
		P.SetFrac = FMath::FInterpConstantTo(P.SetFrac, Want, Dt, 0.34f);
		const float Sf = P.SetFrac;

		switch (P.Kind)
		{
		case 0: // Square yard
			P.Pivot->SetRelativeRotation(FRotator(0.f, YardYaw * (1.f - 0.04f * FMath::Max(P.Tier, 0)), 0.f));
			break;
		case 1: // Square sail (child of yard pivot -> braces automatically; furls vertically up to yard Z=0)
		{
			const float Flut = bLuff ? (0.34f + 0.16f * FMath::Sin(T * 7.2f + i * 0.9f)) : FMath::Lerp(0.58f, 1.36f, Fill * FMath::Clamp(Wf, 0.5f, 1.2f));
			const float BellyX = FMath::Lerp(0.20f, Flut, FMath::Clamp(Sf * 1.4f, 0.f, 1.f));
			const float ScaleZ = FMath::Lerp(0.06f, 1.0f, Sf);
			P.Mesh->SetRelativeScale3D(FVector(BellyX, 1.f, ScaleZ));
			P.Mesh->SetVisibility(Sf > 0.02f);
			if (P.FurlRoll) P.FurlRoll->SetVisibility(Sf < 0.30f);
			if (P.SailMID && Sf > 0.02f)
			{
				const FVector YardFwd = P.Pivot->GetComponentTransform().GetUnitAxis(EAxis::X);
				P.SailMID->SetVectorParameterValue(TEXT("BillowDir"), FLinearColor(YardFwd.X, YardFwd.Y, YardFwd.Z, 0.f));
				P.SailMID->SetScalarParameterValue(TEXT("WindBelly"), BellyCm * Sf);
				P.SailMID->SetScalarParameterValue(TEXT("LuffAmp"), LuffCm * Sf);
			}
			break;
		}
		case 2: // Gaff spanker boom
			P.Pivot->SetRelativeRotation(FRotator(0.f, SpankerYaw, 0.f));
			break;
		case 3: // Gaff spanker gaff
			P.Pivot->SetRelativeRotation(FRotator(0.f, SpankerYaw * 0.98f, 0.f));
			break;
		case 4: // Gaff spanker sail (brails along X toward mainmast luff; camber flips to leeward on port/stbd tack)
		{
			P.Pivot->SetRelativeRotation(FRotator(0.f, SpankerYaw, 0.f));
			const float CamSgn = (Ls >= 0.f ? 1.f : -1.f);
			const float Flut = bLuff ? (0.32f + 0.18f * FMath::Sin(T * 7.8f + 2.1f)) : FMath::Lerp(0.85f, 2.35f, Fill);
			const float BellyY = CamSgn * FMath::Lerp(0.22f, Flut, Sf);
			P.Mesh->SetReverseCulling(BellyY < 0.f);
			P.Mesh->SetRelativeScale3D(FVector(FMath::Lerp(0.07f, 1.0f, Sf), BellyY, FMath::Lerp(0.35f, 1.0f, Sf)));
			P.Mesh->SetVisibility(Sf > 0.02f);
			if (P.FurlRoll) P.FurlRoll->SetVisibility(Sf < 0.30f);
			if (P.SailMID && Sf > 0.02f)
			{
				const FVector LeeDir = HullRight * Ls;
				P.SailMID->SetVectorParameterValue(TEXT("BillowDir"), FLinearColor(LeeDir.X, LeeDir.Y, LeeDir.Z, 0.f));
				P.SailMID->SetScalarParameterValue(TEXT("WindBelly"), BellyCm * 1.4f * Sf);
				P.SailMID->SetScalarParameterValue(TEXT("LuffAmp"), LuffCm * Sf);
			}
			break;
		}
		case 5: // Fore staysail
		case 6: // Outer jib
		{
			// Rotate around inclined stay axis so BOTH tack and masthead cringle stay locked to the stay!
			const float TestY = FQuat(P.StayAxis, FMath::DegreesToRadians(1.f)).RotateVector(FVector(-500.f, 0.f, 0.f)).Y;
			const float SignedStayDeg = HeadsailYaw * (TestY >= 0.f ? 1.f : -1.f);
			P.Pivot->SetRelativeRotation(FQuat(P.StayAxis, FMath::DegreesToRadians(SignedStayDeg)).Rotator());
			const float CamSgn = (Ls >= 0.f ? 1.f : -1.f);
			const float Flut = bLuff ? (0.30f + 0.18f * FMath::Sin(T * 8.3f + i)) : FMath::Lerp(0.80f, 2.15f, Fill);
			const float BellyY = CamSgn * FMath::Lerp(0.20f, Flut, Sf);
			P.Mesh->SetReverseCulling(BellyY < 0.f);
			P.Mesh->SetRelativeScale3D(FVector(FMath::Lerp(0.08f, 1.0f, Sf), BellyY, FMath::Lerp(0.08f, 1.0f, Sf)));
			P.Mesh->SetVisibility(Sf > 0.03f);
			if (P.FurlRoll) P.FurlRoll->SetVisibility(Sf < 0.30f);
			if (P.SailMID && Sf > 0.03f)
			{
				const FVector LeeDir = HullRight * Ls;
				P.SailMID->SetVectorParameterValue(TEXT("BillowDir"), FLinearColor(LeeDir.X, LeeDir.Y, LeeDir.Z, 0.f));
				P.SailMID->SetScalarParameterValue(TEXT("WindBelly"), BellyCm * 0.85f * Sf);
				P.SailMID->SetScalarParameterValue(TEXT("LuffAmp"), LuffCm * Sf);
			}
			break;
		}
		default: break;
		}
	}

			// Animate both masthead truck pennants (`BrigFlags[0..1]`)
			for (int32 f = 0; f < Flags.Num(); ++f)
			{
				FlagPivots[f]->SetRelativeRotation(FRotator(0.f, RelWind + 180.f, 0.f));
				const bool bEns = false;
		const int32 NU = bEns ? EnU : PnU, NV = bEns ? EnV : PnV;
		Prep(NU, NV);
		const float Len = bEns ? 440.f : 680.f, Hgt = bEns ? 280.f : 52.f;
		const float Om = 2.f * PI * (1.0f + 1.1f * Wf), K = 2.f * PI * (bEns ? 1.3f : 2.2f);
		const float Amp = (bEns ? 30.f : 22.f) * FMath::Clamp(Wf, 0.5f, 1.3f);
		const float Droop = (1.f - FMath::Clamp(Wf, 0.f, 1.f)) * 0.5f;
		for (int32 j = 0; j <= NV; ++j)
		{
			const float v = (float)j / NV;
			for (int32 i = 0; i <= NU; ++i)
			{
				const float u = (float)i / NU;
				const float Taper = bEns ? 1.f : (1.f - 0.85f * u);
				const float Yw = Amp * u * FMath::Sin(K * u - Om * T + f * 1.7f) + 0.25f * Amp * u * FMath::Sin(2.3f * K * u - 1.9f * Om * T + v * 2.f);
				const int32 k = j * (NU + 1) + i;
				V[k] = FVector(u * Len * (1.f - 0.08f * Droop), Yw, -v * Hgt * Taper - Droop * u * Len * 0.4f);
				if (bEns)
				{
					const bool bFess = j == NV / 2 || (j == NV / 2 + 1 && NV % 2 == 1);
					C[k] = bFess ? FLinearColor(0.45f, 0.30f, 0.08f) : (i == 0 ? FLinearColor(0.20f, 0.02f, 0.015f) : FLinearColor(0.05f, 0.06f, 0.075f));
				}
				else C[k] = u > 0.8f ? FLinearColor(0.6f, 0.58f, 0.52f) : FLinearColor(0.32f, 0.025f, 0.02f);
			}
		}
		Normals(NU, NV);
		Flags[f]->UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
	}
}

void UIronwakeSailRig::Build()
{
	if (bBuilt) return;
	bBuilt = true;
	if (bHullSailsWPO)
	{
		HullSailMIDs.Reset();
		if (UStaticMeshComponent* ParentHull = Cast<UStaticMeshComponent>(GetAttachParent()))
		{
			for (int32 mi = 0; mi < ParentHull->GetNumMaterials(); ++mi)
			{
				UMaterialInterface* Mat = ParentHull->GetMaterial(mi);
				if (Mat && Mat->GetName().Contains(TEXT("Sail")))
				{
					if (UMaterialInstanceDynamic* Mid = ParentHull->CreateAndSetMaterialInstanceDynamic(mi))
					{
						Mid->SetScalarParameterValue(TEXT("Phase"), 0.85f * mi);
						HullSailMIDs.Add(Mid);
					}
				}
			}
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP32 WPO hull-sail rig bound: %d dynamic sail slots"), HullSailMIDs.Num());
		return;
	}
	if (bTallBrig) { BuildTallBrig(); return; }
	AActor* Owner = GetOwner();
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	auto Tint = [&](const FLinearColor& Cc) -> UMaterialInstanceDynamic*
	{
		if (!Basic) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this);
		M->SetVectorParameterValue(TEXT("Color"), Cc);
		return M;
	};
	UMaterialInstanceDynamic* Wood = Tint(FLinearColor(0.045f, 0.03f, 0.02f));
	UMaterialInstanceDynamic* Furl = Tint(FLinearColor(0.42f, 0.39f, 0.33f));
	// Yards: a pivot at the mast (braces about Z), a tapered spar along Y.
	for (int32 i = 0; i < UE_ARRAY_COUNT(GYards); ++i)
	{
		USceneComponent* P = NewObject<USceneComponent>(Owner, *FString::Printf(TEXT("YardPivot%d"), i));
		P->SetupAttachment(this);
		P->SetRelativeLocation(GYards[i].Pivot);
		P->RegisterComponent();
		YardPivots.Add(P);
		UStaticMeshComponent* Spar = NewObject<UStaticMeshComponent>(Owner, *FString::Printf(TEXT("Yard%d"), i));
		Spar->SetupAttachment(P);
		Spar->SetStaticMesh(Cyl);
		const float Dia = FMath::Lerp(28.f, 46.f, FMath::Clamp(GYards[i].Half / 1170.f, 0.f, 1.f));
		Spar->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		Spar->SetRelativeScale3D(FVector(Dia / 100.f, Dia / 100.f, GYards[i].Half * 2.f / 100.f));
		Spar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Wood) Spar->SetMaterial(0, Wood);
		Spar->RegisterComponent();
	}
	// Square sails hang from their yard (children of the yard pivot, so they brace with it). Furled bundle on the yard.
	for (int32 s = 0; s < UE_ARRAY_COUNT(GSquare); ++s)
	{
		UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(Owner, *FString::Printf(TEXT("SquareSail%d"), s));
		M->SetupAttachment(YardPivots[GSquare[s].Yard]);
		M->SetRelativeLocation(FVector(30.f, 0.f, -20.f));
		M->RegisterComponent();
		BuildGrid(M, SqU, SqV);
		Square.Add(M);
		SetFrac.Add(0.f);
		UStaticMeshComponent* F = NewObject<UStaticMeshComponent>(Owner, *FString::Printf(TEXT("Furl%d"), s));
		F->SetupAttachment(YardPivots[GSquare[s].Yard]);
		F->SetStaticMesh(Cyl);
		F->SetRelativeLocation(FVector(18.f, 0.f, 22.f));
		F->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		F->SetRelativeScale3D(FVector(0.62f, 0.62f, GSquare[s].HeadHalf * 1.9f / 100.f));
		F->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Furl) F->SetMaterial(0, Furl);
		F->RegisterComponent();
		F->ComponentTags.Add(TEXT("Furl"));
	}
	for (int32 s = 0; s < UE_ARRAY_COUNT(GForeAft); ++s)
	{
		UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(Owner, *FString::Printf(TEXT("ForeAftSail%d"), s));
		M->SetupAttachment(this);
		M->RegisterComponent();
		BuildGrid(M, FaU, FaV);
		ForeAft.Add(M);
		FASetFrac.Add(0.f);
	}
	// Ensign (stern staff) + three masthead pennants.
	const FVector FlagAt[] = { GEnsignHoist, GMastTops[0], GMastTops[1], GMastTops[2] };
	for (int32 f = 0; f < 4; ++f)
	{
		USceneComponent* P = NewObject<USceneComponent>(Owner, *FString::Printf(TEXT("FlagPivot%d"), f));
		P->SetupAttachment(this);
		P->SetRelativeLocation(FlagAt[f]);
		P->RegisterComponent();
		FlagPivots.Add(P);
		UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(Owner, *FString::Printf(TEXT("Flag%d"), f));
		M->SetupAttachment(P);
		M->RegisterComponent();
		BuildGrid(M, f == 0 ? EnU : PnU, f == 0 ? EnV : PnV);
		M->SetCastShadow(false);
		Flags.Add(M);
	}
	// Ensign staff (the library flag was stripped with its pole top): a thin dark pole.
	UStaticMeshComponent* Staff = NewObject<UStaticMeshComponent>(Owner, TEXT("EnsignStaff"));
	Staff->SetupAttachment(this);
	Staff->SetStaticMesh(Cyl);
	Staff->SetRelativeLocation(GEnsignHoist + FVector(0.f, 0.f, -420.f));
	Staff->SetRelativeScale3D(FVector(0.12f, 0.12f, 9.f));
	Staff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Wood) Staff->SetMaterial(0, Wood);
	Staff->RegisterComponent();
	UE_LOG(LogIronwake, Display, TEXT("IWS2 sail rig built: %d yards, %d square sails, %d fore-and-aft sails, %d flags"), YardPivots.Num(), Square.Num(), ForeAft.Num(), Flags.Num());
}

void UIronwakeSailRig::TickRig(float Dt, float RelWind, int32 SetLevel, float Fill, float WindKnots, float ApparentRelWind)
{
	if (!bBuilt) return;
	if (bHullSailsWPO)
	{
		T += Dt;
		const float TWA = FMath::Abs(RelWind);
		const float Side = RelWind >= 0.f ? 1.f : -1.f;
		const float Ls = -Side;
		const float Wf = FMath::Clamp(WindKnots / 16.f, 0.25f, 1.5f);
		const bool bLuff = (TWA < 35.f || Fill < 0.05f);
		const float Sf = (SetLevel <= 2) ? 0.15f : FMath::Clamp((SetLevel - 2) / 3.f, 0.45f, 1.0f);
		const float BellyCm = (bLuff ? 2.0f : (Fill * 24.0f * FMath::Clamp(Wf, 0.5f, 1.3f))) * Sf;
		const float LuffCm  = (bLuff ? 16.0f : FMath::Clamp((0.45f - Fill) * 14.f, 0.f, 4.5f)) * Wf * Sf;
		LastBellyM = BellyCm / 100.f;
		LastLuffCm = LuffCm;
		const FVector HullRight = GetComponentTransform().GetUnitAxis(EAxis::Y);
		const FVector HullFwd   = GetComponentTransform().GetUnitAxis(EAxis::X);
		const FVector LeeDir    = (HullRight * Ls * 0.78f + HullFwd * 0.62f).GetSafeNormal();
		for (UMaterialInstanceDynamic* Mid : HullSailMIDs)
		{
			if (!Mid) continue;
			Mid->SetVectorParameterValue(TEXT("BillowDir"), FLinearColor(LeeDir.X, LeeDir.Y, LeeDir.Z, 0.f));
			Mid->SetScalarParameterValue(TEXT("WindBelly"), BellyCm);
			Mid->SetScalarParameterValue(TEXT("LuffAmp"), LuffCm);
		}
		return;
	}
	if (bTallBrig) { TickTallBrig(Dt, RelWind, ApparentRelWind, SetLevel, Fill, WindKnots); return; }
	T += Dt;
	const float TWA = FMath::Abs(RelWind);
	const float Side = RelWind >= 0.f ? 1.f : -1.f;     // wind on the starboard side = +1
	const float Wf = FMath::Clamp(WindKnots / 16.f, 0.2f, 1.5f);
	const bool bLuff = TWA < 37.f || Fill < 0.05f;
	// Braces: square when running, hard up (~38 deg) close-hauled; the windward yardarm goes aft (+yaw moves +Y aft).
	const float Brace = 38.f * FMath::Clamp((180.f - TWA) / 125.f, 0.f, 1.f);
	const float WantYaw = Side * Brace;
	YardYaw += (WantYaw - YardYaw) * (1.f - FMath::Exp(-Dt / 3.0f)); // crew hauls the braces over ~3 s
	for (USceneComponent* P : YardPivots) P->SetRelativeRotation(FRotator(0.f, YardYaw, 0.f));

	// ---- square sails
	const float BreathA = 0.07f * Wf;
	for (int32 s = 0; s < Square.Num(); ++s)
	{
		const FSquareSailDef& D = GSquare[s];
		const float Want = SetLevel >= D.Level ? 1.f : 0.f;
		SetFrac[s] = FMath::FInterpConstantTo(SetFrac[s], Want, Dt, 0.25f); // ~4 s to set or furl
		const float Sf = SetFrac[s];
		UProceduralMeshComponent* M = Square[s];
		// furled bundle on the yard
		TArray<USceneComponent*> Kids;
		YardPivots[D.Yard]->GetChildrenComponents(false, Kids);
		for (USceneComponent* K : Kids) if (K->ComponentHasTag(TEXT("Furl"))) K->SetVisibility(Sf < 0.35f);
		M->SetVisibility(Sf > 0.02f);
		if (Sf <= 0.02f) continue;
		Prep(SqU, SqV);
		const float H = D.Height * Sf * ((D.Level == 3 && WindKnots > 40.f) ? 0.6f : 1.f); // W3: close-reefed topsails in a storm
		const float Depth = (bLuff ? 0.05f : Fill) * 0.17f * FMath::Min(D.Height, D.FootHalf * 2.f) * FMath::Clamp(Wf, 0.4f, 1.2f) * Sf;
		const float Ph = s * 1.37f;
		const float Breath = 1.f + BreathA * FMath::Sin(T * 1.6f + Ph) + 0.04f * FMath::Sin(T * 3.7f + Ph * 2.f);
		const float FlapA = (bLuff ? 90.f : FMath::Clamp((0.45f - Fill) * 150.f, 0.f, 60.f)) * Wf;
		for (int32 j = 0; j <= SqV; ++j)
		{
			const float v = (float)j / SqV;
			const float Half = FMath::Lerp(D.HeadHalf, D.FootHalf, v);
			for (int32 i = 0; i <= SqU; ++i)
			{
				const float u = (float)i / SqU;
				const float Y = (u * 2.f - 1.f) * Half;
				// belly: deepest in the middle, the head held by the yard, the foot by the sheets
				const float Bu = FMath::Max(FMath::Sin(PI * u), 0.f);
				const float Bv = FMath::Max(FMath::Sin(PI * FMath::Lerp(0.06f, 0.92f, v)), 0.f);
				float X = Depth * Breath * FMath::Pow(Bu, 0.8f) * Bv;
				// leech/foot flutter (always a little), luffing ripples when the sail can't draw
				const float Edge = 1.f - Bu;
				X += 14.f * Wf * Edge * FMath::Sin(9.f * v - 11.f * T + Ph) * v;
				X += FlapA * Bv * FMath::Sin(2.f * PI * (1.3f * u + 0.6f * v) - 7.f * T + Ph) * (0.4f + 0.6f * Bu);
				const int32 k = j * (SqU + 1) + i;
				V[k] = FVector(X, Y, -v * H);
				C[k] = FMath::Lerp(Canvas, CanvasFoot, v * v);
			}
		}
		Normals(SqU, SqV);
		M->UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
		if (s == 3) LastBellyM = Depth * Breath / 100.f;
	}

	// ---- fore-and-aft sails: swing to leeward about the luff (sheet angle by point of sail), belly to leeward
	const float Ls = -Side;                                      // leeward sign in hull Y
	const float Sheet = bLuff ? 8.f * FMath::Sin(T * 2.8f) : FMath::Lerp(12.f, 58.f, FMath::Clamp((TWA - 40.f) / 120.f, 0.f, 1.f));
	const float Sr = FMath::DegreesToRadians(Sheet);
	for (int32 s = 0; s < ForeAft.Num(); ++s)
	{
		const FForeAftSailDef& D = GForeAft[s];
		const float Want = SetLevel >= D.Level ? 1.f : 0.f;
		FASetFrac[s] = FMath::FInterpConstantTo(FASetFrac[s], Want, Dt, 0.25f);
		const float Sf = FASetFrac[s];
		UProceduralMeshComponent* M = ForeAft[s];
		M->SetVisibility(Sf > 0.02f);
		if (Sf <= 0.02f) continue;
		const float Ph = 3.1f + s * 0.9f;
		Prep(FaU, FaV);
		for (int32 j = 0; j <= FaV; ++j)
		{
			const float v = (float)j / FaV;
			// hoisted: the head comes down the luff as the sail is lowered
			const FVector L = FMath::Lerp(D.P[2], D.P[0], (1.f - v) * Sf);
			const FVector R = FMath::Lerp(D.P[3], D.P[1], (1.f - v) * Sf);
			const FVector Chord = R - L;
			const float Cl = Chord.Size();
			for (int32 i = 0; i <= FaU; ++i)
			{
				const float u = (float)i / FaU;
				const FVector Dd = Chord * u;
				const FVector Rot(Dd.X * FMath::Cos(Sr), Ls * FMath::Abs(Dd.X) * FMath::Sin(Sr), Dd.Z);
				const float Bel = (bLuff ? 0.03f : Fill) * 0.12f * Cl * FMath::Sin(PI * u) * FMath::Sin(PI * FMath::Lerp(0.05f, 0.95f, v));
				float Fl = (bLuff ? 70.f : 10.f) * Wf * FMath::Sin(2.f * PI * (1.5f * u) - 8.f * T + Ph) * u;
				const int32 k = j * (FaU + 1) + i;
				V[k] = L + Rot + FVector(0.f, Ls * Bel * (1.f + 0.06f * FMath::Sin(T * 1.4f + Ph)) + Fl, 0.f);
				C[k] = FMath::Lerp(Canvas, CanvasFoot, v * 0.6f);
			}
		}
		Normals(FaU, FaV);
		M->UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
	}

	// ---- flags stream downwind (hull-relative yaw = wind-from + 180), travelling wave along the fly
	for (int32 f = 0; f < Flags.Num(); ++f)
	{
		FlagPivots[f]->SetRelativeRotation(FRotator(0.f, RelWind + 180.f, 0.f));
		const bool bEns = f == 0;
		const int32 NU = bEns ? EnU : PnU, NV = bEns ? EnV : PnV;
		Prep(NU, NV);
		const float Len = bEns ? 620.f : 950.f, Hgt = bEns ? 400.f : 70.f;
		const float Om = 2.f * PI * (1.0f + 1.1f * Wf), K = 2.f * PI * (bEns ? 1.3f : 2.2f);
		const float Amp = (bEns ? 42.f : 30.f) * FMath::Clamp(Wf, 0.5f, 1.3f);
		const float Droop = (1.f - FMath::Clamp(Wf, 0.f, 1.f)) * 0.5f;
		for (int32 j = 0; j <= NV; ++j)
		{
			const float v = (float)j / NV;
			for (int32 i = 0; i <= NU; ++i)
			{
				const float u = (float)i / NU;
				const float Taper = bEns ? 1.f : (1.f - 0.85f * u);
				const float Yw = Amp * u * FMath::Sin(K * u - Om * T + f * 1.7f) + 0.25f * Amp * u * FMath::Sin(2.3f * K * u - 1.9f * Om * T + v * 2.f);
				const int32 k = j * (NU + 1) + i;
				V[k] = FVector(u * Len * (1.f - 0.08f * Droop), Yw, -v * Hgt * Taper - Droop * u * Len * 0.4f);
				if (bEns)
				{
					// Ironwake ensign: storm-grey field, brass fess, oxblood hoist band
					const bool bFess = j == NV / 2 || (j == NV / 2 + 1 && NV % 2 == 1);
					C[k] = bFess ? FLinearColor(0.45f, 0.30f, 0.08f) : (i == 0 ? FLinearColor(0.20f, 0.02f, 0.015f) : FLinearColor(0.05f, 0.06f, 0.075f));
				}
				else C[k] = u > 0.8f ? FLinearColor(0.6f, 0.58f, 0.52f) : FLinearColor(0.32f, 0.025f, 0.02f);
			}
		}
		Normals(NU, NV);
		Flags[f]->UpdateMeshSection_LinearColor(0, V, N, UV, C, Tan, false);
	}
}

// A1: Autopilot for the Voyager director. Modes (key P): OFF -> HOLD COURSE -> WAYPOINTS -> TOUR.
// Full-screen chart (M) for waypoints, obstacle-circle route planning with tangent detours, a look-ahead safety
// layer, give-way to merchants, under-fire disengage, and the A1 standalone test director (-iwa1=hold|route|tour|fire).
// Deterministic code only (no LLM).
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeFleet.h"
#include "IronwakeNpc.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShell.h"
#include "IronwakeHydro.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"

#include "Components/BoxComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	const TCHAR* ApNames[] = { TEXT("OFF"), TEXT("HOLD COURSE"), TEXT("WAYPOINTS"), TEXT("TOUR") };
	const TCHAR* TelNames[] = { TEXT("FULL ASTERN"), TEXT("HALF ASTERN"), TEXT("STOP"), TEXT("SLOW AHEAD"), TEXT("HALF AHEAD"), TEXT("FULL AHEAD"), TEXT("FLANK") };
	constexpr int32 TelStop = 2, TelSlow = 3, TelHalf = 4, TelFull = 5;
	constexpr float WpArriveCm = 15000.f;     // 150 m
	constexpr float FinalSlowCm = 60000.f;    // 600 m: ease to SLOW for the last waypoint / anchorage
	constexpr float FinalStopCm = 15000.f;    // 150 m: all stop

	float Bearing(const FVector& Orig, const FVector& To)
	{
		const FVector D = To - Orig;
		return FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)) + 360.f, 360.f);
	}
	float Norm360(float A) { A = FMath::Fmod(A, 360.f); return A < 0.f ? A + 360.f : A; }
	FVector2D V2(const FVector& V) { return FVector2D(V.X, V.Y); }
	/** Distance from C to segment AB (2D) and the closest point. */
	float SegDist(const FVector& A, const FVector& B, const FVector& C, FVector& Q)
	{
		const FVector2D a = V2(A), b = V2(B), c = V2(C);
		const FVector2D ab = b - a;
		const float L2 = ab.SizeSquared();
		const float t = L2 > 1.f ? FMath::Clamp(FVector2D::DotProduct(c - a, ab) / L2, 0.f, 1.f) : 0.f;
		const FVector2D q = a + ab * t;
		Q = FVector(q.X, q.Y, 0.f);
		return (c - q).Size();
	}
	FVector Polar(const FVector& C, float AngDeg, float R)
	{
		const float A = FMath::DegreesToRadians(AngDeg);
		return FVector(C.X + FMath::Cos(A) * R, C.Y + FMath::Sin(A) * R, 0.f);
	}
}

// ============================================================================ obstacles + planning
void AIronwakeVoyage::BuildObstacles()
{
	Obstacles.Reset();
	FVector Island(-50000.f, 190000.f, 0.f);
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld())) Island = LS->IslandCentre;
	Obstacles.Add({ TEXT("St Brannoc island"), Island, 42000.f });
	Obstacles.Add({ TEXT("The Needles"), StacksC, 36000.f });
	Obstacles.Add({ TEXT("Wreck of the Valiant"), WreckC, 17000.f });
	// Greyhaven: harbour frame yaw 45 deg, local +X = seaward (see BuildHarbour). Metres in local frame.
	const FTransform H(FRotator(0.f, 45.f, 0.f), FVector(HarbourC.X, HarbourC.Y, 0.f));
	auto L = [&](float X, float Y) { FVector W = H.TransformPosition(FVector(X * 100.f, Y * 100.f, 0.f)); W.Z = 0.f; return W; };
	auto Wall = [&](const TCHAR* N, float CX, float CY, float YawDeg, float LenM)
	{
		const float Yr = FMath::DegreesToRadians(YawDeg);
		for (float s = -LenM * 0.5f; s <= LenM * 0.5f + 1.f; s += 40.f)
			Obstacles.Add({ N, L(CX + FMath::Cos(Yr) * s, CY + FMath::Sin(Yr) * s), 4500.f });
	};
	Wall(TEXT("Greyhaven south breakwater"), 60.f, -190.f, 12.f, 300.f);
	Wall(TEXT("Greyhaven north breakwater"), 10.f, 200.f, -18.f, 200.f);
	Obstacles.Add({ TEXT("Greyhaven quay"), L(-170.f, -200.f), 17000.f });
	Obstacles.Add({ TEXT("Greyhaven quay"), L(-170.f, 0.f), 17000.f });
	Obstacles.Add({ TEXT("Greyhaven quay"), L(-170.f, 200.f), 17000.f });
	Obstacles.Add({ TEXT("Greyhaven pier"), L(30.f, 10.f), 9000.f });
	for (USceneComponent* B : Buoys) if (B) { FVector P = B->GetComponentLocation(); P.Z = 0.f; Obstacles.Add({ TEXT("channel buoy"), P, 3000.f }); }
	// I1: archipelago islands (hazard circle = shoreline radius x1.35 + 60 m; reefs/graveyard include the outlying rocks).
	// P29: secondary ports replace the hosted island's solid circle with 3 back-quay + 2 flank breakwater circles, leaving the [-68,+68] m harbour mouth open!
	if (AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld()))
	{
		for (int32 i = 0; i < Ar->Isles.Num(); ++i)
		{
			if (Ar->HostedPortIsles.Contains(i)) continue;
			const FIsle& I = Ar->Isles[i];
			Obstacles.Add({ I.Name, FVector(I.C.X, I.C.Y, 0.f), I.HazardR });
		}
	}
	for (const FIwPortSpec& Ps : WorldPorts)
	{
		if (Ps.bMainHarbour) continue;
		auto PL = [&](float X, float Y) { FVector W = Ps.H2W(X, Y, 0.f); W.Z = 0.f; return W; };
		auto PWall = [&](const FString& Nm, float CX, float CY, float YawDeg, float LenM)
		{
			const float Yr = FMath::DegreesToRadians(YawDeg);
			for (float s = -LenM * 0.5f; s <= LenM * 0.5f + 1.f; s += 38.f)
				Obstacles.Add({ Nm, PL(CX + FMath::Cos(Yr) * s, CY + FMath::Sin(Yr) * s), 4200.f });
		};
		PWall(Ps.Name + TEXT(" south breakwater"), 42.f, -114.f, 14.f, 150.f);
		PWall(Ps.Name + TEXT(" north breakwater"), 38.f,  114.f, -14.f, 145.f);
		Obstacles.Add({ Ps.Name + TEXT(" quay"), PL(-145.f, -120.f), 13500.f });
		Obstacles.Add({ Ps.Name + TEXT(" quay"), PL(-145.f,    0.f), 13500.f });
		Obstacles.Add({ Ps.Name + TEXT(" quay"), PL(-145.f,  120.f), 13500.f });
	}
	AnchorPt = L(450.f, 0.f);
	ObsMinDist.Init(1e12f, Obstacles.Num());
	UE_LOG(LogIronwake, Display, TEXT("IWA1 obstacles=%d anchorage=%s"), Obstacles.Num(), *AnchorPt.ToString());
}

void AIronwakeVoyage::PlanLeg(const FVector& A, const FVector& B, TArray<FVector>& Out, int32 Depth, TArray<FString>* Detours) const
{
	// First obstacle circle the straight segment crosses (both ends outside it). Detour = tangent points + an arc
	// on the side the segment already passes, radius 1.15 R (chords kept outside). Recurse on the pieces.
	int32 Hit = INDEX_NONE;
	float HitT = 1e12f;
	for (int32 i = 0; i < Obstacles.Num() && Depth < 7; ++i)
	{
		const FObs& O = Obstacles[i];
		if (FVector::Dist2D(A, O.C) < O.R * 1.02f || FVector::Dist2D(B, O.C) < O.R * 1.02f) continue;
		FVector Q;
		if (SegDist(A, B, O.C, Q) < O.R)
		{
			const float T = FVector::Dist2D(A, Q);
			if (T < HitT) { HitT = T; Hit = i; }
		}
	}
	if (Hit == INDEX_NONE) { Out.Add(B); return; }
	const FObs& O = Obstacles[Hit];
	const float Rm = O.R * 1.15f;
	FVector Q; SegDist(A, B, O.C, Q);
	FVector N = (Q - O.C).GetSafeNormal2D();
	if (N.IsNearlyZero()) { const FVector D = (B - A).GetSafeNormal2D(); N = FVector(-D.Y, D.X, 0.f); } // dead centre: pass to starboard of the obstacle's... left side
	const float AngA = Bearing(O.C, A), AngB = Bearing(O.C, B), AngN = Bearing(O.C, O.C + N);
	// Sweep +1 = increasing angle from A to B; choose the sweep whose arc contains N.
	const float SwPlus = Norm360(AngB - AngA);
	const float S = Norm360(AngN - AngA) <= SwPlus ? 1.f : -1.f;
	const float dA = FMath::Max(FVector::Dist2D(A, O.C), Rm * 1.001f), dB = FMath::Max(FVector::Dist2D(B, O.C), Rm * 1.001f);
	const float TA = FMath::RadiansToDegrees(FMath::Acos(Rm / dA)), TB = FMath::RadiansToDegrees(FMath::Acos(Rm / dB));
	const float AngS = AngA + S * TA, AngE = AngB - S * TB;
	const float Sw = Norm360(S * (AngE - AngS));
	TArray<FVector> Pts;
	if (Sw > 300.f)
	{
		// Tangent points overlap: one point between them is visible from both ends.
		Pts.Add(Polar(O.C, AngS - S * (360.f - Sw) * 0.5f, Rm * 1.05f));
	}
	else
	{
		const int32 NSeg = FMath::Max(1, FMath::CeilToInt(Sw / 40.f));
		const float Rd = Rm / FMath::Cos(FMath::DegreesToRadians(Sw / NSeg * 0.5f));
		for (int32 k = 0; k <= NSeg; ++k) Pts.Add(Polar(O.C, AngS + S * Sw * k / NSeg, k == 0 || k == NSeg ? Rm : Rd));
	}
	if (Detours) Detours->AddUnique(O.Name);
	FVector Prev = A;
	for (const FVector& P : Pts) { PlanLeg(Prev, P, Out, Depth + 1, Detours); Prev = P; }
	PlanLeg(Prev, B, Out, Depth + 1, Detours);
}

void AIronwakeVoyage::BuildPreview(const FVector& Orig, TArray<FVector>& Out, TArray<FString>* Detours) const
{
	Out.Reset();
	if (Waypoints.Num() == 0) return;
	FVector Prev(Orig.X, Orig.Y, 0.f);
	const int32 Start = (ApMode == 2) ? FMath::Clamp(CurWp, 0, Waypoints.Num() - 1) : 0;
	for (int32 i = Start; i < Waypoints.Num(); ++i) { PlanLeg(Prev, Waypoints[i], Out, 0, Detours); Prev = Waypoints[i]; }
	if (bApLoop && Waypoints.Num() > 1) for (int32 i = 0; i < Start + 1 && i < Waypoints.Num(); ++i) { PlanLeg(Prev, Waypoints[i], Out, 0, Detours); Prev = Waypoints[i]; }
}

void AIronwakeVoyage::StartLeg(AIronwakeShipPawn* P, const FVector& Target)
{
	LegPath.Reset();
	LegIdx = 0;
	TArray<FString> Det;
	const FVector Orig(P->GetActorLocation().X, P->GetActorLocation().Y, 0.f);
	PlanLeg(Orig, FVector(Target.X, Target.Y, 0.f), LegPath, 0, &Det);
	UE_LOG(LogIronwake, Display, TEXT("IWA1 leg from (%.0f,%.0f) to (%.0f,%.0f): %d pts, detours: %s"), Orig.X / 100.f, Orig.Y / 100.f, Target.X / 100.f, Target.Y / 100.f, LegPath.Num(),
		Det.Num() ? *FString::Join(Det, TEXT(", ")) : TEXT("none"));
	if (Det.Num()) ShowToast(FString::Printf(TEXT("Autopilot: detour around %s"), *Det[0]), 4.f);
}

// ============================================================================ modes
FString AIronwakeVoyage::AutopilotLabel() const
{
	switch (ApMode)
	{
	case 1: return FString::Printf(TEXT("HOLD COURSE %03.0f"), Norm360(HoldYaw));
	case 2:
		if (Waypoints.Num() == 0) return TEXT("WAYPOINTS (planning: M chart)");
		return FString::Printf(TEXT("WAYPOINTS %d/%d%s"), FMath::Min(CurWp + 1, Waypoints.Num()), Waypoints.Num(), bApLoop ? TEXT(" LOOP") : TEXT(""));
	case 3:
	{
		static const TCHAR* Ph[] = { TEXT("making for"), TEXT("circling") };
		if (TourStop < 3 && POIs.IsValidIndex(TourStop)) return FString::Printf(TEXT("TOUR %d/4: %s %s"), TourStop + 1, Ph[FMath::Clamp(TourPhase, 0, 1)], *POIs[TourStop].Name);
		return TEXT("TOUR 4/4: home to Greyhaven");
	}
	default: return TEXT("OFF");
	}
}

void AIronwakeVoyage::SetApTel(AIronwakeShipPawn* P, int32 Idx)
{
	if (!P) return;
	if (P->TelegraphIndex != Idx) P->IWTelegraph(Idx);
	ApTelSet = Idx;
}

void AIronwakeVoyage::EngageAutopilot(int32 Mode)
{
	AIronwakeShipPawn* P = Player();
	AIronwakePlayerController* C = PC();
	if (!P || !C) return;
	if (Obstacles.Num() == 0) BuildObstacles();
	if (Mode == 2 && Waypoints.Num() == 0)
	{
		// Planning: hold the present heading while the captain charts a route.
		ApMode = 2;
		HoldYaw = P->GetActorRotation().Yaw;
		bApOwnsTel = false;
		CruiseTel = ApTelSet = P->TelegraphIndex;
		LegPath.Reset(); LegIdx = 0; CurWp = 0;
		PrevRudIn = P->GetRudderInputValue();
		if (!bChart) ToggleChart();
		ShowToast(TEXT("AUTOPILOT: WAYPOINTS - click the chart to add points, Enter or P to start"), 6.f);
		P->bAutoHelm = true;
		UE_LOG(LogIronwake, Display, TEXT("IWA1 engage WAYPOINTS (planning, no route yet)"));
		return;
	}
	if (bAnchored) ToggleAnchor();
	bHelm = false;
	ApMode = Mode;
	P->bAutoHelm = true;
	bRouteDone = bTourDone = false;
	PrevRudIn = P->GetRudderInputValue();
	GiveWayUntil = -1.f;
	bGiveWaySlowed = false;
	if (AIronwakeShipPawn* E = P->FindEnemy()) LastEnemyShots = E->ShellsFiredCount();
	// Telegraph: the autopilot rings it only if the captain hasn't set a speed (stopped / astern).
	if (bGiveWaySlowed && ApMode != 0) { P->IWTelegraph(GiveWaySavedTel); bGiveWaySlowed = false; }
	bApOwnsTel = P->TelegraphIndex <= TelStop;
	CruiseTel = bApOwnsTel ? (Mode == 3 ? TelFull : TelHalf) : P->TelegraphIndex;
	ApTelSet = P->TelegraphIndex;
	if (bApOwnsTel) SetApTel(P, CruiseTel);
	const FString SpeedNote = bApOwnsTel ? FString::Printf(TEXT(" - rang %s"), TelNames[CruiseTel]) : FString::Printf(TEXT(" - keeping your %s"), TelNames[FMath::Clamp(CruiseTel, 0, 6)]);
	if (Mode == 1)
	{
		// Lock the present heading, or the camera's bearing if the captain is looking well off the bow.
		const float Hd = P->GetActorRotation().Yaw;
		const float Cam = C->PlayerCameraManager ? C->PlayerCameraManager->GetCameraRotation().Yaw : Hd;
		const bool bCam = FMath::Abs(FMath::FindDeltaAngleDegrees(Hd, Cam)) > 15.f;
		HoldYaw = bCam ? Cam : Hd;
		ShowToast(FString::Printf(TEXT("AUTOPILOT: HOLD COURSE %03.0f%s%s   (A/D: +/-5 deg)"), Norm360(HoldYaw), bCam ? TEXT(" (camera bearing)") : TEXT(""), *SpeedNote), 5.f);
	}
	else if (Mode == 2)
	{
		CurWp = 0;
		StartLeg(P, Waypoints[0]);
		if (bChart) ToggleChart();
		ShowToast(FString::Printf(TEXT("AUTOPILOT: WAYPOINTS - %d points%s%s"), Waypoints.Num(), bApLoop ? TEXT(", looping") : TEXT(""), *SpeedNote), 5.f);
	}
	else if (Mode == 3)
	{
		TourStop = 0; TourPhase = 0;
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = false;
		const FIronwakePOI& Pt = POIs[0];
		const float Ro = Pt.Tag == TEXT("W") ? 50000.f : 60000.f;
		StartLeg(P, Pt.Loc + (FVector(P->GetActorLocation().X, P->GetActorLocation().Y, 0.f) - Pt.Loc).GetSafeNormal2D() * Ro);
		if (bChart) ToggleChart();
		ShowToast(FString::Printf(TEXT("AUTOPILOT: TOUR - every point of interest, then home to Greyhaven%s"), *SpeedNote), 5.f);
	}
	UE_LOG(LogIronwake, Display, TEXT("IWA1 engage %s ownsTel=%d cruise=%s hold=%.1f wps=%d"), ApNames[Mode], bApOwnsTel, TelNames[FMath::Clamp(CruiseTel, 0, 6)], Norm360(HoldYaw), Waypoints.Num());
}

void AIronwakeVoyage::DisengageAutopilot(const FString& Why)
{
	AIronwakeShipPawn* P = Player();
	const int32 Was = ApMode;
	ApMode = 0;
	if (P)
	{
		P->bAutoHelm = false;
		// Leave the wheel under the captain's hands if they grabbed it; otherwise put it amidships.
		if (FMath::Abs(P->GetRudderInputValue()) < 0.1f) P->IWRudder(0);
		if (bGiveWaySlowed) P->IWTelegraph(GiveWaySavedTel);
	}
	bGiveWaySlowed = false;
	ApTelSet = -1;
	ShowToast(Why, 5.f);
	UE_LOG(LogIronwake, Display, TEXT("IWA1 disengage (was %s): %s"), ApNames[FMath::Clamp(Was, 0, 3)], *Why);
}

void AIronwakeVoyage::CycleAutopilot()
{
	if (bChart && Waypoints.Num() > 0) { ChartStart(); return; } // in the chart P = start the route
	if (bChart) ToggleChart(); // empty chart: close it and keep cycling
	switch (ApMode)
	{
	case 0: EngageAutopilot(1); break;
	case 1: EngageAutopilot(2); break;
	case 2: EngageAutopilot(3); break;
	default: DisengageAutopilot(TEXT("Autopilot OFF: you have the wheel")); break;
	}
}

void AIronwakeVoyage::ToggleChart()
{
	AIronwakePlayerController* C = PC();
	if (!C) return;
	if (bPhoto) return;
	bChart = !bChart;
	C->SetChartInput(bChart);
	UE_LOG(LogIronwake, Display, TEXT("IWA1 chart %d"), bChart);
}

void AIronwakeVoyage::ChartClick(const FVector2D& Screen, bool bRight)
{
	if (!bChart) return;
	const FVector W = ChartToWorld(Screen);
	if (FMath::Abs(W.X) > ChartHalfCm || FMath::Abs(W.Y) > ChartHalfCm) return;
	if (bRight)
	{
		int32 Best = INDEX_NONE; float BestD = 1e12f;
		for (int32 i = 0; i < Waypoints.Num(); ++i) { const float D = FVector::Dist2D(Waypoints[i], W); if (D < BestD) { BestD = D; Best = i; } }
		if (Best != INDEX_NONE && BestD < 40000.f)
		{
			Waypoints.RemoveAt(Best);
			if (ApMode == 2 && CurWp >= Best && CurWp > 0) --CurWp;
			UE_LOG(LogIronwake, Display, TEXT("IWA1 waypoint %d removed (%d left)"), Best + 1, Waypoints.Num());
			ShowToast(FString::Printf(TEXT("Waypoint %d removed"), Best + 1), 2.f);
		}
		return;
	}
	// A point inside a hazard is pushed out to its edge (+50 m).
	FVector P = W; P.Z = 0.f;
	if (Obstacles.Num() == 0) BuildObstacles();
	for (const FObs& O : Obstacles)
	{
		if (FVector::Dist2D(P, O.C) < O.R + 5000.f) P = O.C + (P - O.C).GetSafeNormal2D() * (O.R + 5000.f);
	}
	if (Waypoints.Num() >= 12) { ShowToast(TEXT("Chart full: 12 waypoints max"), 2.f); return; }
	Waypoints.Add(P);
	UE_LOG(LogIronwake, Display, TEXT("IWA1 waypoint %d at (%.0f, %.0f) m"), Waypoints.Num(), P.X / 100.f, P.Y / 100.f);
}

void AIronwakeVoyage::ChartStart()
{
	if (Waypoints.Num() == 0) { ShowToast(TEXT("No waypoints: left-click the chart to add some"), 3.f); return; }
	EngageAutopilot(2);
}

void AIronwakeVoyage::ToggleLoop()
{
	bApLoop = !bApLoop;
	ShowToast(bApLoop ? TEXT("Route LOOP on") : TEXT("Route LOOP off: stop at the last waypoint"), 2.5f);
}

void AIronwakeVoyage::ClearWaypoints()
{
	Waypoints.Reset();
	if (ApMode == 2) DisengageAutopilot(TEXT("Route cleared: autopilot OFF"));
	else ShowToast(TEXT("Route cleared"), 2.f);
}

FVector2D AIronwakeVoyage::WorldToChart(const FVector& W) const
{
	return FVector2D(ChartCX + W.Y / ChartHalfCm * ChartHalfPx, ChartCY - W.X / ChartHalfCm * ChartHalfPx);
}

FVector AIronwakeVoyage::ChartToWorld(const FVector2D& S) const
{
	return FVector(-(S.Y - ChartCY) / ChartHalfPx * ChartHalfCm, (S.X - ChartCX) / ChartHalfPx * ChartHalfCm, 0.f);
}

// ============================================================================ steering
void AIronwakeVoyage::SteerAuto(AIronwakeShipPawn* P, float DesiredYaw, float FFRate, float MaxRud)
{
	if (P->HasSails())
	{
		// S2: never steer into irons. If the course is within 50 deg of the wind, beat: close-hauled on the tack nearer the
		// course, with 12 deg hysteresis so the ship tacks only when the other board clearly pays (zig-zag to windward).
		const float WindFrom = IronwakeFleet::WindFromYaw(GetWorld());
		const float Off = FMath::FindDeltaAngleDegrees(WindFrom, DesiredYaw);
		constexpr float Beat = 50.f;
		if (FMath::Abs(Off) < Beat)
		{
			const int32 Want = Off >= 0.f ? 1 : -1;
			if (BeatTack == 0) BeatTack = Want;
			else if (Want != BeatTack && FMath::Abs(Off) > 12.f) { BeatTack = Want; ShowToast(TEXT("Autopilot: ready about - tacking"), 3.f); UE_LOG(LogIronwake, Display, TEXT("IWS2 autopilot tack -> %s board"), BeatTack > 0 ? TEXT("starboard-course") : TEXT("port-course")); }
			DesiredYaw = WindFrom + BeatTack * Beat;
			FFRate = 0.f;
		}
		else BeatTack = 0;
	}
	// P9: safety net behind the planned hazard circles: bend the commanded heading off any solid or shoal ahead.
	{
		const float Safe = P->AvoidLand(DesiredYaw, FMath::Clamp(P->GetSpeedKnots() * 51.4444f * 1.5f * 40.f, 60000.f, 220000.f));
		if (Safe != DesiredYaw) { DesiredYaw = Safe; FFRate = 0.f; }
	}
	// Invert the hull's yaw model (yaw rate = v / Rmin * rudder fraction): a heading loop asks for a yaw rate,
	// a rate loop turns it into rudder, normalised by speed so it behaves the same at SLOW and at fast sail.
	const FVector Fwd = P->GetActorForwardVector().GetSafeNormal2D();
	const float V = FMath::Max(FVector::DotProduct(P->HullBody->GetPhysicsLinearVelocity(), Fwd) / 100.f, 1.0f);
	const float RMax = FMath::RadiansToDegrees(V / FMath::Max(P->MinTurnRadiusM, 1.f));
	const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, DesiredYaw);
	ApErr = Err;
	ApTargetYaw = DesiredYaw;
	const float Rd = FMath::Clamp(0.22f * Err + (FMath::Abs(Err) < 25.f ? FFRate : 0.f), -0.85f * RMax, 0.85f * RMax); // orbit feed-forward only once roughly on the tangent
	const float R = P->HullBody->GetPhysicsAngularVelocityInDegrees().Z;
	const float Cmd = Rd + 1.2f * (Rd - R);
	const float Frac = FMath::Clamp(Cmd / FMath::Max(RMax, 0.05f), -MaxRud, MaxRud);
	P->SetAutoRudder(Frac * P->MaxRudderDeg);
}

// ============================================================================ tick
void AIronwakeVoyage::TickAutopilot(float Dt)
{
	AIronwakeShipPawn* P = Player();
	AIronwakePlayerController* C = PC();
	if (!P || !C || !C->bStarted) return;
	if (Obstacles.Num() == 0) BuildObstacles();
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	const FVector Loc(P->GetActorLocation().X, P->GetActorLocation().Y, 0.f);
	for (int32 i = 0; i < Obstacles.Num(); ++i) ObsMinDist[i] = FMath::Min(ObsMinDist[i], FVector::Dist2D(Loc, Obstacles[i].C) - Obstacles[i].R);

	// Under fire: the enemy's guns speak -> hand the ship back.
	AIronwakeShipPawn* E = P->FindEnemy();
	const int32 Shots = E ? E->ShellsFiredCount() : 0;
	if (Shots > LastEnemyShots && ApMode != 0) { LastEnemyShots = Shots; DisengageAutopilot(TEXT("Autopilot off: under fire")); return; }
	LastEnemyShots = Shots;
	if (ApMode == 0) { PrevRudIn = P->GetRudderInputValue(); return; }
	if (P->bSinking) { DisengageAutopilot(TEXT("Autopilot off")); return; }
	P->bAutoHelm = true;

	// Captain's inputs: W/S sets the speed (autopilot keeps steering); A/D nudges HOLD, or takes the wheel back.
	if (ApTelSet >= 0 && P->TelegraphIndex != ApTelSet)
	{
		CruiseTel = P->TelegraphIndex;
		ApTelSet = CruiseTel;
		bApOwnsTel = false;
		bGiveWaySlowed = false;
		ShowToast(FString::Printf(TEXT("Autopilot steering - speed is yours: %s"), TelNames[FMath::Clamp(CruiseTel, 0, 6)]), 3.f);
		UE_LOG(LogIronwake, Display, TEXT("IWA1 captain set speed %s (autopilot keeps %s)"), TelNames[FMath::Clamp(CruiseTel, 0, 6)], ApNames[ApMode]);
	}
	const float RudIn = P->GetRudderInputValue();
	const bool bPress = FMath::Abs(RudIn) > 0.5f && FMath::Abs(PrevRudIn) <= 0.5f;
	PrevRudIn = RudIn;
	if (bPress)
	{
		if (ApMode == 1 || (ApMode == 2 && Waypoints.Num() == 0))
		{
			HoldYaw = FRotator::NormalizeAxis(HoldYaw + (RudIn > 0.f ? 5.f : -5.f));
			ShowToast(FString::Printf(TEXT("Course %03.0f"), Norm360(HoldYaw)), 2.f);
			UE_LOG(LogIronwake, Display, TEXT("IWA1 nudge %+.0f -> course %.1f"), RudIn > 0.f ? 5.f : -5.f, Norm360(HoldYaw));
		}
		else { DisengageAutopilot(TEXT("Autopilot off: you have the wheel")); return; }
	}

	const FVector Fwd = P->GetActorForwardVector().GetSafeNormal2D();
	const float VCm = FMath::Max(FVector::DotProduct(P->HullBody->GetPhysicsLinearVelocity(), Fwd), 0.f);
	const float Accept = FMath::Max(WpArriveCm, VCm * 8.f);
	float Desired = HoldYaw, FF = 0.f, MaxRud = 1.f;
	bool bOrbit = false;
	int32 WantTel = CruiseTel;
	FVector Target = Loc;
	bool bHasTarget = false;

	auto FollowLeg = [&](float FinalRadius) -> bool // true when the final point of the leg is reached
	{
		while (LegPath.IsValidIndex(LegIdx) && LegIdx < LegPath.Num() - 1)
		{
			const FVector& Pt = LegPath[LegIdx];
			const FVector Nx = LegPath[LegIdx + 1];
			const bool bPassed = FVector::DotProduct((Nx - Pt).GetSafeNormal2D(), Loc - Pt) > 0.f && FVector::Dist2D(Loc, Pt) < Accept * 3.f;
			if (FVector::Dist2D(Loc, Pt) < Accept || bPassed) ++LegIdx; else break;
		}
		if (!LegPath.IsValidIndex(LegIdx)) return true;
		Target = LegPath[LegIdx];
		bHasTarget = true;
		Desired = Bearing(Loc, Target);
		return LegIdx == LegPath.Num() - 1 && FVector::Dist2D(Loc, Target) < FinalRadius;
	};

	if (ApMode == 2 && Waypoints.Num() > 0)
	{
		CurWp = FMath::Clamp(CurWp, 0, Waypoints.Num() - 1);
		const bool bLast = CurWp == Waypoints.Num() - 1 && !bApLoop;
		const float DistWp = FVector::Dist2D(Loc, Waypoints[CurWp]);
		if (bLast)
		{
			FollowLeg(FinalStopCm);
			if (DistWp < FinalSlowCm * FMath::Max(1.f, P->SpeedCompression / 1.5f * 0.5f)) WantTel = FMath::Min(CruiseTel, TelSlow);
			if (DistWp < FinalStopCm || (DistWp < 50000.f && FVector::DotProduct(Fwd, Waypoints[CurWp] - Loc) < 0.f)) WantTel = TelStop; // at it, or passing it
			if (DistWp < 50000.f && WantTel == TelStop && FMath::Abs(P->GetSpeedKnots()) < 0.6f)
			{
				SetApTel(P, TelStop);
				bRouteDone = true;
				UE_LOG(LogIronwake, Display, TEXT("IWA1 ROUTE COMPLETE at (%.0f,%.0f) m, %.0f m from the last waypoint"), Loc.X / 100.f, Loc.Y / 100.f, DistWp / 100.f);
				DisengageAutopilot(TEXT("Route complete: all stop"));
				return;
			}
		}
		else if (FollowLeg(Accept))
		{
			UE_LOG(LogIronwake, Display, TEXT("IWA1 waypoint %d reached (%.0f m off)"), CurWp + 1, DistWp / 100.f);
			ShowToast(FString::Printf(TEXT("Waypoint %d reached"), CurWp + 1), 2.5f);
			CurWp = (CurWp + 1) % Waypoints.Num();
			StartLeg(P, Waypoints[CurWp]);
			FollowLeg(0.f);
		}
		// Replan if something now lies across the line to the next path point (e.g. after a give-way swerve).
		if (bHasTarget && FMath::Fmod(Now, 1.f) < Dt)
		{
			for (const FObs& O : Obstacles)
			{
				FVector Q;
				if (O.R < 5000.f) continue; // buoys: the look-ahead layer handles them; replanning on them churns
				if (FVector::Dist2D(Loc, O.C) > O.R && FVector::Dist2D(Target, O.C) > O.R && SegDist(Loc, Target, O.C, Q) < O.R * 0.9f) { StartLeg(P, Waypoints[CurWp]); FollowLeg(0.f); break; }
			}
		}
	}
	else if (ApMode == 3)
	{
		if (TourStop < 3 && POIs.IsValidIndex(TourStop))
		{
			FIronwakePOI& Pt = POIs[TourStop];
			const float Ro = Pt.Tag == TEXT("W") ? 50000.f : (Pt.Tag == TEXT("N") ? 60000.f : 65000.f);
			if (TourPhase == 0)
			{
				if (FollowLeg(Accept) || FMath::Abs(FVector::Dist2D(Loc, Pt.Loc) - Ro) < 8000.f)
				{
					TourPhase = 1;
					OrbitSwept = 0.f;
					OrbitPrevAng = Bearing(Pt.Loc, Loc);
					ShowToast(FString::Printf(TEXT("Tour: circling %s"), *Pt.Name), 4.f);
					UE_LOG(LogIronwake, Display, TEXT("IWA1 TOUR orbit start %s r=%.0f m"), *Pt.Name, FVector::Dist2D(Loc, Pt.Loc) / 100.f);
				}
			}
			if (TourPhase == 1)
			{
				// Clockwise orbit (keeps the sight on the starboard side, where the default camera looks out).
				bOrbit = true;
				const float Ang = Bearing(Pt.Loc, Loc);
				OrbitSwept += FMath::FindDeltaAngleDegrees(OrbitPrevAng, Ang);
				OrbitPrevAng = Ang;
				const float Rm = FVector::Dist2D(Loc, Pt.Loc);
				Desired = Ang + 90.f + FMath::Clamp((Rm - Ro) / 100.f * 0.15f, -35.f, 35.f);
				FF = FMath::RadiansToDegrees((VCm / 100.f) / FMath::Max(Ro / 100.f, 1.f));
				WantTel = bApOwnsTel ? TelSlow : FMath::Min(CruiseTel, TelSlow);
				Target = Pt.Loc; bHasTarget = true;
				if (OrbitSwept >= 360.f)
				{
					Pt.bVisited = true;
					UE_LOG(LogIronwake, Display, TEXT("IWA1 TOUR visited %s (orbit %.0f deg, r=%.0f m)"), *Pt.Name, OrbitSwept, Rm / 100.f);
					ShowToast(FString::Printf(TEXT("Tour: %s visited"), *Pt.Name), 3.f);
					++TourStop; TourPhase = 0;
					if (TourStop < 3)
					{
						const FIronwakePOI& N = POIs[TourStop];
						const float Rn = N.Tag == TEXT("W") ? 50000.f : (N.Tag == TEXT("N") ? 60000.f : 65000.f);
						StartLeg(P, N.Loc + (Loc - N.Loc).GetSafeNormal2D() * Rn);
					}
					else StartLeg(P, AnchorPt);
				}
			}
		}
		else
		{
			// Home: run in to the anchorage off the harbour mouth, slow, stop, let go the anchor.
			const float D = FVector::Dist2D(Loc, AnchorPt);
			FollowLeg(FinalStopCm);
			if (D < FinalSlowCm * FMath::Max(1.f, P->SpeedCompression / 1.5f * 0.5f)) WantTel = FMath::Min(CruiseTel, TelSlow);
			if (D < FinalStopCm || (D < 50000.f && FVector::DotProduct(Fwd, AnchorPt - Loc) < 0.f)) WantTel = TelStop;
			if (D < 50000.f && WantTel == TelStop && FMath::Abs(P->GetSpeedKnots()) < 0.6f)
			{
				if (POIs.IsValidIndex(3)) POIs[3].bVisited = true;
				bTourDone = true;
				UE_LOG(LogIronwake, Display, TEXT("IWA1 TOUR COMPLETE: anchored %.0f m from the anchorage, %.0f m from Greyhaven"), D / 100.f, FVector::Dist2D(Loc, HarbourC) / 100.f);
				ApMode = 0; // quietly, then the anchor (its toast wins)
				P->bAutoHelm = false;
				P->IWRudder(0);
				ApTelSet = -1;
				if (!bAnchored) ToggleAnchor();
				ShowToast(TEXT("Tour complete: anchored off Greyhaven"), 6.f);
				return;
			}
		}
	}

	// Give way: a merchant within ~400 m ahead -> steer to starboard and ease one step until clear.
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(W))
	{
		const FVector Right(-Fwd.Y, Fwd.X, 0.f);
		for (AIronwakeMerchant* M : LS->Merchants)
		{
			if (!IsValid(M)) continue;
			const FVector Rel = M->GetActorLocation() - Loc;
			const float Along = FVector::DotProduct(FVector(Rel.X, Rel.Y, 0.f), Fwd), Lat = FVector::DotProduct(FVector(Rel.X, Rel.Y, 0.f), Right);
			if (Along > 0.f && Along < 40000.f + M->LengthCm && FMath::Abs(Lat) < 15000.f)
			{
				if (Now > GiveWayUntil)
				{
					++GiveWayCount;
					GiveWayYaw = P->GetActorRotation().Yaw + 35.f;
					GiveWayName = M->ShipName;
					ShowToast(FString::Printf(TEXT("Autopilot: giving way to %s"), *M->ShipName), 3.f);
					UE_LOG(LogIronwake, Display, TEXT("IWA1 give way to %s at %.0f m ahead, %.0f m abeam"), *M->ShipName, Along / 100.f, Lat / 100.f);
				}
				GiveWayUntil = Now + 8.f;
			}
		}
	}
	const bool bGiveWay = Now < GiveWayUntil && !bOrbit;
	if (bGiveWay)
	{
		Desired = GiveWayYaw; // 35 deg to starboard of the course at the moment the merchant was seen
		WantTel = FMath::Max(TelSlow, FMath::Min(WantTel, CruiseTel - 1));
	}

	// Look-ahead safety: if the course would run onto a hazard, steer for its tangent instead.
	if (!bGiveWay)
	{
		const float Look = 60000.f + VCm * 20.f;
		const FVector Dir = FRotator(0.f, Desired, 0.f).Vector().GetSafeNormal2D();
		const float RayLen = bHasTarget && !bOrbit ? FMath::Min(Look, FVector::Dist2D(Loc, Target)) : Look;
		for (int32 i = 0; i < Obstacles.Num(); ++i)
		{
			const FObs& O = Obstacles[i];
			const float D = FVector::Dist2D(Loc, O.C);
			if (bOrbit && D > O.R + 5000.f) continue;
			if (D < O.R * 0.98f)
			{
				// Inside a hazard circle: head straight out, slow.
				Desired = Bearing(O.C, Loc);
				WantTel = TelSlow;
				static float LastIn = -100.f;
				if (Now - LastIn > 5.f) { LastIn = Now; UE_LOG(LogIronwake, Display, TEXT("IWA1 safety: inside %s circle (%.0f m in), heading out"), *O.Name, (O.R - D) / 100.f); }
				break;
			}
			FVector Q;
			if (SegDist(Loc, Loc + Dir * RayLen, O.C, Q) < O.R)
			{
				const float Bc = Bearing(Loc, O.C);
				const float Half = FMath::RadiansToDegrees(FMath::Asin(FMath::Min(1.f, O.R * 1.15f / FMath::Max(D, 1.f))));
				const float A1 = Bc + Half, A2 = Bc - Half;
				Desired = FMath::Abs(FMath::FindDeltaAngleDegrees(Desired, A1)) < FMath::Abs(FMath::FindDeltaAngleDegrees(Desired, A2)) ? A1 : A2;
				static float LastLog = -100.f;
				if (Now - LastLog > 5.f) { LastLog = Now; UE_LOG(LogIronwake, Display, TEXT("IWA1 safety: steering clear of %s (%.0f m)"), *O.Name, (D - O.R) / 100.f); }
				break;
			}
		}
	}

	// Telegraph: arrival / orbit / give-way overrides, otherwise cruise.
	if (bGiveWay && !bGiveWaySlowed) { bGiveWaySlowed = true; GiveWaySavedTel = CruiseTel; }
	if (!bGiveWay) bGiveWaySlowed = false;
	SetApTel(P, WantTel);

	SteerAuto(P, Desired, FF, MaxRud);
}

// ============================================================================ chart + HUD bits
void AIronwakeVoyage::DrawChart(AHUD* H, UCanvas* Cv, AIronwakeShipPawn* Ship, UFont* Font, float S)
{
	const FLinearColor Brass(0.86f, 0.74f, 0.47f, 1.f), Dim(0.62f, 0.66f, 0.70f, 1.f), Hi(1.f, 0.85f, 0.45f, 1.f), Land(0.30f, 0.26f, 0.18f, 1.f), Haz(0.75f, 0.30f, 0.20f, 0.85f);
	ChartHalfPx = FMath::Min(Cv->ClipX * 0.36f, Cv->ClipY * 0.45f);
	ChartCX = Cv->ClipX * 0.40f;
	ChartCY = Cv->ClipY * 0.5f;
	H->DrawRect(FLinearColor(0.005f, 0.01f, 0.015f, 0.88f), 0.f, 0.f, Cv->ClipX, Cv->ClipY);
	const float X0 = ChartCX - ChartHalfPx, Y0 = ChartCY - ChartHalfPx, Sz = 2.f * ChartHalfPx;
	H->DrawRect(FLinearColor(0.06f, 0.10f, 0.13f, 1.f), X0, Y0, Sz, Sz);
	// 5 km maritime chart grid (P32: 29 km x 29 km archipelago chart)
	const float GridStepCm = ChartHalfCm > 600000.f ? 500000.f : 100000.f;
	for (float G = -FMath::FloorToFloat(ChartHalfCm / GridStepCm) * GridStepCm; G <= ChartHalfCm + 1.f; G += GridStepCm)
	{
		const FVector2D A = WorldToChart(FVector(G, -ChartHalfCm, 0)), B = WorldToChart(FVector(G, ChartHalfCm, 0));
		const FVector2D C2 = WorldToChart(FVector(-ChartHalfCm, G, 0)), D2 = WorldToChart(FVector(ChartHalfCm, G, 0));
		const FLinearColor Gc(0.25f, 0.32f, 0.36f, FMath::IsNearlyZero(G) ? 0.8f : 0.45f);
		H->DrawLine(A.X, A.Y, B.X, B.Y, Gc, 1.f);
		H->DrawLine(C2.X, C2.Y, D2.X, D2.Y, Gc, 1.f);
	}
	H->DrawLine(X0, Y0, X0 + Sz, Y0, Brass, 2.f); H->DrawLine(X0, Y0 + Sz, X0 + Sz, Y0 + Sz, Brass, 2.f);
	H->DrawLine(X0, Y0, X0, Y0 + Sz, Brass, 2.f); H->DrawLine(X0 + Sz, Y0, X0 + Sz, Y0 + Sz, Brass, 2.f);
	H->DrawText(TEXT("N"), Brass, ChartCX - 6.f * S, Y0 - 26.f * S, Font, 1.1f * S);
	// Scale bar (5 km on 30 km chart)
	{
		const float BarCm = ChartHalfCm > 600000.f ? 500000.f : 100000.f;
		const FVector2D A = WorldToChart(FVector(-ChartHalfCm + 60000.f, -ChartHalfCm + 60000.f, 0)), B = WorldToChart(FVector(-ChartHalfCm + 60000.f, -ChartHalfCm + 60000.f + BarCm, 0));
		H->DrawLine(A.X, A.Y, B.X, B.Y, Brass, 3.f);
		H->DrawText(ChartHalfCm > 600000.f ? TEXT("5 km") : TEXT("1 km"), Brass, A.X, A.Y - 22.f * S, Font, 0.8f * S);
	}
	auto Circle = [&](const FVector& C, float R, const FLinearColor& Col, float Th, bool bDash)
	{
		const int32 N = 40;
		for (int32 k = 0; k < N; ++k)
		{
			if (bDash && (k & 1)) continue;
			const FVector2D A = WorldToChart(Polar(C, 360.f * k / N, R)), B = WorldToChart(Polar(C, 360.f * (k + 1) / N, R));
			H->DrawLine(A.X, A.Y, B.X, B.Y, Col, Th);
		}
	};
	auto Fill = [&](const FVector& C, float R, const FLinearColor& Col)
	{
		const FVector2D Cc = WorldToChart(C);
		const float Rp = R / ChartHalfCm * ChartHalfPx;
		for (float dy = -Rp; dy <= Rp; dy += 2.f) { const float hw = FMath::Sqrt(FMath::Max(Rp * Rp - dy * dy, 0.f)); H->DrawRect(Col, Cc.X - hw, Cc.Y + dy, 2.f * hw, 2.f); }
	};
	// Land / hazards
	if (Obstacles.Num() == 0) BuildObstacles();
	AIronwakeArchipelago* Arch = AIronwakeArchipelago::Get(GetWorld());
	if (Arch) Arch->DrawChartLand(H, Cv, [this](const FVector& W) { return WorldToChart(W); }, S, Font);
	for (const FObs& O : Obstacles)
	{
		if (O.Name == TEXT("channel buoy")) continue;
		if (Arch && Arch->Isles.ContainsByPredicate([&](const FIsle& I) { return I.Name == O.Name; })) { Circle(O.C, O.R, Haz, 1.f, true); continue; }
		const bool bWall = O.Name.Contains(TEXT("breakwater")) || O.Name.Contains(TEXT("pier"));
		Fill(O.C, bWall ? O.R * 0.55f : O.R * 0.62f, Land);
		Circle(O.C, O.R, Haz, 1.f, true);
	}
	// Buoys
	for (int32 i = 0; i < Buoys.Num(); ++i) if (Buoys[i])
	{
		const FVector2D P = WorldToChart(Buoys[i]->GetComponentLocation());
		H->DrawRect(i % 2 == 0 ? FLinearColor(0.9f, 0.15f, 0.1f, 1.f) : FLinearColor(0.2f, 0.85f, 0.3f, 1.f), P.X - 3.f * S, P.Y - 3.f * S, 6.f * S, 6.f * S);
	}
	// Merchants + P29 3-Port Sea-Lane Triangle & NPC Trade Contacts
	if (WorldPorts.Num() >= 3)
	{
		const FLinearColor LaneCol(0.55f, 0.78f, 0.86f, 0.42f), PortCol(1.f, 0.86f, 0.42f, 0.95f);
		for (int32 p = 0; p < WorldPorts.Num(); ++p)
		{
			const FVector A0 = WorldPorts[p].OffingWorldCm;
			const FVector B0 = WorldPorts[(p + 1) % WorldPorts.Num()].OffingWorldCm;
			TArray<FVector> TradeLeg;
			PlanLeg(A0, B0, TradeLeg, 0, nullptr);
			FVector PrevPt = A0;
			for (const FVector& NextPt : TradeLeg)
			{
				const int32 Segs = FMath::Clamp(FMath::RoundToInt(FVector::Dist2D(PrevPt, NextPt) / 22000.f), 2, 28);
				for (int32 s = 0; s < Segs; s += 2)
				{
					const FVector2D Sa = WorldToChart(FMath::Lerp(PrevPt, NextPt, (float)s / Segs));
					const FVector2D Sb = WorldToChart(FMath::Lerp(PrevPt, NextPt, (float)(s + 1) / Segs));
					H->DrawLine(Sa.X, Sa.Y, Sb.X, Sb.Y, LaneCol, 1.6f);
				}
				PrevPt = NextPt;
			}
			Circle(WorldPorts[p].MoorWorldCm, WorldPorts[p].MoorRadiusM * 100.f, PortCol, 2.2f, false);
		}
	}
	if (AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(GetWorld()))
	{
		for (const FNpcRec& Rc : Nd->Recs)
		{
			if (Rc.bAway || (Rc.Def && Rc.Def->bAir)) continue;
			if (Rc.Role != ENpcRole::Merchant && Rc.Role != ENpcRole::Ferry) continue;
			const FVector2D P = WorldToChart(Rc.Pos);
			H->DrawRect(FLinearColor(0.82f, 0.76f, 0.52f, 0.92f), P.X - 3.5f * S, P.Y - 3.5f * S, 7.f * S, 7.f * S);
		}
	}
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld()))
	{
		for (AIronwakeMerchant* M : LS->Merchants) if (IsValid(M))
		{
			const FVector2D P = WorldToChart(M->GetActorLocation());
			H->DrawRect(FLinearColor(0.75f, 0.77f, 0.80f, 0.95f), P.X - 4.f * S, P.Y - 4.f * S, 8.f * S, 8.f * S);
			H->DrawText(M->ShipName, Dim, P.X + 7.f * S, P.Y - 7.f * S, Font, 0.6f * S);
		}
	}
	// Planned route (with detours) and waypoints — drawn BEFORE POI pill badges so lines never cross over text!
	TArray<FVector> Path; TArray<FString> Det;
	const FVector Orig = Ship->GetActorLocation();
	if (ApMode == 2 && Waypoints.Num() > 0 && LegPath.IsValidIndex(LegIdx))
	{
		for (int32 i = LegIdx; i < LegPath.Num(); ++i) Path.Add(LegPath[i]);
		FVector Prev = Waypoints[CurWp];
		for (int32 i = CurWp + 1; i < Waypoints.Num(); ++i) { PlanLeg(Prev, Waypoints[i], Path, 0, &Det); Prev = Waypoints[i]; }
		if (bApLoop && Waypoints.Num() > 1) PlanLeg(Prev, Waypoints[0], Path, 0, &Det);
	}
	else BuildPreview(Orig, Path, &Det);
	FVector2D Prev = WorldToChart(Orig);
	for (const FVector& Pt : Path)
	{
		const FVector2D Q = WorldToChart(Pt);
		H->DrawLine(Prev.X, Prev.Y, Q.X, Q.Y, FLinearColor(1.f, 0.82f, 0.4f, 0.9f), 2.f);
		Prev = Q;
	}
	for (int32 i = 0; i < Waypoints.Num(); ++i)
	{
		const FVector2D P = WorldToChart(Waypoints[i]);
		const FLinearColor Col = (ApMode == 2 && i == CurWp) ? Hi : Brass;
		Circle(Waypoints[i], 9000.f, Col, 2.5f, false);
		H->DrawText(FString::FromInt(i + 1), Col, P.X + 9.f * S, P.Y - 22.f * S, Font, 1.0f * S);
	}
	// Points of interest: show all 6 working ports, major lighthouses, and wreck sites cleanly on the 30 km chart without inner-islet clutter!
	for (int32 i = 0; i < POIs.Num(); ++i)
	{
		const FVector2D P = WorldToChart(POIs[i].Loc);
		H->DrawRect(POIs[i].bVisited ? Dim : Hi, P.X - 4.5f * S, P.Y - 4.5f * S, 9.f * S, 9.f * S);
		const FString& Tg = POIs[i].Tag;
		const bool bMajorLabel = (Tg == TEXT("H") || Tg == TEXT("Ic") || Tg == TEXT("St") || Tg == TEXT("Kc") || Tg == TEXT("Ng") || Tg == TEXT("Sh")
			|| Tg == TEXT("L") || Tg == TEXT("Sl") || Tg == TEXT("Br") || Tg == TEXT("W") || Tg == TEXT("K") || Tg == TEXT("F"));
		if (!bMajorLabel && ChartHalfCm > 600000.f) continue;
		float Tw, Th; H->GetTextSize(POIs[i].Name, Tw, Th, Font, 0.72f * S);
		float Ox = 9.f * S, Oy = 6.f * S;
		if      (Tg == TEXT("H"))  { Ox = 14.f * S; Oy = -24.f * S; }
		else if (Tg == TEXT("Ic")) { Ox = 14.f * S; Oy = -22.f * S; }
		else if (Tg == TEXT("St")) { Ox = 12.f * S; Oy = 8.f * S; }
		else if (Tg == TEXT("Kc")) { Ox = -Tw - 14.f * S; Oy = -10.f * S; }
		else if (Tg == TEXT("Ng")) { Ox = -Tw - 14.f * S; Oy = 8.f * S; }
		else if (Tg == TEXT("Sh")) { Ox = 12.f * S; Oy = -12.f * S; }
		else if (Tg == TEXT("L"))  { Ox = 12.f * S; Oy = -20.f * S; }
		H->DrawRect(FLinearColor(0.01f, 0.02f, 0.03f, 0.94f), P.X + Ox - 4.f * S, P.Y + Oy - 2.f * S, Tw + 8.f * S, Th + 4.f * S);
		H->DrawText(POIs[i].Name, Brass, P.X + Ox, P.Y + Oy, Font, 0.72f * S);
	}
	{
		const FVector2D P = WorldToChart(AnchorPt);
		H->DrawText(TEXT("(anchorage)"), Dim, P.X + 10.f * S, P.Y + 12.f * S, Font, 0.6f * S);
		H->DrawLine(P.X - 4.f * S, P.Y, P.X + 4.f * S, P.Y, Dim, 2.f); H->DrawLine(P.X, P.Y - 5.f * S, P.X, P.Y + 5.f * S, Dim, 2.f);
	}
	// Ship
	{
		const FVector2D P = WorldToChart(Orig);
		const float Hd = FMath::DegreesToRadians(Ship->GetActorRotation().Yaw);
		const FVector2D F(FMath::Sin(Hd), -FMath::Cos(Hd)), Rt(-F.Y, F.X);
		const FVector2D Tip = P + F * 13.f * S, L = P - F * 8.f * S + Rt * 7.f * S, R = P - F * 8.f * S - Rt * 7.f * S;
		H->DrawLine(Tip.X, Tip.Y, L.X, L.Y, Hi, 2.5f); H->DrawLine(L.X, L.Y, R.X, R.Y, Hi, 2.5f); H->DrawLine(R.X, R.Y, Tip.X, Tip.Y, Hi, 2.5f);
		H->DrawText(TEXT("YOU"), Hi, P.X + 12.f * S, P.Y + 4.f * S, Font, 0.7f * S);
	}
	// Legend panel
	float LX = X0 + Sz + 30.f * S, LY = Y0;
	auto Txt = [&](const FString& T, const FLinearColor& Col, float Sc) { H->DrawText(T, Col, LX, LY, Font, Sc * S); LY += 26.f * Sc * S; };
	Txt(TEXT("CHART — 6-PORT ARCHIPELAGO"), Brass, 1.45f);
	Txt(TEXT("Greyhaven • Ironcrag • Stigsby • Kestrel • Grimscar • Sable"), Dim, 0.76f);
	LY += 10.f * S;
	Txt(FString::Printf(TEXT("AUTOPILOT: %s"), *AutopilotLabel()), Hi, 0.9f);
	float LenCm = 0.f; { FVector A = Orig; for (const FVector& Pt : Path) { LenCm += FVector::Dist2D(A, Pt); A = Pt; } }
	const float Kn = FMath::Max(Ship->GetSpeedKnots(), 5.f);
	const float Mps = Kn * 0.5144f * Ship->SpeedCompression;
	Txt(FString::Printf(TEXT("Route %.1f km   ~%.0f min at %.0f kn%s"), LenCm / 100000.f, LenCm / 100.f / FMath::Max(Mps, 0.1f) / 60.f, Kn, bFastSail ? TEXT(" (fast sail)") : TEXT("")), Dim, 0.8f);
	Txt(FString::Printf(TEXT("Waypoints %d   LOOP %s"), Waypoints.Num(), bApLoop ? TEXT("ON") : TEXT("OFF")), Dim, 0.8f);
	if (Det.Num())
	{
		TArray<FString> Uniq;
		for (const FString& D : Det) if (D != TEXT("channel buoy")) Uniq.AddUnique(D);
		FString DetLine;
		int32 Shown = 0;
		for (const FString& D : Uniq)
		{
			const FString Cand = DetLine.IsEmpty() ? D : (DetLine + TEXT(", ") + D);
			if (Cand.Len() > 28 && Shown > 0) break;
			DetLine = Cand;
			++Shown;
		}
		if (Shown < Uniq.Num()) DetLine += FString::Printf(TEXT(" +%d more"), Uniq.Num() - Shown);
		Txt(FString::Printf(TEXT("Detours: %s"), *DetLine), FLinearColor(0.9f, 0.55f, 0.4f, 1.f), 0.75f);
	}
	LY += 14.f * S;
	for (const TCHAR* Line : { TEXT("Left-click    add waypoint"), TEXT("Right-click   remove nearest"), TEXT("Enter / P     start the route"), TEXT("L             loop on / off"),
		TEXT("Backspace     clear route"), TEXT("M             close chart"), TEXT("W / S         speed (kept by the autopilot)") })
		Txt(Line, Dim, 0.8f);
	LY += 14.f * S;
	Txt(TEXT("Hazards (dashed) are given a wide berth;"), Dim, 0.7f);
	Txt(TEXT("the route bends around them automatically."), Dim, 0.7f);
}

// ============================================================================ A1 test director
void AIronwakeVoyage::A1Log(const TCHAR* What)
{
	AIronwakeShipPawn* P = Player();
	if (!P) return;
	UE_LOG(LogIronwake, Display, TEXT("IWA1 %s t=%.1f pos=(%.0f,%.0f) hdg=%.1f kn=%.1f tel=%s rud=%.1f ap=%s err=%.2f wp=%d/%d leg=%d/%d tour=%d.%d giveway=%d fast=%d photo=%d dil=%.2f"), What, A1T,
		P->GetActorLocation().X / 100.f, P->GetActorLocation().Y / 100.f, P->GetHeadingDeg(), P->GetSpeedKnots(), *P->GetTelegraphName(), P->RudderAngleDeg, *AutopilotLabel(), ApErr,
		CurWp + 1, Waypoints.Num(), LegIdx, LegPath.Num(), TourStop, TourPhase, GiveWayCount, bFastSail, bPhoto, UGameplayStatics::GetGlobalTimeDilation(this));
}

void AIronwakeVoyage::TickA1Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	const float Dt = W->GetDeltaSeconds();
	A1T += RealDt;
	const float Since = A1T - A1Mark;
	auto Go = [&](int32 S) { A1Step = S; A1Mark = A1T; };
	auto Quit = [&]()
	{
		UE_LOG(LogIronwake, Display, TEXT("IWA1 test done"));
		A1Mode.Empty();
		if (FParse::Param(FCommandLine::Get(), TEXT("iwa1quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
	};
	auto Shot = [&](const TCHAR* N) { C->IWShot(N); A1Log(*FString::Printf(TEXT("shot %s"), N)); };
	A1LogAcc += RealDt;
	if (A1LogAcc >= 10.f) { A1LogAcc = 0.f; A1Log(TEXT("tel")); }
	if (A1Step == 0 && A1T >= 4.f)
	{
		// Unattended test: the -game window takes keyboard focus, so stray typing on the Mac must not steer the ship.
		if (ULocalPlayer* LP = C->GetLocalPlayer()) if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Sub->ClearAllMappings();
		if (!C->bStarted) C->StartGame(A1Mode != TEXT("fire"));
		P->ResetCameraToHorizon();
	}

	if (A1Mode == TEXT("hold"))
	{
		switch (A1Step)
		{
		case 0:
			if (A1T >= 4.f)
			{
				IronwakeSea::SetSeaScale(W, 1.6f); P->IWTelegraph(TelHalf); P->IWRudder(0);
				A1Overshoot = P->GetActorRotation().Yaw; A1ErrMax = 0.f; A1Log(TEXT("baseline: HALF AHEAD, wheel amidships, no autopilot (storm sea x1.6)")); Go(100);
			}
			break;
		case 100:
			A1ErrMax = FMath::Max(A1ErrMax, FMath::Abs(P->WaveYawRate));
			if (Since >= 45.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT baseline 45 s unsteered: heading drift %+.1f deg, peak wave yaw rate %.2f deg/s"), FMath::FindDeltaAngleDegrees(A1Overshoot, P->GetActorRotation().Yaw), A1ErrMax);
				P->IWTelegraph(TelStop); // so the autopilot rings its own speed
				EngageAutopilot(1); A1Log(TEXT("hold engaged")); Go(1);
			}
			break;
		case 1: if (Since >= 25.f) { A1ErrSq = 0.0; A1ErrMax = 0.f; A1N = 0; A1Settle = 0.f; A1Log(TEXT("hold measure start")); Go(2); } break;
		case 2:
		{
			const float E = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, HoldYaw);
			A1ErrSq += E * E; A1ErrMax = FMath::Max(A1ErrMax, FMath::Abs(E)); ++A1N;
			A1Settle = FMath::Max(A1Settle, FMath::Abs(P->WaveYawRate));
			if (Since >= 120.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT hold 120s: n=%d rms=%.2f deg max=%.2f deg course=%.1f kn=%.1f seaScale=%.2f roll=%.2f pitch=%.2f peakWaveYaw=%.2f deg/s -> %s"), A1N, FMath::Sqrt(A1ErrSq / FMath::Max(A1N, 1)), A1ErrMax,
					Norm360(HoldYaw), P->GetSpeedKnots(), IronwakeSea::GetSeaScale(), P->GetActorRotation().Roll, P->GetActorRotation().Pitch, A1Settle, A1ErrMax < 3.f ? TEXT("PASS") : TEXT("FAIL"));
				Go(3);
			}
			break;
		}
		case 3:
			if (Since >= 0.5f && !(A1ShotMask & 1)) { A1ShotMask |= 1; Shot(TEXT("A1_autopilot_hud")); }
			if (Since >= 2.f) { A1Overshoot = HoldYaw; A1Settle = -1.f; A1ErrMax = 0.f; P->TestRudderInput(1.f); Go(4); } // press D
			break;
		case 4:
			if (Since >= 0.25f && FMath::Abs(P->GetRudderInputValue()) > 0.f) P->TestRudderInput(0.f);
			{
				const float E = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, HoldYaw);
				if (A1Settle < 0.f && Since > 1.f && FMath::Abs(E) < 1.f) A1Settle = Since;
				if (Since > 1.f) A1ErrMax = FMath::Max(A1ErrMax, -E); // overshoot past the new course
			}
			if (Since >= 40.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT nudge D: course %.1f -> %.1f (delta %+.1f) settled(<1deg) in %.1f s, still %s"), Norm360(A1Overshoot), Norm360(HoldYaw),
					FMath::FindDeltaAngleDegrees(A1Overshoot, HoldYaw), A1Settle, *AutopilotLabel());
				P->StepTelegraph(+1); // W press
				Go(5);
			}
			break;
		case 5:
			if (Since >= 3.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT W press: tel=%s autopilot=%s ownsTel=%d"), *P->GetTelegraphName(), *AutopilotLabel(), bApOwnsTel);
				// Give way: put a merchant 350 m ahead, crossing.
				if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(W)) if (LS->Merchants.Num() > 0 && IsValid(LS->Merchants[0]))
				{
					AIronwakeMerchant* M = LS->Merchants[0];
					const FVector Fwd = P->GetActorForwardVector().GetSafeNormal2D();
					const FVector L = P->GetActorLocation() + Fwd * 35000.f;
					M->Yaw = P->GetActorRotation().Yaw + 90.f;
					M->SetActorLocation(FVector(L.X, L.Y, M->GetActorLocation().Z));
					A1Log(*FString::Printf(TEXT("placed %s 350 m ahead"), *M->ShipName));
				}
				Go(6);
			}
			break;
		case 6:
			if (Since >= 25.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT giveway: events=%d last=%s ap=%s tel=%s"), GiveWayCount, *GiveWayName, *AutopilotLabel(), *P->GetTelegraphName());
				Go(7);
			}
			break;
		case 7:
		{
			// Key P cycle + A/D in TOUR.
			FString Seq;
			CycleAutopilot(); Seq += AutopilotLabel() + TEXT(" | chart=") + (bChart ? TEXT("1") : TEXT("0"));
			ToggleChart();
			CycleAutopilot(); Seq += TEXT(" -> ") + AutopilotLabel();
			CycleAutopilot(); Seq += TEXT(" -> ") + AutopilotLabel();
			CycleAutopilot(); Seq += TEXT(" -> ") + AutopilotLabel();
			UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT P cycle (from HOLD): %s"), *Seq);
			Go(8);
			break;
		}
		case 8:
			if (Since >= 2.f) { EngageAutopilot(3); Go(9); }
			break;
		case 9:
			if (Since >= 2.f) { P->TestRudderInput(-1.f); Go(10); }
			break;
		case 10:
			if (Since >= 0.3f)
			{
				P->TestRudderInput(0.f);
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT A in TOUR: autopilot=%s toast='%s'"), *AutopilotLabel(), *Toast);
				Go(11);
			}
			break;
		case 11: if (Since >= 2.f) Quit(); break;
		default: break;
		}
	}
	else if (A1Mode == TEXT("route"))
	{
		switch (A1Step)
		{
		case 0: if (A1T >= 4.f) { ToggleChart(); Go(1); } break;
		case 1:
			if (Since >= 1.5f)
			{
				const FVector Wps[] = { FVector(20000.f, -160000.f, 0.f), FVector(200000.f, -20000.f, 0.f), FVector(60000.f, 60000.f, 0.f) };
				for (const FVector& Wp : Wps) ChartClick(WorldToChart(Wp), false);
				ChartClick(WorldToChart(FVector(-200000.f, 120000.f, 0.f)), false); // extra one ...
				ChartClick(WorldToChart(FVector(-200000.f, 120000.f, 0.f)), true);  // ... removed with a right-click
				TArray<FVector> Path; TArray<FString> Det;
				BuildPreview(P->GetActorLocation(), Path, &Det);
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT chart: %d waypoints, planned path %d points, detours: %s"), Waypoints.Num(), Path.Num(), *FString::Join(Det, TEXT(", ")));
				Go(2);
			}
			break;
		case 2: if (Since >= 1.5f) { Shot(TEXT("A1_chart")); Go(3); } break;
		case 3: if (Since >= 1.f) { ChartStart(); ToggleFastSail(); A1Log(TEXT("route started (fast sail)")); Go(4); } break;
		case 4:
			if (Since >= 30.f && !(A1ShotMask & 2)) { A1ShotMask |= 2; Shot(TEXT("A1_route_hud")); }
			if (Since >= 50.f && !(A1ShotMask & 4)) { A1ShotMask |= 4; TogglePhoto(); A1Log(TEXT("photo ON during route")); }
			if (Since >= 70.f && !(A1ShotMask & 8)) { A1ShotMask |= 8; A1Log(TEXT("photo still ON")); TogglePhoto(); }
			if (bRouteDone)
			{
				FString Mins;
				for (int32 i = 0; i < Obstacles.Num(); ++i) if (ObsMinDist[i] < 60000.f && !Mins.Contains(Obstacles[i].Name)) Mins += FString::Printf(TEXT("%s %.0f m; "), *Obstacles[i].Name, ObsMinDist[i] / 100.f);
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT route: completed in %.0f s, end %.0f m from WP3, kn=%.1f, closest approach beyond hazard radius: %s"), Since,
					FVector::Dist2D(P->GetActorLocation(), FVector(60000.f, 60000.f, 0.f)) / 100.f, P->GetSpeedKnots(), *Mins);
				Shot(TEXT("A1_route_end"));
				Go(5);
			}
			if (Since > 1500.f) { A1Log(TEXT("RESULT route TIMEOUT")); Go(5); }
			break;
		case 5: if (Since >= 3.f) Quit(); break;
		default: break;
		}
	}
	else if (A1Mode == TEXT("voyage"))
	{
		// P3: sail to every place (4 core + 14 islands), nearest-neighbour order, hull-side photo at each arrival.
		static TArray<FVector> VPoiLoc; static TArray<FString> VPoiName; static int32 VLastWp = -1, VPhotoState = 0; static float PhotoT = 0.f, VFrameAcc = 0.f, VFpsMin = 1e9f; static int32 VFrameN = 0; static double VTotalM = 0.0; static FVector VLastLoc = FVector::ZeroVector;
		if (A1Step == 0 && A1T >= 5.f)
		{
			if (Obstacles.Num() == 0) BuildObstacles();
			TArray<FIronwakePOI> Left = POIs; FVector VCur = P->GetActorLocation(); VCur.Z = 0.f;
			Waypoints.Reset();
			while (Left.Num() > 0)
			{
				int32 Best = 0; float BD = 1e12f;
				for (int32 i = 0; i < Left.Num(); ++i) { const float D = FVector::Dist2D(VCur, Left[i].Loc); if (D < BD) { BD = D; Best = i; } }
				const FIronwakePOI Q = Left[Best]; Left.RemoveAt(Best);
				float Hz = 20000.f; for (const FObs& O : Obstacles) if (FVector::Dist2D(O.C, Q.Loc) < 100.f) Hz = FMath::Max(Hz, O.R);
				const FVector W2 = Q.Loc + (VCur - Q.Loc).GetSafeNormal2D() * (Hz + 12000.f);
				Waypoints.Add(FVector(W2.X, W2.Y, 0.f)); VCur = Waypoints.Last(); VPoiLoc.Add(Q.Loc); VPoiName.Add(Q.Name);
				A1Log(*FString::Printf(TEXT("voyage plan %d: %s hazard=%.0f m wp=(%.0f,%.0f)"), Waypoints.Num(), *Q.Name, Hz / 100.f, W2.X / 100.f, W2.Y / 100.f));
			}
			P->IWSteam(1); P->IWTelegraph(5);
			bApLoop = false; ChartStart(); ToggleFastSail(); VLastWp = 0; VLastLoc = P->GetActorLocation();
			A1Log(TEXT("voyage started")); Go(1);
		}
		else if (A1Step == 1)
		{
			VFrameAcc += RealDt; ++VFrameN; if (RealDt > 0.f) VFpsMin = FMath::Min(VFpsMin, 1.f / RealDt);
			VTotalM += FVector::Dist2D(P->GetActorLocation(), VLastLoc) / 100.0; VLastLoc = P->GetActorLocation();
			if (VPhotoState == 0 && CurWp != VLastWp)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP3 reached wp %d/%d, leg fps avg=%.1f min=%.1f frames=%d dist_total=%.1f km"), VLastWp + 1, Waypoints.Num(), VFrameN / FMath::Max(VFrameAcc, 0.01f), VFpsMin, VFrameN, VTotalM / 1000.0);
				// hull-side view: 60 m off the starboard side, looking at the hull's mid-length, level camera
				const float L = P->HullLengthM;
				if (!bPhoto) TogglePhoto();
				const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
				const FVector Eye = X.TransformPosition(FVector(L * 20.f, L * 55.f, 700.f)), Look = X.TransformPosition(FVector(-L * 10.f, 0.f, 500.f));
				SetPhotoView(Eye, (Look - Eye).Rotation());
				VPhotoState = 1; PhotoT = A1T; VFrameAcc = 0.f; VFrameN = 0; VFpsMin = 1e9f;
			}
			if (VPhotoState == 1 && A1T - PhotoT > 1.5f) { Shot(*FString::Printf(TEXT("P3_voyage_%02d_hull"), VLastWp + 1)); VPhotoState = 2; PhotoT = A1T; }
			if (VPhotoState == 2 && A1T - PhotoT > 1.5f)
			{
				if (bPhoto) TogglePhoto();
				if (VPoiLoc.IsValidIndex(VLastWp)) { const FVector D = VPoiLoc[VLastWp] - P->GetActorLocation(); C->SetControlRotation(FRotator(-5.f, FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)), 0.f)); }
				VPhotoState = 3; PhotoT = A1T;
			}
			if (VPhotoState == 3 && A1T - PhotoT > 1.5f)
			{
				FString Nm = VPoiName.IsValidIndex(VLastWp) ? VPoiName[VLastWp] : FString(TEXT("x"));
				Nm = Nm.Replace(TEXT(" "), TEXT("")).Replace(TEXT("'"), TEXT("")).Replace(TEXT("("), TEXT("")).Replace(TEXT(")"), TEXT(""));
				Shot(*FString::Printf(TEXT("P3_voyage_%02d_%s"), VLastWp + 1, *Nm)); VPhotoState = 4; PhotoT = A1T;
			}
			if (VPhotoState == 4 && A1T - PhotoT > 1.5f) { P->ResetCameraToHorizon(); VPhotoState = 0; VLastWp = CurWp; }
			if (bRouteDone && VPhotoState == 0)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP3 RESULT voyage complete: %d waypoints, %.1f km sailed, t=%.0f s"), Waypoints.Num(), VTotalM / 1000.0, A1T);
				Go(2);
			}
			if (Since > 3000.f) { UE_LOG(LogIronwake, Display, TEXT("IWP3 RESULT voyage TIMEOUT at wp %d/%d"), CurWp + 1, Waypoints.Num()); Go(2); }
		}
		else if (A1Step == 2 && Since >= 2.f) Quit();
	}
	else if (A1Mode == TEXT("tour"))
	{
		switch (A1Step)
		{
		case 0: if (A1T >= 4.f) { EngageAutopilot(3); ToggleFastSail(); A1Log(TEXT("tour started (fast sail)")); Go(1); } break;
		case 1:
		{
			static int32 LastStop = -1, LastPhase = -1;
			if (TourStop != LastStop || TourPhase != LastPhase)
			{
				LastStop = TourStop; LastPhase = TourPhase;
				A1Log(TEXT("tour phase"));
				if (TourPhase == 1) Shot(*FString::Printf(TEXT("A1_tour_%d"), TourStop));
			}
			if (bTourDone) { A1Log(TEXT("tour complete")); Go(2); }
			if (Since > 2400.f) { A1Log(TEXT("RESULT tour TIMEOUT")); Go(3); }
			break;
		}
		case 2:
			if (Since >= 12.f)
			{
				FString Vis;
				for (const FIronwakePOI& Pt : POIs) Vis += FString::Printf(TEXT("%s=%d; "), *Pt.Name, Pt.bVisited);
				FString Mins;
				for (int32 i = 0; i < Obstacles.Num(); ++i) if (ObsMinDist[i] < 60000.f && !Mins.Contains(Obstacles[i].Name)) Mins += FString::Printf(TEXT("%s %.0f m; "), *Obstacles[i].Name, ObsMinDist[i] / 100.f);
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT tour: %s anchored=%d kn=%.2f %.0f m from anchorage, %.0f m from Greyhaven, t=%.0f s; closest: %s"), *Vis, bAnchored, P->GetSpeedKnots(),
					FVector::Dist2D(P->GetActorLocation(), AnchorPt) / 100.f, FVector::Dist2D(P->GetActorLocation(), HarbourC) / 100.f, A1T, *Mins);
				Shot(TEXT("A1_tour_anchored"));
				Go(3);
			}
			break;
		case 3: if (Since >= 3.f) Quit(); break;
		default: break;
		}
	}
	else if (A1Mode == TEXT("waterline"))
	{
		// Chuck: 'the ship looks like it's sinking'. 30 s calm (overcast sea, HALF) then 30 s storm (x1.6, FULL):
		// boot-top clearance over the rendered sea at stem/mid/stern stations, and forecastle awash time.
		auto Measure = [&]()
		{
			float MinC = 1e9f, MidC = 0.f;
			for (int32 k = 0; k <= 4; ++k)
			{
				const float XM = FMath::Lerp(P->GetHullX0() + 5.f, P->GetHullX1() - 5.f, k / 4.f);
				const FVector BT = P->HullMesh->GetComponentTransform().TransformPosition(FVector(XM * 100.f, 0.f, 0.f) / P->HullMesh->GetComponentScale());
				const float C2 = (BT.Z - IronwakeSea::RenderedZ(W, BT)) / 100.f;
				MinC = FMath::Min(MinC, C2); if (k == 2) MidC = C2;
			}
			A1ErrMax = FMath::Min(A1ErrMax, MinC); A1ErrSq += MidC; ++A1N;
			if (P->GetBowSubmergeM() > 0.f) A1Settle += Dt;
			A1Overshoot = FMath::Max(A1Overshoot, P->GetBowSubmergeM());
		};
		auto Reset = [&]() { A1ErrMax = 1e9f; A1ErrSq = 0.0; A1N = 0; A1Settle = 0.f; A1Overshoot = -99.f; };
		auto Report = [&](const TCHAR* Band)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT waterline %s: mean boot-top clearance midships %.2f m, min over stem..stern %.2f m, forecastle awash %.1f s of 30, max bow submerge %.2f m, pitch %.2f -> %s"),
				Band, A1ErrSq / FMath::Max(A1N, 1), A1ErrMax, A1Settle, A1Overshoot, P->GetActorRotation().Pitch, (FCString::Strcmp(Band, TEXT("calm")) != 0 || A1Settle <= 0.f) ? TEXT("PASS") : TEXT("FAIL"));
		};
		auto SideCam = [&]()
		{
			const FVector R = P->GetActorRightVector().GetSafeNormal2D();
			C->SetControlRotation(FRotator(-3.f, (-R).Rotation().Yaw + 20.f, 0.f));
		};
		switch (A1Step)
		{
		case 0: if (A1T >= 4.f) { IronwakeSea::SetSeaScale(W, 1.f); P->IWTelegraph(TelHalf); P->ZoomIndex = 1; Go(1); } break;
		case 1: SideCam(); if (Since >= 20.f) { Reset(); Go(2); } break;
		case 2: SideCam(); Measure(); if (Since >= 15.f && !(A1ShotMask & 1)) { A1ShotMask |= 1; Shot(TEXT("A1_waterline_calm")); } if (Since >= 30.f) { Report(TEXT("calm")); IronwakeSea::SetSeaScale(W, 1.6f); P->IWTelegraph(TelFull); Go(3); } break;
		case 3: SideCam(); if (Since >= 20.f) { Reset(); Go(4); } break;
		case 4: SideCam(); Measure(); if (Since >= 15.f && !(A1ShotMask & 2)) { A1ShotMask |= 2; Shot(TEXT("A1_waterline_storm")); } if (Since >= 30.f) { Report(TEXT("storm")); Go(5); } break;
		case 5: if (Since >= 3.f) Quit(); break;
		default: break;
		}
	}
	else if (A1Mode == TEXT("fire"))
	{
		switch (A1Step)
		{
		case 0:
			if (A1T >= 4.f)
			{
				EngageAutopilot(1);
				if (AIronwakeShipPawn* E = P->FindEnemy()) { E->bGunsFree = true; A1Log(*FString::Printf(TEXT("enemy at %.0f m, guns free"), FVector::Dist2D(E->GetActorLocation(), P->GetActorLocation()) / 100.f)); }
				else A1Log(TEXT("NO ENEMY"));
				Go(1);
			}
			break;
		case 1:
			if (ApMode == 0)
			{
				AIronwakeShipPawn* E = P->FindEnemy();
				UE_LOG(LogIronwake, Display, TEXT("IWA1 RESULT fire: autopilot disengaged after %.1f s, enemy shots=%d, toast='%s'"), Since, E ? E->ShellsFiredCount() : -1, *Toast);
				Shot(TEXT("A1_under_fire"));
				Go(2);
			}
			if (Since > 240.f) { A1Log(TEXT("RESULT fire TIMEOUT (no enemy fire)")); Go(2); }
			break;
		case 2: if (Since >= 3.f) Quit(); break;
		default: break;
		}
	}
	(void)Dt;
}

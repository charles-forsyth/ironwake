// P28 silent offscreen verification director: -iwp28=shots [-iwp28quit] [-iwoarlog] [-iwproplog].
// Batches all P28 multi-frame motion proofs (0.15 s simulation-time spacing) + town/sign/4K-ground/quay/helmsman captures into ONE launch.
#include "IronwakeVoyage.h"
#include "IronwakeShell.h"
#include "IronwakeNpc.h"
#include "IronwakeStorm.h"
#include "IronwakeHydro.h"
#include "IronwakeFleet.h"
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

void AIronwakeVoyage::TickP28Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(W);
	P28T += RealDt;
	const float NowSim = (float)W->GetTimeSeconds();
	const float Since = P28T - P28Mark;
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp28quit"));

	auto Next = [&]()
	{
		++P28Step;
		P28Mark = P28T;
		P28Sub = 0;
		P28LastShotT = -10.f;
	};
	auto Shot = [&](const FString& Name)
	{
		const FString Full = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png")));
		FScreenshotRequest::RequestScreenshot(Full, false, false);
		P28LastShotT = NowSim;
		UE_LOG(LogIronwake, Display, TEXT("IWP28 shot %s simT=%.3f realT=%.3f"), *Name, NowSim, P28T);
	};
	auto ReadyForNextShot = [&](float MinSimGap) -> bool
	{
		return !FScreenshotRequest::IsScreenshotRequested() && (P28LastShotT < 0.f || (NowSim - P28LastShotT) >= MinSimGap - 0.004f);
	};
	auto View = [&](const FVector& Eye, const FVector& Look, float Fov)
	{
		if (!bPhoto)
		{
			TogglePhoto();
			UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		}
		if (UGameplayStatics::GetGlobalTimeDilation(this) < 0.99f)
			UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		SetPhotoView(Eye, (Look - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};

	if (!Nd)
	{
		if (P28T > 35.f) { UE_LOG(LogIronwake, Warning, TEXT("IWP28 no NPC director")); P28Mode.Empty(); }
		return;
	}
	auto RecByDef = [&](FName DefId) -> FNpcRec* { return Nd->FindByDef(DefId, 0); };
	const FTransform HfT(FRotator(0.f, 45.f, 0.f), FVector(HarbourC.X, HarbourC.Y, AIronwakeShell::WaterZAt(W, HarbourC) + AIronwakeShell::RenderedSeaOffsetCm));
	auto HPos = [&](float Xm, float Ym, float Zm) -> FVector { return HfT.TransformPosition(FVector(Xm * 100.f, Ym * 100.f, Zm * 100.f)); };

	if (P28Mode == TEXT("p34"))
	{
		// P34 Verification Suite:
		//  Step 0: EXACT player spawn (-3000, -2700 m) looking back SW toward Greyhaven harbour (3 sequential frames 0.25s apart to prove ZERO water flickering & ZERO grey haze!) + look NE across clear archipelago!
		//  Step 1: Progressive wake growth from standstill (t=2s small eddy, t=10s medium wake, t=24s full 12+L dispersive Kelvin wake fading gradually outward — NO instant giant triangle!).
		//  Step 2: Close bow wave & hull-side waterline view at 13.5 kn (clean white Voronoi peel & streaming lace, NO grey detached collar slab or dark seam!).
		//  Step 3: Wake ship-rocking test: steam player Ironclad past stationary NPC Chapman Sloop 32 m abeam and log/capture NPC roll response!
		switch (P28Step)
		{
		case 0:
		{
			if (P28T < 3.0f) return;
			if (!C->bStarted) C->StartGame(true);
			C->ConsoleCommand(TEXT("r.DynamicRes.OperationMode 0"));
			C->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
			// Place at exact spawn (-300000, -270000) heading 45 deg at FULL STOP (Telegraph 2)
			S2Place(FVector(-300000.f, -270000.f, 150.f), 45.f, false);
			P->IWTelegraph(2);
			Next();
			return;
		}
		case 1: // Look-back toward Greyhaven harbour from start spawn (3 frames 0.25s apart -> pixel-diff proves 0 flicker!) + Look-ahead NE
		{
			const FVector Loc = P->GetActorLocation();
			if (P28Sub < 3)
			{
				const FVector Eye = Loc + FVector(3200.f, 3200.f, 1850.f);
				const FVector Look = FVector(HarbourC.X, HarbourC.Y, 600.f);
				View(Eye, Look, 58.f);
				if (Since > 3.5f && ReadyForNextShot(0.25f))
				{
					Shot(FString::Printf(TEXT("P34_start_lookback_%d"), P28Sub));
					++P28Sub;
				}
			}
			else if (P28Sub == 3)
			{
				const FVector Eye = Loc - FVector(2800.f, 2800.f, -1950.f);
				const FVector Look = Loc + FVector(180000.f, 160000.f, 1200.f);
				View(Eye, Look, 58.f);
				if (Since > 5.0f && ReadyForNextShot(0.4f))
				{
					Shot(TEXT("P34_start_clear_horizon"));
					Next();
				}
			}
			return;
		}
		case 2: // Progressive wake growth from standstill (0 kn -> FULL AHEAD: capture at 2.5s, 9.5s, 22.0s)
		{
			if (P28Sub == 0)
			{
				if (bPhoto) TogglePhoto();
				S2Place(FVector(-120000.f, -120000.f, 150.f), 45.f, false);
				if (P->HullBody) P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
				P->bAuxSteam = true;
				P->IWTelegraph(2);
				P28Sub = 1; P28Mark = P28T; return;
			}
			if (P28Sub == 1 && Since > 1.2f)
			{
				P->IWTelegraph(6); // Order Flank Ahead from standstill!
				P28Sub = 2; P28Mark = P28T; return;
			}
			const FVector Loc = P->GetActorLocation();
			const FVector Fw = P->GetActorForwardVector(), Rt = P->GetActorRightVector();
			// High chase-quarter camera showing stern + 600 m wake trail behind ship
			View(Loc - Fw * 15500.f + Rt * 6500.f + FVector(0.f, 0.f, 7800.f), Loc - Fw * 6500.f + FVector(0.f, 0.f, 100.f), 58.f);
			if (P28Sub == 2 && Since >= 2.5f && ReadyForNextShot(0.3f))
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP34 WAKE_GROW t=2.5s kn=%.2f"), P->GetSpeedKnots());
				Shot(TEXT("P34_wake_grow_02s"));
				P28Sub = 3;
			}
			else if (P28Sub == 3 && Since >= 9.5f && ReadyForNextShot(0.3f))
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP34 WAKE_GROW t=9.5s kn=%.2f"), P->GetSpeedKnots());
				Shot(TEXT("P34_wake_grow_10s"));
				P28Sub = 4;
			}
			else if (P28Sub == 4 && Since >= 23.0f && ReadyForNextShot(0.3f))
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP34 WAKE_GROW t=23.0s kn=%.2f"), P->GetSpeedKnots());
				Shot(TEXT("P34_wake_grow_23s"));
				Next();
			}
			return;
		}
		case 3: // Close Bow Wave & Hull-Side Waterline Flow at 13.5+ kn
		{
			const FVector Loc = P->GetActorLocation();
			const FVector Fw = P->GetActorForwardVector(), Rt = P->GetActorRightVector();
			const float X1 = P->GetHullX1() * 100.f;
			if (P28Sub == 0)
			{
				View(Loc + Fw * (X1 + 2900.f) + Rt * 2400.f + FVector(0.f, 0.f, 680.f), Loc + Fw * (X1 - 950.f) + FVector(0.f, 0.f, 120.f), 52.f);
				if (Since >= 2.0f && ReadyForNextShot(0.3f)) { Shot(TEXT("P34_bow_wave_close")); P28Sub = 1; P28Mark = P28T; }
			}
			else if (P28Sub == 1)
			{
				View(Loc + Fw * 600.f + Rt * 3600.f + FVector(0.f, 0.f, 760.f), Loc - Fw * 400.f + FVector(0.f, 0.f, 90.f), 54.f);
				if (Since >= 2.0f && ReadyForNextShot(0.3f)) { Shot(TEXT("P34_hull_waterline_side")); Next(); }
			}
			return;
		}
		case 4: // Wake-to-Ship Rocking Proof: Steam Ironclad at 14 kn 34 m past stationary NPC Cutter in deep open sea and measure peak roll!
		{
			FNpcRec* R = RecByDef(TEXT("cutter"));
			if (!R) R = RecByDef(TEXT("schooner"));
			if (!R && Nd->Recs.Num() > 0) R = &Nd->Recs[0];
			static float PeakNpcRoll = 0.f;
			if (P28Sub == 0)
			{
				PeakNpcRoll = 0.f;
				C->bPeaceful = true;
				for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
				P->HP = P->MaxHP;
				const FVector Base(140000.f, -240000.f, 0.f); // Deep open sea 1.4 km E, 2.4 km S of origin — zero islands or reefs!
				S2Place(Base, 0.f, true);
				P->bAuxSteam = true; P->IWTelegraph(6);
				if (R)
				{
					Nd->TeleportRec(*R, Base + FVector(16500.f, 3400.f, 0.f), 0.f, ENpcState::Work, 0.f);
					R->bHold = true; R->SpeedKn = 0.f; R->DesKn = 0.f;
					Nd->SpawnBody(*R);
				}
				P28Sub = 1; P28Mark = P28T; return;
			}
			AIronwakeNpcShip* B = R ? R->Body.Get() : nullptr;
			if (B)
			{
				PeakNpcRoll = FMath::Max(PeakNpcRoll, FMath::Abs(B->RollDeg));
				const FVector NLoc(B->Pos2.X, B->Pos2.Y, B->HeaveZ);
				View(NLoc + FVector(-5200.f, 4200.f, 1450.f), NLoc + FVector(400.f, -800.f, 220.f), 54.f);
			}
			if (Since >= 16.5f && P28Sub == 1 && ReadyForNextShot(0.3f))
			{
				Shot(TEXT("P34_wake_rocks_npc"));
				P28Sub = 2;
			}
			if (Since >= 20.0f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP34 WAKE_ROCK peak_npc_roll_deg=%.2f %s"), PeakNpcRoll, PeakNpcRoll >= 0.8f ? TEXT("PASS") : TEXT("LOW"));
				P28Mode.Empty();
				if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
			}
			return;
		}
		}
		return;
	}

	if (P28Mode == TEXT("p35cam"))
	{
		// P35 Fast-Camera-Whip Verification:
		// Tests REAL gameplay camera (PlayerCameraManager + CalcCamera, bPhoto=false) whipping at 360 deg/sec across Yaw=135 deg vs stationary at Yaw=135 deg!
		switch (P28Step)
		{
		case 0:
			if (P28T < 3.0f) return;
			if (!C->bStarted) C->StartGame(true);
			C->bPeaceful = true;
			for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
			C->ConsoleCommand(TEXT("r.DynamicRes.OperationMode 0"));
			C->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
			if (bPhoto) TogglePhoto();
			S2Place(FVector(140000.f, -240000.f, 0.f), 0.f, true);
			P->bAuxSteam = true; P->IWTelegraph(6); P->ZoomIndex = 2;
			C->SetControlRotation(FRotator(-16.f, 15.f, 0.f));
			Next();
			return;
		case 1: // Settle full 14 kn wake for 6.0 s, then whip camera at 360 deg/s from Yaw=15 -> 135 deg and snap mid-whip!
			if (Since < 6.0f)
			{
				C->SetControlRotation(FRotator(-16.f, 15.f, 0.f));
			}
			else
			{
				const float WhipT = Since - 6.0f;
				const float CurYaw = 15.f + WhipT * 360.f; // 360 deg/sec fast camera whip!
				C->SetControlRotation(FRotator(-16.f, CurYaw, 0.f));
				if (CurYaw >= 135.f && P28Sub == 0 && !FScreenshotRequest::IsScreenshotRequested())
				{
					UE_LOG(LogIronwake, Display, TEXT("IWP35 CAM_WHIP_FAST yaw=%.1f cam=%s ship=%s"), CurYaw, *C->PlayerCameraManager->GetCameraLocation().ToString(), *P->GetActorLocation().ToString());
					Shot(TEXT("P35_camwhip_fast"));
					P28Sub = 1;
				}
				if (CurYaw >= 210.f) Next();
			}
			return;
		case 2: // Now hold camera completely STILL at the exact same Yaw=135 deg for 1.5 s and snap reference!
			C->SetControlRotation(FRotator(-16.f, 135.f, 0.f));
			if (Since >= 1.5f && P28Sub == 0 && !FScreenshotRequest::IsScreenshotRequested())
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP35 CAM_WHIP_STILL yaw=135.0 cam=%s ship=%s"), *C->PlayerCameraManager->GetCameraLocation().ToString(), *P->GetActorLocation().ToString());
				Shot(TEXT("P35_camwhip_still"));
				P28Sub = 1;
			}
			if (Since >= 2.6f)
			{
				P28Mode.Empty();
				if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
			}
			return;
		}
		return;
	}

	switch (P28Step)
	{
	case 0: // Initialize calm sunny late-morning sea, unlock & spawn all live NPCs
	{
		if (P28T < 3.0f) return;
		if (!C->bStarted) C->StartGame(true);
		for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(10.8f); ForceWeatherState(0, 0.05f);
		if (AIronwakeStorm* St = AIronwakeStorm::Get(W)) { St->SetTarget(0.f, 0.05f); St->SetOpenImmediate(0.f); }
		IronwakeSea::SetSeaScale(W, 0.18f);
		IronwakeFleet::SetFixedWind(315.f, 14.f);
		Nd->ForceLive(40); Nd->MarkAllKnown();
		S2Place(Nd->H2W(880.f, -420.f), 135.f, false);
		P->IWRudder(2);
		Next();
		return;
	}
	case 1: // Settle textures/Nanite/TSR for 5 s
		if (Since > 5.0f) Next();
		return;

	case 2: // (a) PaddleSteamerV3 spinning sponson wheel + water churn + P27 Kelvin wake: 4 frames 0.15 s apart + wide steaming 3/4
	{
		FNpcRec* R = RecByDef(TEXT("paddle"));
		if (!R) { Next(); return; }
		if (P28Sub == 0)
		{
			Nd->TeleportRec(*R, Nd->H2W(540.f, -80.f), 135.f, ENpcState::Transit, 7.8f);
			R->Route = { Nd->H2W(540.f, 1800.f) }; R->RouteIdx = 0; R->bHold = false;
			R->DesYaw = 135.f; R->DesKn = 7.8f;
			Nd->SpawnBody(*R);
			if (AIronwakeNpcShip* B = R->Body.Get()) { B->EngineKn = 7.8f; B->V = 7.8f * 51.4444f * 1.5f; B->SetFx(true); }
			P28Sub = 1; P28Mark = P28T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get();
		if (!B) { Nd->SpawnBody(*R); return; }
		R->bHold = false; R->State = ENpcState::Transit; R->DesKn = 7.8f; R->DesYaw = 135.f;
		B->EngineKn = 7.8f; B->V = FMath::Max(B->V, 7.6f * 51.4444f * 1.5f);
		if (!B->bFx) B->SetFx(true);
		const FVector ShipBase(B->Pos2.X, B->Pos2.Y, B->HeaveZ);
		const FVector Fw = FRotator(0.f, B->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
		if (P28Sub <= 4)
		{
			const FVector WheelLook = ShipBase - Fw * 135.f + Rt * 500.f + FVector(0.f, 0.f, 155.f);
			const FVector Eye = ShipBase - Fw * 1650.f + Rt * 2050.f + FVector(0.f, 0.f, 340.f);
			View(Eye, WheelLook, 45.f);
			if (Since > 3.5f && ReadyForNextShot(0.15f))
			{
				float WheelAng = 0.f, WheelRate = 0.f;
				for (const FNpcPart& Pt : B->Parts) if (Pt.Kind == 1 && Pt.Idx == 1) { WheelAng = Pt.Angle; WheelRate = Pt.Rate; }
				UE_LOG(LogIronwake, Display, TEXT("IWP28 paddle frame=%d wheel_deg=%.1f rev_s=%.3f kn=%.2f"), P28Sub, WheelAng, WheelRate, R->SpeedKn);
				Shot(FString::Printf(TEXT("P28_paddle_%d"), P28Sub));
				++P28Sub;
			}
		}
		else if (P28Sub == 5)
		{
			// Steaming 3/4 stern-starboard view showing PaddleSteamerV3 0.35 m boot-top waterline + P27 Kelvin wake + wheel foam + jackstaff ensign
			View(ShipBase - Fw * 3100.f + Rt * 2950.f + FVector(0.f, 0.f, 920.f), ShipBase - Fw * 200.f + FVector(0.f, 0.f, 340.f), 48.f);
			if (Since > 5.0f && ReadyForNextShot(0.5f)) { Shot(TEXT("P28_paddle_wide")); Next(); }
		}
		return;
	}

	case 3: // (b) SalvageTenderV3 slewing A-frame crane & pendulum diving bell: 4 frames 0.15 s apart + opposite slew extremum
	{
		FNpcRec* R = RecByDef(TEXT("tender"));
		if (!R) { Next(); return; }
		if (P28Sub == 0)
		{
			Nd->TeleportRec(*R, Nd->H2W(540.f, 160.f), 195.f, ENpcState::Work, 0.f);
			R->bHold = true; R->SpeedKn = 0.f; R->DesKn = 0.f;
			Nd->SpawnBody(*R);
			if (AIronwakeNpcShip* B = R->Body.Get())
			{
				// Start at -22 deg port slew so frames 1..3 sweep across centerline and frame 5 lands at +25 deg starboard slew!
				for (FNpcPart& Pt : B->Parts) if (Pt.Kind == 6 || Pt.Kind == 7) Pt.Phase = -NowSim * 1.15f - 4.45f;
			}
			P28Sub = 1; P28Mark = P28T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get();
		if (!B) { Nd->SpawnBody(*R); return; }
		const FVector ShipBase(B->Pos2.X, B->Pos2.Y, B->HeaveZ);
		const FVector Fw = FRotator(0.f, B->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
		const FVector Eye = ShipBase - Fw * 1950.f - Rt * 1550.f + FVector(0.f, 0.f, 960.f);
		const FVector Look = ShipBase - Fw * 980.f + FVector(0.f, 0.f, 440.f);
		View(Eye, Look, 48.f);
		if (Since > 3.0f && P28Sub >= 1 && P28Sub <= 4 && ReadyForNextShot(0.15f))
		{
			float SlewDeg = 0.f;
			for (const FNpcPart& Pt : B->Parts) if (Pt.Kind == 6) SlewDeg = Pt.Angle;
			UE_LOG(LogIronwake, Display, TEXT("IWP28 tender frame=%d crane_slew_deg=%.2f"), P28Sub, SlewDeg);
			Shot(FString::Printf(TEXT("P28_tender_%d"), P28Sub));
			++P28Sub;
		}
		if (P28Sub == 5 && ReadyForNextShot(1.65f))
		{
			float SlewDeg = 0.f;
			for (const FNpcPart& Pt : B->Parts) if (Pt.Kind == 6) SlewDeg = Pt.Angle;
			UE_LOG(LogIronwake, Display, TEXT("IWP28 tender wide_slew crane_slew_deg=%.2f"), SlewDeg);
			Shot(TEXT("P28_tender_slew_wide"));
			Next();
		}
		return;
	}

	case 4: // (c) PatrolAirshipV4 clean bladeless pods + 4-blade props + translucent motion disc (>1.7 rev/s): 4 frames 0.15 s apart + all-3-props wide view
	{
		FNpcRec* R = RecByDef(TEXT("patrol_airship"));
		if (!R) { Next(); return; }
		if (P28Sub == 0)
		{
			Nd->TeleportRec(*R, Nd->H2W(320.f, 40.f), 145.f, ENpcState::Transit, 20.f);
			R->DesKn = 20.f; R->DesYaw = 145.f;
			Nd->SpawnBody(*R);
			if (AIronwakeNpcShip* B = R->Body.Get()) B->EngineKn = 20.f;
			P28Sub = 1; P28Mark = P28T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get();
		if (!B) { Nd->SpawnBody(*R); return; }
		B->EngineKn = 20.f; R->DesKn = 20.f; R->DesYaw = 145.f;
		const FTransform Ht = B->HullPivot->GetComponentTransform();
		if (P28Sub <= 4)
		{
			const FVector PropStbd = Ht.TransformPosition(FVector(1303.6f, 1575.93f, 992.27f));
			const FVector Eye = Ht.TransformPosition(FVector(2950.f, 3250.f, 1420.f));
			const FVector Look = PropStbd + Ht.TransformVectorNoScale(FVector(-220.f, -380.f, 180.f));
			View(Eye, Look, 46.f);
			if (Since > 2.8f && ReadyForNextShot(0.15f))
			{
				for (const FNpcPart& Pt : B->Parts)
					if (Pt.Kind == 0) UE_LOG(LogIronwake, Display, TEXT("IWP28 airship frame=%d prop=%d rev_s=%.2f disc=%d deg=%.1f"), P28Sub, Pt.Idx, Pt.Rate, Pt.bDisc ? 1 : 0, Pt.Angle);
				Shot(FString::Printf(TEXT("P28_airship_%d"), P28Sub));
				++P28Sub;
			}
		}
		else if (P28Sub == 5)
		{
			// Wider aft-starboard quarter view showing ALL 3 propellers (port pod, stbd pod, stern pusher) at once!
			const FVector Eye = Ht.TransformPosition(FVector(-5800.f, 4400.f, 820.f));
			const FVector Look = Ht.TransformPosition(FVector(-650.f, 0.f, 2250.f));
			View(Eye, Look, 54.f);
			if (ReadyForNextShot(0.6f)) { Shot(TEXT("P28_airship_stern")); Next(); }
		}
		return;
	}

	case 5: // (d) Rowing gig with SailorCrewRower crew & clamped oar stroke: 4 frames 0.15 s apart + 4 quarter-cycle stroke phases (0.375 s apart)
	{
		FNpcRec* R = RecByDef(TEXT("gig"));
		if (!R) { Next(); return; }
		if (P28Sub == 0)
		{
			Nd->TeleportRec(*R, Nd->H2W(420.f, -120.f), 140.f, ENpcState::Transit, 2.9f);
			R->Route = { Nd->H2W(420.f, 800.f) }; R->RouteIdx = 0; R->DesYaw = 140.f; R->DesKn = 2.9f; R->bHold = false;
			Nd->SpawnBody(*R);
			if (AIronwakeNpcShip* B = R->Body.Get())
			{
				B->EngineKn = 2.9f; B->V = 2.9f * 51.4444f * 1.5f;
				for (FNpcPart& Pt : B->Parts) if (Pt.Kind == 4 || Pt.Kind == 5) Pt.Angle = 0.02f;
			}
			P28Sub = 1; P28Mark = P28T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get();
		if (!B) { Nd->SpawnBody(*R); return; }
		R->State = ENpcState::Transit; R->DesKn = 2.9f; R->DesYaw = 140.f; R->bHold = false; B->EngineKn = 2.9f; B->V = FMath::Max(B->V, 185.f);
		const FVector ShipBase(B->Pos2.X, B->Pos2.Y, B->HeaveZ);
		const FVector Fw = FRotator(0.f, B->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
		const FVector Eye = ShipBase + Rt * 680.f + Fw * 420.f + FVector(0.f, 0.f, 340.f);
		const FVector Look = ShipBase - Fw * 30.f + FVector(0.f, 0.f, 75.f);
		View(Eye, Look, 45.f);
		if (Since > 2.6f && P28Sub >= 1 && P28Sub <= 4 && ReadyForNextShot(0.15f))
		{
			Shot(FString::Printf(TEXT("P28_gig_%d"), P28Sub));
			++P28Sub;
		}
		else if (P28Sub >= 5 && P28Sub <= 8 && ReadyForNextShot(0.375f))
		{
			Shot(FString::Printf(TEXT("P28_gig_phase_%d"), P28Sub - 4));
			++P28Sub;
		}
		else if (P28Sub == 9) Next();
		return;
	}

	case 6: // SeaGullV4 2-part hinged wing flap/glide: 4 frames 0.15 s apart
	{
		if (Gulls.Num() == 0 || !Gulls[0]) { Next(); return; }
		AIronwakeGull* G = Gulls[0];
		if (P28Sub == 0)
		{
			G->Follow = nullptr;
			G->Centre = Nd->H2W(300.f, 20.f);
			G->RadiusCm = 4500.f;
			G->AltCm = 3200.f;
			G->FlapHz = 2.75f;
			G->Phase = 1.0f - FMath::Fmod(G->T, 5.2f);
			P28Sub = 1; P28Mark = P28T; return;
		}
		const FVector Gl = G->GetActorLocation();
		const FVector Fw = G->GetActorForwardVector(), Rt = G->GetActorRightVector();
		View(Gl + Fw * 210.f + Rt * 290.f + FVector(0.f, 0.f, 85.f), Gl + FVector(0.f, 0.f, 14.f), 42.f);
		if (Since > 1.8f && P28Sub >= 1 && P28Sub <= 4 && ReadyForNextShot(0.15f))
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP28 gull frame=%d wingL_roll=%.1f wingR_roll=%.1f"),
				P28Sub, G->PivotL->GetRelativeRotation().Roll, G->PivotR->GetRelativeRotation().Roll);
			Shot(FString::Printf(TEXT("P28_gull_%d"), P28Sub));
			++P28Sub;
		}
		if (P28Sub == 5)
		{
			// Berth TallBrig (N2, Y=46) cleanly with taut bow hawsers onto the N2 bollards (keep N1 clear so zero off-screen hull rope enters lower-left!)
			if (FNpcRec* N1 = Nd->FindByDef(TEXT("goliath"), 0))
			{
				Nd->TeleportRec(*N1, Nd->H2W(-25.0f, -89.f), 225.f, ENpcState::Docked, 0.f);
				N1->bHold = true; N1->NextActionT = 9999.f; Nd->SpawnBody(*N1);
			}
			if (FNpcRec* N2 = Nd->FindByDef(TEXT("brig"), 1))
			{
				Nd->TeleportRec(*N2, Nd->H2W(-28.5f, 46.f), 225.f, ENpcState::Docked, 0.f);
				N2->bHold = true; N2->NextActionT = 9999.f; Nd->SpawnBody(*N2);
			}
			Next();
		}
		return;
	}

	case 7: // (e) Street/tavern/chandlery signs, 1.5 m -25 deg 4K ground micro-detail, occupied quay north berth, helmsman & 400 m town
	{
		// Note: Quay apron surface is at Z = Q + 0.18 m = 3.98 m!
		// P28_ground_4k: 1.50 m eye height above cobble street (Z = 5.48 m) pitched -25 deg down toward tavern cobbles & threshold (dx=6.0m, dz=-2.80m -> -25.0 deg)!
		struct FStill { const TCHAR* Name; FVector EyeM; FVector LookM; float Fov; bool bPlayerHelm; };
		static const FStill Stills[] = {
			{ TEXT("P28_street_tavern"),    FVector(-66.5f,  30.5f,  5.55f), FVector(-83.0f,  30.5f,  6.80f), 60.f, false },
			{ TEXT("P28_street_chandlery"), FVector(-65.5f, -26.3f,  5.55f), FVector(-83.0f, -26.3f,  6.85f), 60.f, false },
			{ TEXT("P28_ground_4k"),        FVector(-71.2f,  30.2f,  5.48f), FVector(-77.2f,  30.5f,  2.68f), 62.f, false },
			{ TEXT("P28_quay_south"),       FVector(-56.5f, -76.0f,  6.10f), FVector(-46.0f, -104.f,  5.10f), 62.f, false },
			{ TEXT("P28_quay_north"),       FVector(-55.8f,   7.5f,  6.20f), FVector(-44.0f,  40.0f,  4.95f), 62.f, false },
			{ TEXT("P28_town_noon_400"),    FVector(400.0f,   0.0f,  8.00f), FVector(-140.f,  20.0f, 16.00f), 60.f, false },
			{ TEXT("P28_helmsman"),         FVector::ZeroVector,             FVector::ZeroVector,             48.f, true  },
		};
		constexpr int32 NStills = UE_ARRAY_COUNT(Stills);
		if (P28Sub >= NStills) { Next(); return; }
		const FStill& S = Stills[P28Sub];
		if (S.bPlayerHelm && P->HelmsmanPivot)
		{
			const FTransform HmT = P->HelmsmanPivot->GetComponentTransform();
			const FVector Base = HmT.GetLocation();
			const FVector Fw = P->GetActorForwardVector(), Rt = P->GetActorRightVector();
			View(Base - Fw * 390.f + Rt * 280.f + FVector(0.f, 0.f, 155.f), Base + Fw * 24.f + FVector(0.f, 0.f, 82.f), S.Fov);
		}
		else
		{
			View(HPos(S.EyeM.X, S.EyeM.Y, S.EyeM.Z), HPos(S.LookM.X, S.LookM.Y, S.LookM.Z), S.Fov);
		}
		if (Since > 3.2f && !FScreenshotRequest::IsScreenshotRequested())
		{
			Shot(S.Name);
			++P28Sub;
			P28Mark = P28T;
		}
		return;
	}

	default:
		UE_LOG(LogIronwake, Display, TEXT("IWP28 DONE moving_parts=%d"), Nd->CountMovingParts() + AIronwakeNpcDirector::Windmills.Num());
		P28Mode.Empty();
		if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		return;
	}
}

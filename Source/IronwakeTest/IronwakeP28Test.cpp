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

// P21 test director: -iwp21=shots [-iwp21quit]. Silent -game capture of the living harbour: aerial overview, merchant arriving + mooring (5 frames),
// patrol cutter passing, airship props (frames 0.3 s apart), windmill sails, rowing gig (frames 0.3 s apart). Shots in Saved/shots/P21_*.png
#include "IronwakeVoyage.h"
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

void AIronwakeVoyage::TickP21Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(W);
	P21T += RealDt;
	const float Since = P21T - P21Mark;
	auto Next = [&]() { ++P21Step; P21Mark = P21T; P21Sub = 0; };
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp21quit"));
	auto Shot = [&](const FString& N) { FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (N + TEXT(".png"))), false, false); UE_LOG(LogIronwake, Display, TEXT("IWP21 shot %s"), *N); };
	auto View = [&](const FVector& Eye, const FVector& Look, float Fov)
	{
		if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		SetPhotoView(Eye, (Look - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};
	if (!Nd) { if (P21T > 40.f) { UE_LOG(LogIronwake, Display, TEXT("IWP21 no director")); P21Mode.Empty(); } return; }
	auto Rec = [&](const TCHAR* Name) -> FNpcRec* { for (FNpcRec& R : Nd->Recs) if (R.Name == Name) return &R; return nullptr; };
	auto Body = [&](const TCHAR* Name) -> AIronwakeNpcShip* { FNpcRec* R = Rec(Name); return R ? R->Body.Get() : nullptr; };
	switch (P21Step)
	{
	case 0:
		if (P21T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
		bWxDrift = false; ClockSpeedIdx = 0; SetClockHour(11.f); ForceWeatherState(0, 0.05f);
		if (AIronwakeStorm* St = AIronwakeStorm::Get(W)) { St->SetTarget(0.f, 0.05f); St->SetOpenImmediate(0.f); }
		IronwakeSea::SetSeaScale(W, 0.25f);
		IronwakeFleet::SetFixedWind(340.f, 14.f);
		Nd->ForceLive(40); Nd->MarkAllKnown();
		S2Place(FVector(-50000.f, -300000.f, 0.f), -120.f, false);
		Next(); { int32 St = 0; if (FParse::Value(FCommandLine::Get(), TEXT("iwp21step="), St) && St > 0) P21Step = St; } return;
	case 1:   // settle: bodies spawn, ships get under way
		if (Since > 25.f) Next(); return;
	case 2:   // P25 aerial harbour shots: oblique seaward aerial (step 2: roads + mouth + quays + town) and inner basin overlook (step 3)
	case 3:
	{
		TArray<FVector> Pts; int32 Moving = 0, Air = 0;
		const FVector Mouth = Nd->H2W(160.f, 0.f);
		for (FNpcRec& R : Nd->Recs) if (AIronwakeNpcShip* B = R.Body.Get()) if (FVector::Dist2D(B->GetActorLocation(), Mouth) < 230000.f) { if (R.Def->bAir) { ++Air; continue; } Pts.Add(B->GetActorLocation()); if (R.SpeedKn > 0.8f) ++Moving; }
		if (Since > 3.f && P21Step == 2) for (FNpcRec& R : Nd->Recs) if (AIronwakeNpcShip* B = R.Body.Get()) { const FVector2D H = Nd->W2H(B->GetActorLocation()); UE_LOG(LogIronwake, Display, TEXT("IWP22 ship %s %s h=(%.0f,%.0f) kn=%.1f state=%s"), *R.Name, *R.Def->Id.ToString(), H.X, H.Y, R.SpeedKn, NpcStateName(R.State)); }
		const FVector Eye = P21Step == 2
			? (Nd->H2W(620.f, -230.f) + FVector(0.f, 0.f, 19500.f))
			: (Nd->H2W(265.f, 145.f) + FVector(0.f, 0.f, 10800.f));
		const FVector Look = P21Step == 2
			? (Nd->H2W(45.f, 20.f) + FVector(0.f, 0.f, 1100.f))
			: (Nd->H2W(-25.f, -20.f) + FVector(0.f, 0.f, 600.f));
		const float Fov = P21Step == 2 ? 60.f : 58.f;
		View(Eye, Look, Fov);
		if (Since > 3.f)
		{
			int32 Vis = 0; FVector2D Sp; for (const FVector& Q : Pts) if (C->ProjectWorldLocationToScreen(Q, Sp, false) && Sp.X > 0 && Sp.Y > 0) ++Vis;
			UE_LOG(LogIronwake, Display, TEXT("IWP22 aerial step=%d centre ships=%d moving=%d airships=%d projected_on_screen=%d height=%.0f m radius=2300 m"), P21Step, Pts.Num(), Moving, Air, Vis, Eye.Z / 100.f);
			Shot(P21Step == 2 ? TEXT("P21_aerial_a") : TEXT("P21_aerial_b")); Next();
		}
		return;
	}
	case 4:   // merchant arrives and moors: teleport SS Calloway to the waiting point, send it home to its berth, 5 frames
	{
		FNpcRec* R = Rec(TEXT("SS Calloway"));
		if (!R) { Next(); return; }
		if (P21Sub == 0)
		{
			Nd->TeleportRec(*R, Nd->H2W(215.f, 22.f), 225.f, ENpcState::Transit, 4.f);
			Nd->TestRoute(*R, true); P21Sub = 1; P21Mark = P21T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get();
		if (!B) return;
		const FVector Pc = B->GetActorLocation(); const FVector Rt = B->GetActorRightVector(), Fw = B->GetActorForwardVector();
		View(Pc + Rt * -7000.f + Fw * 3500.f + FVector(0, 0, 2800.f), Pc + FVector(0, 0, 600.f), 55.f);
		const float Gap = 14.f;
		const int32 Idx = P21Sub - 1;
		if (Idx < 5 && Since > Gap * (Idx + 1)) { Shot(FString::Printf(TEXT("P21_arrive_%d"), Idx + 1)); ++P21Sub; }
		if (P21Sub > 5 && Since > Gap * 5 + 3.f) { UE_LOG(LogIronwake, Display, TEXT("IWP21 merchant state=%s lane=%d"), NpcStateName(R->State), R->Lane); Next(); }
		return;
	}
	case 5:   // patrol cutter passing
	{
		FNpcRec* R = Rec(TEXT("HMRC Vigilant"));
		if (!R) { Next(); return; }
		if (P21Sub == 0) { Nd->TeleportRec(*R, Nd->H2W(300.f, -90.f), 180.f, ENpcState::Patrol, 8.f); R->Route.Reset(); R->RouteIdx = 0; P21Sub = 1; P21Mark = P21T; return; }
		AIronwakeNpcShip* B = R->Body.Get(); if (!B) return;
		const FVector Pc = B->GetActorLocation(); const FVector Rt = B->GetActorRightVector(), Fw = B->GetActorForwardVector();
		View(Pc + Rt * 6500.f + Fw * 2500.f + FVector(0, 0, 1800.f), Pc + FVector(0, 0, 500.f), 55.f);
		if (P21Sub == 1 && Since > 10.f) { Shot(TEXT("P21_cutter_1")); P21Sub = 2; }
		if (P21Sub == 2 && Since > 16.f) { Shot(TEXT("P21_cutter_2")); Next(); }
		return;
	}
	case 6:   // patrol airship, props spinning: 3 frames 0.3 s apart on the starboard side prop
	{
		FNpcRec* R = Rec(TEXT("Aurora Watch"));
		if (!R) { Next(); return; }
		if (P21Sub == 0) { Nd->TeleportRec(*R, Nd->H2W(0.f, 0.f) + FVector(0.f, 0.f, 0.f), 225.f, ENpcState::Transit, 16.f); P21Sub = 1; P21Mark = P21T; return; }
		AIronwakeNpcShip* B = R->Body.Get(); if (!B) return;
		const FVector Prop = B->GetActorTransform().TransformPosition(FVector(-590.f, 800.f, 1577.f) );
		const FVector Eye = B->GetActorTransform().TransformPosition(FVector(-590.f, 800.f + 4200.f, 1577.f + 900.f));
		View(Eye, Prop, 28.f);
		if (P21Sub >= 1 && P21Sub <= 3 && Since > 6.f + 0.1f * (P21Sub - 1)) { Shot(FString::Printf(TEXT("P21_airship_prop_%d"), P21Sub)); ++P21Sub; }
		if (P21Sub == 4 && Since > 9.f) Next();
		return;
	}
	case 7:   // the high loops: cargo airship + patrol airship in view
	{
		FNpcRec* R = Rec(TEXT("Long Haul"));
		if (!R) { Next(); return; }
		if (P21Sub == 0) { Nd->TeleportRec(*R, Nd->H2W(-60.f, 60.f), 225.f, ENpcState::Transit, 10.f); P21Sub = 1; P21Mark = P21T; return; }
		AIronwakeNpcShip* B = R->Body.Get(); if (!B) return;
		const FVector Pp = B->GetActorTransform().TransformPosition(FVector(-1274.f, 4198.f, 4209.f));
		const FVector Rt = B->GetActorRightVector();
		View(Pp + Rt * 4500.f + FVector(0, 0, 600.f), Pp, 45.f);
		if (P21Sub >= 1 && P21Sub <= 3 && Since > 6.f + 0.1f * (P21Sub - 1)) { Shot(FString::Printf(TEXT("P21_cargo_%d"), P21Sub)); ++P21Sub; }
		if (P21Sub == 4 && Since > 9.f) Next();
		return;
	}
	case 8:   // windmill sails: 3 frames 1.0 s apart
	{
		USceneComponent* Wm = AIronwakeNpcDirector::Windmills.Num() ? AIronwakeNpcDirector::Windmills[0].Get() : nullptr;
		if (!Wm) { UE_LOG(LogIronwake, Display, TEXT("IWP21 no windmill")); Next(); return; }
		const FVector Pv = Wm->GetComponentLocation(), Fw = Wm->GetForwardVector();
		View(Pv + Fw * 5200.f + FVector(0, 0, 0), Pv, 50.f);
		if (P21Sub < 3 && Since > 3.f + 1.0f * P21Sub) { Shot(FString::Printf(TEXT("P21_windmill_%d"), P21Sub + 1)); ++P21Sub; }
		if (P21Sub == 3 && Since > 7.f) Next();
		return;
	}
	case 9:   // rowing gig: underway, 4 frames 0.3 s apart from the beam
	{
		FNpcRec* R = Rec(TEXT("Wherry Anne"));
		if (!R) { Next(); return; }
		if (P21Sub == 0) { Nd->TeleportRec(*R, Nd->H2W(250.f, 80.f), 225.f, ENpcState::Transit, 2.5f); Nd->TestRoute(*R, true); P21Sub = 1; P21Mark = P21T; return; }
		AIronwakeNpcShip* B = R->Body.Get(); if (!B) return;
		const FVector Pc = B->HullPivot->GetComponentLocation() + FVector(0, 0, 150.f); const FVector Rt = B->GetActorRightVector();
		if (FParse::Param(FCommandLine::Get(), TEXT("iwgigtop"))) View(Pc + FVector(0, 0, 1400.f) + Rt * 120.f, Pc, 45.f); else View(Pc + Rt * 1400.f + FVector(0, 0, 150.f), Pc, 45.f);
		if (P21Sub == 1) UE_LOG(LogIronwake, Display, TEXT("IWP21 gig V=%.1f kn state=%s hold=%d desKn=%.1f eng=%.1f"), B->V / 77.2f, NpcStateName(R->State), R->bHold ? 1 : 0, R->DesKn, B->EngineKn);
		if (P21Sub >= 1 && P21Sub <= 4 && Since > 14.f + 0.3f * (P21Sub - 1)) { Shot(FString::Printf(TEXT("P21_gig_%d"), P21Sub)); ++P21Sub; }
		if (P21Sub == 5 && Since > 17.f) Next();
		return;
	}
	case 10: case 11:   // P22 wake continuity: HMRC Vigilant near the player at 8 kn (step 10) / 13 kn (step 11), 8 frames 0.5 s apart from astern-above
	{
		const float Kn = P21Step == 10 ? 8.f : 13.f;
		FNpcRec* R = Rec(TEXT("HMRC Vigilant"));
		if (!R) { Next(); return; }
		if (P21Sub == 0)
		{
			const FVector Pp = P->GetActorLocation();
			Nd->TeleportRec(*R, FVector(Pp.X + 40000.f, Pp.Y + 40000.f, 0.f), 225.f, ENpcState::Patrol, Kn); R->Route.Reset(); R->RouteIdx = 0; P21Sub = 1; P21Mark = P21T; return;
		}
		AIronwakeNpcShip* B = R->Body.Get(); if (!B) return;
		B->EngineKn = Kn; R->DesKn = Kn;
		const FVector Pc = B->GetActorLocation(); const FVector Rt = B->GetActorRightVector(), Fw = B->GetActorForwardVector();
		View(Pc - Fw * 4500.f + Rt * 2500.f + FVector(0, 0, 1800.f), Pc - Fw * 1500.f, 60.f);
		const int32 Idx = P21Sub - 1;
		if (Idx < 8 && Since > 25.f + 0.5f * Idx) { Shot(FString::Printf(TEXT("P22_wake_%02dkn_%d"), (int32)Kn, Idx + 1)); UE_LOG(LogIronwake, Display, TEXT("IWP22 wake kn=%.1f V=%.1f fx=%d dist=%.0f m"), Kn, B->V / 51.44f / 1.5f, B->bFx ? 1 : 0, FVector::Dist2D(Pc, P->GetActorLocation()) / 100.f); ++P21Sub; }
		if (P21Sub > 8 && Since > 31.f) Next();
		return;
	}
	default:
		UE_LOG(LogIronwake, Display, TEXT("IWP21 DONE moving_parts=%d"), Nd->CountMovingParts() + AIronwakeNpcDirector::Windmills.Num());
		P21Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		return;
	}
}

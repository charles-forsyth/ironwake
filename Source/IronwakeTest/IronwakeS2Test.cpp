// S2 test director: -iws2=<mode> [-iws2quit]   (standalone -game run of the editor build; see build/tools/s2run.sh)
//   sail     : frigate, fixed wind from N 16 kn. Steady speed on 6 points of sail (incl. in irons), aux steam in irons,
//              captures: reach (billowing sails), yards from above on two headings, HUD view.
//   handling : current ship (-iwship=...): stop -> FULL timing, hard-rudder turn rate / radius, captures (HUD + side view).
//   progress : reset progression, sail into each point of interest (real arrival code), shipyard capture, LAUNCH the
//              frigate through the shipyard (map reload), verify it spawned and the choice persisted.
#include "IronwakeVoyage.h"
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeGameMode.h"
#include "IronwakeLivingSea.h"
#include "IronwakeArchipelago.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "IronwakePlayerController.h"
#include "IronwakeShell.h"
#include "IronwakeHydro.h"
#include "Camera/PlayerCameraManager.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "IronwakeStorm.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

void AIronwakeVoyage::S2Place(const FVector& LocCm, float Yaw, bool bKeepSpeed)
{
	AIronwakeShipPawn* P = Player();
	if (!P) return;
	const FVector V = P->HullBody->GetPhysicsLinearVelocity();
	const float Spd = FVector::DotProduct(V, P->GetActorForwardVector().GetSafeNormal2D());
	P->SetActorLocationAndRotation(FVector(LocCm.X, LocCm.Y, P->GetActorLocation().Z), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	P->HullBody->SetPhysicsLinearVelocity(bKeepSpeed ? FRotator(0.f, Yaw, 0.f).Vector() * Spd : FVector::ZeroVector);
	P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	P->IWRudder(0);
}

void AIronwakeVoyage::S2Shot(const TCHAR* Name, bool bPhotoView)
{
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (FString(Name) + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(File, !bPhotoView, false);
	AIronwakeShipPawn* P = Player();
	UE_LOG(LogIronwake, Display, TEXT("IWS2 shot %s ship=%s hdg=%.0f kn=%.1f twa=%.0f yard=%.1f belly=%.2f m tel=%s"), *File, P ? *P->GetShipName() : TEXT("-"),
		P ? P->GetHeadingDeg() : 0.f, P ? P->GetSpeedKnots() : 0.f, P ? P->TrueWindAngle : 0.f, (P && P->SailRig) ? P->SailRig->GetYardYaw() : 0.f,
		(P && P->SailRig) ? P->SailRig->GetBellyM() : 0.f, P ? *P->GetTelegraphName() : TEXT("-"));
	++S2Shots;
}

void AIronwakeVoyage::TickS2Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	S2T += RealDt;
	// Unattended test: the -game window takes keyboard focus; stray typing on the Mac (Tab -> menu -> QUIT) must not
	// reach the game. Mappings are cleared every second (the menu/ship re-add theirs on restart/possess).
	if (FMath::Fmod(S2T, 1.f) < RealDt + 1e-3f)
		if (ULocalPlayer* LP = C->GetLocalPlayer()) if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Sub->ClearAllMappings();
	const float Since = S2T - S2Mark;
	auto Next = [&]() { ++S2Step; S2Mark = S2T; };
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iws2quit"));
	auto Finish = [&]()
	{
		UE_LOG(LogIronwake, Display, TEXT("IWS2 DONE mode=%s shots=%d"), *S2Mode, S2Shots);
		S2Mode.Empty();
		if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
	};
	// Open water 1.5 km SE of the wreck, inside the tessellated tile, nothing in the way for ~3 km.
	const FVector Sea(-50000.f, -300000.f, 0.f);
	auto PhotoAt = [&](const FVector& RelEye, const FVector& RelLook)
	{
		if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		const FVector Eye = X.TransformPosition(RelEye), Look = X.TransformPosition(RelLook);
		SetPhotoView(Eye, (Look - Eye).Rotation());
	};
	auto PhotoOff = [&]() { if (bPhoto) TogglePhoto(); };
	S2LogAcc += RealDt;
	if (S2LogAcc >= 2.f)
	{
		S2LogAcc = 0.f;
		UE_LOG(LogIronwake, Display, TEXT("IWS2 t=%.0f step=%d ship=%s hdg=%.0f kn=%.2f order=%.1f tel=%s twa=%.0f (%s) sailKn=%.1f fill=%.2f yard=%.1f steam=%d yawrate=%.2f"),
			S2T, S2Step, *P->GetShipName(), P->GetHeadingDeg(), P->GetSpeedKnots(), P->EngineOrderKnots, *P->GetTelegraphName(), P->TrueWindAngle,
			P->SailRig ? IronwakeFleet::PointOfSail(P->TrueWindAngle) : TEXT("-"), P->SailKn, P->SailFill, P->SailRig ? P->SailRig->GetYardYaw() : 0.f, P->bAuxSteam,
			P->HullBody->GetPhysicsAngularVelocityInDegrees().Z);
	}

	// ------------------------------------------------------------------------------------------------ sail
	if (S2Mode == TEXT("sail"))
	{
		// Wind from N (yaw 0) at 16 kn. Heading yaw = -TWA puts the wind TWA deg on the starboard side.
		struct FLeg { float Twa; const TCHAR* Tag; float Hold; };
		static const FLeg Legs[] = { { 120.f, TEXT("broad"), 45.f }, { 90.f, TEXT("beam"), 40.f }, { 55.f, TEXT("closehauled"), 40.f },
			{ 175.f, TEXT("running"), 40.f }, { 20.f, TEXT("irons"), 40.f } };
		const int32 NL = UE_ARRAY_COUNT(Legs);
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 16.f);
			S2Place(Sea, -Legs[0].Twa, false);
			P->IWTelegraph(6);
			UE_LOG(LogIronwake, Display, TEXT("IWS2 sail test: wind from 000 16 kn fixed, ship=%s"), *P->GetShipName());
			Next(); return;
		}
		if (S2Step >= 1 && S2Step <= NL)
		{
			const FLeg& L = Legs[S2Step - 1];
			// hold the heading exactly (keeps the measurement about the wind angle, not the helm)
			const float Yaw = -L.Twa;
			const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, Yaw);
			P->bAutoHelm = true; P->SetAutoRudder(FMath::Clamp(Err * 2.f, -P->MaxRudderDeg, P->MaxRudderDeg));
			if (L.Twa == 120.f && Since >= 30.f && S2Shots == 0) { PhotoAt(FVector(4800.f, -9500.f, 2200.f), FVector(-400.f, 0.f, 2600.f)); }
			if (L.Twa == 120.f && Since >= 31.5f && S2Shots == 0) S2Shot(TEXT("S2_frigate_reach_sails"), true);
			if (L.Twa == 120.f && Since >= 33.f && S2Shots == 1) { PhotoAt(FVector(-14000.f, 6000.f, 4200.f), FVector(1000.f, 0.f, 2400.f)); }
			if (L.Twa == 120.f && Since >= 34.5f && S2Shots == 1) S2Shot(TEXT("S2_frigate_reach_quarter"), true);
			if (L.Twa == 120.f && Since >= 36.f && S2Shots == 2) { PhotoAt(FVector(-2500.f, 0.f, 11000.f), FVector(-200.f, 0.f, 2000.f)); }
			if (L.Twa == 120.f && Since >= 37.5f && S2Shots == 2) S2Shot(TEXT("S2_frigate_yards_broadreach"), true);
			if (L.Twa == 120.f && Since >= 39.f && S2Shots == 3) { PhotoOff(); P->ResetCameraToHorizon(); C->SetControlRotation(FRotator(-9.f, P->GetActorRotation().Yaw + 140.f, 0.f)); }
			if (L.Twa == 120.f && Since >= 42.f && S2Shots == 3) S2Shot(TEXT("S2_frigate_hud"), false);
			if (L.Twa == 55.f && Since >= 34.f && S2Shots == 4) { PhotoAt(FVector(-2500.f, 0.f, 11000.f), FVector(-200.f, 0.f, 2000.f)); }
			if (L.Twa == 55.f && Since >= 35.5f && S2Shots == 4) S2Shot(TEXT("S2_frigate_yards_closehauled"), true);
			if (L.Twa == 55.f && Since >= 37.f && S2Shots == 5) { PhotoAt(FVector(5000.f, -9000.f, 2600.f), FVector(-400.f, 0.f, 2600.f)); }
			if (L.Twa == 55.f && Since >= 38.5f && S2Shots == 5) { S2Shot(TEXT("S2_frigate_closehauled_side"), true); }
			if (L.Twa == 175.f && Since >= 32.f && S2Shots == 6) { PhotoAt(FVector(-2500.f, 0.f, 11000.f), FVector(-200.f, 0.f, 2000.f)); }
			if (L.Twa == 175.f && Since >= 33.5f && S2Shots == 6) S2Shot(TEXT("S2_frigate_yards_running"), true);
			if (L.Twa == 20.f && Since >= 30.f && S2Shots == 7) { PhotoAt(FVector(5000.f, -9000.f, 2600.f), FVector(-400.f, 0.f, 2600.f)); }
			if (L.Twa == 20.f && Since >= 31.5f && S2Shots == 7) S2Shot(TEXT("S2_frigate_irons_luffing"), true);
			if (Since >= L.Hold)
			{
				PhotoOff();
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT point=%s twa=%.0f kn=%.2f sailKn=%.2f polar=%.2f yard=%.1f fill=%.2f heel=%.1f roll=%.1f leeway=%.1f"), L.Tag, P->TrueWindAngle, P->GetSpeedKnots(), P->SailKn,
					IronwakeFleet::Polar(P->TrueWindAngle), P->SailRig ? P->SailRig->GetYardYaw() : 0.f, P->SailFill, P->SailHeelDeg, P->GetActorRotation().Roll,
					FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(P->HullBody->GetPhysicsLinearVelocity(), P->GetActorRightVector()), FMath::Max(FVector::DotProduct(P->HullBody->GetPhysicsLinearVelocity(), P->GetActorForwardVector()), 1.f))));
				Next();
				if (S2Step <= NL) S2Place(Sea, -Legs[S2Step - 1].Twa, true);
				else { P->IWSteam(1); P->IWTelegraph(6); } // still in irons: raise steam
			}
			return;
		}
		if (S2Step == NL + 1)
		{
			const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, -20.f);
			P->SetAutoRudder(FMath::Clamp(Err * 2.f, -P->MaxRudderDeg, P->MaxRudderDeg));
			if (Since >= 30.f && S2Shots == 8) { PhotoAt(FVector(6000.f, -10000.f, 3000.f), FVector(-400.f, 0.f, 2400.f)); }
			if (Since >= 31.5f && S2Shots == 8) S2Shot(TEXT("S2_frigate_irons_steam"), true);
			if (Since >= 40.f)
			{
				PhotoOff();
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT point=irons_steam twa=%.0f kn=%.2f steam=%d"), P->TrueWindAngle, P->GetSpeedKnots(), P->bAuxSteam);
				P->bAutoHelm = false;
				Next();
			}
			return;
		}
		if (S2Step == NL + 2 && Since > 2.f) Finish();
		return;
	}

	// ------------------------------------------------------------------------------------------------ water (Chuck 23:50 waterline)
	if (S2Mode == TEXT("water"))
	{
		const float L = P->HullLengthM;
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 12.f);
			IronwakeSea::SetSeaScale(GetWorld(), 0.25f); // calm water: the cutter rides the default chop (+/-2.5 m heave), useless for a waterline photo
			S2Place(Sea, -90.f, false);
			P->IWTelegraph(2);
			Next(); return;
		}
		static double WlSum = 0.0; static int32 WlN = 0;
		const float MeshBootZ = (P->ShipId == TEXT("brig")) ? 435.f : 0.f; // SM_TallBrig z=0 is at keel, boot-top at z=4.35 m
		if (S2Step == 1 && Since >= 9.f && Since < 19.f)
		{
			const FVector B = P->HullMesh->GetComponentTransform().TransformPosition(FVector(0.f, 0.f, MeshBootZ));
			WlSum += (B.Z - IronwakeSea::RenderedZ(GetWorld(), B)) / 100.0; ++WlN;
		}
		if (S2Step == 1 && Since >= 19.f && Since < 19.f + RealDt * 1.5f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT waterline_avg10s ship=%s bootTop_over_sea=%.2f m (n=%d)"), *P->GetShipName(), WlN ? WlSum / WlN : 0.0, WlN);
			const FVector BootW = P->HullMesh->GetComponentTransform().TransformPosition(FVector(0.f, 0.f, MeshBootZ));
			const FVector KeelW = P->HullMesh->GetComponentTransform().TransformPosition(FVector::ZeroVector);
			const float SeaR = IronwakeSea::RenderedZ(GetWorld(), BootW);
			UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT waterline ship=%s actorZ=%.0f meshRelZ=%.0f keelZ=%.0f bootTopZ=%.0f renderedSea=%.0f bootTop_over_sea=%.2f m keel_under_sea=%.2f m queryZ=%.0f"), *P->GetShipName(),
				P->GetActorLocation().Z, P->HullMesh->GetRelativeLocation().Z, KeelW.Z, BootW.Z, SeaR, (BootW.Z - SeaR) / 100.f, (SeaR - KeelW.Z) / 100.f, AIronwakeShell::WaterZAtWaves(GetWorld(), BootW));
		}
		if (S2Step == 1 && Since >= 20.f)
		{
			// Eye 1.2 m over the rendered sea, 3.5 hull-lengths off the beam, looking level at the waterline (no up/down tilt).
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			FVector Eye = X.TransformPosition(FVector(L * 20.f, -FMath::Max(L * 100.f * 0.9f, 4500.f), 0.f));
			FVector Look = X.TransformPosition(FVector(0.f, 0.f, 0.f));
			Eye.Z = IronwakeSea::RenderedZ(GetWorld(), Eye) + 120.f; Look.Z = IronwakeSea::RenderedZ(GetWorld(), Look) + 120.f;
			SetPhotoView(Eye, (Look - Eye).Rotation());
			Next(); return;
		}
		if (S2Step == 2 && Since < RealDt * 1.5f && S2Mode == TEXT("water"))
		{
			// Gauge post 6 m off the beam (camera side): 1 m bands, orange/white above RenderedZ, black/purple below.
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			const FVector G = X.TransformPosition(FVector(L * 30.f, -(L * 9.f + 700.f), 0.f));
			const float GS = IronwakeSea::RenderedZ(GetWorld(), G);
			for (int32 k = -4; k < 6; ++k)
			{
				UStaticMeshComponent* Cc = NewObject<UStaticMeshComponent>(this); Cc->SetStaticMesh(Cube); Cc->SetupAttachment(RootComponent);
				Cc->RegisterComponent(); Cc->SetWorldLocation(FVector(G.X, G.Y, GS + k * 100.f + 50.f)); Cc->SetWorldScale3D(FVector(0.6f, 0.6f, 0.98f));
				Cc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				if (UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this)) { M->SetVectorParameterValue(TEXT("Color"), k >= 0 ? (k % 2 ? FLinearColor(0.9f, 0.9f, 0.9f) : FLinearColor(0.9f, 0.5f, 0.05f)) : (k % 2 ? FLinearColor(0.02f, 0.02f, 0.02f) : FLinearColor(0.15f, 0.02f, 0.4f))); Cc->SetMaterial(0, M); }
			}
			UE_LOG(LogIronwake, Display, TEXT("IWS2 gauge at renderedZ=%.0f (queryZ=%.0f)"), GS, AIronwakeShell::WaterZAtWaves(GetWorld(), G));
			// Trace the water mesh itself (visibility channel) at 5 points around the hull: what actually renders.
			const FVector Bt = P->HullMesh->GetComponentTransform().TransformPosition(FVector::ZeroVector);
			{
				const FBox Bb = P->HullMesh->Bounds.GetBox();
				const FBox Lb = P->HullMesh->GetStaticMesh() ? P->HullMesh->GetStaticMesh()->GetBoundingBox() : FBox(ForceInit);
				UE_LOG(LogIronwake, Display, TEXT("IWS2 hull bounds world zmin=%.0f zmax=%.0f local zmin=%.0f zmax=%.0f mesh=%s renderedZ_here=%.0f"), Bb.Min.Z, Bb.Max.Z, Lb.Min.Z, Lb.Max.Z,
					*GetNameSafe(P->HullMesh->GetStaticMesh()), IronwakeSea::RenderedZ(GetWorld(), Bt));
			}
			for (const FVector& O : { FVector(0, -1, 0), FVector(0, 1, 0), FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0.5f, -0.6f, 0) })
			{
				const FVector Q = X.TransformPosition(FVector(O.X * L * 70.f, O.Y * (L * 8.f + 600.f), 0.f));
				FHitResult H; FCollisionQueryParams QP; QP.AddIgnoredActor(P);
				const bool bHit = GetWorld()->LineTraceSingleByChannel(H, FVector(Q.X, Q.Y, Bt.Z + 3000.f), FVector(Q.X, Q.Y, Bt.Z - 3000.f), ECC_Visibility, QP);
				UE_LOG(LogIronwake, Display, TEXT("IWS2 trace water hit=%d actor=%s comp=%s z=%.0f renderedZ=%.0f bootTopZ=%.0f"), bHit, bHit ? *GetNameSafe(H.GetActor()) : TEXT("-"),
					bHit ? *GetNameSafe(H.GetComponent()) : TEXT("-"), bHit ? H.ImpactPoint.Z : 0.f, IronwakeSea::RenderedZ(GetWorld(), Q), Bt.Z);
			}
		}
		if (S2Step == 2 && Since >= 1.5f && S2Shots == 0) S2Shot(*FString::Printf(TEXT("S2_waterline_%s"), *P->ShipId.ToString()), true);
		if (S2Step == 2 && Since >= 3.f)
		{
			// sail ship: also under full sail beam reach, worst steady heel
			if (P->HasSails()) { PhotoOff(); P->IWTelegraph(5); S2Place(Sea, -90.f, false); IronwakeSea::SetSeaScale(GetWorld(), 1.f); IronwakeFleet::SetFixedWind(0.f, 16.f); Next(); }
			else { PhotoOff(); S2Step = 9; S2Mark = S2T; }
			return;
		}
		if (S2Step == 3)
		{
			const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, -90.f);
			P->bAutoHelm = true; P->SetAutoRudder(FMath::Clamp(Err * 2.f, -P->MaxRudderDeg, P->MaxRudderDeg));
			if (Since >= 40.f && S2Shots == 1) PhotoAt(FVector(L * 20.f, -L * 85.f, -100.f), FVector(-L * 5.f, 0.f, -350.f)); // lee side (wind from N on stbd... heading W: wind on stbd, lee = port)
			if (Since >= 41.5f && S2Shots == 1) S2Shot(TEXT("S2_waterline_frigate_heeled_lee"), true);
			if (Since >= 43.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT heel beam reach 16 kn full sail: heel=%.1f roll=%.1f kn=%.1f"), P->SailHeelDeg, P->GetActorRotation().Roll, P->GetSpeedKnots());
				PhotoOff(); P->bAutoHelm = false; S2Step = 9; S2Mark = S2T;
			}
			return;
		}
		if (S2Step == 9 && Since > 1.f) Finish();
		return;
	}

	// ------------------------------------------------------------------------------------------------ zoom (Chuck 22:30)
	if (S2Mode == TEXT("zoom"))
	{
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 16.f);
			S2Place(Sea, -120.f, false);
			P->IWTelegraph(5);
			Next(); return;
		}
		auto Hold = [&]() { const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, -120.f); P->bAutoHelm = true; P->SetAutoRudder(FMath::Clamp(Err * 2.f, -P->MaxRudderDeg, P->MaxRudderDeg)); };
		Hold();
		if (S2Step == 1 && Since >= 25.f) { P->IWZoom(1); C->SetControlRotation(FRotator(-6.f, P->GetActorRotation().Yaw + 100.f, 0.f)); Next(); return; }
		if (S2Step == 2 && Since >= 4.f && S2Shots == 0) S2Shot(TEXT("C_zoom_close"), false);
		if (S2Step == 2 && Since >= 5.5f) { P->IWZoom(2); C->SetControlRotation(FRotator(-42.f, P->GetActorRotation().Yaw + 95.f, 0.f)); Next(); return; }
		if (S2Step == 3 && Since >= 4.f && S2Shots == 1) S2Shot(TEXT("C_zoom_deck"), false);
		if (S2Step == 3 && Since >= 5.5f) { P->IWZoom(2); C->SetControlRotation(FRotator(12.f, P->GetActorRotation().Yaw - 60.f, 0.f)); Next(); return; }
		if (S2Step == 4 && Since >= 4.f && S2Shots == 2) S2Shot(TEXT("C_zoom_lookup"), false);
		if (S2Step == 4 && Since >= 5.5f) { P->IWZoom(0); C->SetControlRotation(FRotator(-2.f, P->GetActorRotation().Yaw + 90.f, 0.f)); Next(); return; }
		if (S2Step == 5 && Since >= 4.f && S2Shots == 3) S2Shot(TEXT("C_zoom_beam_pushout"), false);
		if (S2Step == 5 && Since >= 5.5f)
		{
			const FVector Cam = C->PlayerCameraManager ? C->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
			UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT zoom closest arm=%.0f m cam-to-hull=%.0f m camZ-over-sea=%.1f m"), P->ZoomStepsL[0] * P->HullLengthM,
				FVector::Dist(Cam, P->GetActorLocation()) / 100.f, (Cam.Z - IronwakeSea::RenderedZ(GetWorld(), Cam)) / 100.f);
			P->ResetCameraToHorizon(); Next(); return;
		}
		if (S2Step == 6 && Since >= 1.f) { P->bAutoHelm = false; Finish(); }
		return;
	}

	// ------------------------------------------------------------------------------------------------ beat (sailing autopilot upwind)
	if (S2Mode == TEXT("beat"))
	{
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 16.f);
			S2Place(Sea + FVector(-150000.f, 0.f, 0.f), 60.f, false);  // start on a beam reach heading NE
			P->IWTelegraph(5);
			Next(); return;
		}
		if (S2Step == 1 && Since >= 15.f)
		{
			EngageAutopilot(1);
			HoldYaw = 0.f; // dead upwind: the autopilot must beat
			UE_LOG(LogIronwake, Display, TEXT("IWS2 beat: autopilot HOLD 000 (dead into a N wind)"));
			S2YawStart = 999.f; S2TurnT = 0.f; Next(); return;
		}
		if (S2Step == 2)
		{
			static int32 LastBoard = 0; static int32 Tacks = 0; static float MinTwa = 999.f, IronsT = 0.f; static float X0 = 0.f;
			if (Since < RealDt * 1.5f) { LastBoard = 0; Tacks = 0; MinTwa = 999.f; IronsT = 0.f; X0 = P->GetActorLocation().X; }
			// force a board change midway by nudging the hold 15 deg to the other side
			if (Since > 55.f && Since < 55.f + RealDt * 1.5f) HoldYaw = BeatTack > 0 ? -15.f : 15.f;
			const int32 Board = P->TrueWindAngle >= 0.f ? 1 : -1;
			if (Since > 20.f) { if (LastBoard != 0 && Board != LastBoard) ++Tacks; LastBoard = Board; MinTwa = FMath::Min(MinTwa, FMath::Abs(P->TrueWindAngle)); if (FMath::Abs(P->TrueWindAngle) < 35.f) IronsT += RealDt; }
			if (Since >= 60.f && S2Shots == 0) { PhotoAt(FVector(-2500.f, 0.f, 11000.f), FVector(-200.f, 0.f, 2000.f)); }
			if (Since >= 61.5f && S2Shots == 0) S2Shot(TEXT("S2_frigate_beating_yards"), true);
			if (Since >= 63.f && S2Shots == 1) PhotoOff();
			if (Since >= 140.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT beat tacks=%d min_twa=%.0f time_in_irons=%.1f s made_good_upwind=%.0f m in 130 s kn_now=%.1f heel=%.1f"),
					Tacks, MinTwa, IronsT, (P->GetActorLocation().X - X0) / 100.f, P->GetSpeedKnots(), P->SailHeelDeg);
				DisengageAutopilot(TEXT("test done")); Next();
			}
			return;
		}
		if (S2Step == 3 && Since > 1.f) Finish();
		return;
	}

	// ------------------------------------------------------------------------------------------------ handling
	if (S2Mode == TEXT("handling"))
	{
		const float L = P->HullLengthM;
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 16.f);
			S2Place(Sea, -120.f, false);
			P->IWTelegraph(5);
			UE_LOG(LogIronwake, Display, TEXT("IWS2 handling test ship=%s L=%.0f m"), *P->GetShipName(), L);
			S2YawStart = -1.f; S2TurnT = -1.f;
			Next(); return;
		}
		if (S2Step == 1)
		{
			const float Full = P->TelegraphKnots[5];
			const float Kn = P->GetSpeedKnots();
			if (S2YawStart < 0.f && (Kn >= 0.9f * (P->SailRig ? P->SailKn + 0.01f : Full)))
			{
				S2YawStart = Since;
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT ship=%s time_to_90pct_full=%.1f s (%.1f kn)"), *P->GetShipName(), Since, Kn);
			}
			if (Since >= 40.f && S2Shots == 0) { P->ResetCameraToHorizon(); C->SetControlRotation(FRotator(-8.f, P->GetActorRotation().Yaw + 150.f, 0.f)); }
			if (Since >= 43.f && S2Shots == 0) S2Shot(*FString::Printf(TEXT("S2_%s_sailing_hud"), *P->ShipId.ToString()), false);
			if (Since >= 45.f && S2Shots == 1) PhotoAt(FVector(L * 60.f, -L * 140.f, L * 22.f), FVector(-L * 5.f, 0.f, L * 12.f));
			if (Since >= 46.5f && S2Shots == 1) S2Shot(*FString::Printf(TEXT("S2_%s_sailing_side"), *P->ShipId.ToString()), true);
			if (Since >= 48.f)
			{
				PhotoOff();
				if (S2YawStart < 0.f) UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT ship=%s time_to_90pct_full=>48 s (%.1f kn)"), *P->GetShipName(), Kn);
				P->bAutoHelm = false;
				P->IWRudder(3); // hard starboard
				Next();
			}
			return;
		}
		if (S2Step == 2)
		{
			if (Since >= 25.f && S2Shots == 2)
			{
				const float R = P->HullBody->GetPhysicsAngularVelocityInDegrees().Z;
				const float V = P->GetSpeedKnots() * 0.5144f * P->SpeedCompression;
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT ship=%s hard_turn yawrate=%.2f deg/s speed=%.1f kn turn_radius=%.0f m (sim, compression %.1f)"),
					*P->GetShipName(), R, P->GetSpeedKnots(), V / FMath::Max(FMath::DegreesToRadians(FMath::Abs(R)), 1e-3f), P->SpeedCompression);
				if (S2Shots == 2) PhotoAt(FVector(-L * 30.f, L * 120.f, L * 45.f), FVector(0.f, 0.f, L * 5.f));
			}
			if (Since >= 26.5f && S2Shots == 2) S2Shot(*FString::Printf(TEXT("S2_%s_turning"), *P->ShipId.ToString()), true);
			if (Since >= 28.f) { PhotoOff(); P->IWRudder(0); Next(); }
			return;
		}
		if (S2Step == 3 && Since > 2.f) Finish();
		return;
	}

	// ------------------------------------------------------------------------------------------------ circle (P3)
	if (S2Mode == TEXT("circle"))
	{
		const float L = P->HullLengthM;
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 0.f);
			{ FVector Start = Sea; FString At; if (FParse::Value(FCommandLine::Get(), TEXT("iwcircleat="), At)) { FString A, B; At.Split(TEXT(","), &A, &B); Start = FVector(FCString::Atof(*A), FCString::Atof(*B), 0.f); } S2Place(Start, -120.f, false); }
			P->IWSteam(1); P->IWTelegraph(5);
			UE_LOG(LogIronwake, Display, TEXT("IWP3 circle test ship=%s R=%.0f m"), *P->GetShipName(), P->MinTurnRadiusM);
			Next(); return;
		}
		if (S2Step == 1)
		{
			const float Full = P->TelegraphKnots[5];
			if (P->GetSpeedKnots() >= 0.97f * Full || Since > 90.f)
			{
				P->bAutoHelm = false; P->IWRudder(3);
				P3T0 = S2T; P3Unwrap = 0.f; P3LastHdg = P->GetHeadingDeg(); P3Start = P->GetActorLocation(); P3MaxD = 0.f; P3Done90 = P3Done180 = false;
				UE_LOG(LogIronwake, Display, TEXT("IWP3 rudder hard at %.1f kn after %.1f s"), P->GetSpeedKnots(), Since);
				Next();
			}
			return;
		}
		if (S2Step == 2)
		{
			const float H = P->GetHeadingDeg();
			float D = H - P3LastHdg; while (D > 180.f) D -= 360.f; while (D < -180.f) D += 360.f;
			P3Unwrap += D; P3LastHdg = H;
			const float T = S2T - P3T0;
			P3MaxD = FMath::Max(P3MaxD, (float)FVector::Dist2D(P->GetActorLocation(), P3Start) / 100.f);
			if (!P3Done90 && FMath::Abs(P3Unwrap) >= 90.f) { P3Done90 = true; UE_LOG(LogIronwake, Display, TEXT("IWP3 t90=%.1f s"), T); }
			if (!P3Done180 && FMath::Abs(P3Unwrap) >= 180.f) { P3Done180 = true; UE_LOG(LogIronwake, Display, TEXT("IWP3 t180=%.1f s"), T); }
			if (T > 6.f && S2Shots == 0 && FMath::Abs(P3Unwrap) > 40.f) { PhotoAt(FVector(-L * 30.f, L * 120.f, L * 45.f), FVector(0.f, 0.f, L * 5.f)); }
			if (T > 7.5f && S2Shots == 0 && FMath::Abs(P3Unwrap) > 40.f) S2Shot(*FString::Printf(TEXT("P3_turn_%s"), *P->ShipId.ToString()), true);
			if (S2Shots == 1 && T > 9.f) PhotoOff();
			if (FMath::Abs(P3Unwrap) >= 360.f || T > 600.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP3 RESULT ship=%s full_circle=%.1f s (unwrapped %.0f deg) diameter~%.0f m speed=%.1f kn yawrate=%.2f R_coded=%.0f"),
					*P->GetShipName(), T, P3Unwrap, P3MaxD, P->GetSpeedKnots(), P->HullBody->GetPhysicsAngularVelocityInDegrees().Z, P->MinTurnRadiusM);
				P->IWRudder(0); PhotoOff(); Finish();
			}
		}
		return;
	}

	// ------------------------------------------------------------------------------------------------ fxshot (P3): hull-side photo at speed, same framing as the voyage captures
	if (S2Mode == TEXT("fxshot"))
	{
		const float L = P->HullLengthM;
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 0.f);
			S2Place(Sea, -120.f, false);
			P->IWSteam(1); P->IWTelegraph(5); if (FParse::Param(FCommandLine::Get(), TEXT("iwfast"))) P->SpeedCompression = 6.f;
			Next(); return;
		}
		if (S2Step == 1)
		{
			if (Since > 45.f) { Next(); }
			return;
		}
		if (S2Step == 2)
		{
			if (S2Shots == 0) PhotoAt(FVector(L * 20.f, L * 85.f, FMath::Max(700.f, L * 38.f)), FVector(-L * 10.f, 0.f, FMath::Max(500.f, L * 12.f))); // P5c: aim up so funnel + smoke are in frame
			if (Since > 1.5f && S2Shots == 0) { FString Tag; FParse::Value(FCommandLine::Get(), TEXT("iwtag="), Tag); S2Shot(*FString::Printf(TEXT("P3fx_%s_%s"), *P->ShipId.ToString(), *Tag), true); }
			if (Since > 3.5f) { PhotoOff(); Finish(); }
		}
		return;
	}

	// ------------------------------------------------------------------------------------------------ p5b: speed ladder (hull-side + chase pair) and slam counting
	if (S2Mode == TEXT("p5b"))
	{
		const float L = P->HullLengthM;
		float Kn = 8.f, SeaScale = 0.25f, Hold = 50.f; FString Tag;
		FParse::Value(FCommandLine::Get(), TEXT("iwkn="), Kn); FParse::Value(FCommandLine::Get(), TEXT("iwsea="), SeaScale);
		FParse::Value(FCommandLine::Get(), TEXT("iwhold="), Hold); FParse::Value(FCommandLine::Get(), TEXT("iwtag="), Tag);
		const bool bCount = FParse::Param(FCommandLine::Get(), TEXT("iwcount"));
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 0.f);
			IronwakeSea::SetSeaScale(GetWorld(), SeaScale);
			{ // P5c: the storm actor re-applies its own state every tick (it overrode calm/moderate in P5b): pin its target to the requested sea
				const float St = SeaScale <= 1.f ? FMath::Clamp((SeaScale - 0.25f) / 0.75f, 0.f, 1.f) : FMath::Lerp(1.f, 2.f, FMath::Clamp((SeaScale - 1.f) / 0.6f, 0.f, 1.5f));
				if (AIronwakeStorm* Sx = AIronwakeStorm::Get(GetWorld())) Sx->SetTarget(St, 0.1f);
			}
			S2Place(Sea, -120.f, false);
			P->IWSteam(1);
			if (P->TelegraphKnots.Num() > 5) { P->TelegraphKnots[5] = Kn; P->IWTelegraph(5); }
			UE_LOG(LogIronwake, Display, TEXT("IWP5B start ship=%s kn=%.1f sea=%.2f"), *P->ShipId.ToString(), Kn, SeaScale);
			Next(); return;
		}
		if (S2Step == 1)
		{
			if (Since > Hold) { UE_LOG(LogIronwake, Display, TEXT("IWP5B hold done kn=%.2f"), P->GetSpeedKnots()); Next(); }
			return;
		}
		if (bCount) { if (Since > 120.f) { UE_LOG(LogIronwake, Display, TEXT("IWP5B count window done")); Finish(); } return; }
		if (S2Step == 2)
		{
			if (S2Shots == 0) PhotoAt(FVector(L * 20.f, L * 85.f, FMath::Max(700.f, L * 38.f)), FVector(-L * 10.f, 0.f, FMath::Max(500.f, L * 12.f))); // P5c: aim up so funnel + smoke are in frame
			if (Since > 1.5f && S2Shots == 0) S2Shot(*FString::Printf(TEXT("P5b_%s_%s_side"), *P->ShipId.ToString(), *Tag), true);
			if (Since > 3.5f) { PhotoOff(); Next(); }
			return;
		}
		if (S2Step == 3)
		{
			if (Since < 0.2f) { P->ResetCameraToHorizon(); C->SetControlRotation(FRotator(-9.f, P->GetActorRotation().Yaw + 180.f, 0.f)); }
			if (Since > 2.f && S2Shots == 1) { UE_LOG(LogIronwake, Display, TEXT("IWP5B chase kn=%.2f"), P->GetSpeedKnots()); S2Shot(*FString::Printf(TEXT("P5b_%s_%s_chase"), *P->ShipId.ToString(), *Tag), false); }
			if (Since > 4.f) Finish();
		}
		return;
	}

	// ------------------------------------------------------------------------------------------------ p5c: seam (clip type-4 high wide) + turn frames
	if (S2Mode == TEXT("p5cwide"))
	{
		const float L = P->HullLengthM, HL = L * 100.f;
		float Kn = 8.f, SeaScale = 0.9f; FString Tag; FParse::Value(FCommandLine::Get(), TEXT("iwkn="), Kn); FParse::Value(FCommandLine::Get(), TEXT("iwsea="), SeaScale); FParse::Value(FCommandLine::Get(), TEXT("iwtag="), Tag);
		const bool bTurn = FParse::Param(FCommandLine::Get(), TEXT("iwturn"));
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 0.f);
			IronwakeSea::SetSeaScale(GetWorld(), SeaScale);
			S2Place(Sea, -120.f, false);
			P->IWSteam(1);
			if (P->TelegraphKnots.Num() > 5) { P->TelegraphKnots[5] = Kn; P->IWTelegraph(5); }
			Next(); return;
		}
		if (S2Step == 1) { if (Since > 45.f) { if (bTurn) P->IWRudder(3); Next(); } return; }
		if (S2Step == 2)
		{
			if (bTurn && Since < 14.f) return; // let the turn develop
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			FVector Eye = bTurn ? X.TransformPosition(FVector(-1.2f, 1.6f, 1.6f) * HL) : X.TransformPosition(FVector(3.8f, .5f, .35f) * HL);
			const FVector Look = bTurn ? X.TransformPosition(FVector(-1.6f, 0.f, 0.f) * HL) : X.TransformPosition(FVector(0.f, 0.2f, 0.2f) * HL);
			Eye.Z = FMath::Max(Eye.Z, AIronwakeShell::WaterZAtWaves(GetWorld(), Eye) + 200.f);
			if (S2Shots == 0) PhotoAt(Eye, Look);
			SetPhotoView(Eye, (Look - Eye).Rotation());
			if (Since > (bTurn ? 15.5f : 1.5f) && S2Shots == 0) S2Shot(*FString::Printf(TEXT("P5cw_%s_%s"), *P->ShipId.ToString(), *Tag), true);
			if (Since > (bTurn ? 18.f : 3.5f)) { PhotoOff(); Finish(); }
		}
		return;
	}

	// ------------------------------------------------------------------------------------------------ clip (P4)
	// Peaceful gameplay clip director: warm up to cruise speed, then cut between scripted camera shots. The player keeps
	// its speed across teleports (S2Place keep-speed). Each shot runs Len s; the first SettleS are discarded in the edit.
	// All views use the photo camera, so no HUD. Frame numbers are logged for the edit (-benchmark -fps=30 -dumpmovie).
	if (S2Mode == TEXT("clip"))
	{
		struct FClipShot { const TCHAR* Poi; float Bearing, OffM, Sea; int32 Type; };
		static const FClipShot Shots[] = {
			{ TEXT("Greyhaven Harbour"), 215.f, 700.f, 0.3f, 1 },
			{ TEXT("Fir Holm"), 160.f, 450.f, 0.4f, 0 },
			{ TEXT("Fort Grimsby"), 250.f, 400.f, 0.4f, 1 },
			{ nullptr, 0.f, 0.f, 0.9f, 4 },
			{ TEXT("St Brannoc"), 190.f, 600.f, 0.5f, 2 },
			{ TEXT("Gannet Stacks"), 300.f, 450.f, 0.4f, 1 },
			{ TEXT("Kestrel Isle"), 120.f, 900.f, 0.4f, 0 },
			{ TEXT("The Needles"), 20.f, 550.f, 0.4f, 2 },
			{ nullptr, 0.f, 0.f, 0.4f, 3 },
		};

		const int32 NShots = UE_ARRAY_COUNT(Shots);
		constexpr float Len = 7.f, SettleS = 2.f;
		static int32 Idx = -1; static float ShotT = 0.f; static bool bCam = false; static FVector FixEyeUnused;
		const float L = P->HullLengthM, HL = L * 100.f;
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			IronwakeFleet::SetFixedWind(0.f, 0.f);
			IronwakeSea::SetSeaScale(GetWorld(), 0.4f);
			S2Place(Sea, -120.f, false);
			P->IWSteam(1);
			const float Cruise = FParse::Param(FCommandLine::Get(), TEXT("iwslow")) ? 6.f : 8.f;
			if (P->TelegraphKnots.Num() > 5) { P->TelegraphKnots[5] = Cruise; P->IWTelegraph(5); }
			UE_LOG(LogIronwake, Display, TEXT("IWP4 warmup ship=%s cruise=%.1f frame=%llu"), *P->ShipId.ToString(), Cruise, (uint64)GFrameCounter);
			Next(); return;
		}
		if (S2Step == 1)
		{
			if (Since > 45.f || (Since > 20.f && P->GetSpeedKnots() > 0.95f * P->TelegraphKnots[5]))
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP4 warmup done kn=%.2f t=%.1f"), P->GetSpeedKnots(), Since);
				Idx = -1; ShotT = Len; Next();
			}
			return;
		}
		AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld());
		// advance to the next shot
		if (ShotT >= Len)
		{
			++Idx; ShotT = 0.f;
			if (Idx >= NShots) { PhotoOff(); UE_LOG(LogIronwake, Display, TEXT("IWP4 DONE frame=%llu"), (uint64)GFrameCounter); Finish(); return; }
			const FClipShot& Sh = Shots[Idx];
			IronwakeSea::SetSeaScale(GetWorld(), Sh.Sea);
			FVector Loc = Sea; float Yaw = -120.f;
			if (Sh.Type == 3 && LS && LS->Airship)
			{
				const FVector A = LS->Airship->GetActorLocation();
				const FVector Pos = FVector(A.X, A.Y, 0.f) + FRotator(0.f, 200.f, 0.f).Vector() * 45000.f;
				Loc = Pos; Yaw = (FVector(A.X, A.Y, 0.f) - Pos).Rotation().Yaw;
			}
			else if (Sh.Poi)
			{
				float Hz = 30000.f; FVector C0 = FVector::ZeroVector; bool bFound = false;
				for (const FIronwakePOI& Q : POIs) if (Q.Name.Contains(Sh.Poi)) { C0 = Q.Loc; bFound = true; Hz = Q.HardRadiusCm > 0.f ? Q.HardRadiusCm : 30000.f; }
				if (AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld())) for (const FIsle& I : Ar->Isles) if (I.Name.Contains(Sh.Poi)) Hz = I.HazardR;
				if (FString(Sh.Poi).Contains(TEXT("St Brannoc"))) Hz = 42000.f;
				if (FString(Sh.Poi).Contains(TEXT("Greyhaven"))) Hz = 20000.f;
				if (FString(Sh.Poi).Contains(TEXT("Needles"))) Hz = 36000.f;
				if (!bFound) UE_LOG(LogIronwake, Warning, TEXT("IWP4 POI '%s' not found"), Sh.Poi);
				Loc = C0 + FRotator(0.f, Sh.Bearing, 0.f).Vector() * (Hz + Sh.OffM * 100.f);
				Yaw = Sh.Bearing + 90.f;
				UE_LOG(LogIronwake, Display, TEXT("IWP4 poi %s at (%.0f,%.0f) hazard=%.0f m"), Sh.Poi, C0.X / 100.f, C0.Y / 100.f, Hz / 100.f);
			}
			S2Place(Loc, Yaw, true);
			PhotoAt(FVector(-HL * 2.f, 0.f, HL * 0.6f), FVector(HL * 0.8f, 0.f, HL * 0.15f));
			UE_LOG(LogIronwake, Display, TEXT("IWP4 shot %d type=%d poi=%s sea=%.2f start_frame=%llu kn=%.2f"), Idx, Sh.Type, Sh.Poi ? Sh.Poi : TEXT("-"), Sh.Sea, (uint64)GFrameCounter, P->GetSpeedKnots());
		}
		ShotT += RealDt;
		const FClipShot& Sh = Shots[FMath::Clamp(Idx, 0, NShots - 1)];
		const float U = FMath::Clamp(ShotT / Len, 0.f, 1.f);
		FVector A(-2.2f, .3f, .6f), B(-1.8f, -.3f, .5f), LA(.8f, 0.f, .15f), LB = LA;
		if (Sh.Type == 1) { A = FVector(.6f, 3.2f, .5f); B = FVector(-.6f, 3.4f, .65f); LA = LB = FVector(0.f, 0.f, .15f); }
		if (Sh.Type == 2) { A = FVector(-3.f, 2.5f, 1.6f); B = FVector(1.f, 3.2f, 1.9f); LA = LB = FVector(0.f, 0.f, 0.f); }
		if (Sh.Type == 3) { A = FVector(-1.4f, 1.1f, .5f); B = FVector(-1.0f, 1.5f, .6f); }
		if (Sh.Type == 4) { A = FVector(3.8f, .5f, .35f); B = FVector(3.2f, 1.1f, .45f); LA = FVector(0.f, 0.f, .2f); LB = FVector(.3f, 0.f, .2f); }
		const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		const float Ease = U * U * (3.f - 2.f * U);
		FVector Eye = X.TransformPosition(FMath::Lerp(A, B, Ease) * HL);
		FVector Look = X.TransformPosition(FMath::Lerp(LA, LB, Ease) * HL);
		if (Sh.Type == 3 && LS && LS->Airship) Look = FMath::Lerp(P->GetActorLocation(), LS->Airship->GetActorLocation(), 0.4f);
		Eye.Z = FMath::Max(Eye.Z, AIronwakeShell::WaterZAtWaves(GetWorld(), Eye) + 200.f);
		SetPhotoView(Eye, (Look - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Sh.Type == 3 ? 62.f : 55.f);
		if (FMath::Fmod(S2T, 2.f) < RealDt + 1e-3f) UE_LOG(LogIronwake, Display, TEXT("IWP4 t shot=%d u=%.2f kn=%.2f frame=%llu"), Idx, U, P->GetSpeedKnots(), (uint64)GFrameCounter);
		return;
	}

	// ------------------------------------------------------------------------------------------------ progress
	if (S2Mode == TEXT("progress"))
	{
		AIronwakeGameMode* GM = GetWorld()->GetAuthGameMode<AIronwakeGameMode>();
		if (GM && GM->bRestarted)
		{
			// After LAUNCH through the shipyard: the new hull must be in command and the choice saved.
			if (S2Step == 0 && S2T >= 6.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT after_launch ship=%s selected_saved=%s unlocked frigate=%d ironclad=%d progress='%s'"), *P->GetShipName(),
					*IronwakeFleet::GetSelected().ToString(), IronwakeFleet::IsUnlocked(TEXT("frigate")), IronwakeFleet::IsUnlocked(TEXT("ironclad")), *IronwakeFleet::ProgressLine());
				P->ResetCameraToHorizon(); C->SetControlRotation(FRotator(-7.f, P->GetActorRotation().Yaw + 155.f, 0.f));
				Next();
			}
			if (S2Step == 1 && Since >= 4.f && S2Shots == 0) S2Shot(TEXT("S2_after_launch_frigate"), false);
			if (S2Step == 1 && Since >= 6.f) Finish();
			return;
		}
		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			IronwakeFleet::ResetProgress();
			if (!C->bStarted) C->StartGame(true);
			UE_LOG(LogIronwake, Display, TEXT("IWS2 progress start: frigate=%d ironclad=%d next=%d"), IronwakeFleet::IsUnlocked(TEXT("frigate")), IronwakeFleet::IsUnlocked(TEXT("ironclad")), NextPOI);
			C->IWYard(-1);
			Next(); return;
		}
		if (S2Step == 1)
		{
			if (Since >= 2.f && S2Shots == 0) S2Shot(TEXT("S2_shipyard_start"), false);
			if (Since >= 3.5f) { C->HideMenu(); Next(); }
			return;
		}
		// Steps 2..5: arrive at each point of interest in order (the real arrival code in TickSailing records it).
		if (S2Step >= 2 && S2Step <= 5)
		{
			if (Since < 0.2f) return;
			if (S2TurnT != (float)S2Step)
			{
				S2TurnT = (float)S2Step;
				const FIronwakePOI& Pt = POIs[NextPOI];
				const FVector Away = (FVector(-300000.f, -270000.f, 0.f) - Pt.Loc).GetSafeNormal2D();
				const float Off = FMath::Max(Pt.HardRadiusCm + 15000.f, Pt.ArriveRadiusCm - 20000.f);
				S2Place(Pt.Loc + Away * Off, (-Away).Rotation().Yaw, false);
				UE_LOG(LogIronwake, Display, TEXT("IWS2 progress: placed at %s (%.0f m off)"), *Pt.Name, Off / 100.f);
			}
			if (S2Step == 5 && Since >= 3.f && S2Shots == 1) { P->ResetCameraToHorizon(); }
			if (S2Step == 5 && Since >= 4.f && S2Shots == 1) S2Shot(TEXT("S2_unlock_toast"), false);
			if (Since >= 5.f) Next();
			return;
		}
		if (S2Step == 6)
		{
			if (Since < 0.5f) return;
			UE_LOG(LogIronwake, Display, TEXT("IWS2 RESULT progress after 4 arrivals: frigate=%d ironclad=%d '%s'"), IronwakeFleet::IsUnlocked(TEXT("frigate")), IronwakeFleet::IsUnlocked(TEXT("ironclad")), *IronwakeFleet::ProgressLine());
			C->IWYard(1);
			Next(); return;
		}
		if (S2Step == 7)
		{
			if (Since >= 2.f && S2Shots == 2) S2Shot(TEXT("S2_shipyard_frigate_unlocked"), false);
			if (Since >= 4.f) { UE_LOG(LogIronwake, Display, TEXT("IWS2 progress: LAUNCH frigate via shipyard")); C->ChooseShip(TEXT("frigate")); Next(); }
			return;
		}
		return;
	}

	// ------------------------------------------------------------------------------------------------ p26 (TallBrig verification suite)
	if (S2Mode == TEXT("p26"))
	{
		UWorld* W = GetWorld();
		AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(W);
		auto HoldYawTo = [&](float TargetYaw)
		{
			const float Err = FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw, TargetYaw);
			P->bAutoHelm = true;
			P->SetAutoRudder(FMath::Clamp(Err * 2.f, -P->MaxRudderDeg, P->MaxRudderDeg));
		};
		auto SunlitPhoto = [&](const FVector& RelEye, const FVector& RelLook, float Fov = 54.f)
		{
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			FVector Eye = X.TransformPosition(RelEye);
			const FVector Look = X.TransformPosition(RelLook);
			Eye.Z = FMath::Max(Eye.Z, IronwakeSea::RenderedZ(W, Eye) + 135.f);
			SetPhotoView(Eye, (Look - Eye).Rotation());
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
		};
		static double P26BootSum = 0.0, P26KeelSum = 0.0; static int32 P26WlN = 0;

		if (S2Step == 0)
		{
			if (S2T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
			bWxDrift = false; ClockSpeedIdx = 0; SetClockHour(11.5f); ForceWeatherState(0, 0.05f);
			if (AIronwakeStorm* St = AIronwakeStorm::Get(W)) { St->SetTarget(0.f, 0.05f); St->SetOpenImmediate(0.f); }
			// Step 1: glassy CALM sea waterline measurement & close-up (heading +65 ENE -> starboard +Y faces SSE noon sun; reefed topsails+jib + standing shrouds!)
			IronwakeSea::SetSeaScale(W, 0.12f);
			IronwakeFleet::SetFixedWind(305.f, 5.f);
			S2Place(Sea, 65.f, false);
			P->IWTelegraph(3);
			P26BootSum = P26KeelSum = 0.0; P26WlN = 0;
			UE_LOG(LogIronwake, Display, TEXT("IWP26 start ship=%s (%s) rigParts=%d"), *P->GetShipName(), *P->ShipId.ToString(), P->SailRig ? P->SailRig->BrigParts.Num() : 0);
			Next(); return;
		}
		if (S2Step == 1) // Calm-sea waterline settle + close-up capture
		{
			HoldYawTo(65.f);
			if (Since >= 7.f && Since < 14.f)
			{
				const FVector BootW = P->HullMesh->GetComponentTransform().TransformPosition(FVector(0.f, 0.f, 350.f)); // top of dark boot-top band below white Nelson gunport stripe
				const FVector KeelW = P->HullMesh->GetComponentTransform().TransformPosition(FVector::ZeroVector);
				const float SeaR = IronwakeSea::RenderedZ(W, BootW);
				P26BootSum += (BootW.Z - SeaR) / 100.0;
				P26KeelSum += (SeaR - KeelW.Z) / 100.0;
				++P26WlN;
			}
			if (Since >= 14.f && S2Shots == 0)
			{
				SunlitPhoto(FVector(1650.f, 3550.f, 420.f), FVector(-180.f, 0.f, 880.f), 48.f);
			}
			if (Since >= 15.5f && S2Shots == 0)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT calm_waterline ship=%s bootTop_above_sea=%.3f m keel_below_sea=%.3f m roll=%.2f deg (n=%d)"),
					*P->GetShipName(), P26WlN ? P26BootSum / P26WlN : 0.0, P26WlN ? P26KeelSum / P26WlN : 0.0, P->GetActorRotation().Roll, P26WlN);
				S2Shot(TEXT("P26_brig_waterline_calm"), true);
			}
			if (Since >= 17.f)
			{
				PhotoOff();
				// Step 2: Broad reach (TWA = -120 deg from port quarter, 16 kn wind, moderate sea) -> ALL 9 sails set, yards braced, spanker sheeted out to sunlit starboard!
				IronwakeSea::SetSeaScale(W, 0.65f);
				IronwakeFleet::SetFixedWind(305.f, 16.f);
				S2Place(Sea, 65.f, false);
				P->IWTelegraph(6);
				Next();
			}
			return;
		}
		if (S2Step == 2) // Broad-reach full sail: 3/4 bow & 3/4 stern captures
		{
			HoldYawTo(65.f);
			if (Since >= 24.f && S2Shots == 1) SunlitPhoto(FVector(3750.f, 3650.f, 1080.f), FVector(-180.f, 0.f, 1060.f), 50.f);
			if (Since >= 25.5f && S2Shots == 1)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT broad_reach kn=%.2f twa=%.1f awa=%.1f yardYaw=%.1f spankerYaw=%.1f headYaw=%.1f heel=%.2f roll=%.2f bellyM=%.2f luffCm=%.1f"),
					P->GetSpeedKnots(), P->TrueWindAngle, P->ApparentAngle,
					P->SailRig ? P->SailRig->GetYardYaw() : 0.f, P->SailRig ? P->SailRig->GetSpankerYaw() : 0.f, P->SailRig ? P->SailRig->GetHeadsailYaw() : 0.f,
					P->SailHeelDeg, P->GetActorRotation().Roll, P->SailRig ? P->SailRig->GetBellyM() : 0.f, P->SailRig ? P->SailRig->GetLastLuffCm() : 0.f);
				S2Shot(TEXT("P26_brig_broad_3q_bow"), true);
			}
			if (Since >= 27.f && S2Shots == 2) SunlitPhoto(FVector(-4150.f, 3850.f, 1180.f), FVector(-250.f, 0.f, 960.f), 50.f);
			if (Since >= 28.5f && S2Shots == 2) S2Shot(TEXT("P26_brig_broad_3q_stern"), true);
			if (Since >= 30.f)
			{
				// Step 3: Reefed Topsails (SetLevel = 3)
				P->IWTelegraph(3);
				Next();
			}
			return;
		}
		if (S2Step == 3) // Reefed vs Furled comparison from matching sunlit 3/4 bow camera
		{
			HoldYawTo(65.f);
			if (Since >= 5.5f && S2Shots == 3) SunlitPhoto(FVector(3550.f, 3550.f, 980.f), FVector(-220.f, 0.f, 1180.f), 52.f);
			if (Since >= 7.0f && S2Shots == 3)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT reefed tel=%s kn=%.2f yardYaw=%.1f spankerYaw=%.1f"),
					*P->GetTelegraphName(), P->GetSpeedKnots(), P->SailRig ? P->SailRig->GetYardYaw() : 0.f, P->SailRig ? P->SailRig->GetSpankerYaw() : 0.f);
				S2Shot(TEXT("P26_brig_reefed"), true);
				P->IWTelegraph(2); // Furl all sails
			}
			if (Since >= 13.5f && S2Shots == 4) SunlitPhoto(FVector(3550.f, 3550.f, 980.f), FVector(-220.f, 0.f, 1180.f), 52.f);
			if (Since >= 15.0f && S2Shots == 4)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT furled tel=%s kn=%.2f yardYaw=%.1f spankerYaw=%.1f"),
					*P->GetTelegraphName(), P->GetSpeedKnots(), P->SailRig ? P->SailRig->GetYardYaw() : 0.f, P->SailRig ? P->SailRig->GetSpankerYaw() : 0.f);
				S2Shot(TEXT("P26_brig_furled"), true);
			}
			if (Since >= 16.5f)
			{
				// Step 4: Luffing in irons (pinched at TWA = 18 deg inside 35 deg no-go zone)
				P->IWTelegraph(6);
				S2Place(Sea, 323.f, false); // Wind from 305 -> TWA = -18 deg
				Next();
			}
			return;
		}
		if (S2Step == 4) // Luffing ripple telemetry & capture
		{
			HoldYawTo(323.f);
			if (Since >= 8.f && S2Shots == 5) SunlitPhoto(FVector(3400.f, -3500.f, 960.f), FVector(-220.f, 0.f, 1180.f), 52.f);
			if (Since >= 9.5f && S2Shots == 5)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT luffing_irons twa=%.1f fill=%.2f bellyM=%.2f luffCm=%.1f spankerYaw=%.1f"),
					P->TrueWindAngle, P->SailFill, P->SailRig ? P->SailRig->GetBellyM() : 0.f, P->SailRig ? P->SailRig->GetLastLuffCm() : 0.f, P->SailRig ? P->SailRig->GetSpankerYaw() : 0.f);
				S2Shot(TEXT("P26_brig_luffing"), true);
			}
			if (Since >= 11.f)
			{
				PhotoOff();
				P->bAutoHelm = false;
				// Step 5: Harbour NPC Brig transit (park player in outer harbour roads so NPC wake FX & embodiment are active!)
				if (Nd)
				{
					IronwakeSea::SetSeaScale(W, 0.30f);
					IronwakeFleet::SetFixedWind(335.f, 14.f);
					S2Place(Nd->H2W(480.f, -210.f), 225.f, false);
					P->IWTelegraph(2);
					Nd->ForceLive(40);
					if (FNpcRec* Br = Nd->FindByDef(TEXT("brig"), 0))
					{
						Nd->TeleportRec(*Br, Nd->H2W(235.f, -34.f), 225.f, ENpcState::Transit, 4.5f);
						Nd->TestRoute(*Br, true);
					}
				}
				Next();
			}
			return;
		}
		if (S2Step == 5) // Harbour NPC brig transit captures
		{
			FNpcRec* Br = Nd ? Nd->FindByDef(TEXT("brig"), 0) : nullptr;
			AIronwakeNpcShip* B = Br ? Br->Body.Get() : nullptr;
			if (!B && Since < 10.f) return;
			if (B)
			{
				const FVector Pc = B->GetActorLocation();
				const FVector Fw = B->GetActorForwardVector(), Rt = B->GetActorRightVector();
				if (Since < 11.5f)
				{
					if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
					const FVector Eye = Pc + Fw * 4650.f - Rt * 4550.f + FVector(0.f, 0.f, 1280.f);
					SetPhotoView(Eye, (Pc + Fw * 450.f + FVector(0.f, 0.f, 1020.f) - Eye).Rotation());
					if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(52.f);
				}
				else
				{
					const FVector Eye = Pc - Fw * 4650.f - Rt * 3950.f + FVector(0.f, 0.f, 1380.f);
					SetPhotoView(Eye, (Pc + Fw * 550.f + FVector(0.f, 0.f, 960.f) - Eye).Rotation());
					if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(52.f);
				}
				if (Since >= 9.5f && S2Shots == 6)
				{
					UE_LOG(LogIronwake, Display, TEXT("IWP26 RESULT npc_brig name='%s' kn=%.1f state=%s movingParts=%d rigParts=%d"),
						*Br->Name, Br->SpeedKn, NpcStateName(Br->State), B->MovingParts, B->Rig ? B->Rig->BrigParts.Num() : 0);
					S2Shot(TEXT("P26_npc_brig_harbour_1"), true);
				}
				if (Since >= 13.5f && S2Shots == 7) S2Shot(TEXT("P26_npc_brig_harbour_2"), true);
			}
			if (Since >= 15.f)
			{
				PhotoOff();
				C->IWYard(1); // Open 4-ship Shipyard UI highlighting Curlew (Merchant brig)
				UGameplayStatics::SetGamePaused(W, false); // keep TickS2Test running for screenshot!
				Next();
			}
			return;
		}
		if (S2Step == 6) // 4-ship Shipyard UI capture
		{
			if (Since >= 2.0f && S2Shots == 8) S2Shot(TEXT("P26_shipyard_4ships"), false);
			if (Since >= 3.5f) { C->HideMenu(); Finish(); }
			return;
		}
		return;
	}
}

// P16 test director: -iwp16=measure|side|chase|resp|menu [-iwp16quit]. Silent offscreen runs, log lines "IWP16 ...",
// CSV in Saved/p16_*.csv, shots in Saved/shots. Deterministic code only.
#include "IronwakeVoyage.h"
#include "IronwakeStorm.h"
#include "IronwakeHydro.h"
#include "IronwakeShell.h"
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeSettings.h"
#include "IronwakeTest.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

void AIronwakeVoyage::TickP16Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	P16T += RealDt;
	const float Since = P16T - P16Mark;
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp16quit"));
	AIronwakeStorm* St = AIronwakeStorm::Get(W);
	const FVector Sea0(60000.f, 40000.f, 0.f);
	auto Next = [&]() { ++P16Step; P16Mark = P16T; };
	auto Append = [&](const TCHAR* File, const FString& S) { FFileHelper::SaveStringToFile(S, *(FPaths::ProjectSavedDir() / File), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append); };
	auto Shot = [&](const FString& Name, bool bHUD)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png"))), bHUD, false);
		UE_LOG(LogIronwake, Display, TEXT("IWP16 shot %s state=%.2f Hs=%.1f"), *Name, St ? St->GetLocalState() : -1.f, IronwakeSea::SigWaveHeightM());
	};
	auto Place = [&](float Yaw, int32 Tel)
	{
		const FVector V = FVector::ZeroVector;
		P->SetActorLocationAndRotation(FVector(Sea0.X, Sea0.Y, P->GetActorLocation().Z), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		P->HullBody->SetPhysicsLinearVelocity(V); P->HullBody->SetPhysicsAngularVelocityInDegrees(V);
		P->IWRudder(0); P->IWTelegraph(Tel);
	};
	auto SetSea = [&](float S) { SeaOverride = S; if (St) { St->SetTarget(S, 0.05f); St->SetOpenImmediate(S); } };
	// (enemy ships are removed at step 0)
	// Unattended: strip input so stray typing can't open the menu.
	if (P16Step == 0)
	{
		if (P16T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();   // no enemy ironclad in measurements
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(12.f); ForceWeatherState(0, 0.05f);
		Place(IronwakeSea::WaveTravelYaw() + 180.f - 65.f, 2);
		SetSea(0.f);
		UE_LOG(LogIronwake, Display, TEXT("IWP16 start mode=%s ship=%s cammotion=%d"), *P16Mode, *P->GetShipName(), IronwakeSettings::GetCameraMotion());
		Next(); return;
	}

	// ---------------------------------------------------------------- measure
	if (P16Mode == TEXT("measure"))
	{
		static const float Sts[4] = { 0.f, 1.f, 2.f, 3.f };
		const int32 Si = (P16Step - 1) / 2;   // step 1,2 -> state 0 ; 3,4 -> state 1 ...
		if (Si >= 4) { UE_LOG(LogIronwake, Display, TEXT("IWP16 DONE measure")); P16Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); return; }
		const bool bSettle = ((P16Step - 1) % 2) == 0;
		if (bSettle)
		{
			if (Since < 0.1f) { SetSea(Sts[Si]); P16N = 0; }
			Place(IronwakeSea::WaveTravelYaw() + 180.f - 65.f, 2);
			if (Since > 30.f) { UE_LOG(LogIronwake, Display, TEXT("IWP16 measure state=%.0f Hs=%.2f settled"), Sts[Si], IronwakeSea::SigWaveHeightM()); Next(); P16Buf.Empty(); }
			return;
		}
		// sample 10 points at 10 Hz for 120 s
		if (Since >= 120.f) { Append(TEXT("p16_measure.csv"), P16Buf); P16Buf.Empty(); Next(); return; }
		if (Since >= P16N * 0.1f)
		{
			++P16N;
			const float Tw = W->GetTimeSeconds();
			for (int32 i = 0; i < 10; ++i)
			{
				const FVector Pt(Sea0.X + 900.f * (i % 5) * 1.37f - 2000.f, Sea0.Y + 1700.f * (i / 5) + 130.f * i, 0.f);
				const float Plane = AIronwakeShell::WaterZAt(W, Pt);
				const float An = IronwakeSea::AnalyticZ(W, Pt) - Plane;
				const float Q = AIronwakeShell::WaterZAtWaves(W, Pt) - Plane;
				P16Buf += FString::Printf(TEXT("%.0f,%.2f,%d,%.1f,%.1f\n"), Sts[Si], Tw, i, An, Q);
			}
			if (P16N % 100 == 0) { Append(TEXT("p16_measure.csv"), P16Buf); P16Buf.Empty(); }
		}
		return;
	}

	// ---------------------------------------------------------------- side-on triplets (fixed striped post + hull)
	if (P16Mode == TEXT("side"))
	{
		static const float Sts[4] = { 0.f, 1.f, 2.f, 3.f };
		static FVector G = FVector::ZeroVector, Eye = FVector::ZeroVector, Look = FVector::ZeroVector; static float Plane = 0.f;
		const int32 Si = (P16Step - 1) / 5;   // per state: settle, build, shot a, b, c
		const int32 Ph = (P16Step - 1) % 5;
		if (Si >= 3) { UE_LOG(LogIronwake, Display, TEXT("IWP16 DONE side")); P16Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); return; }
		if (Ph == 0)
		{
			if (Since < 0.1f) { if (bPhoto) TogglePhoto(); SetSea(Sts[Si]); }
			Place(IronwakeSea::WaveTravelYaw() + 90.f, 2);   // beam-on to the swell
			if (Since > 35.f)
			{
				const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
				Plane = AIronwakeShell::WaterZAt(W, P->GetActorLocation());
				const FVector Fw = X.GetRotation().GetForwardVector(), Rt = X.GetRotation().GetRightVector();
				const FVector S = P->GetActorLocation();
				G = FVector(S.X, S.Y, Plane) + Fw * 3800.f;
				const FVector M = FVector(S.X, S.Y, Plane) + Fw * 1300.f;
				Eye = M + Rt * 4000.f + FVector(0, 0, 800.f);
				Look = FVector(M.X, M.Y, Eye.Z - 80.f) ;
				Next();
			}
			return;
		}
		if (Ph == 1)
		{
			static TArray<TWeakObjectPtr<UStaticMeshComponent>> Bands;
			for (auto& B : Bands) if (B.IsValid()) B->DestroyComponent();
			Bands.Reset();
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			for (int32 k = -6; k < 8; ++k)
			{
				UStaticMeshComponent* Cc = NewObject<UStaticMeshComponent>(this); Cc->SetStaticMesh(Cube); Cc->SetupAttachment(RootComponent);
				Cc->RegisterComponent(); Cc->SetWorldLocation(FVector(G.X, G.Y, Plane + k * 100.f + 50.f)); Cc->SetWorldScale3D(FVector(1.5f, 1.5f, 0.98f));
				Cc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Cc->SetCastShadow(false);
				if (UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this)) { M->SetVectorParameterValue(TEXT("Color"), (k & 1) ? FLinearColor(1.f, 0.1f, 0.1f) : FLinearColor(1.f, 1.f, 0.9f)); Cc->SetMaterial(0, M); }
				Bands.Add(Cc);
			}
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			Next(); return;
		}
		// shots at Since = 2, 4, 6 s after build
		{
			const int32 Kx = Ph - 2;   // 0,1,2
			SetPhotoView(Eye, (Look - Eye).Rotation());
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(70.f);
			if (Since > 2.f + 0.f)
			{
				const float Qp = AIronwakeShell::WaterZAtWaves(W, G) - Plane, Ap = IronwakeSea::AnalyticZ(W, G) - Plane;
				const FString Name = FString::Printf(TEXT("P16_side_s%d_%c"), (int32)Sts[Si], TEXT('a') + Kx);
				Shot(Name, false);
				UE_LOG(LogIronwake, Display, TEXT("IWP16 sidepost %s postWater=%.2f m analytic=%.2f m hullZ=%.2f m pitch=%.2f roll=%.2f"), *Name, Qp / 100.f, Ap / 100.f, (P->GetActorLocation().Z - Plane) / 100.f, P->GetActorRotation().Pitch, P->GetActorRotation().Roll);
				Next();
			}
		}
		return;
	}

	// ---------------------------------------------------------------- chase camera in rough sea
	if (P16Mode == TEXT("chase"))
	{
		static float Acc = 0.f; static int32 Shots = 0; static bool bSet = false;
		float S = 2.f; FParse::Value(FCommandLine::Get(), TEXT("iwp16sea="), S);
		if (!bSet) { bSet = true; SetSea(S); Place(IronwakeSea::WaveTravelYaw() + 180.f - 65.f, 3); }
		const float WarmUp = 40.f;
		if (bPhoto) TogglePhoto();
		C->SetControlRotation(FRotator(-2.5f, P->GetActorRotation().Yaw + 20.f, 0.f));
		Acc += RealDt;
		if (P16T > WarmUp && Acc >= 0.1f)
		{
			Acc = 0.f;
			const FRotator CR = C->PlayerCameraManager->GetCameraRotation();
			const FVector CL = C->PlayerCameraManager->GetCameraLocation();
			const FRotator HR = P->GetActorRotation();
			Append(TEXT("p16_chase.csv"), FString::Printf(TEXT("%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.1f,%.1f\n"), P16T, HR.Pitch, HR.Roll, CR.Pitch - C->GetControlRotation().Pitch, CR.Roll, CL.Z, P->GetActorLocation().Z, P->GetSpeedKnots()));
		}
		const float T0 = WarmUp + 10.f;
		for (int32 k = 0; k < 3; ++k)
			if (Shots == k && P16T > T0 + k * 1.5f) { Shot(FString::Printf(TEXT("P16_chase_%c"), TEXT('a') + k), false); ++Shots; }
		if (P16T > WarmUp + 80.f) { UE_LOG(LogIronwake, Display, TEXT("IWP16 DONE chase")); P16Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		return;
	}

	// ---------------------------------------------------------------- ship response (stopped, beam-to-sea)
	if (P16Mode == TEXT("resp"))
	{
		static double Ac[8]; static int32 N = 0; static float Acc = 0.f; static float Mn[3], Mx[3];
		const int32 Si = (P16Step - 1) / 2; const bool bSettle = ((P16Step - 1) % 2) == 0;
		static const float Sts[3] = { 0.f, 1.f, 2.f };
		if (Si >= 3) { UE_LOG(LogIronwake, Display, TEXT("IWP16 DONE resp")); P16Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); return; }
		if (bSettle)
		{
			if (Since < 0.1f) SetSea(Sts[Si]);
			Place(IronwakeSea::WaveTravelYaw() + 180.f - 65.f, 2);
			if (Since > 30.f) { FMemory::Memzero(Ac, sizeof(Ac)); N = 0; Acc = 0.f; for (int i = 0; i < 3; ++i) { Mn[i] = 1e9f; Mx[i] = -1e9f; } Next(); }
			return;
		}
		Acc += RealDt;
		if (Acc >= 0.1f)
		{
			Acc = 0.f;
			const FVector L = P->GetActorLocation(); const FRotator R = P->GetActorRotation();
			const float Plane = AIronwakeShell::WaterZAt(W, L);
			const float Hv = (L.Z - Plane) / 100.f; // heave about the flat plane (m)
			const float V[3] = { Hv, R.Pitch, R.Roll };
			for (int i = 0; i < 3; ++i) { Ac[i * 2] += V[i]; Ac[i * 2 + 1] += V[i] * V[i]; Mn[i] = FMath::Min(Mn[i], V[i]); Mx[i] = FMath::Max(Mx[i], V[i]); }
			++N;
		}
		if (Since >= 120.f)
		{
			auto Sd = [&](int k) { const double M = Ac[k * 2] / N; return FMath::Sqrt(FMath::Max(Ac[k * 2 + 1] / N - M * M, 0.0)); };
			UE_LOG(LogIronwake, Display, TEXT("IWP16 RESP ship=%s state=%.0f Hs=%.2f n=%d heave_sd=%.3f m (p2p %.2f) pitch_sd=%.3f deg (p2p %.2f) roll_sd=%.3f deg (p2p %.2f)"),
				*P->GetShipName(), Sts[Si], IronwakeSea::SigWaveHeightM(), N, Sd(0), Mx[0] - Mn[0], Sd(1), Mx[1] - Mn[1], Sd(2), Mx[2] - Mn[2]);
			Next();
		}
		return;
	}

	// ---------------------------------------------------------------- CAMERA MOTION menu shots
	if (P16Mode == TEXT("menu"))
	{
		if (P16Step == 1) { C->ShowMenu(1); Next(); return; }
		if (P16Step == 2 && Since > 2.5f) { Shot(TEXT("P16_menu"), true); Next(); return; }
		if (P16Step == 3 && Since > 2.5f) { C->HideMenu(); if (!C->IsHelpOpen()) C->ToggleHelp(); Next(); return; }
		if (P16Step == 4 && Since > 2.5f) { Shot(TEXT("P16_help"), true); Next(); return; }
		if (P16Step == 5 && Since > 2.5f) { UE_LOG(LogIronwake, Display, TEXT("IWP16 DONE menu")); P16Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		return;
	}
	P16Mode.Empty();
}

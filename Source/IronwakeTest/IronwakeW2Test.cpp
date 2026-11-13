// W2 test director + STORM AT SEA scenario.
//   -iwstorm           : STORM AT SEA start (menu entry too): frigate ~6 km out of Greyhaven, storm already up, sails reefed.
//   -iww2=all [-iww2quit] : one batched standalone run: calm -> rough -> storm captures, ship-in-storm stats, ship FX shots.
//   -iww2=clip         : storm, 10 s of fixed-step frames into Saved/w2clip (run with -benchmark -fps=30).
//   -iww2=bench        : storm, chase camera, no captures (FPS profiling with -csvCaptureFrames).
#include "IronwakeVoyage.h"
#include "IronwakeStorm.h"
#include "IronwakeHydro.h"
#include "IronwakeShell.h"
#include "IronwakeFleet.h"
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Components/BoxComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UObjectIterator.h"
namespace { float W2Peak = 0.f; }

namespace
{
	// Open water between the Needles and St Brannoc, ~5.5 km out of Greyhaven harbour, inside the tessellated ocean tile.
	const FVector StormSea(60000.f, 40000.f, 0.f);
}

void AIronwakeVoyage::StartStormAtSea()
{
	AIronwakeShipPawn* P = Player();
	if (!P) return;
	const float Down = IronwakeSea::WaveTravelYaw();
	// Head into the seas a little off the bow (wind and waves from ~30 deg off the starboard bow), close reach under storm canvas.
	S2Place(StormSea, Down + 180.f - 65.f, false);
	SetWeather(4, 0.5f);
	if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld())) St->SetTarget(4.f, 0.5f);
	P->IWTelegraph(4);
	ShowToast(TEXT("STORM AT SEA: 6 km out of Greyhaven. Sails reefed. Hold her head to the sea."), 8.f);
	UE_LOG(LogIronwake, Display, TEXT("IWW2 STORM AT SEA start at %s hdg=%.0f"), *StormSea.ToString(), P->GetHeadingDeg());
}

void AIronwakeVoyage::TickW2Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	if (bStormStart) { bStormStart = false; StartStormAtSea(); if (W2Mode.IsEmpty()) return; }
	if (W2Mode.IsEmpty()) return;
	W2T += RealDt;
	const float Since = W2T - W2Mark;
	auto Next = [&]() { ++W2Step; W2Mark = W2T; };
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iww2quit"));
	AIronwakeStorm* St = AIronwakeStorm::Get(W);
	// Unattended: strip input so stray typing can't open the menu.
	if (FMath::Fmod(W2T, 1.f) < RealDt + 1e-3f)
		if (ULocalPlayer* LP = C->GetLocalPlayer()) if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Sub->ClearAllMappings();
	auto Shot = [&](const TCHAR* Name, bool bHUD)
	{
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (FString(Name) + TEXT(".png")));
		FScreenshotRequest::RequestScreenshot(File, bHUD, false);
		UE_LOG(LogIronwake, Display, TEXT("IWW2 shot %s state=%.2f Hs=%.1f m pitch=%.2f roll=%.2f kn=%.1f bowSub=%.2f spume=%d"), *File, St ? St->GetLocalState() : -1.f,
			IronwakeSea::SigWaveHeightM(), P->GetActorRotation().Pitch, P->GetActorRotation().Roll, P->GetSpeedKnots(), P->GetBowSubmergeM(), St ? St->SpumeLive : 0);
	};
	auto PhotoAt = [&](const FVector& RelEye, const FVector& RelLook)
	{
		if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		FVector Eye = X.TransformPosition(RelEye); const FVector Look = X.TransformPosition(RelLook);
		Eye.Z = FMath::Max(Eye.Z, IronwakeSea::FastZ(W, Eye) + 400.f);
		SetPhotoView(Eye, (Look - Eye).Rotation());
	};
	auto PhotoOff = [&]() { if (bPhoto) TogglePhoto(); };
	auto Chase = [&](float Yaw, float Pitch, int32 Zoom) { PhotoOff(); P->ZoomIndex = Zoom; C->SetControlRotation(FRotator(Pitch, P->GetActorRotation().Yaw + Yaw, 0.f)); };
	// Ship-in-sea stats: hull heave/pitch/roll vs the sea, rendered sea vs boot-top amidships (riding, not floating/clipping).
	static double Acc[10]; static int32 N = 0;
	auto Reset = [&]() { FMemory::Memzero(Acc, sizeof(Acc)); N = 0; };
	auto Sample = [&]()
	{
		const FVector L = P->GetActorLocation();
		const float Sea = IronwakeSea::RenderedZ(W, L);
		const float Pi = P->GetActorRotation().Pitch, Ro = P->GetActorRotation().Roll;
		Acc[0] += Sea; Acc[1] += Sea * Sea; Acc[2] += L.Z; Acc[3] += L.Z * L.Z; Acc[4] += Pi; Acc[5] += Pi * Pi; Acc[6] += Ro; Acc[7] += Ro * Ro;
		const float Gap = (L.Z - Sea); Acc[8] += Gap; Acc[9] += Gap * Gap; ++N;
	};
	auto Sd = [&](int32 k) { const double M = Acc[k] / FMath::Max(N, 1); return FMath::Sqrt(FMath::Max(Acc[k + 1] / FMath::Max(N, 1) - M * M, 0.0)); };
	auto Report = [&](const TCHAR* Tag)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWW2 RESULT %s n=%d Hs=%.1f m maxH=%.1f sea_sd=%.2f m hull_heave_sd=%.2f m pitch_sd=%.2f deg roll_sd=%.2f deg hull-minus-sea mean=%.2f sd=%.2f m wind=%.0f kn sailset=%d"),
			Tag, N, IronwakeSea::SigWaveHeightM(), 0.f, Sd(0) / 100.0, Sd(2) / 100.0, Sd(4), Sd(6), Acc[8] / FMath::Max(N, 1) / 100.0, Sd(8) / 100.0, IronwakeFleet::WindKn(W), P->TelegraphIndex);
	};

	if (W2Mode == TEXT("gauge"))
	{
		// Numeric rendered-vs-query test: fixed post (1 m magenta/green bands, -12..+12 m about the ocean plane) 300 m from the
		// ship, fixed level camera 70 m away. Each 0.5 s: screenshot + screen row of every band edge + query/analytic z at the post.
		static FVector G; static TArray<TWeakObjectPtr<UStaticMeshComponent>> Bands; static float Plane = 0.f;
		if (W2Step == 0) { StartStormAtSea();
			{ FString At; if (FParse::Value(FCommandLine::Get(), TEXT("iwgaugeat="), At)) { FString A, B; At.Split(TEXT(","), &A, &B); S2Place(FVector(FCString::Atof(*A), FCString::Atof(*B), 0.f), 45.f, false); P->IWTelegraph(2); } }
			float GS = 4.f; FParse::Value(FCommandLine::Get(), TEXT("iwgaugestate="), GS); SetWeather(GS >= 3.5f ? 4 : 1, 0.5f); if (St) St->SetTarget(GS, 0.5f); Next(); return; }
		if (W2Step == 1 && Since > 25.f)
		{
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			G = X.TransformPosition(FVector(0.f, -30000.f, 0.f));
			Plane = AIronwakeShell::WaterZAt(W, G);
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			for (int32 k = -12; k < 12; ++k)
			{
				UStaticMeshComponent* Cc = NewObject<UStaticMeshComponent>(this); Cc->SetStaticMesh(Cube); Cc->SetupAttachment(RootComponent);
				Cc->RegisterComponent(); Cc->SetWorldLocation(FVector(G.X, G.Y, Plane + k * 100.f + 50.f)); Cc->SetWorldScale3D(FVector(1.2f, 1.2f, 0.98f));
				Cc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Cc->SetCastShadow(false);
				if (UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this)) { M->SetVectorParameterValue(TEXT("Color"), (k & 1) ? FLinearColor(1.f, 0.f, 1.f) : FLinearColor(0.f, 1.f, 0.f)); Cc->SetMaterial(0, M); }
				Bands.Add(Cc);
			}
			const FVector Eye = G + FVector(7000.f, 0.f, 0.f); 
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			SetPhotoView(FVector(Eye.X, Eye.Y, Plane + 1600.f), (FVector(G.X, G.Y, Plane) - FVector(Eye.X, Eye.Y, Plane + 1600.f)).Rotation());
			UE_LOG(LogIronwake, Display, TEXT("IWW2G post at %s plane=%.0f maxWaveHeight=%.0f cm Hs=%.1f m"), *G.ToString(), Plane, IronwakeSea::SigWaveHeightM(), IronwakeSea::SigWaveHeightM());
			W2N = 0; Next(); return;
		}
		if (W2Step == 2 && Since > 2.f)
		{
			if (FMath::Fmod(Since, 0.5f) < RealDt + 1e-4f)
			{
				FString Rows;
				for (int32 k = -12; k <= 12; ++k) { FVector2D S2; C->ProjectWorldLocationToScreen(FVector(G.X, G.Y, Plane + k * 100.f), S2, false); Rows += FString::Printf(TEXT("%.1f,"), S2.Y); }
				float J = 1.f;
				const float Q = AIronwakeShell::WaterZAtWaves(W, G), A = IronwakeSea::FastZ(W, G, &J);
				FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("w2gauge") / FString::Printf(TEXT("g%03d.png"), W2N)), false, false);
				FVector2D SX; C->ProjectWorldLocationToScreen(G, SX, false);
				UE_LOG(LogIronwake, Display, TEXT("IWW2G f=%03d t=%.2f query=%.0f fast=%.0f rendOffset=%.0f col=%.1f rows=%s"), W2N, W->GetTimeSeconds(), Q - Plane, A - Plane, AIronwakeShell::RenderedSeaOffsetCm, SX.X, *Rows);
				++W2N;
			}
			if (W2N >= 40) { W2Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		}
		return;
	}
	if (W2Mode == TEXT("bench") || W2Mode == TEXT("clip"))
	{
		if (W2Step == 0) { StartStormAtSea(); Next(); return; }
		if (W2Step == 1)
		{
			Chase(150.f, -7.f, 2);
			if (W2Mode == TEXT("clip") && Since > 35.f) { W2N = 0; Next(); }
			return;
		}
		if (W2Step == 2)
		{
			// Low 3/4 bow camera in the storm, fixed-step frames.
			{ const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			  FVector Eye = X.TransformPosition(FVector(8000.f - Since * 150.f, 12000.f, 0.f)); Eye.Z = FMath::Max(Eye.Z, IronwakeSea::FastZ(W, Eye) + 600.f);
			  const FVector Look = X.TransformPosition(FVector(0.f, 0.f, 600.f));
			  if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			  SetPhotoView(Eye, (Look - Eye).Rotation()); }
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("w2clip") / FString::Printf(TEXT("f%04d.png"), W2N++)), false, false);
			if (W2N >= 300) { UE_LOG(LogIronwake, Display, TEXT("IWW2 clip end")); W2Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		}
		return;
	}

	if (W2Mode == TEXT("layers"))
	{
		// Two-layer water bug hunt: normal start (no teleport, no weather override), tour autopilot, MODERATE. Every 2 s list
		// every non-ocean primitive whose bounds contain a vertical line at the ship's bow/mid/stern near the sea surface
		// (shelf discs, surf ribbons, foam planes...), with its Z vs the rendered sea. Hull-side captures at 60/120/180 s.
		if (W2Step == 0) { if (W2T > 4.f) { if (St) St->SetTarget(1.f, 0.5f); EngageAutopilot(3); Next(); } return; }
		if (FMath::Fmod(Since, 2.f) < RealDt + 1e-4f)
		{
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			for (float Xm : { P->GetHullX1() - 5.f, 0.f, P->GetHullX0() + 5.f })
			{
				const FVector Q = X.TransformPosition(FVector(Xm * 100.f, 0.f, 0.f));
				const float Sea = IronwakeSea::RenderedZ(W, Q);
				FString Hits;
				for (TObjectIterator<UPrimitiveComponent> It; It; ++It)
				{
					UPrimitiveComponent* Pc = *It;
					if (!Pc || Pc->GetWorld() != W || !Pc->IsRegistered() || !Pc->IsVisible() || Pc->GetOwner() == P) continue;
					const FBox B = Pc->Bounds.GetBox();
					if (Q.X < B.Min.X || Q.X > B.Max.X || Q.Y < B.Min.Y || Q.Y > B.Max.Y) continue;
					if (B.Max.Z < Sea - 1500.f || B.Min.Z > Sea + 1500.f) continue;
					if (B.GetSize().X > 2000000.f) continue; // ocean / zone
					Hits += FString::Printf(TEXT(" [%s.%s z=%.0f..%.0f]"), *GetNameSafe(Pc->GetOwner()), *Pc->GetName(), B.Min.Z - Sea, B.Max.Z - Sea);
				}
				if (!Hits.IsEmpty()) UE_LOG(LogIronwake, Display, TEXT("IWLAY t=%.0f x=%.0f at %s sea=%.0f deck=%.0f :%s"), Since, Xm, *Q.ToString(), Sea, P->GetActorLocation().Z - Sea, *Hits);
			}
			if (FMath::Fmod(Since, 10.f) < RealDt + 1e-4f) UE_LOG(LogIronwake, Display, TEXT("IWLAY tel t=%.0f loc=%s kn=%.1f state=%.2f"), Since, *P->GetActorLocation().ToString(), P->GetSpeedKnots(), St ? St->GetLocalState() : -1.f);
		}
		static int32 ShotsDone = 0;
		const float ShotT[3] = { 60.f, 120.f, 180.f };
		if (ShotsDone < 3 && Since > ShotT[ShotsDone] - 2.5f)
		{
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			FVector Eye = X.TransformPosition(FVector(P->HullLengthM * 15.f, -P->HullLengthM * 90.f, 0.f)); Eye.Z = IronwakeSea::RenderedZ(W, Eye) + 250.f;
			FVector Look = X.GetLocation(); Look.Z = Eye.Z;
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			SetPhotoView(Eye, (Look - Eye).Rotation());
			if (Since > ShotT[ShotsDone]) { Shot(*FString::Printf(TEXT("LAY_hull_%d"), ShotsDone), false); ++ShotsDone; }
		}
		else if (bPhoto) TogglePhoto();
		if (Since > 190.f) { W2Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		return;
	}
	if (W2Mode == TEXT("discs"))
	{
		// W4: identify blown-out discs: STORM, composited frames (no debug) from the chase cam and the two w4cov views.
		// Run once plain, once with -iwnospume / -iwnolamps to A/B the suspects.
		if (W2Step == 0) { if (W2T > 3.f) { S2Place(StormSea, IronwakeSea::WaveTravelYaw() + 180.f - 65.f, false); P->IWTelegraph(6); SetWeather(4, 0.5f); if (St) St->SetTarget(4.f, 0.5f); Next(); } return; }
		FString Tag; FParse::Value(FCommandLine::Get(), TEXT("iwtag="), Tag);
		if (W2Step == 1) { Chase(150.f, -6.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 40.f) { Shot(*FString::Printf(TEXT("W4D%s_chase"), *Tag), false); Next(); } return; }
		if (W2Step == 2) { PhotoAt(FVector(-5000.f, -8000.f, 2600.f), FVector(15000.f, 12000.f, -600.f)); if (Since > 2.5f) { Shot(*FString::Printf(TEXT("W4D%s_v0"), *Tag), false); Next(); } return; }
		if (W2Step == 3) { PhotoAt(FVector(-4000.f, -14000.f, 12000.f), FVector(6000.f, 8000.f, 0.f)); if (Since > 2.5f) { Shot(*FString::Printf(TEXT("W4D%s_v1"), *Tag), false); Next(); } return; }
		if (W2Step == 4 && Since > 2.f) { W2Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		return;
	}
	if (W2Mode == TEXT("w4cov"))
	{
		// W4 numeric foam coverage: per sea state (ROUGH, STORM) the same two fixed views are shot with the foam mask forced
		// to binary white (IW_Debug 7) and with no foam (8); the pixel difference is the foam coverage (w4cov.py). Plus the
		// streak (6), lace (5) and cap (2) layers from the oblique view for the stripe check.
		auto Sea = [&](float S, int32 Wx) { SetWeather(Wx, 0.5f); if (St) St->SetTarget(S, 0.5f); };
		static const float States[2] = { 2.f, 4.f };
		static const TCHAR* SN[2] = { TEXT("rough"), TEXT("storm") };
		static const float Modes[5] = { 7.f, 8.f, 2.f, 5.f, 6.f };
		const int32 Si = W2Step / 20, Sub = W2Step % 20;
		if (W2Step == 0) { if (W2T > 3.f) { S2Place(StormSea, IronwakeSea::WaveTravelYaw() + 180.f - 65.f, false); P->IWTelegraph(2); Sea(States[0], 0); W2Step = 1; W2Mark = W2T; } return; }
		if (Si >= 2) { IronwakeSea::SetFoamDebug(-1.f); W2Mode.Empty(); UE_LOG(LogIronwake, Display, TEXT("IWW4 cov DONE")); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); return; }
		if (Sub == 0) { Sea(States[Si], Si == 1 ? 4 : 0); W2Step = Si * 20 + 1; W2Mark = W2T; return; }
		if (Sub == 1) { Chase(150.f, -7.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 30.f) Next(); return; }
		// Sub 2..: view v = (Sub-2)/5, mode m = (Sub-2)%5 ; views: 0 = oblique 26 m up (whitecaps), 1 = steep 120 m up
		const int32 K = Sub - 2, V = K / 5, Mi = K % 5;
		if (V >= 2) { W2Step = (Si + 1) * 20; W2Mark = W2T; if (Si + 1 < 2) { Sea(States[Si + 1], 4); W2Step = (Si + 1) * 20 + 1; } return; }
		if (V == 0) PhotoAt(FVector(-5000.f, -8000.f, 2600.f), FVector(15000.f, 12000.f, -600.f));
		else PhotoAt(FVector(-4000.f, -14000.f, 12000.f), FVector(6000.f, 8000.f, 0.f));
		IronwakeSea::SetFoamDebug(Modes[Mi]);
		UGameplayStatics::SetGlobalTimeDilation(this, 0.0001f); // freeze the sea so mask/no-foam pairs match pixel for pixel
		if (Since > 1.2f)
		{
			Shot(*FString::Printf(TEXT("W4C_%s_v%d_m%d"), SN[Si], V, (int32)Modes[Mi]), false);
			UE_LOG(LogIronwake, Display, TEXT("IWW4 cov shot %s v%d mode %d state=%.2f"), SN[Si], V, (int32)Modes[Mi], St ? St->GetLocalState() : -1.f);
			Next();
			if (Mi == 4) UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		}
		return;
	}
	if (W2Mode == TEXT("w3") || W2Mode == TEXT("w3storm"))
	{
		// W3 batch: CALM (chase, bow close) -> MODERATE (wake high) -> ROUGH (bow close, whitecaps) -> STORM (chase, whitecaps,
		// spume/rain, trough, slam) + stats. Day lighting below STORM so the captures judge the water, not the night.
		auto Sea = [&](float S, int32 Wx) { SetWeather(Wx, 0.5f); if (St) St->SetTarget(S, 0.5f); };
		const float X1 = P->GetHullX1() * 100.f, X0 = P->GetHullX0() * 100.f;
		const float Lm = P->HullLengthM;
		auto ShotN = [&](const TCHAR* N, bool bHUD) { Shot(N, bHUD); };
		switch (W2Step)
		{
		case 0:
			if (W2T > 3.f)
			{
				S2Place(StormSea, IronwakeSea::WaveTravelYaw() + 180.f - 65.f, false);
				P->IWTelegraph(6);
				if (W2Mode == TEXT("w3storm")) { Sea(4.f, 4); Reset(); W2Step = 40; W2Mark = W2T; break; }
				Sea(0.f, 0); Next();
			}
			break;
		case 1: Chase(150.f, -7.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 28.f) { ShotN(TEXT("W3_calm_chase"), false); Next(); } break;
		case 2: PhotoAt(FVector(X1 + 2600.f, 2400.f, 500.f), FVector(X1 - 900.f, 0.f, 150.f)); if (Since > 2.5f) { ShotN(TEXT("W3_bow_close_calm"), false); Next(); } break;
		case 3: if (Since > 1.5f) { PhotoOff(); Sea(1.f, 0); Next(); } break;
		case 4: Chase(150.f, -7.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 30.f) { ShotN(TEXT("W3_moderate_chase"), false); Next(); } break;
		case 5: PhotoAt(FVector(X0 - 16000.f, 7000.f, 20000.f), FVector(X0 - 22000.f, 0.f, 0.f)); if (Since > 2.5f) { ShotN(TEXT("W3_wake_high"), false); W2Step = 11; W2Mark = W2T; } break;
		case 11: // W4: 200 m up, 300 m astern, looking forward-down at the ship with the whole wake in frame
			PhotoAt(FVector(X0 - 30000.f, 9000.f, 20000.f), FVector(X0 - 5000.f, 0.f, 0.f)); if (Since > 2.5f) { ShotN(TEXT("W3_wake_high2"), false); W2Step = 6; W2Mark = W2T; } break;
		case 6: if (Since > 1.5f) { PhotoOff(); Sea(2.f, 0); Next(); } break;
		case 7: Chase(150.f, -7.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 30.f) { ShotN(TEXT("W3_rough_chase"), false); Next(); } break;
		case 8: PhotoAt(FVector(X1 + 2600.f, 2400.f, 500.f), FVector(X1 - 900.f, 0.f, 150.f)); if (Since > 2.5f) { ShotN(TEXT("W3_bow_close_rough"), false); Next(); } break;
		case 9: PhotoAt(FVector(-6000.f, -9000.f, 3500.f), FVector(20000.f, 14000.f, 0.f)); if (Since > 2.5f) { ShotN(TEXT("W3_whitecaps_rough"), false); Next(); } break;
		case 10: if (Since > 1.5f) { PhotoOff(); Sea(4.f, 4); Reset(); W2Step = 40; W2Mark = W2T; } break;
		// ---- STORM
		case 40: Chase(150.f, -6.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 22.f) Sample(); if (Since > 45.f) { Report(TEXT("STORM")); ShotN(TEXT("W3_storm_chase"), false); Next(); } break;
		case 41: Chase(150.f, -6.f, AIronwakeShipPawn::DefaultZoomIndex); if (Since > 2.f) { ShotN(TEXT("W3_storm_chase_hud"), true); Next(); } break;
		case 42: PhotoAt(FVector(-5000.f, -8000.f, 2600.f), FVector(15000.f, 12000.f, -600.f)); if (Since > 2.5f) { ShotN(TEXT("W3_whitecaps"), false); Next(); } break;
		case 43: // spume + rain: low, looking across the wind past the bow
			PhotoAt(FVector(X1 + 4000.f, -5000.f, 700.f), FVector(X1 - 3000.f, 3000.f, 900.f)); if (Since > 3.f) { ShotN(TEXT("W3_spume_rain"), false); Next(); } break;
		case 44: // trough: eye 3 m over the sea 140 m off the beam; wait until the ship sits in a trough below the eye's sea
		{
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			FVector Eye = X.TransformPosition(FVector(Lm * 20.f, -11000.f, 0.f)); Eye.Z = IronwakeSea::FastZ(W, Eye) + 250.f;
			FVector Look = X.TransformPosition(FVector(0.f, 0.f, 0.f)); Look.Z = Eye.Z + 600.f;
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			SetPhotoView(Eye, (Look - Eye).Rotation());
			const FVector Mid = FMath::Lerp(Eye, X.GetLocation(), 0.5f);
			const float CrestOver = IronwakeSea::FastZ(W, Mid) - IronwakeSea::FastZ(W, X.GetLocation());
			W2Peak = FMath::Max(W2Peak, CrestOver);
			const bool bTrough = IronwakeSea::FastZ(W, X.GetLocation()) < IronwakeSea::FastZ(W, Eye) - 350.f;
			if ((Since > 3.f && CrestOver > 650.f && bTrough) || Since > 70.f) { UE_LOG(LogIronwake, Display, TEXT("IWW3 trough crest-over-ship=%.1f m (peak %.1f)"), CrestOver / 100.f, W2Peak / 100.f); ShotN(TEXT("W3_storm_trough"), false); W2Peak = 0.f; Next(); }
			break;
		}
		case 45: // slam: bow quarter, wait for the bow to meet a crest
			PhotoAt(FVector(X1 + 3500.f, 3000.f, 900.f), FVector(X1 - 900.f, 0.f, 300.f));
			if ((Since > 3.f && P->GetBowSubmergeM() > -0.6f) || Since > 45.f) { UE_LOG(LogIronwake, Display, TEXT("IWW3 slam bowSub=%.2f"), P->GetBowSubmergeM()); W2Mark = W2T - 100.f; Next(); }
			break;
		case 46: if (W2T - W2Mark > 100.4f) { ShotN(TEXT("W3_slam"), false); Next(); } break;
		case 47:
			if (Since > 2.f)
			{
				PhotoOff();
				UE_LOG(LogIronwake, Display, TEXT("IWW3 DONE spume_total=%d strikes=%d"), St ? St->SpumeTotal : 0, St ? St->Strikes : 0);
				W2Mode.Empty();
				if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
			}
			break;
		default: break;
		}
		return;
	}

	// ---------------------------------------------------------------- all: calm -> rough -> storm, captures + stats
	switch (W2Step)
	{
	case 0:
		if (W2T > 3.f && (W2Mode == TEXT("storm") || W2Mode == TEXT("quick"))) { S2Place(StormSea, IronwakeSea::WaveTravelYaw() + 180.f - 65.f, false); W2Step = 6; W2Mark = W2T; break; }
		if (W2T > 3.f) { S2Place(StormSea, IronwakeSea::WaveTravelYaw() + 180.f - 65.f, false); SetWeather(1, 0.5f); if (St) St->SetTarget(0.f, 0.5f); P->IWTelegraph(5); Reset(); Next(); }
		break;
	case 1: // CALM
		Chase(140.f, -9.f, 2);
		if (Since > 14.f) Sample();
		if (Since > 30.f) { Report(TEXT("CALM")); PhotoAt(FVector(-9000.f, 11000.f, 1800.f), FVector(1500.f, 0.f, 0.f)); Next(); }
		break;
	case 2: if (Since > 1.5f) { Shot(TEXT("W2_calm"), false); Next(); } break;
	case 3: if (Since > 1.5f) { PhotoOff(); SetWeather(2, 0.5f); if (St) St->SetTarget(2.f, 0.5f); Reset(); Next(); } break;
	case 4: // ROUGH
		Chase(140.f, -8.f, 2);
		if (Since > 18.f) Sample();
		if (Since > 34.f) { Report(TEXT("ROUGH")); PhotoAt(FVector(-9000.f, 11000.f, 1600.f), FVector(1500.f, 0.f, 0.f)); Next(); }
		break;
	case 5: if (Since > 1.5f) { Shot(TEXT("W2_rough"), false); Next(); } break;
	case 6: if (Since > 1.5f) { PhotoOff(); SetWeather(4, 0.5f); if (St) St->SetTarget(4.f, 0.5f); P->IWTelegraph(5); Reset(); Next(); } break;
	case 7: // STORM: settle, measure, chase-camera storm shot (game view, HUD on)
		Chase(150.f, -7.f, 2);
		if (Since > 20.f) Sample();
		if (W2Mode == TEXT("quick") && Since > 14.f) { Shot(TEXT("Q_chase"), false); W2Step = 20; W2Mark = W2T; break; }
		if (Since > 45.f) { Report(TEXT("STORM")); Shot(TEXT("W2_storm_1"), true); Next(); }
		break;
	case 8: // wait for the bow to rise on a wave face, then the low quarter shot
		{ const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		  FVector Eye = X.TransformPosition(FVector(9000.f, 13000.f, 0.f)); Eye.Z = IronwakeSea::FastZ(W, Eye) + 500.f;
		  const FVector Look = X.TransformPosition(FVector(500.f, 0.f, 600.f));
		  if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		  SetPhotoView(Eye, (Look - Eye).Rotation()); }
		if ((Since > 3.f && P->GetActorRotation().Pitch > 2.5f) || Since > 40.f) { Shot(TEXT("W2_storm_2"), false); Next(); }
		break;
	case 9: // bow close, full sail (storm)
		if (Since > 1.f) { const float X1 = P->GetHullX1() * 100.f; PhotoAt(FVector(X1 + 4500.f, 3200.f, 800.f), FVector(X1 - 800.f, 0.f, 200.f)); }
		if (Since > 4.f) { Shot(TEXT("W2_bow_slam"), false); Next(); }
		break;
	case 10: // moderate, full sail: close bow + high wake
		if (Since < RealDt * 1.5f) { SetWeather(0, 0.5f); if (St) St->SetTarget(1.f, 0.5f); P->IWTelegraph(6); }
		if (Since > 20.f) { const float X1 = P->GetHullX1() * 100.f; PhotoAt(FVector(X1 + 4000.f, 3000.f, 700.f), FVector(X1 - 1000.f, 0.f, 150.f)); } else Chase(150.f, -8.f, 2);
		if (Since > 24.f) { Shot(TEXT("W2_bow_close"), false); Next(); }
		break;
	case 11:
		Chase(175.f, -38.f, P->ZoomStepsL.Num() - 1);
		if (Since > 6.f) { Shot(TEXT("W2_wake_high"), false); Next(); }
		break;
	case 12:
		if (Since > 2.f)
		{
			PhotoOff();
			UE_LOG(LogIronwake, Display, TEXT("IWW2 DONE spume_total=%d strikes=%d"), St ? St->SpumeTotal : 0, St ? St->Strikes : 0);
			W2Mode.Empty();
			if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		}
		break;
	case 20: // quick: low quarter view, then quit
		{ const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		  FVector Eye = X.TransformPosition(FVector(9000.f, 13000.f, 0.f)); Eye.Z = IronwakeSea::FastZ(W, Eye) + 900.f;
		  const FVector Look = X.TransformPosition(FVector(500.f, 0.f, 600.f));
		  if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		  SetPhotoView(Eye, (Look - Eye).Rotation()); }
		if (Since > 2.5f && W2N == 0) { Shot(TEXT("Q_low"), false); W2N = 1; }
		if (Since > 5.f) { W2Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); }
		break;
	default: break;
	}
}

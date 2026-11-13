// P9 test director: sails the selected hull at flank into a beach, a rock stack, the breakwater, a moored ship, then backs off
// astern; a last run sends the autopilot round an island. Logs IWP9 RESULT lines and takes side + overhead frames at contact.
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeCollision.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeShell.h"
#include "IronwakeTest.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AIronwakeVoyage::TickP9Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
	AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld());
	if (!C || !P || !Co || !Ar) return;
	P9T += RealDt;
	if (FMath::Fmod(P9T, 1.f) < RealDt + 1e-3f)
		if (ULocalPlayer* LP = C->GetLocalPlayer()) if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Sub->ClearAllMappings();
	const float Since = P9T - P9Mark;
	const FString ShipN = P->ShipId.ToString();
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp9quit"));

	struct FRun { const TCHAR* Name; };
	static const TCHAR* const Order[] = { TEXT("beach"), TEXT("stack"), TEXT("breakwater"), TEXT("moored"), TEXT("ship"), TEXT("autopilot") };
	TArray<const TCHAR*> Runs;
	for (const TCHAR* N : Order) if (P9Mode == TEXT("all") || P9Mode == N) Runs.Add(N);
	if (Runs.Num() == 0) { P9Mode.Empty(); return; }

	auto Place = [&](const FVector& Loc, float Yaw)
	{
		P->SetActorLocationAndRotation(FVector(Loc.X, Loc.Y, P->GetActorLocation().Z), FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
		P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		P->IWRudder(0);
	};
	auto Dir = [](float YawDeg) { return FVector(FMath::Cos(FMath::DegreesToRadians(YawDeg)), FMath::Sin(FMath::DegreesToRadians(YawDeg)), 0.f); };
	const FTransform HT(FRotator(0.f, 45.f, 0.f), FVector(HarbourC.X, HarbourC.Y, 0.f));
	auto HL = [&](float X, float Y) { FVector W = HT.TransformPosition(FVector(X * 100.f, Y * 100.f, 0.f)); W.Z = 0.f; return W; };
	auto Photo = [&](const FVector& Eye, const FVector& Look, float Fov)
	{
		if (!bPhoto) TogglePhoto();
		SetPhotoView(Eye, (Look - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};
	auto Snap = [&](const FString& N) { C->IWShot(N); ++P9Shots; UE_LOG(LogIronwake, Display, TEXT("IWP9 shot %s"), *N); };
	auto Next = [&]() { ++P9Step; P9Mark = P9T; };
	const FString RunName = Runs.IsValidIndex(P9Run) ? FString(Runs[P9Run]) : FString();

	if (P9Step == 0)
	{
		if (P9T < 4.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		SetWeather(0, 0.1f);
		IronwakeSea::SetSeaState(GetWorld(), 0.f);
		if (AIronwakeShipPawn* En = P->FindEnemy()) { En->HoldFireSeconds = 1.0e9f; En->HoldFireRangeCm = 0.f; En->SetActorLocation(En->GetActorLocation() + FVector(4.0e6f, 4.0e6f, 0.f), false, nullptr, ETeleportType::TeleportPhysics); }
		UE_LOG(LogIronwake, Display, TEXT("IWP9 start ship=%s draught=%.1f m solids=%d hulls=%d"), *ShipN, P->DraughtCm / 100.f, Co->NumObbs(), Co->NumHulls());
		Next(); return;
	}
	if (P9Run >= Runs.Num())
	{
		UE_LOG(LogIronwake, Display, TEXT("IWP9 DONE ship=%s shots=%d"), *ShipN, P9Shots);
		P9Mode.Empty();
		if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		return;
	}

	// ------------------------------------------------------------------ ramming runs: step 1 place, 2 sail, 3 settle+frames, 4 astern
	if (RunName != TEXT("autopilot"))
	{
		if (P9Step == 1)
		{
			if (bPhoto) TogglePhoto();
			FVector StartAt; float Yaw = 0.f; FString Desc;
			if (RunName == TEXT("beach"))
			{
				const FIsle& I = Ar->Isles[0]; // Kestrel Isle (wooded, beach cove)
				const float By = I.CoveYaw;
				StartAt = FVector(I.C.X, I.C.Y, 0.f) + Dir(By) * 90000.f; Yaw = By + 180.f; Desc = I.Name + TEXT(" beach cove");
			}
			else if (RunName == TEXT("stack"))
			{
				const FIsle& I = Ar->Isles[3]; // Gannet Stacks
				StartAt = FVector(I.C.X, I.C.Y, 0.f) + Dir(200.f) * 90000.f; Yaw = 20.f; Desc = I.Name;
			}
			else if (RunName == TEXT("breakwater"))
			{
				StartAt = HL(60.f, -520.f); Yaw = 45.f + 90.f; Desc = TEXT("Greyhaven south breakwater");
			}
			else if (RunName == TEXT("moored"))
			{
				StartAt = HL(330.f, 10.f); Yaw = 45.f + 180.f; Desc = TEXT("moored SS Wren + pier (ram from seaward along the pier)");
			}
			else
			{
				// Ship-vs-ship: a static merchant in open water 700 m ahead, rammed bow-on.
				const FVector Sea(-50000.f, -300000.f, 0.f);
				StartAt = Sea; Yaw = 0.f; Desc = TEXT("static test merchant (ship vs ship)");
				if (!TestTarget.IsValid())
				{
					FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AIronwakeMerchant* Mt = GetWorld()->SpawnActor<AIronwakeMerchant>(AIronwakeMerchant::StaticClass(), FTransform(FRotator(0.f, 0.f, 0.f), Sea + FVector(70000.f, 0.f, AIronwakeShell::WaterZAt(GetWorld(), Sea))), SP);
					if (Mt) { Mt->ShipName = TEXT("test target"); TestTarget = Mt; }
				}
			}
			Place(StartAt, Yaw);
			P->IWSteam(1);
			P->IWTelegraph(6);
			if (RunName == TEXT("ship")) P->IWTelegraph(5);
			P9Start = StartAt; P9Hits0 = P->CollisionHits; P9Ground0 = P->GroundingCount; P9Contact = -1.f;
			UE_LOG(LogIronwake, Display, TEXT("IWP9 RUN %s ship=%s from (%.0f,%.0f) m yaw %.0f -> %s"), *RunName, *ShipN, StartAt.X / 100.f, StartAt.Y / 100.f, Yaw, *Desc);
			Next(); return;
		}
		if (P9Step == 2)
		{
			const bool bContact = P->CollisionHits > P9Hits0 || P->GroundingCount > P9Ground0;
			if (bContact && P9Contact < 0.f)
			{
				P9Contact = P9T;
				UE_LOG(LogIronwake, Display, TEXT("IWP9 CONTACT %s ship=%s after %.0f s, %.0f m run, speed %.1f kn, aground=%d hits=%d tag=%s depth=%.1f m"), *RunName, *ShipN, Since, FVector::Dist2D(P->GetActorLocation(), P9Start) / 100.f,
					P->GetSpeedKnots(), P->bAground, P->CollisionHits - P9Hits0, *P->LastHitTag, P->MinDepthCm / 100.f);
			}
			if (P9Contact > 0.f && P9T - P9Contact > 2.2f) { Next(); return; }
			if (Since > 170.f) { UE_LOG(LogIronwake, Display, TEXT("IWP9 RESULT %s ship=%s NO CONTACT in 170 s: FAIL (at %.0f,%.0f m)"), *RunName, *ShipN, P->GetActorLocation().X / 100.f, P->GetActorLocation().Y / 100.f); P9Step = 5; P9Mark = P9T; }
			return;
		}
		if (P9Step == 3)
		{
			// Frames: side (hull flank, level) and overhead, ship held in photo mode.
			const FVector L = P->GetActorLocation();
			const FVector Fw = P->GetActorForwardVector().GetSafeNormal2D(), Rt(-Fw.Y, Fw.X, 0.f);
			const float Ln = P->HullLengthM * 100.f;
			const FVector Bow = L + Fw * Ln * 0.3f;
			if (Since < 0.1f) P->IWTelegraph(2);
			if (Since > 0.3f && P9Shots % 2 == 0 && Since < 0.6f) Photo(Bow + Rt * Ln * 0.55f + Fw * Ln * 0.25f + FVector(0, 0, Ln * 0.5f), Bow + Fw * Ln * 0.12f, 60.f);
			if (Since > 3.5f && P9Shots % 2 == 0) Snap(FString::Printf(TEXT("P9_%s_%s_side"), *ShipN, *RunName));
			if (Since > 5.5f && P9Shots % 2 == 1 && Since < 5.8f) Photo(Bow + FVector(0, 0, FMath::Max(Ln * 1.6f, 6000.f)), Bow, 60.f);
			if (Since > 9.f && P9Shots % 2 == 1)
			{
				Snap(FString::Printf(TEXT("P9_%s_%s_top"), *ShipN, *RunName));
				const FIwHit End = Co->TestPose(P, P->CollShape, FVector2D(L.X, L.Y), P->GetActorRotation().Yaw);
				UE_LOG(LogIronwake, Display, TEXT("IWP9 FINAL %s ship=%s run=%.0f m pen=%.0f cm (%s) minDepth=%.1f m draught=%.1f m aground=%d hits=%d speed=%.2f kn"), *RunName, *ShipN, FVector::Dist2D(L, P9Start) / 100.f, End.bHit ? End.Pen : 0.f, End.bHit ? *End.Tag : TEXT("-"),
					P->MinDepthCm / 100.f, P->DraughtCm / 100.f, P->bAground, P->CollisionHits - P9Hits0, P->GetSpeedKnots());
				Next();
			}
			return;
		}
		if (P9Step == 4)
		{
			// Back off with astern: frees in deep water, hull clear.
			if (bPhoto) TogglePhoto();
			if (P9Back0 == 0.f) { P->IWTelegraph(0); P9Back0 = 1.f; P9BackAt = P->GetActorLocation(); }
			const FVector L = P->GetActorLocation();
			const bool bFree = !P->bAground && P->MinDepthCm > P->DraughtCm * 1.5f && FVector::Dist2D(L, P9BackAt) > P->HullLengthM * 100.f * 0.6f && P->GetSpeedKnots() < -0.5f;
			const FIwHit End = Co->TestPose(P, P->CollShape, FVector2D(L.X, L.Y), P->GetActorRotation().Yaw);
			if ((bFree && !End.bHit && Since > 3.f) || Since > 120.f)
			{
				P->IWTelegraph(2);
				UE_LOG(LogIronwake, Display, TEXT("IWP9 RESULT %s ship=%s astern: %s after %.0f s, backed %.0f m, speed %.1f kn, depth %.1f m, overlap=%d"), *RunName, *ShipN, (bFree && !End.bHit) ? TEXT("FREED PASS") : TEXT("STUCK FAIL"), Since, FVector::Dist2D(L, P9BackAt) / 100.f, P->GetSpeedKnots(), P->MinDepthCm / 100.f, End.bHit);
				P9Step = 5; P9Mark = P9T;
			}
			return;
		}
		if (P9Step == 5)
		{
			if (Since < 2.f) return;
			if (bPhoto) TogglePhoto();
			P->IWTelegraph(2);
			++P9Run; P9Step = 1; P9Mark = P9T; P9Shots = 0; P9Back0 = 0.f;
			return;
		}
		return;
	}

	// ------------------------------------------------------------------ autopilot behind an island: Fort Grimsby -> across Heron Holm/Anvil cluster
	if (P9Step == 1)
	{
		if (bPhoto) TogglePhoto();
		// StartAt the far side of Kestrel Isle to the other far side (straight line crosses the island).
		const FIsle& I = Ar->Isles[0];
		const FVector A = FVector(I.C.X, I.C.Y, 0.f) + Dir(90.f) * (I.R * 2.6f), B = FVector(I.C.X, I.C.Y, 0.f) + Dir(270.f) * (I.R * 2.6f);
		Place(A, 270.f); P->IWSteam(1);
		Waypoints.Reset(); Waypoints.Add(B);
		if (!bFastSail) ToggleFastSail();
		P->IWTelegraph(4);
		EngageAutopilot(2);
		P9Start = A; P9Hits0 = P->CollisionHits; P9Ground0 = P->GroundingCount; P9Back0 = 1e9f; bP9Auto = true;
		UE_LOG(LogIronwake, Display, TEXT("IWP9 RUN autopilot ship=%s %s (%.0f,%.0f) -> (%.0f,%.0f) m across %s"), *ShipN, *I.Name, A.X / 100.f, A.Y / 100.f, B.X / 100.f, B.Y / 100.f, *I.Name);
		Next(); return;
	}
	if (P9Step == 2)
	{
		const FVector L = P->GetActorLocation();
		for (const FIsle& I : Ar->Isles) P9Back0 = FMath::Min(P9Back0, (float)(FVector::Dist2D(L, FVector(I.C.X, I.C.Y, 0.f)) - I.R));
		const float Done = FVector::Dist2D(L, Waypoints.Num() ? Waypoints[0] : L);
		static float Acc = 0.f; Acc += RealDt;
		if (Acc > 20.f) { Acc = 0.f; UE_LOG(LogIronwake, Display, TEXT("IWP9 autopilot t=%.0f pos=(%.0f,%.0f) m to go %.0f m kn=%.1f depth=%.1f m aground=%d hits=%d"), Since, L.X / 100.f, L.Y / 100.f, Done / 100.f, P->GetSpeedKnots(), P->MinDepthCm / 100.f, P->bAground, P->CollisionHits - P9Hits0); }
		const bool bArrived = bRouteDone || Done < 30000.f;
		if (bArrived || Since > 400.f || P->bAground || P->CollisionHits > P9Hits0)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP9 RESULT autopilot ship=%s %s in %.0f s: groundings=%d impacts=%d closest shore %.0f m, end %.0f m from goal -> %s"), *ShipN, bArrived ? TEXT("ARRIVED") : TEXT("STOPPED"), Since,
				P->GroundingCount - P9Ground0, P->CollisionHits - P9Hits0, P9Back0 / 100.f, Done / 100.f, (bArrived && P->GroundingCount == P9Ground0 && P->CollisionHits == P9Hits0) ? TEXT("PASS") : TEXT("FAIL"));
			DisengageAutopilot(TEXT("test done"));
			if (bFastSail) ToggleFastSail();
			P->IWTelegraph(2);
			P9Step = 3; P9Mark = P9T;
		}
		return;
	}
	if (P9Step == 3)
	{
		if (Since < 2.f) return;
		++P9Run; P9Step = 1; P9Mark = P9T;
	}
}

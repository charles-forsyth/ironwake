// P19 + P27 test director: -iwp19=shots|p27|npc6 -iwship=<id> -iwkns=3,8,14 [-iwsail] [-iwsea=0] [-iwturn] [-iwp19quit]
// Per speed: hold, log numeric Havelock LUT measurements (synchronized with M_IW_WaterRoot), then bow / side / chase / aerial / top shots.
#include "IronwakeVoyage.h"
#include "IronwakeStorm.h"
#include "IronwakeHydro.h"
#include "IronwakeShipWave.h"
#include "IronwakeFleet.h"
#include "IronwakeNpc.h"
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "EngineUtils.h"

void AIronwakeVoyage::TickP19Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	UWorld* W = GetWorld();
	P19T += RealDt;
	const float Since = P19T - P19Mark;
	auto Next = [&]() { ++P19Step; P19Mark = P19T; };
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp19quit"));
	const bool bP27 = (P19Mode == TEXT("p27"));
	const bool bNpc6 = (P19Mode == TEXT("npc6"));
	const FString Prefix = bP27 ? TEXT("P27_") : TEXT("P19_");
	const FVector Sea(-50000.f, -300000.f, 0.f);
	TArray<FString> Kns; { FString S = bP27 ? TEXT("3,8,14") : TEXT("8,13,16"); FParse::Value(FCommandLine::Get(), TEXT("iwkns="), S, false); S.ParseIntoArray(Kns, TEXT(",")); }
	float SeaS = bP27 ? 0.12f : 0.f; FParse::Value(FCommandLine::Get(), TEXT("iwsea="), SeaS);
	const bool bSail = FParse::Param(FCommandLine::Get(), TEXT("iwsail")), bTurn = FParse::Param(FCommandLine::Get(), TEXT("iwturn"));
	const float L = P->HullLengthM;
	const FString Hull = P->ShipId.ToString() + (bSail ? TEXT("_sail") : TEXT("")) + (bTurn ? TEXT("_turn") : TEXT(""));
	AIronwakeStorm* St = AIronwakeStorm::Get(W);
	auto Shot = [&](const FString& N, bool bHUD) { FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (N + TEXT(".png"))), bHUD, false); UE_LOG(LogIronwake, Display, TEXT("IWP19 shot %s kn=%.2f slots=%d"), *N, P->GetSpeedKnots(), IronwakeSea::ActiveShipWaveSlots()); };
	auto Cam = [&](const FVector& RelEye, const FVector& RelLook, float Fov = 62.f)
	{
		if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		const FVector Eye = X.TransformPosition(RelEye), Look = X.TransformPosition(RelLook);
		SetPhotoView(Eye, (Look - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};

	// P27 multi-ship 7-source verification mode (-iwp19=npc6): player + 6 moving NPC ships in formation under aerial camera
	if (bNpc6)
	{
		AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(W);
		if (P19Step == 0)
		{
			if (P19T < 3.5f || !Nd || !Nd->bInit) return;
			if (!C->bStarted) C->StartGame(true);
			bWxDrift = false; ClockSpeedIdx = 0; SetClockHour(12.f); ForceWeatherState(0, 0.05f);
			if (St) { St->SetTarget(0.15f, 0.05f); St->SetOpenImmediate(0.15f); }
			IronwakeSea::SetSeaScale(W, 0.35f);
			IronwakeFleet::SetFixedWind(340.f, 16.f);
			Nd->ForceLive(40); Nd->MarkAllKnown();
			S2Place(Sea, -120.f, false);
			P->IWSteam(1);
			if (P->TelegraphKnots.Num() > 5) { P->TelegraphKnots[5] = 12.f; P->IWTelegraph(5); }
			// Place 6 diverse NPC steamers & sailing brigs on open-sea parallel tracks around the player
			const FVector Fw = FRotator(0.f, -120.f, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
			const FVector2D Offs[6] = { { 120.f, -140.f }, { 80.f, 145.f }, { -90.f, -260.f }, { -110.f, 270.f }, { 240.f, -20.f }, { -230.f, 60.f } };
			int32 Placed = 0;
			for (FNpcRec& R : Nd->Recs)
			{
				if (Placed >= 6) break;
				if (!R.Def || R.Def->bAir || R.Def->LengthM < 20.f) continue;
				const FVector Pos = Sea + Fw * Offs[Placed].X * 100.f + Rt * Offs[Placed].Y * 100.f;
				Nd->TeleportRec(R, Pos, -120.f, ENpcState::Patrol, FMath::Min(R.Def->MaxKn, 9.5f));
				Nd->SpawnBody(R);
				if (AIronwakeNpcShip* B = R.Body.Get())
				{
					B->V = FMath::Min(R.Def->MaxKn, 9.5f) * 51.4444f * 1.5f;
					B->EngineKn = FMath::Min(R.Def->MaxKn, 9.5f);
					B->SetFx(true);
				}
				R.DesYaw = -120.f; R.DesKn = FMath::Min(R.Def->MaxKn, 9.5f);
				++Placed;
			}
			Next(); return;
		}
		if (P19Step == 1)
		{
			// Keep all 6 NPC ships steaming steadily along -120 deg during the 24 s wake build-up
			int32 Held = 0;
			if (Nd) for (FNpcRec& R : Nd->Recs) if (R.Body.IsValid() && !R.Def->bAir && FVector::Dist2D(R.Pos, P->GetActorLocation()) < 90000.f && Held < 6)
			{
				R.State = ENpcState::Drift; R.bHold = false; R.DesYaw = -120.f; R.DesKn = FMath::Min(R.Def->MaxKn, 9.5f);
				if (AIronwakeNpcShip* B = R.Body.Get()) { B->EngineKn = R.DesKn; B->V = FMath::Max(B->V, R.DesKn * 51.4444f * 1.5f); }
				++Held;
			}
			Cam(FVector(-38000.f, 16000.f, 31000.f), FVector(-4000.f, 0.f, 0.f), 64.f);
			if (Since > 24.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWP27 active_lut_sources=%d total_registered=%d"), IronwakeSea::ActiveShipWaveSlots(), IronwakeSea::ShipWaveCount());
				Shot(TEXT("P27_npc6_aerial"), false);
				Next();
			}
			return;
		}
		if (Since > 2.f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP19 DONE npc6 slots=%d"), IronwakeSea::ActiveShipWaveSlots());
			P19Mode.Empty();
			if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		}
		return;
	}

	if (P19Step == 0)
	{
		if (P19T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (TActorIterator<AIronwakeShipPawn> It(W); It; ++It) if (*It != P) It->Destroy();
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(11.5f); ForceWeatherState(0, 0.05f);
		SeaOverride = SeaS; if (St) { St->SetTarget(SeaS, 0.05f); St->SetOpenImmediate(SeaS); }
		IronwakeSea::SetSeaScale(W, SeaS > 0.01f ? FMath::Lerp(0.25f, 1.f, FMath::Clamp(SeaS, 0.f, 1.f)) : 0.25f);
		if (St) { St->SetTarget(SeaS, 0.05f); St->SetOpenImmediate(SeaS); }
		IronwakeFleet::SetFixedWind(340.f, bSail ? 16.f : 0.f);
		S2Place(Sea, -120.f, false);
		P->IWSteam(bSail ? 0 : 1);
		UE_LOG(LogIronwake, Display, TEXT("IWP19 start %s mode=%s L=%.0f kns=%d sea=%.2f sail=%d"), *Hull, *P19Mode, L, Kns.Num(), SeaS, bSail);
		Next(); return;
	}
	const int32 Ki = P19Spd;
	if (Ki >= Kns.Num()) { UE_LOG(LogIronwake, Display, TEXT("IWP19 DONE %s"), *Hull); P19Mode.Empty(); if (bQuit) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false); return; }
	const float Kn = FCString::Atof(*Kns[Ki]);
	const FString Tag = FString::Printf(TEXT("%s_%02.0fkn"), *Hull, Kn);
	if (P19Step == 1)   // set speed (exact under both steam and pure sail!), hold for wake development
	{
		if (Since < 0.1f)
		{
			if (Ki > 0 && !bTurn) S2Place(Sea, -120.f, false);
			if (!bSail)
			{
				P->IWSteam(1);
				if (P->TelegraphKnots.Num() > 5) { P->TelegraphKnots[5] = Kn; P->IWTelegraph(5); }
			}
			else
			{
				P->IWSteam(0);
				// Heading -120 deg (240) with wind from 340 deg => TWA = +100 deg broad/beam reach (Polar ~ 1.0)
				const float Polar = FMath::Max(IronwakeFleet::Polar(100.f), 0.85f);
				int32 SailSet = 6; float Plan = 1.f;
				if (Kn <= 4.5f) { SailSet = 3; Plan = 0.30f; P->bReefOverride = false; }
				else if (Kn <= 9.5f) { SailSet = 5; Plan = 0.90f; P->bReefOverride = false; }
				else { SailSet = 6; Plan = 1.00f; P->bReefOverride = true; }
				const float RawFromExp = -14.f * FMath::Loge(FMath::Max(1.f - Kn / 17.5f, 0.06f));
				const float RawNeed = FMath::Max(Kn, RawFromExp);
				const float WindNeed = FMath::Clamp(RawNeed / (0.72f * Polar * FMath::Sqrt(Plan)), 4.f, 34.f);
				IronwakeFleet::SetFixedWind(340.f, WindNeed);
				P->IWTelegraph(SailSet);
			}
			if (bTurn) P->IWRudder(2);
			else P->CenterRudder();
			// Seed initial hull velocity near target so heavy hulls settle cleanly at exact speed during Hold
			if (P->HullBody && P->HullBody->IsSimulatingPhysics())
			{
				P->EngineOrderKnots = Kn;
				P->HullBody->SetPhysicsLinearVelocity(P->GetActorForwardVector().GetSafeNormal2D() * (Kn * 51.4444f * P->SpeedCompression));
			}
		}
		const float Hold = bP27 ? FMath::Clamp(L * 0.32f, 22.f, 38.f) : FMath::Max(40.f, L * 0.9f);
		if (Since > Hold) { UE_LOG(LogIronwake, Display, TEXT("IWP19 hold done %s kn=%.2f"), *Tag, P->GetSpeedKnots()); Next(); }
		return;
	}
	if (P19Step == 2)   // numeric measurements on the live Havelock LUT source
	{
		FIwShipWaveSrc Sw; const FVector Lc = P->GetActorLocation(); const FVector Fw = P->GetActorForwardVector().GetSafeNormal2D();
		Sw.PosCm = FVector2D(Lc.X, Lc.Y); Sw.CosH = Fw.X; Sw.SinH = Fw.Y;
		Sw.V = FMath::Max(P->GetSpeedKnots(), 0.f) * 0.514444f * P->SpeedCompression; Sw.L = P->GetHullProf().Length(); Sw.X1 = P->GetHullProf().X1;
		float MaxB = 0.f; for (float B : P->GetHullProf().HalfB) MaxB = FMath::Max(MaxB, B);
		Sw.HalfBeam = MaxB;
		if (P->ShipDef) { Sw.Draft = P->ShipDef->DraftM; Sw.Freeboard = P->ShipDef->FreeboardM; Sw.CrestK = P->ShipDef->WaveCrestK; Sw.TroughK = P->ShipDef->WaveTroughK; Sw.QuarterK = P->ShipDef->WaveQuarterK; Sw.WedgeK = P->ShipDef->WaveWedgeK; Sw.Bluff = P->ShipDef->WaveBluff; }
		Sw.ThrustSteam = (!P->HasSails() || P->bAuxSteam) ? FMath::Clamp(P->GetSpeedKnots() / 13.f, 0.f, 1.15f) : 0.f;
		Sw.RollOutM = FMath::Sin(FMath::DegreesToRadians(FMath::Abs(P->GetActorRotation().Roll))) * Sw.Freeboard * 0.65f;
		const float Lw = Sw.L, Fn = Sw.V / FMath::Sqrt(9.81f * Lw);
		const auto At = [&](float U, float Y, float* Parts, float* Fm) { const FVector2D Pt = Sw.PosCm + (FVector2D(Fw.X, Fw.Y) * (Sw.X1 - U) + FVector2D(-Fw.Y, Fw.X) * Y) * 100.f; return IronwakeSea::ShipWaveEval(Sw, Pt, 0.f, Fm, Parts); };
		float Pa[4]; float BowMax = 0.f, BowU = 0.f, TrMin = 0.f, TrU = 0.f, QMax = 0.f, QU = 0.f;
		float InsideHullFoamMax = 0.f;
		for (float U = -0.1f * Lw; U < 1.0f * Lw; U += 0.01f * Lw)
		{
			const float HbAt = P->GetHullProf().HalfBreadthAt(Sw.X1 - U);
			const float Y = FMath::Max(HbAt, 0.f) + Sw.RollOutM + 0.65f;
			At(U, Y, Pa, nullptr);
			if (Pa[0] > BowMax) { BowMax = Pa[0]; BowU = U; }
			if (Pa[1] < TrMin) { TrMin = Pa[1]; TrU = U; }
			if (Pa[2] > QMax) { QMax = Pa[2]; QU = U; }
			if (U > 0.05f * Lw && U < 0.95f * Lw && HbAt > 0.4f)
			{
				float FmIn = 0.f;
				At(U, HbAt * 0.85f, nullptr, &FmIn);
				InsideHullFoamMax = FMath::Max(InsideHullFoamMax, FmIn);
			}
		}
		double AngSum = 0.0; int32 AngN = 0;
		for (float Ub : { 1.4f, 1.8f, 2.3f, 2.8f })
		{
			float Best = 0.f, BestY = 0.f;
			for (float Y = Sw.HalfBeam; Y < Ub * Lw * 0.6f; Y += 0.25f) { At(Ub * Lw, Y, Pa, nullptr); if (FMath::Abs(Pa[3]) > Best) { Best = FMath::Abs(Pa[3]); BestY = Y; } }
			if (Best > 1e-3f) { AngSum += FMath::RadiansToDegrees(FMath::Atan2(BestY, Ub * Lw)); ++AngN; }
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP19 NUM %s kn=%.2f Fn=%.3f L=%.1f freeboard=%.2f draft=%.2f thrust=%.2f | bow crest %.2f m (%.0f%% freeboard) at %.1f%% LWL | trough %.2f m (%.1f%% draft) at %.1f%% LWL | quarter crest %.2f m at %.1f%% LWL | wedge half angle %.1f deg (n=%d, expected %.1f) | insideHullFoamMax=%.4f"),
			*Tag, P->GetSpeedKnots(), Fn, Lw, Sw.Freeboard, Sw.Draft, Sw.ThrustSteam, BowMax, 100.f * BowMax / FMath::Max(Sw.Freeboard, 0.1f), 100.f * BowU / Lw, TrMin, -100.f * TrMin / FMath::Max(Sw.Draft, 0.1f), 100.f * TrU / Lw, QMax, 100.f * QU / Lw, AngN ? AngSum / AngN : 0.0, AngN, IronwakeSea::WedgeHalfAngleDeg(Fn), InsideHullFoamMax);
		Next(); return;
	}
	if (P19Step == 3)   // hull-side waterline (proves bow crest + clear midbody trough + zero foam climbing dark hull plating!)
	{
		if (bP27) Cam(FVector(L * 6.f, L * 54.f, FMath::Max(260.f, L * 6.5f)), FVector(L * 2.f, 0.f, 60.f), 58.f);
		else Cam(FVector(L * 38.f, L * 46.f, FMath::Max(400.f, L * 10.f)), FVector(L * 22.f, 0.f, 0.f), 65.f);
		if (Since > 1.5f) { Shot(Prefix + Tag + TEXT("_side"), false); Next(); }
		return;
	}
	if (P19Step == 4)   // astern chase camera framing twin-screw prop-wash boil & inner Kelvin wake (avoids low-angle SSR hull-occlusion void)
	{
		if (bP27) Cam(FVector(-L * 150.f, L * 16.f, FMath::Max(900.f, L * 42.f)), FVector(-L * 32.f, 0.f, 0.f), 60.f);
		else if (Since > 0.2f) Cam(FVector(-L * 85.f, L * 45.f, FMath::Max(700.f, L * 28.f)), FVector(L * 20.f, 0.f, 0.f), 65.f);
		if (Since > (bP27 ? 1.6f : 3.3f)) { Shot(Prefix + Tag + TEXT("_chase"), false); Next(); }
		return;
	}
	if (P19Step == 5)   // top-down / high oblique wake aerial
	{
		if (bP27)
		{
			// High oblique astern aerial (matches REF 08 frigate aerial wake: frames ship + 4.5 L of echelon arms, transverse troughs & prop wash)
			Cam(FVector(-L * 310.f, L * 92.f, FMath::Max(1600.f, L * 235.f)), FVector(-L * 95.f, 0.f, 0.f), 60.f);
			if (Since > 1.6f) { Shot(Prefix + Tag + TEXT("_aerial"), false); Next(); }
		}
		else
		{
			const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
			const FVector Eye = X.TransformPosition(FVector(-L * 80.f, 0.f, L * 340.f));
			if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			SetPhotoView(Eye, FRotator(-90.f, P->GetActorRotation().Yaw, 0.f));
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(60.f);
			if (Since > 1.5f) { Shot(Prefix + Tag + TEXT("_top"), false); Next(); }
		}
		return;
	}
	if (P19Step == 6)   // bow close-up
	{
		Cam(FVector(L * 36.f, L * 23.f, FMath::Max(240.f, L * 5.8f)), FVector(L * 26.f, 0.f, 90.f), 60.f);
		if (Since > 1.5f) { Shot(Prefix + Tag + TEXT("_bow"), false); Next(); }
		return;
	}
	if (P19Step == 7 && bP27)   // also capture top-down wake for 1:1 P19_top comparison!
	{
		const FTransform X(FRotator(0.f, P->GetActorRotation().Yaw, 0.f), P->GetActorLocation());
		const FVector Eye = X.TransformPosition(FVector(-L * 115.f, 0.f, FMath::Max(2400.f, L * 330.f)));
		if (!bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
		SetPhotoView(Eye, FRotator(-86.f, P->GetActorRotation().Yaw, 0.f));
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(60.f);
		if (Since > 1.5f) { Shot(Prefix + Tag + TEXT("_top"), false); Next(); }
		return;
	}
	if (P19Step >= (bP27 ? 8 : 7)) { if (bPhoto) TogglePhoto(); ++P19Spd; P19Step = 1; P19Mark = P19T; return; }
}

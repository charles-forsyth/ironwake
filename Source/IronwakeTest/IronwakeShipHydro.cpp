// W1: heavy-ship hydrostatics, pushed-water bow wave, water audio, W1 test director.
#include "IronwakeShipPawn.h"
#include "IronwakeHydro.h"
#include "IronwakeShipWave.h"
#include "IronwakeShell.h"
#include "IronwakeFleet.h"
#include "IronwakeTest.h"
#include "IronwakeVoyage.h"
#include "BuoyancyComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"

namespace
{
	constexpr float KnCmS = 51.4444f;
	// Hull-frame stations for the waterplane fit (fraction of waterline length) x 3 across (centre + both sides).
	constexpr int32 NStations = 9;
}

// ---------------------------------------------------------------------------- hydrostatics
// Replaces the plugin pontoons (cork-like: each sphere answers its own ripple). The hull is a rigid waterplane:
// sample the wave surface at 9 stations x 3 across, weight each sample by local waterplane breadth, and least-squares
// fit a plane (heave, pitch, roll). Short chop averages out over 125 m; only swell longer than the hull moves it.
// The hull then follows that plane as a damped 2nd-order system with the natural periods of a ~9,000 t ironclad
// (heave ~8 s, pitch ~7.5 s, roll ~12 s), so it lags the wave and overshoots slowly instead of bobbing.
// Pitch also responds to the BOW alone (extra bow-down torque when the stem is buried) so the bow digs into a crest.
void AIronwakeShipPawn::TickHydro(float Dt, float DesiredYawRateDeg)
{
	UWorld* W = GetWorld();
	if (!W || Dt <= 0.f) return;
	const FVector Loc = GetActorLocation();
	const float Yaw = GetActorRotation().Yaw;
	const FVector Fwd = FRotator(0.f, Yaw, 0.f).Vector();
	const FVector Right(-Fwd.Y, Fwd.X, 0.f);
	const float L = HullProf.Length();
	double Sw = 0, Sx = 0, Sy = 0, Sz = 0, Sxx = 0, Syy = 0, Sxz = 0, Syz = 0;
	float BowSea = 0.f;
	double TwN = 0, TwXS = 0, TwX2 = 0; // A1: lateral wave slope vs keel station (twist -> wave yaw moment)
	for (int32 i = 0; i < NStations; ++i)
	{
		const float U = (i + 0.5f) / NStations;
		const float XM = FMath::Lerp(HullProf.X0, HullProf.X1, 0.5f + (U - 0.5f) * FitSpan);   // P16: central window
		const float HB = FMath::Max(HullProf.HalfBreadthAt(XM), 0.3f);
		float ZL = 0.f, ZR = 0.f;
		for (int32 j = -1; j <= 1; ++j)
		{
			const float YM = j * HB * 0.8f;
			const FVector P = Loc + Fwd * (XM * 100.f) + Right * (YM * 100.f);
			// P34: full 1.15x wave coupling from other vessels' wakes so passing ships visibly roll & pitch in each other's Kelvin wakes!
			const float Z = AIronwakeShell::WaterZAtWaves(W, P) + IronwakeSea::ShipWaveZ(P, this) * 1.15f;
			const double Wt = HB * (j == 0 ? 2.0 : 1.0);
			Sw += Wt; Sx += Wt * XM; Sy += Wt * YM; Sz += Wt * Z;
			Sxx += Wt * XM * XM; Syy += Wt * YM * YM; Sxz += Wt * XM * Z; Syz += Wt * YM * Z;
			if (j < 0) ZL = Z; else if (j > 0) ZR = Z;
		}
		const double LatSlope = (ZR - ZL) / 100.0 / (2.0 * HB * 0.8); // m/m, + = sea higher to starboard
		TwN += 1.0; TwXS += XM * LatSlope; TwX2 += XM * XM;
	}
	const double Mx = Sx / Sw, My = Sy / Sw, Mz = Sz / Sw;
	const double VarX = FMath::Max(Sxx / Sw - Mx * Mx, 1e-3), VarY = FMath::Max(Syy / Sw - My * My, 1e-3);
	const double SlopeX = (Sxz / Sw - Mx * Mz) / VarX;   // cm per m along the keel
	const double SlopeY = (Syz / Sw - My * Mz) / VarY;
	SeaFitZ = (float)(Mz - SlopeX * Mx - SlopeY * My);   // plane height at the body origin
	{
		// A1: wave yaw moment. The sea leaning one way at the bow and the other at the stern twists the hull; this is
		// what makes a real ship wander off course in a seaway (the autopilot has to hold it). Low-passed over ~2 s.
		const double Twist = TwX2 > 1e-3 ? TwXS / TwX2 : 0.0;          // lateral slope change per metre of keel
		const float Raw = (float)FMath::Clamp(-Twist * HullProf.Length() * WaveYawGain, -1.5, 1.5);
		WaveYawRate += (Raw - WaveYawRate) * (1.f - FMath::Exp(-Dt / 2.f));
	}
	SeaFitPitch = FMath::RadiansToDegrees(FMath::Atan((float)SlopeX / 100.f));
	SeaFitRoll = -FMath::RadiansToDegrees(FMath::Atan((float)SlopeY / 100.f));
	// Stem: rendered sea vs the forecastle deck (green water when > 0).
	{
		const FVector StemP = GetActorTransform().TransformPosition(FVector((HullProf.X1 - 3.f) * 100.f, 0.f, HullMeshZ + HullProf.BowDeckM * 100.f));
		BowSea = IronwakeSea::RenderedZ(W, StemP);
		BowSubmergeM = (BowSea - StemP.Z) / 100.f;
	}
	FRotator R = GetActorRotation();
	if (!bHydroInit)
	{
		bHydroInit = true;
		HeaveVel = 0.f; PitchRate = 0.f; RollRate = 0.f;
		const FVector L0(Loc.X, Loc.Y, SeaFitZ - (StaticSeaBodyZ));
		SetActorLocationAndRotation(L0, FRotator(SeaFitPitch, Yaw, SeaFitRoll), false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}
	auto Spring = [Dt](float& X, float& V, float Target, float Period, float Zeta, float Extra)
	{
		const float Wn = 2.f * PI / FMath::Max(Period, 0.5f);
		// semi-implicit Euler, sub-stepped for stability at low fps
		const int32 N = FMath::Clamp(FMath::CeilToInt(Dt / 0.02f), 1, 8);
		const float H = Dt / N;
		for (int32 k = 0; k < N; ++k)
		{
			const float A = Wn * Wn * (Target - X) - 2.f * Zeta * Wn * V + Extra;
			V += A * H;
			X += V * H;
		}
	};
	float Z = Loc.Z;
	const float PlaneZ = AIronwakeShell::WaterZAt(W, Loc);
	const float TargetZ = PlaneZ + (SeaFitZ - PlaneZ) * SeaFollowHeave - StaticSeaBodyZ + 45.f * FMath::Clamp(IronwakeSea::GetSeaState() - 2.f, 0.f, 2.f); // W2 lift was mostly compensating the bogus -4.7 m render offset; a little stays for crests the fit averages away
	const float SeaQ = FMath::Clamp((IronwakeSea::GetSeaState() - 2.f) / 2.f, 0.f, 1.f);
	Spring(Z, HeaveVel, TargetZ, HeavePeriod * FMath::Lerp(1.f, 0.6f, SeaQ), HeaveZeta, 0.f);
	float Pitch = R.Pitch, Roll = R.Roll;
	// Bow digging: a buried stem adds bow-down acceleration (the plane fit can't see water piling over the deck), and the
	// pitch target leans forward slightly with speed (squat by the head at FULL).
	const float SpeedFrac = FMath::Clamp(GetSpeedKnots() / 13.f, 0.f, 1.3f);
	// Bounded (no feedback on own pitch): a crest standing above the fitted plane at the stem pulls the bow down by up
	// to 2.5 deg at FULL, so the bow ploughs into the swell and lifts slowly after it passes.
	const FVector StemW = Loc + Fwd * ((HullProf.X1 - 4.f) * 100.f);
	const float CrestOverPlaneM = (AIronwakeShell::WaterZAtWaves(W, StemW) - (SeaFitZ + (float)SlopeX * (HullProf.X1 - 4.f))) / 100.f;
	// A1 (Chuck: 'looks like it's sinking'): dig max 1.0 deg and squat by the head only at FULL and above.
	const bool bFullOrMore = TelegraphIndex >= 5;
	const float DigDeg = bFullOrMore ? 1.0f * FMath::Clamp(CrestOverPlaneM / 1.2f, 0.f, 1.f) * FMath::Clamp(SpeedFrac, 0.f, 1.f) : 0.f;
	const float Squat = bFullOrMore ? 0.25f * SpeedFrac : 0.f;
	Spring(Pitch, PitchRate, SeaFitPitch * SeaFollowPitch - Squat - DigDeg, PitchPeriod * FMath::Lerp(1.f, 0.6f, SeaQ), PitchZeta, 0.f);
	float Heel = DesiredYawRateDeg * 1.1f; // outward heel in a hard turn (~2 deg)
	if (SailRig)
	{
		// S2: wind pressure heels her to leeward: max abeam/close-hauled, ~0 running; ~7 deg in 16 kn under full sail.
		const float Twa = FMath::Abs(TrueWindAngle);
		const float Side = FMath::Sin(FMath::DegreesToRadians(FMath::Clamp(Twa, 0.f, 180.f)));
		const float WindF = IronwakeFleet::WindKn(GetWorld()) / 16.f;
		// Capped: frigate freeboard ~4 m at the waist on an 8.2 m half-beam puts the lee rail under at ~26 deg, so
		// steady heel stays <= 9 deg below a gale (34 kn) and <= 14 deg above it (sea roll adds on top).
		// W2: pressure ~ wind^2 x canvas set; reefing in a gale/storm keeps steady heel <= 8 deg so the waist stays readable.
		SailHeelDeg = FMath::Clamp(7.f * SailFill * Side * FMath::Min(WindF * WindF, 2.2f) * SailPlanFrac / 0.9f, 0.f, 8.f);
		Heel += -FMath::Sign(TrueWindAngle) * SailHeelDeg; // wind from stbd (+) -> heel to port (roll -)
	}
	Spring(Roll, RollRate, SeaFitRoll * SeaFollowRoll + Heel, RollPeriod, RollZeta, 0.f);
	Pitch = FMath::Clamp(Pitch, -8.f, 8.f);   // P16 caps: pitch 8, roll 12 (were 14 / 10) // W2: storm swells (150-250 m, 10-14 m) lift the bow up to ~10 deg
	Roll = FMath::Clamp(Roll, -12.f, 12.f);
	const FVector V = HullBody->GetPhysicsLinearVelocity();
	HullBody->SetWorldLocationAndRotation(FVector(Loc.X, Loc.Y, Z), FRotator(Pitch, R.Yaw, Roll), false, nullptr, ETeleportType::TeleportPhysics);
	HullBody->SetPhysicsLinearVelocity(FVector(V.X, V.Y, HeaveVel));
	FVector Av = HullBody->GetPhysicsAngularVelocityInDegrees();
	HullBody->SetPhysicsAngularVelocityInDegrees(FVector(0.f, 0.f, Av.Z));
}

// ---------------------------------------------------------------------------- bow wave + spray
void AIronwakeShipPawn::TickBowFX(float Dt)
{
	const float Kn = FMath::Max(GetSpeedKnots(), 0.f);
	const float Frac = FMath::Clamp(Kn / 13.f, 0.f, 1.2f); // FULL AHEAD = 13 kn = 1.0
	static const bool bNoBow = FParse::Param(FCommandLine::Get(), TEXT("iwnobow")), bNoSheet = FParse::Param(FCommandLine::Get(), TEXT("iwnosheet"));
	if (BowWave && !bNoBow) BowWave->UpdateWave(GetActorTransform(), Frac, Kn * KnCmS * SpeedCompression / 100.f, (IronwakeSea::GetSeaState() >= 2.8f ? FMath::Clamp((BowSubmergeM + 0.6f) / 1.2f, 0.f, 1.f) : 0.f), HullMeshZ + HullProf.BowDeckM * 100.f);
	// Spray only when the stem meets a crest (rendered sea climbs the stem fast or buries the deck), emitted forward and
	// outward from points OUTSIDE the hull at the stem, so no sprite starts inside the mesh.
	SlamCooldown -= Dt;
	// P5b: funnel smoke is a thin wisp at a crawl and fuller at full speed (sprite size by speed; base scale from ShipDef).
	if (FunnelSmoke && !bIsNPC) FunnelSmoke->SetWorldScale3D(FVector((SailRig ? 0.9f : (ShipDef ? ShipDef->SmokeScale * (ShipDef->LengthM < 60.f ? 1.2f : 1.f) : 0.3f)) * (0.45f + 0.55f * FMath::Clamp(Frac, 0.f, 1.f))));
	if (FunnelSmoke && !bIsNPC && FParse::Param(FCommandLine::Get(), TEXT("iwsmokedbg")))
	{
		static float Acc = 0.f; Acc += Dt;
		if (Acc > 3.f) { Acc = 0.f; UE_LOG(LogIronwake, Display, TEXT("IWP5C smoke active=%d visible=%d loc=%s scale=%s hullLoc=%s asset=%s"), FunnelSmoke->IsActive(), FunnelSmoke->IsVisible(), *FunnelSmoke->GetComponentLocation().ToString(), *FunnelSmoke->GetComponentScale().ToString(), *GetActorLocation().ToString(), FunnelSmoke->GetAsset() ? *FunnelSmoke->GetAsset()->GetName() : TEXT("none")); }
	}
	// W3: continuous sideways spray sheets off both bow shoulders at speed (rate and size with speed + sea state), plus the
	// slam bursts below. Pooled one-shot bursts of NS_IW_BowSheet, small and frequent.
	{
		static TWeakObjectPtr<UNiagaraSystem> SheetC;
		if (!SheetC.IsValid()) SheetC = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_BowSheet.NS_IW_BowSheet"));
		// Chuck 2026-11-10: bow spray is small and tied to speed; only a big wave makes a big splash. Continuous sheets are
		// light feathering at the shoulders, only from ~8 kn up, sized by speed^2 (not by hull length).
		const float SeaK = FMath::Clamp(0.7f + IronwakeSea::GetSeaState() * 0.1f, 0.7f, 1.1f);
		const float Rate = Frac > 0.6f ? FMath::Lerp(0.3f, 1.5f, FMath::Clamp((Frac - 0.6f) / 0.4f, 0.f, 1.f)) * SeaK : 0.f; // bursts/s per side
		SprayAcc += Dt * Rate;
		if (SheetC.IsValid() && SprayAcc >= 1.f && !bIsNPC && !bNoSheet)
		{
			SprayAcc = FMath::Fmod(SprayAcc, 1.f);
			const float Yaw = GetActorRotation().Yaw;
			const float Hs = HullLengthM / 70.f;
			for (int32 s = -1; s <= 1; s += 2)
			{
				const float X = HullProf.X1 - FMath::FRandRange(5.f, 12.f) * Hs;
				FVector P2 = GetActorTransform().TransformPosition(FVector(X * 100.f, s * (HullProf.HalfBreadthAt(X) + 1.2f) * 100.f, 0.f));
				P2.Z = IronwakeSea::RenderedZ(GetWorld(), P2) + 60.f;
				const FVector Ax = FRotator(0.f, Yaw, 0.f).RotateVector(FVector(0.15f, s * 1.f, 0.55f)).GetSafeNormal();
				if (UNiagaraComponent* N2 = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SheetC.Get(), P2, FRotationMatrix::MakeFromZ(Ax).Rotator(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease))
					N2->SetWorldScale3D(FVector(FMath::Clamp((0.10f + 0.30f * Frac * Frac) * FMath::Clamp(Hs, 0.6f, 1.3f) * SeaK, 0.06f, 0.5f)));
			}
		}
	}
	static TMap<const AIronwakeShipPawn*, float> LastSub;
	float& Prev = LastSub.FindOrAdd(this);
	const float Rise = (BowSubmergeM - Prev) / FMath::Max(Dt, 1e-3f);
	Prev = BowSubmergeM;
	// A slam needs the stem actually going INTO the water fast (was: within 2 m of it and rising 0.6 m/s, which fired on
	// almost every swell). Strength = speed^2 x how hard the bow meets the wave; small impacts give no splash at all.
	const float SeaSt = IronwakeSea::GetSeaState(); // P5c: calm (<0.7) never slams; rough opens the gate wider so it fires more often
	const float GateSub = FMath::Lerp(-1.1f, -0.5f, FMath::Clamp((SeaSt - 1.f) / 1.f, 0.f, 1.f)); const float GateRise = FMath::Lerp(1.5f, 1.0f, FMath::Clamp((SeaSt - 1.f) / 1.f, 0.f, 1.f));
	const bool bMeet = SeaSt > 0.7f && (BowSubmergeM > GateSub && Rise > GateRise) || (SeaSt > 0.7f && BowSubmergeM > 0.f); // P5c: stem rising to meet the sea fast near the deck edge, or buried
	{ static float MaxRise = 0.f, MaxSub = -9.f, Acc2 = 0.f; MaxRise = FMath::Max(MaxRise, Rise); MaxSub = FMath::Max(MaxSub, BowSubmergeM); Acc2 += Dt; if (Acc2 > 5.f && !bIsNPC && FParse::Param(FCommandLine::Get(), TEXT("iwcount"))) { UE_LOG(LogIronwake, Display, TEXT("IWP5B bowstat kn=%.1f maxRise=%.2f maxSub=%.2f"), Kn, MaxRise, MaxSub); MaxRise = 0.f; MaxSub = -9.f; Acc2 = 0.f; } }
	const float Impact = FMath::Clamp(Rise / 3.f + FMath::Max(BowSubmergeM + 0.8f, 0.f) * 0.5f, 0.f, 1.5f);
	const float SlamStrength = Frac * Frac * Impact;
	if (!bNoSheet && Kn > 4.f && SlamCooldown <= 0.f && SlamStrength > 0.08f && (bMeet || (BowSubmergeM > 1.0f && IronwakeSea::GetSeaState() >= 3.f)))
	{
		SlamCooldown = FMath::Lerp(4.f, 1.5f, FMath::Clamp(Frac, 0.f, 1.f));
		if (!BowSprayFX) BowSprayFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_BowSpray.NS_IW_BowSpray"));
		UNiagaraSystem* Spray = BowSprayFX;
		if (Spray)
		{
			const float Strength = SlamStrength;
			const float Yaw = GetActorRotation().Yaw;
			for (int32 s = -1; s <= 1; s += 2)
			{
				const float X = HullProf.X1 - 4.f;
				const float Y = s * (HullProf.HalfBreadthAt(X) + 2.5f);
				FVector P = GetActorTransform().TransformPosition(FVector(X * 100.f, Y * 100.f, 0.f));
				P.Z = IronwakeSea::RenderedZ(GetWorld(), P) + 50.f;
				// Cone axis of the splash is +Z: tilt it outward and forward (~35 deg out, 20 deg forward).
				const FRotator Rot(-20.f, Yaw, 0.f);
				const FRotator Tilt = (FRotator(0.f, Yaw, 0.f).RotateVector(FVector(0.35f, s * 0.6f, 1.f))).Rotation();
				if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Spray, P, FRotator(Tilt.Pitch - 90.f, Tilt.Yaw, 0.f)))
					N->SetWorldScale3D(FVector(0.05f + 0.12f * Strength));
				(void)Rot;
			}
			// W2 slam sheets: heavier sideways sheets of water thrown out from both bows, scaled by how hard she meets it.
			static TWeakObjectPtr<UNiagaraSystem> SheetFX;
			if (!SheetFX.IsValid()) SheetFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_BowSheet.NS_IW_BowSheet"));
			if (UNiagaraSystem* Sh = SheetFX.Get())
			{
				const float Sea = FMath::Clamp(IronwakeSea::GetSeaState() / 4.f, 0.25f, 1.f);
				const float Hs = HullLengthM / 70.f;
				for (int32 s = -1; s <= 1; s += 2)
				{
					const float X = HullProf.X1 - 9.f * Hs;
					FVector P2 = GetActorTransform().TransformPosition(FVector(X * 100.f, s * (HullProf.HalfBreadthAt(X) + 1.5f) * 100.f, 0.f));
					P2.Z = IronwakeSea::RenderedZ(GetWorld(), P2) + 80.f;
					const FVector Ax = FRotator(0.f, Yaw, 0.f).RotateVector(FVector(0.25f, s * 0.95f, 0.75f)).GetSafeNormal();
					if (UNiagaraComponent* N2 = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Sh, P2, FRotationMatrix::MakeFromZ(Ax).Rotator(), FVector(1.f), true, true, ENCPoolMethod::AutoRelease))
						N2->SetWorldScale3D(FVector(FMath::Clamp((0.12f + 0.6f * Strength) * Sea * FMath::Clamp(Hs, 0.6f, 1.3f), 0.08f, 1.1f)));
				}
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP5B slam kn=%.1f strength=%.2f rise=%.2f bowSub=%.2f sea=%.2f"), Kn, SlamStrength, Rise, BowSubmergeM, SeaSt);
			if (bW1Test) UE_LOG(LogIronwake, Display, TEXT("IWW1 spray kn=%.1f bowSub=%.2f m rise=%.2f m/s"), Kn, BowSubmergeM, Rise);
		}
	}
}

// ---------------------------------------------------------------------------- water audio (approved loops)
void AIronwakeShipPawn::TickWaterAudio(float Dt)
{
	if (!SndBow)
	{
		USoundAttenuation* Att = NewObject<USoundAttenuation>(this);
		Att->Attenuation.bAttenuate = true; Att->Attenuation.bSpatialize = true;
		Att->Attenuation.AttenuationShapeExtents = FVector(bIsNPC ? 4000.f : 40000.f, 0.f, 0.f);
		Att->Attenuation.FalloffDistance = bIsNPC ? 60000.f : 120000.f;
		auto Make = [&](const TCHAR* Name, const FVector& Rel) -> UAudioComponent*
		{
			USoundBase* S = Snd(Name);
			if (!S) return nullptr;
			UAudioComponent* A = NewObject<UAudioComponent>(this);
			A->SetupAttachment(HullBody);
			A->SetRelativeLocation(Rel);
			A->SetSound(S);
			A->AttenuationSettings = Att;
			A->bAutoActivate = false;
			A->RegisterComponent();
			A->SetVolumeMultiplier(0.001f);
			A->Play(FMath::FRand() * 4.f);
			return A;
		};
		SndBow = Make(TEXT("bow_wash_loop"), FVector(HullProf.X1 * 100.f, 0.f, 0.f));
		SndWake = Make(TEXT("hull_wake_loop"), FVector(HullProf.X0 * 100.f, 0.f, 0.f));
		SndLap = Make(TEXT("slow_lap_loop"), FVector::ZeroVector);
		// Audio fix (Chuck 09:55): generated broadband moving-water layers (no tonal content).
		SndRush = Make(TEXT("bow_rush_proc"), FVector(HullProf.X1 * 100.f, 0.f, 0.f));
		SndHiss = Make(TEXT("hull_hiss_proc"), FVector::ZeroVector);
		if (!SndBow) { SndBow = NewObject<UAudioComponent>(this); return; }
	}
	// Same speed curve as the bow wave: slow_lap at 0-3 kn, bow wash + wake rising with speed, pitch +/-5 %.
	const float Kn = FMath::Max(GetSpeedKnots(), 0.f);
	const float Frac = FMath::Clamp(Kn / 13.f, 0.f, 1.2f);
	const float Own = bIsNPC ? 0.45f : 1.f;
	const float Lap = FMath::Clamp(1.f - Kn / 4.f, 0.15f, 1.f) * 0.7f * Own;
	const float Bow = (FMath::SmoothStep(0.05f, 0.9f, Frac) * 0.95f + FMath::Clamp(BowSubmergeM + 1.f, 0.f, 2.f) * 0.15f * Frac) * Own;
	const float Wk = FMath::SmoothStep(0.1f, 1.f, Frac) * 0.8f * Own;
	if (SndBow && SndBow->Sound) { SndBow->SetVolumeMultiplier(FMath::Max(Bow, 0.001f)); SndBow->SetPitchMultiplier(0.95f + 0.1f * FMath::Clamp(Frac, 0.f, 1.f)); }
	if (SndWake && SndWake->Sound) { SndWake->SetVolumeMultiplier(FMath::Max(Wk, 0.001f)); SndWake->SetPitchMultiplier(0.95f + 0.1f * FMath::Clamp(Frac, 0.f, 1.f)); }
	if (SndLap && SndLap->Sound) SndLap->SetVolumeMultiplier(FMath::Max(Lap, 0.001f));
	// bow_rush 0 -> full at 10 kn (pitch 0.9-1.1), hull_hiss 0 -> full at 8 kn; slow random drift so the loops don't repeat audibly.
	AudDrift += Dt; const float Dr1 = 0.88f + 0.12f * FMath::Sin(AudDrift * 0.23f + 1.3f) * FMath::Sin(AudDrift * 0.071f);
	const float Dr2 = 0.88f + 0.12f * FMath::Sin(AudDrift * 0.17f + 0.4f) * FMath::Sin(AudDrift * 0.053f + 2.f);
	if (SndRush && SndRush->Sound) { SndRush->SetVolumeMultiplier(FMath::Max(FMath::Clamp(Kn / 10.f, 0.f, 1.f) * 0.9f * Own * Dr1, 0.001f)); SndRush->SetPitchMultiplier(0.9f + 0.2f * FMath::Clamp(Kn / 12.f, 0.f, 1.f)); }
	if (SndHiss && SndHiss->Sound) SndHiss->SetVolumeMultiplier(FMath::Max(FMath::Clamp(Kn / 8.f, 0.f, 1.f) * 0.7f * Own * Dr2, 0.001f));
}

// ---------------------------------------------------------------------------- W1 test director (-iww1 [-iww1quit])
void AIronwakeShipPawn::W1Shot(const TCHAR* Name)
{
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (FString(Name) + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(File, false, false);
	UE_LOG(LogIronwake, Display, TEXT("IWW1 shot %s t=%.1f kn=%.1f pitch=%.2f roll=%.2f bowSub=%.2f m"), *File, W1T, GetSpeedKnots(), GetActorRotation().Pitch, GetActorRotation().Roll, BowSubmergeM);
}

void AIronwakeShipPawn::TickW1Test(float Dt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	W1T += Dt;
	UWorld* W = GetWorld();
	const float Since = W1T - W1Mark;
	auto Next = [&](int32 S) { W1Stage = S; W1Mark = W1T; W1N = 0; FMemory::Memzero(W1Acc, sizeof(W1Acc)); W1Peak = 0.f; };
	const float Z = GetActorLocation().Z, Pitch = GetActorRotation().Pitch, Roll = GetActorRotation().Roll;
	const float SeaC = AIronwakeShell::WaterZAtWaves(W, GetActorLocation());
	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	const float Lh = HullProf.Length() * 50.f;
	const float WavePitch = FMath::RadiansToDegrees(FMath::Atan2(AIronwakeShell::WaterZAtWaves(W, GetActorLocation() + Fwd * Lh) - AIronwakeShell::WaterZAtWaves(W, GetActorLocation() - Fwd * Lh), 2.f * Lh));
	// Accumulate std-devs: [0] sea at midships z, [1] hull z, [2] wave slope over the hull length, [3] hull pitch
	auto Acc = [&]()
	{
		W1Acc[0] += SeaC; W1Acc[1] += SeaC * SeaC; W1Acc[2] += Z; W1Acc[3] += Z * Z;
		W1Acc[4] += WavePitch; W1Acc[5] += WavePitch * WavePitch; W1Acc[6] += Pitch; W1Acc[7] += Pitch * Pitch; ++W1N;
	};
	auto Sd = [&](int32 k) { const double M = W1Acc[k] / FMath::Max(W1N, 1); return FMath::Sqrt(FMath::Max(W1Acc[k + 1] / FMath::Max(W1N, 1) - M * M, 0.0)); };
	auto Report = [&](const TCHAR* Band)
	{
		const double SeaSd = Sd(0) / 100.0, HullSd = Sd(2) / 100.0, WSl = Sd(4), PSd = Sd(6);
		UE_LOG(LogIronwake, Display, TEXT("IWW1 RESULT band=%s n=%d sea_heave_sd=%.2f m (amp~%.2f) hull_heave_sd=%.2f m ratio=%.2f | wave_slope_sd=%.2f deg hull_pitch_sd=%.2f deg ratio=%.2f"),
			Band, W1N, SeaSd, SeaSd * 1.414, HullSd, HullSd / FMath::Max(SeaSd, 0.001), WSl, PSd, PSd / FMath::Max(WSl, 0.001));
	};
	if (FMath::Fmod(W1T, 1.f) < Dt)
		UE_LOG(LogIronwake, Display, TEXT("IWW1 tel t=%.1f st=%d kn=%.1f z=%.0f sea=%.0f fitZ=%.0f pitch=%.2f fitP=%.2f roll=%.2f fitR=%.2f bowSub=%.2f"), W1T, W1Stage, GetSpeedKnots(), Z, SeaC, SeaFitZ, Pitch, SeaFitPitch, Roll, SeaFitRoll, BowSubmergeM);
	// Wave bands are isolated by temporarily regenerating the spectrum (amplitude-only), measured at STOP then at FULL.
	if (W1Mode == TEXT("clip"))
	{
		// -iww1clip: FULL AHEAD into the storm sea, low 3/4 bow camera, 10 s of fixed-step frames (-benchmark -fps=30).
		if (W1Stage == 0 && W1T > 2.f) { IWTelegraph(5); IronwakeSea::SetSeaScale(W, 1.6f); ZoomIndex = 1; W1Stage = 1; }
		PC->SetControlRotation(FRotator(-4.f, GetActorRotation().Yaw + 125.f, 0.f));
		if (W1Stage == 1 && W1T > 40.f) { W1Stage = 2; W1N = 0; UE_LOG(LogIronwake, Display, TEXT("IWW1 clip start")); }
		if (W1Stage == 2)
		{
			// one numbered frame per tick (run with -benchmark -fps=30: fixed 1/30 s steps)
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("w1clip") / FString::Printf(TEXT("f%04d.png"), W1N++)), false, false);
		}
		if (W1Stage == 2 && W1N >= 300)
		{
			W1Stage = 3; UE_LOG(LogIronwake, Display, TEXT("IWW1 clip end"));
			UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
		}
		return;
	}
	switch (W1Stage)
	{
	case 0: if (W1T > 3.f) { IWTelegraph(2); IronwakeSea::SetSeaScale(W, 1.f); Next(1); } break;
	case 1: // settle at rest, measure draft (rest waterline vs boot-top) on the moderate sea
		if (Since > 12.f) { Acc(); }
		if (Since > 26.f)
		{
			const float BootTopZ = GetActorLocation().Z + HullMeshZ;
			UE_LOG(LogIronwake, Display, TEXT("IWW1 RESULT rest: hull z mean %.0f, sea fit mean (query) %.0f, rendered sea %.0f, boot-top %.0f -> boot-top %.2f m under the rendered sea; keel draft %.2f m"),
				W1Acc[2] / FMath::Max(W1N, 1), W1Acc[0] / FMath::Max(W1N, 1), W1Acc[0] / FMath::Max(W1N, 1) + AIronwakeShell::RenderedSeaOffsetCm, BootTopZ,
				(W1Acc[0] / FMath::Max(W1N, 1) + AIronwakeShell::RenderedSeaOffsetCm - (W1Acc[2] / FMath::Max(W1N, 1) + HullMeshZ)) / 100.f,
				HullProf.KeelM + (W1Acc[0] / FMath::Max(W1N, 1) + AIronwakeShell::RenderedSeaOffsetCm - (W1Acc[2] / FMath::Max(W1N, 1) + HullMeshZ)) / 100.f);
			Report(TEXT("moderate_full_spectrum_stopped"));
			Next(2);
		}
		break;
	case 2: // short chop only: wavelengths < 40 m (amplitude-only regen of the generator would change the set, so scale the sea)
		// the moderate sea's short band at scale 1 (seatune3: 8-34 m, 9-24 cm amplitude)
		if (Since < Dt * 1.5f) { W1Mode = TEXT("chop"); IronwakeSea::SetSeaBand(W, 9, 700.f, 3500.f, 13.f, 29.f); }
		if (Since > 12.f) Acc();
		if (Since > 34.f) { Report(TEXT("short_chop_only_(7-35m)")); Next(3); }
		break;
	case 3: // long swell only
		// the moderate sea's long band (100-200 m, 37-52 cm per component)
		if (Since < Dt * 1.5f) { IronwakeSea::SetSeaBand(W, 7, 10000.f, 20000.f, 37.f, 52.f); }
		if (Since > 12.f) Acc();
		if (Since > 44.f) { Report(TEXT("long_swell_only_(100-200m)")); IronwakeSea::SetSeaScale(W, 1.f); Next(4); }
		break;
	case 4: // settle after a big wave: kick the hull 2.5 m down + 4 deg bow-down, time to settle under 10 % (flat sea)
		if (Since < Dt * 1.5f) { IronwakeSea::SetSeaScale(W, 0.f); }
		if (Since > 8.f && W1SettleStart < 0.f)
		{
			HullBody->SetWorldLocation(GetActorLocation() - FVector(0, 0, 250.f), false, nullptr, ETeleportType::TeleportPhysics);
			HullBody->SetWorldRotation(FRotator(-4.f, GetActorRotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			HeaveVel = 0.f; PitchRate = 0.f;
			W1SettleStart = W1T; W1LastOut = W1T; W1Peak = SeaFitZ;
		}
		if (W1SettleStart > 0.f)
		{
			const float Eq = SeaFitZ - StaticSeaBodyZ;
			if (FMath::Abs(Z - Eq) > 25.f || FMath::Abs(Pitch - SeaFitPitch) > 0.4f) W1LastOut = W1T;
			if (W1T - W1SettleStart > 35.f)
			{
				UE_LOG(LogIronwake, Display, TEXT("IWW1 RESULT settle after 2.5 m / 4 deg kick: inside 0.25 m / 0.4 deg after %.1f s"), W1LastOut - W1SettleStart);
				IronwakeSea::SetSeaScale(W, 1.f);
				Next(5);
			}
		}
		break;
	case 5: // FULL AHEAD in the moderate swell, measure moving
		if (Since < Dt * 1.5f) { IWTelegraph(5); }
		if (Since > 30.f) Acc();
		if (Since > 62.f) { Report(TEXT("moderate_full_spectrum_FULL_AHEAD")); Next(6); }
		break;
	case 6: // captures from the default camera at FULL: bow wave; then hunt for a buried bow
		if (Since < Dt * 1.5f) { ZoomIndex = 2; PC->SetControlRotation(FRotator(DefaultCamPitch, GetActorRotation().Yaw + 155.f, 0.f)); }
		if (Since > 3.f) { W1Shot(TEXT("W1_bow_wave")); Next(7); }
		break;
	case 7:
		// storm sea (foggy-night preset scale) for the buried-bow capture
		if (Since < Dt * 1.5f) { IronwakeSea::SetSeaScale(W, 1.6f); ZoomIndex = 1; }
		PC->SetControlRotation(FRotator(-5.f, GetActorRotation().Yaw + 115.f, 0.f));
		if (Since > 15.f && BowSubmergeM > 0.8f) { W1Shot(TEXT("W1_bow_dig")); Next(8); }
		if (Since > 120.f) { UE_LOG(LogIronwake, Display, TEXT("IWW1 no buried bow in 120 s (max %.2f)"), W1Peak); W1Shot(TEXT("W1_bow_dig_nodig")); Next(8); }
		W1Peak = FMath::Max(W1Peak, BowSubmergeM);
		break;
	case 8:
		if (Since < Dt * 1.5f) { IronwakeSea::SetSeaScale(W, 1.f); ZoomIndex = 3; }
		PC->SetControlRotation(FRotator(-3.f, GetActorRotation().Yaw - 100.f, 0.f));
		if (Since > 8.f) { W1Shot(TEXT("W1_sea")); Next(9); }
		break;
	case 9:
		if (Since > 2.f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWW1 done"));
			bW1Test = false;
			if (FParse::Param(FCommandLine::Get(), TEXT("iww1quit"))) UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
		}
		break;
	default: break;
	}
}

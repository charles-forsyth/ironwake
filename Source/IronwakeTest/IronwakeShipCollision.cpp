// P9: ship collision + grounding. The hull is a velocity-driven physics box (ApplyShipForces plans a world velocity and yaw
// rate each tick); TickCollision edits that plan: predicted-pose outline tests against the UIronwakeColl registry (static
// solids, other hulls) and the seabed depth (island heightfield) vs the hull's draught.
#include "IronwakeShipPawn.h"
#include "IronwakeCollision.h"
#include "IronwakeFleet.h"
#include "IronwakeLivingSea.h"
#include "IronwakeP8.h"
#include "IronwakeVoyage.h"
#include "IronwakeTest.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

static constexpr float KnotToCmS = 51.4444f;

void AIronwakeShipPawn::RegisterCollisionHull()
{
	bNoCollision = FParse::Param(FCommandLine::Get(), TEXT("iwnocoll"));
	if (ShipId == TEXT("cutter")) DraughtCm = 200.f;
	else if (ShipId == TEXT("brig")) DraughtCm = 310.f;
	else if (ShipId == TEXT("frigate")) DraughtCm = 500.f;
	else DraughtCm = bIsNPC ? 600.f : 700.f;
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
	if (!Co) return;
	const float PadCm = FMath::Clamp(0.012f * HullLengthM * 100.f, 60.f, 160.f);
	TArray<FVector2D> Poly;
	UIronwakeColl::OutlineFromProfile(HullProf, Poly, PadCm);
	CollShape = UIronwakeColl::MakeShape(Poly, DraughtCm);
	// Keel line + narrowed outline for the depth check (the keel is narrower than the waterline).
	CollKeel.Reset();
	for (float X = HullProf.X0; X <= HullProf.X1 + 0.1f; X += 6.f) CollKeel.Add(FVector2D(X * 100.f, 0.f));
	for (const FVector2D& P : CollShape.Dense) CollKeel.Add(FVector2D(P.X, P.Y * 0.55f));
	Co->RegisterHull(this, CollShape, !bIsNPC || true);
	bCollInit = true;
	UE_LOG(LogIronwake, Display, TEXT("IWP9 hull registered %s %s draught=%.1f m outline=%d pts radius=%.0f m pad=%.0f cm"), *GetShipName(), bIsNPC ? TEXT("NPC") : TEXT("PLY"), DraughtCm / 100.f, Poly.Num(), CollShape.R / 100.f, PadCm);
}

void AIronwakeShipPawn::TickCollision(float Dt, FVector& V, float& YawRate)
{
	if (bNoCollision || !bCollInit || bSinking) return;
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
	if (!Co) return;
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	const FVector Loc = GetActorLocation();
	const FVector2D P(Loc.X, Loc.Y);
	const float Yaw = GetActorRotation().Yaw;
	const float Look = FMath::Max(Dt, 0.02f) * 2.f;     // two ticks ahead
	const FVector2D V2(V.X, V.Y);
	const float Speed = V2.Size();

	// ---- 1. Hard solids and other hulls: predicted pose, remove the inward velocity, bounce a little, lose speed.
	const uint8 Mask = UIronwakeColl::MaskAll;
	FIwHit Now = Co->TestPose(this, CollShape, P, Yaw, Mask);
	FVector2D PN = P + V2 * Look;
	float YN = Yaw + YawRate * Look;
	FIwHit Next = Co->TestPose(this, CollShape, PN, YN, Mask);
	FIwHit H = Next.Pen > Now.Pen ? Next : Now;
	const double Tn = FPlatformTime::Seconds();
	if (H.bHit)
	{
		FVector2D N = H.N.GetSafeNormal();
		if (N.IsNearlyZero()) N = -V2.GetSafeNormal();
		const float Vn = FVector2D::DotProduct(V2, N);   // < 0 = into the obstacle
		FVector2D NewV = V2;
		if (Vn < 0.f)
		{
			const float Impact = -Vn;
			NewV = V2 - N * Vn * (1.f + 0.10f);            // slide + a little bounce
			NewV *= (1.f - FMath::Clamp(Impact / 1800.f, 0.f, 0.5f));  // loss grows with the impact speed
			if (Impact > 120.f && Tn - LastImpactWall > 0.6)
			{
				LastImpactWall = Tn;
				++CollisionHits; LastImpactCmS = Impact; LastHitTag = H.Tag;
				ShakeAmpCm = FMath::Max(ShakeAmpCm, FMath::Clamp(Impact * 0.12f, 8.f, 160.f) * FMath::Clamp(HullLengthM / 100.f, 0.5f, 1.f));
				ShakeT = 0.f;
				PitchRate += (FMath::FRand() > 0.5f ? 1.f : -1.f) * FMath::Clamp(Impact * 0.004f, 0.3f, 4.f);
				RollRate += (FMath::FRand() > 0.5f ? 1.f : -1.f) * FMath::Clamp(Impact * 0.006f, 0.3f, 6.f);
				if (!bIsNPC && Vy) Vy->ShowToast(FString::Printf(TEXT("COLLISION: %s (%.1f kn)"), *H.Tag, Impact / KnotToCmS / FMath::Max(SpeedCompression, 1.f)), 2.5f);
				UE_LOG(LogIronwake, Display, TEXT("IWP9 IMPACT %s %s at %.1f kn (pen %.0f cm) with %s%s"), *GetShipName(), bIsNPC ? TEXT("NPC") : TEXT("PLY"), Impact / KnotToCmS / FMath::Max(SpeedCompression, 1.f), H.Pen, *H.Tag, H.Other.IsValid() ? TEXT(" [ship]") : TEXT(""));
				if (!bIsNPC) PlayAt(Snd(TEXT("creak_storm_strain")), Loc, FMath::Clamp(Impact / 600.f, 0.3f, 1.4f), 0.55f); // TODO: dedicated scrape/impact SFX (none in audio/sfx)
				// Other ship: slow it too (kinematic craft and merchants have no physics).
				if (AIronwakeMerchant* Mc = Cast<AIronwakeMerchant>(H.Other.Get())) Mc->SlowUntil = GetWorld()->GetTimeSeconds() + 4.f;
				if (AIronwakeNpcCraft* Nc = Cast<AIronwakeNpcCraft>(H.Other.Get())) Nc->SlowUntil = GetWorld()->GetTimeSeconds() + 4.f;
			}
			V.X = NewV.X; V.Y = NewV.Y;
		}
		// Overlapping now: push out (bounded), and stop turning into it.
		if (Now.bHit && Now.Pen > 0.f)
		{
			const FVector2D Push = Now.N.GetSafeNormal() * FMath::Min(Now.Pen * 3.f, 350.f);
			V.X += Push.X; V.Y += Push.Y;
		}
		FIwHit YawTest = Co->TestPose(this, CollShape, P + FVector2D(V.X, V.Y) * Look, Yaw + YawRate * Look, Mask);
		if (YawTest.bHit && YawTest.Pen > 5.f) YawRate *= FMath::Exp(-8.f * Dt);   // no turning into the obstacle
	}

	// ---- 2. Seabed: aground / shoal from the shallowest water under the keel (bow, midships, quarters, narrowed outline).
	float MinD = 1e7f;
	{
		TArray<FVector2D> KW; UIronwakeColl::ToWorld(CollKeel, P, Yaw, KW);
		for (const FVector2D& K : KW) MinD = FMath::Min(MinD, Co->DepthAt(K));
	}
	MinDepthCm = MinD;
	float MinNext = MinD;
	if (MinD < DraughtCm * 2.2f)
	{
		TArray<FVector2D> KW; UIronwakeColl::ToWorld(CollKeel, P + FVector2D(V.X, V.Y) * (Look * 2.f), Yaw, KW);
		MinNext = 1e7f;
		for (const FVector2D& K : KW) MinNext = FMath::Min(MinNext, Co->DepthAt(K));
	}
	const bool bWasAground = bAground;
	bAground = MinD < DraughtCm;
	bShoal = !bAground && MinD < DraughtCm * 2.f;
	const float Now2 = GetWorld()->GetTimeSeconds();
	const bool bToShallower = MinNext < MinD - 0.5f || (bAground && Speed > 20.f && MinNext <= MinD + 0.5f);
	if (bAground)
	{
		if (!bWasAground)
		{
			++GroundingCount;
			ShakeAmpCm = FMath::Max(ShakeAmpCm, FMath::Clamp(Speed * 0.10f, 6.f, 120.f) * FMath::Clamp(HullLengthM / 100.f, 0.5f, 1.f)); ShakeT = 0.f;
			PitchRate += FMath::Clamp(Speed * 0.003f, 0.2f, 3.f); RollRate += FMath::Clamp(Speed * 0.004f, 0.2f, 4.f) * (FMath::FRand() > 0.5f ? 1.f : -1.f);
			if (!bIsNPC) PlayAt(Snd(TEXT("creak_storm_strain")), Loc, FMath::Clamp(Speed / 500.f, 0.3f, 1.4f), 0.5f); // TODO: dedicated grinding SFX
			UE_LOG(LogIronwake, Display, TEXT("IWP9 AGROUND %s %s at (%.0f,%.0f) m speed %.1f kn depth %.1f m < draught %.1f m"), *GetShipName(), bIsNPC ? TEXT("NPC") : TEXT("PLY"), Loc.X / 100.f, Loc.Y / 100.f, Speed / KnotToCmS / FMath::Max(SpeedCompression, 1.f), MinD / 100.f, DraughtCm / 100.f);
		}
		if (bToShallower)
		{
			V.X *= FMath::Exp(-2.4f * Dt); V.Y *= FMath::Exp(-2.4f * Dt);     // ~1.5 s to a standstill
			if (Speed < 40.f) { V.X = 0.f; V.Y = 0.f; }
		}
		YawRate *= FMath::Exp(-7.f * Dt);                                    // rudder has no bite
		if (!bIsNPC && Vy && Now2 - GroundMsgT > 1.2f) { GroundMsgT = Now2; Vy->ShowToast(TEXT("AGROUND - engines astern to back off"), 2.f); }
		if (!bIsNPC && Now2 - ScrapeSndT > 1.4f && Speed > 80.f) { ScrapeSndT = Now2; PlayAt(Snd(TEXT("creak_wooden_hull")), Loc, 0.9f, 0.7f); }
	}
	else if (bShoal && bToShallower)
	{
		const float Frac = 1.f - (MinD - DraughtCm) / DraughtCm;        // 0 at 2x draught .. 1 at the draught
		const float K = 0.05f + 0.35f * Frac * Frac;
		V.X *= FMath::Exp(-K * Dt); V.Y *= FMath::Exp(-K * Dt);
		if (!bIsNPC && Vy && Now2 - ShoalMsgT > 2.5f) { ShoalMsgT = Now2; Vy->ShowToast(TEXT("SHOAL WATER"), 2.5f); }
	}
}

float AIronwakeShipPawn::AvoidLand(float DesiredYaw, float LookAheadCm) const
{
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
	if (!Co || bNoCollision || !bCollInit) return DesiredYaw;
	const FVector Loc = GetActorLocation();
	const FVector2D P(Loc.X, Loc.Y);
	const float HalfB = CollShape.R * 0.18f + 600.f;
	const float C0 = Co->PathClear(P, DesiredYaw, LookAheadCm, HalfB, DraughtCm);
	if (C0 >= LookAheadCm) return DesiredYaw;
	// Pick the nearest-to-desired heading that is clear for the look-ahead.
	for (float Off = 15.f; Off <= 120.f; Off += 15.f)
		for (int32 S = -1; S <= 1; S += 2)
		{
			const float Y = DesiredYaw + S * Off;
			if (Co->PathClear(P, Y, LookAheadCm, HalfB, DraughtCm) >= LookAheadCm) return Y;
		}
	return DesiredYaw + 90.f;
}

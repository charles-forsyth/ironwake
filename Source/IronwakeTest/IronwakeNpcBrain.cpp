// P20: NPC brain: deterministic utility/state machine (no LLM). Roles pick destinations from timetables, weather, time of day and the ledger;
// states drive the steering layer (DesYaw, DesKn) of the embodied ship, or advance the ledger position of a far one.
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShell.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	constexpr float KnCmS = 51.4444f, Comp = 1.5f;
	inline float Sq(float x) { return x * x; }
	inline float BrgDeg(const FVector& A, const FVector& B) { return FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X)); }
	inline float Norm360(float a) { a = FMath::Fmod(a, 360.f); return a < 0.f ? a + 360.f : a; }
	inline FVector PolarPt(const FVector& C, float AngDeg, float R) { const float A = FMath::DegreesToRadians(AngDeg); return C + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f); }
	float SegDistQ(const FVector& A, const FVector& B, const FVector& P, FVector& Q)
	{
		const FVector AB = B - A; const float L2 = FMath::Max(AB.SizeSquared2D(), 1.f);
		const float T = FMath::Clamp(((P.X - A.X) * AB.X + (P.Y - A.Y) * AB.Y) / L2, 0.f, 1.f);
		Q = A + AB * T; return FVector::Dist2D(Q, P);
	}
	inline float KnToSim(float Kn) { return Kn * KnCmS * Comp; }
	inline bool IsMerchantish(const FNpcRec& R) { return R.Role == ENpcRole::Merchant || R.Role == ENpcRole::Ferry; }
}

float AIronwakeNpcDirector::MyLenCm(const FNpcRec& R) const { return R.Def->LengthM * 100.f; }

// ---------------------------------------------------------------------------- ports, obstacles
bool AIronwakeNpcDirector::InHarbour(const FVector& P, float MarginM) const
{
	const FVector2D H = W2H(P); return H.X > -60.f - MarginM && H.X < 125.f + MarginM && H.Y > -140.f - MarginM && H.Y < 175.f + MarginM;
}

void AIronwakeNpcDirector::BuildPorts()
{
	Ports.Reset();
	{
		FNpcPort P; P.Name = TEXT("Greyhaven"); P.Tag = TEXT("H"); P.bHarbour = true; P.Offing = H2W(260.f, 0.f);
		auto Lane = [&](const TCHAR* N, float Y, float XBow, float MaxLen)
		{
			FNpcSpan S; S.Name = N; S.Y0 = Y; S.Y1 = Y; S.XBow = XBow; S.MaxLenM = MaxLen;
			P.Spans.Add(S);
		};
		// P25: >= 20 m main-quay spacing, rock/jetty/pier-clear XBow (auto-verified in BuildObstacles)
		Lane(TEXT("N1"), 25.f, -38.f, 52.f); Lane(TEXT("N2"), 46.f, -36.f, 52.f); Lane(TEXT("N3"), 67.f, -20.f, 46.f);
		Lane(TEXT("S1"), -89.f, -36.f, 52.f); Lane(TEXT("S2"), -109.f, -28.f, 52.f); Lane(TEXT("S3"), -129.f, -12.f, 48.f);
		Lane(TEXT("Lighter"), -42.f, -34.f, 32.f); Lane(TEXT("Tender"), 139.f, -24.f, 30.f);
		Lane(TEXT("Pilot"), 106.f, -36.f, 18.f); Lane(TEXT("Gig"), 84.f, -38.f, 12.f); Lane(TEXT("Gig2"), 95.f, -38.f, 12.f);
		Ports.Add(P);
	}
	InitWaitSlots();
	AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld());
	auto IsleAnch = [&](const TCHAR* Name, const TCHAR* Tag, int32 IsleIdx, float TowardYaw)
	{
		FNpcPort P; P.Name = Name; P.Tag = Tag; P.bHarbour = false;
		FVector C(0, 0, 0); float Hz = 40000.f;
		if (Ar && Ar->Isles.IsValidIndex(IsleIdx)) { C = FVector(Ar->Isles[IsleIdx].C.X, Ar->Isles[IsleIdx].C.Y, 0.f); Hz = Ar->Isles[IsleIdx].HazardR; }
		P.Offing = PolarPt(C, TowardYaw, Hz * 1.25f + 9000.f);
		Ports.Add(P);
	};
	// facing the harbour (south-west of each isle)
	IsleAnch(TEXT("Kestrel Isle"), TEXT("K"), 0, -150.f);
	IsleAnch(TEXT("Fort Grimsby"), TEXT("F"), 1, -135.f);
	{
		FNpcPort P; P.Name = TEXT("St Brannoc Light"); P.Tag = TEXT("L");
		const FVector C = Voy.IsValid() ? FVector(0.f, 0.f, 0.f) : FVector::ZeroVector;
		FVector Isl(-50000.f, 190000.f, 0.f);
		if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld())) Isl = LS->IslandCentre;
		(void)C; P.Offing = PolarPt(FVector(Isl.X, Isl.Y, 0.f), -120.f, 42000.f * 1.5f + 9000.f);
		Ports.Add(P);
	}
	// P29: Secondary data-driven working ports (Ironcrag = Ports[4], Stigsby = Ports[5])
	if (Voy.IsValid())
	{
		for (const FIwPortSpec& Ps : Voy->WorldPorts)
		{
			if (Ps.bMainHarbour) continue;
			FNpcPort P; P.Name = Ps.Name; P.Tag = Ps.Tag; P.bHarbour = false;
			P.Offing = Ps.H2W(440.f, 0.f); P.Offing.Z = 0.f;
			Ports.Add(P);
		}
	}
}

void AIronwakeNpcDirector::BuildObstacles()
{
	Obs.Reset();
	if (AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(GetWorld()))
	{
		for (int32 i = 0; i < Ar->Isles.Num(); ++i)
		{
			if (Ar->HostedPortIsles.Contains(i)) continue;
			const FIsle& I = Ar->Isles[i];
			Obs.Add({ FVector(I.C.X, I.C.Y, 0.f), I.HazardR * 1.08f });
		}
	}
	if (Voy.IsValid())
	{
		for (const FIwPortSpec& Ps : Voy->WorldPorts)
		{
			if (Ps.bMainHarbour) continue;
			auto PL = [&](float X, float Y) { FVector W = Ps.H2W(X, Y, 0.f); W.Z = 0.f; return W; };
			auto PWall = [&](float CX, float CY, float YawDeg, float LenM)
			{
				const float Yr = FMath::DegreesToRadians(YawDeg);
				for (float s = -LenM * 0.5f; s <= LenM * 0.5f + 1.f; s += 38.f)
					Obs.Add({ PL(CX + FMath::Cos(Yr) * s, CY + FMath::Sin(Yr) * s), 6400.f });
			};
			PWall(42.f, -114.f, 14.f, 150.f);
			PWall(38.f,  114.f, -14.f, 145.f);
			Obs.Add({ PL(-145.f, -120.f), 15000.f });
			Obs.Add({ PL(-145.f,    0.f), 15000.f });
			Obs.Add({ PL(-145.f,  120.f), 15000.f });
		}
	}
	FVector Isl(-50000.f, 190000.f, 0.f);
	if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(GetWorld())) Isl = LS->IslandCentre;
	Obs.Add({ FVector(Isl.X, Isl.Y, 0.f), 42000.f });
	if (Voy.IsValid())
	{
		Obs.Add({ FVector(Voy->StacksC.X, Voy->StacksC.Y, 0.f), 36000.f });
		Obs.Add({ FVector(Voy->WreckC.X, Voy->WreckC.Y, 0.f), 17000.f });
		// P22: the legacy moored steamer (SS Wren) sits across lanes N1/N2 at H(40,10) (13 of the soak collisions): the NPC traffic owns the quay now
		if (Voy->Moored && !FParse::Param(FCommandLine::Get(), TEXT("iwoldnpc"))) { Voy->Moored->Destroy(); Voy->Moored = nullptr; UE_LOG(LogIronwake, Display, TEXT("IWNPC legacy moored steamer removed")); }
		for (USceneComponent* B : Voy->Buoys) if (B) { FVector P = B->GetComponentLocation(); P.Z = 0.f; Obs.Add({ P, 6800.f }); }
	}
	auto Wall = [&](float CX, float CY, float YawDeg, float LenM)
	{
		const float Yr = FMath::DegreesToRadians(YawDeg);
		for (float s = -LenM * 0.5f; s <= LenM * 0.5f + 1.f; s += 35.f) Obs.Add({ H2W(CX + FMath::Cos(Yr) * s, CY + FMath::Sin(Yr) * s), 6800.f });
	};
	Wall(60.f, -190.f, 12.f, 300.f); Wall(10.f, 200.f, -18.f, 200.f);
	Obs.Add({ H2W(205.f, -160.f), 6800.f }); Obs.Add({ H2W(112.f, 170.f), 6400.f });   // harbour mouth tip rocks (signal tower, beacon)
	Obs.Add({ H2W(-170.f, -200.f), 17000.f }); Obs.Add({ H2W(-170.f, 0.f), 17000.f }); Obs.Add({ H2W(-170.f, 200.f), 17000.f });
	// Auto-calibrate every berth span XBow clear of static quay/rock/jetty OBBs
	if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()); Co && Ports.Num() > 0)
	{
		Co->Bake();
		for (int32 i = 0; i < Ports[0].Spans.Num(); ++i)
		{
			FNpcSpan& S = Ports[0].Spans[i];
			const float TestB = i < 6 ? 11.f : (i < 8 ? 12.5f : 5.f);
			const FIronwakeHullProfile Pr = AIronwakeNpcShip::GenericProfile(S.MaxLenM, TestB);
			TArray<FVector2D> Poly; UIronwakeColl::OutlineFromProfile(Pr, Poly, 80.f);
			const FIwShape Sh = UIronwakeColl::MakeShape(Poly, i < 6 ? 350.f : (i < 8 ? 300.f : 180.f));
			for (int32 Step = 0; Step < 20; ++Step)
			{
				const float OriginX = S.XBow + Pr.X1 + 0.025f * S.MaxLenM + 1.0f;
				const FVector Wp = H2W(OriginX, S.Y0);
				const FVector2D P2(Wp.X, Wp.Y);
				if (!Co->TestPose(nullptr, Sh, P2, 225.f, UIronwakeColl::MaskSolids).bHit && Co->MinDepthAt(Sh, P2, 225.f) >= Sh.DraughtCm) break;
				S.XBow += 2.f;
			}
		}
		// Ensure every outer waiting slot is >= 100 m clear of all buoys & static obstacles
		for (FWaitSlot& Ws : WaitSlots)
		{
			for (int32 Try = 0; Try < 16; ++Try)
			{
				const FVector Wp = H2W(Ws.H.X, Ws.H.Y);
				bool bNearObs = Co->TestPose(nullptr, UIronwakeColl::MakeShape(TArray<FVector2D>{ FVector2D(-3000, -1000), FVector2D(3000, -1000), FVector2D(3000, 1000), FVector2D(-3000, 1000) }, 350.f), FVector2D(Wp.X, Wp.Y), 0.f, UIronwakeColl::MaskSolids).bHit;
				for (const FObs& O : Obs) if (FVector::Dist2D(Wp, O.C) < O.R + 4500.f) { bNearObs = true; break; }
				if (!bNearObs) break;
				const float CurR = Ws.H.Size(), CurA = FMath::RadiansToDegrees(FMath::Atan2(Ws.H.Y, Ws.H.X)) + 9.f;
				const float NextR = FMath::Clamp(CurR + 18.f, 355.f, 515.f);
				Ws.H = FVector2D(NextR * FMath::Cos(FMath::DegreesToRadians(CurA)), NextR * FMath::Sin(FMath::DegreesToRadians(CurA)));
			}
		}
	}
}

void AIronwakeNpcDirector::PlanRoute(const FVector& A, const FVector& B, TArray<FVector>& Out, int32 Depth) const
{
	int32 Hit = INDEX_NONE; float HitT = 1e12f;
	for (int32 i = 0; i < Obs.Num() && Depth < 7; ++i)
	{
		const FObs& O = Obs[i];
		if (FVector::Dist2D(A, O.C) < O.R * 1.02f || FVector::Dist2D(B, O.C) < O.R * 1.02f) continue;
		FVector Q;
		if (SegDistQ(A, B, O.C, Q) < O.R) { const float T = FVector::Dist2D(A, Q); if (T < HitT) { HitT = T; Hit = i; } }
	}
	if (Hit == INDEX_NONE) { Out.Add(B); return; }
	const FObs& O = Obs[Hit];
	const float Rm = O.R * 1.15f;
	FVector Q; SegDistQ(A, B, O.C, Q);
	FVector N = (Q - O.C).GetSafeNormal2D();
	if (N.IsNearlyZero()) { const FVector D = (B - A).GetSafeNormal2D(); N = FVector(-D.Y, D.X, 0.f); }
	const float AngA = BrgDeg(O.C, A), AngB = BrgDeg(O.C, B), AngN = BrgDeg(O.C, O.C + N);
	const float SwPlus = Norm360(AngB - AngA);
	const float S = Norm360(AngN - AngA) <= SwPlus ? 1.f : -1.f;
	const float dA = FMath::Max(FVector::Dist2D(A, O.C), Rm * 1.001f), dB = FMath::Max(FVector::Dist2D(B, O.C), Rm * 1.001f);
	const float TA = FMath::RadiansToDegrees(FMath::Acos(Rm / dA)), TB = FMath::RadiansToDegrees(FMath::Acos(Rm / dB));
	const float AngS = AngA + S * TA, AngE = AngB - S * TB;
	const float Sw = Norm360(S * (AngE - AngS));
	TArray<FVector> Pts;
	if (Sw > 300.f) Pts.Add(PolarPt(O.C, AngS - S * (360.f - Sw) * 0.5f, Rm * 1.05f));
	else
	{
		const int32 NSeg = FMath::Max(1, FMath::CeilToInt(Sw / 40.f));
		const float Rd = Rm / FMath::Cos(FMath::DegreesToRadians(Sw / NSeg * 0.5f));
		for (int32 k = 0; k <= NSeg; ++k) Pts.Add(PolarPt(O.C, AngS + S * Sw * k / NSeg, (k == 0 || k == NSeg) ? Rm : Rd));
	}
	FVector Prev = A;
	for (const FVector& P : Pts) { PlanRoute(Prev, P, Out, Depth + 1); Prev = P; }
	PlanRoute(Prev, B, Out, Depth + 1);
}

int32 AIronwakeNpcDirector::NearestPort(const FVector& P) const
{
	int32 Best = 0; float BD = 1e12f;
	for (int32 i = 0; i < Ports.Num(); ++i) { const float D = FVector::DistSquared2D(Ports[i].Offing, P); if (D < BD) { BD = D; Best = i; } }
	return Best;
}

bool AIronwakeNpcDirector::ValidPose(const FNpcDef& D, const FVector& P, float Yaw, bool bShoal) const
{
	if (D.bAir) return true;
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()); if (!Co) return true;
	float Xc; const FIronwakeHullProfile Pr = AIronwakeNpcShip::ProfileFor(D, Xc);
	TArray<FVector2D> Poly; UIronwakeColl::OutlineFromProfile(Pr, Poly, FMath::Clamp(0.012f * D.LengthM * 100.f, 40.f, 120.f));
	const FIwShape Sh = UIronwakeColl::MakeShape(Poly, FMath::Max(D.DraftM * 100.f, 60.f));
	const FVector2D P2(P.X, P.Y);
	if (Co->TestPose(nullptr, Sh, P2, Yaw).bHit) return false;
	if (bShoal && Co->MinDepthAt(Sh, P2, Yaw) < Sh.DraughtCm) return false;
	return true;
}

// ---------------------------------------------------------------------------- berth & channel reservation arbitrator (P25)
void AIronwakeNpcDirector::InitWaitSlots()
{
	WaitSlots.Reset();
	// 9 outer anchorage/roads waiting slots: R = 385..505 m (in 340..520 m), >= 25 deg angular separation, all H.X >= 247 m seaward of breakwaters
	static const float A1[] = { -50.f, -25.f, 0.f, 25.f, 50.f };
	static const float A2[] = { -37.5f, -12.5f, 12.5f, 37.5f };
	for (float A : A1) WaitSlots.Add({ FVector2D(385.f * FMath::Cos(FMath::DegreesToRadians(A)), 385.f * FMath::Sin(FMath::DegreesToRadians(A))), -1, 0.0 });
	for (float A : A2) WaitSlots.Add({ FVector2D(505.f * FMath::Cos(FMath::DegreesToRadians(A)), 505.f * FMath::Sin(FMath::DegreesToRadians(A))), -1, 0.0 });
}

int32 AIronwakeNpcDirector::ReserveWaitSlot(FNpcRec& R)
{
	if (R.WaitSlot >= 0 && WaitSlots.IsValidIndex(R.WaitSlot) && WaitSlots[R.WaitSlot].Owner == R.Id) return R.WaitSlot;
	const FVector2D H = W2H(R.Pos);
	int32 Best = -1; float BestD = 1e18f;
	for (int32 i = 0; i < WaitSlots.Num(); ++i)
	{
		if (WaitSlots[i].Owner >= 0 && WaitSlots[i].Owner != R.Id) continue;
		const float D = FVector2D::DistSquared(H, WaitSlots[i].H);
		if (D < BestD) { BestD = D; Best = i; }
	}
	if (Best >= 0)
	{
		WaitSlots[Best].Owner = R.Id;
		WaitSlots[Best].ReservedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		R.WaitSlot = Best;
	}
	return Best;
}

void AIronwakeNpcDirector::ReleaseWaitSlot(FNpcRec& R)
{
	if (R.WaitSlot >= 0 && WaitSlots.IsValidIndex(R.WaitSlot) && WaitSlots[R.WaitSlot].Owner == R.Id) WaitSlots[R.WaitSlot].Owner = -1;
	R.WaitSlot = -1;
}

void AIronwakeNpcDirector::ExpireStaleReservations(double Now)
{
	// 1) Harbour mouth transit token timeout (> 180 game-s or owner clear of throat / inactive)
	if (MouthOwner >= 0)
	{
		const FNpcRec* O = RecAt(MouthOwner);
		const bool bActive = O && (O->State == ENpcState::Depart || O->State == ENpcState::Transit || O->State == ENpcState::Arrive);
		const bool bClearedOut = O && MouthDir > 0 && W2H(O->Pos).X > 268.f;
		if (!bActive || bClearedOut || (Now - MouthReservedAt) > 180.0) { MouthOwner = -1; MouthDir = 0; }
	}
	// 2) Berth & quay-lane corridor reservations (> 180 game-s stale timeout)
	if (Ports.Num() > 0)
	{
		for (int32 i = 0; i < Ports[0].Spans.Num(); ++i)
		{
			FNpcSpan& S = Ports[0].Spans[i];
			if (S.CorridorOwner >= 0)
			{
				const FNpcRec* O = RecAt(S.CorridorOwner);
				const bool bUsing = O && O->Lane == i && (O->State == ENpcState::Depart || O->State == ENpcState::Transit || O->State == ENpcState::Arrive);
				if (!bUsing || (Now - S.ReservedAt) > 180.0) S.CorridorOwner = -1;
			}
			if (S.Occupant >= 0)
			{
				const FNpcRec* O = RecAt(S.Occupant);
				// Refresh berth timestamp while ship is still inbound outside the waiting ring; enforce 180s inside basin/throat
				if (O && O->Lane == i && O->State == ENpcState::Transit && O->WaitIdx >= 0 && !O->bBasinWait) S.ReservedAt = Now;
				const bool bValid = O && O->Lane == i && (O->State == ENpcState::Docked || O->State == ENpcState::Depart || O->State == ENpcState::Transit || O->State == ENpcState::Arrive);
				if (!bValid || (O->State != ENpcState::Docked && (Now - S.ReservedAt) > 180.0))
				{
					if (S.HomeOf != S.Occupant) S.Occupant = -1;
					S.Occ.Reset();
					if (S.CorridorOwner == (O ? O->Id : -2)) S.CorridorOwner = -1;
				}
			}
		}
	}
	// 3) Waiting slots cleanup
	for (int32 i = 0; i < WaitSlots.Num(); ++i)
	{
		if (WaitSlots[i].Owner >= 0)
		{
			const FNpcRec* O = RecAt(WaitSlots[i].Owner);
			if (!O || O->WaitSlot != i || O->WaitIdx < 0 || O->State != ENpcState::Transit || (Now - WaitSlots[i].ReservedAt) > 360.0)
				WaitSlots[i].Owner = -1;
		}
	}
}

void AIronwakeNpcDirector::BerthPose(const FNpcRec& R, int32 Lane, FVector& OutPos, float& OutYaw) const
{
	const FNpcSpan& S = Ports[0].Spans[Lane];
	float Xc; const FIronwakeHullProfile Pr = AIronwakeNpcShip::ProfileFor(*R.Def, Xc);
	const float OriginX = S.XBow + Pr.X1 + 0.025f * R.Def->LengthM + 1.0f;
	OutPos = H2W(OriginX, S.Y0);
	OutYaw = 180.f + 45.f;
}

bool AIronwakeNpcDirector::ReserveBerthAndCorridor(FNpcRec& R, bool bNeedCorridor)
{
	FNpcPort& P = Ports[0];
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float BowSternClearM = 0.35f * R.Def->LengthM + 10.f;
	const float HalfY = 0.5f * R.Def->BeamM + 4.5f;

	auto CanUse = [&](int32 Idx) -> bool
	{
		FNpcSpan& S = P.Spans[Idx];
		if (S.MaxLenM + 0.5f < R.Def->LengthM) return false;
		// Verify >= 0.35 * LengthM + 10 m longitudinal clearance before basin turn line (X = +62 m)
		if (62.f - (S.XBow + R.Def->LengthM) < BowSternClearM - 16.f && R.Def->LengthM > S.MaxLenM) return false;
		if (S.Occupant >= 0 && S.Occupant != R.Id) return false;
		if (S.HomeOf >= 0 && S.HomeOf != R.Id) return false;
		const float YMin = S.Y0 - HalfY, YMax = S.Y0 + HalfY;
		for (int32 j = 0; j < P.Spans.Num(); ++j)
		{
			for (const FVector& Iv : P.Spans[j].Occ)
			{
				if ((int32)FMath::RoundToInt(Iv.Z) == R.Id) continue;
				if (YMin < Iv.Y && YMax > Iv.X) return false;
			}
		}
		if (bNeedCorridor)
		{
			if (S.CorridorOwner >= 0 && S.CorridorOwner != R.Id) return false;
			for (int32 j = 0; j < P.Spans.Num(); ++j)
				if (j != Idx && P.Spans[j].CorridorOwner >= 0 && P.Spans[j].CorridorOwner != R.Id && FMath::Abs(P.Spans[j].Y0 - S.Y0) < 24.f)
					return false;
		}
		for (const FNpcRec& Other : Recs)
		{
			if (Other.Id == R.Id || Other.Lane != Idx) continue;
			if (Other.State == ENpcState::Docked || Other.State == ENpcState::Arrive || Other.State == ENpcState::Depart) return false;
		}
		return true;
	};

	auto Commit = [&](int32 Idx)
	{
		if (R.Lane >= 0 && R.Lane != Idx && P.Spans.IsValidIndex(R.Lane))
		{
			FNpcSpan& Old = P.Spans[R.Lane];
			if (Old.Occupant == R.Id && Old.HomeOf != R.Id) Old.Occupant = -1;
			if (Old.CorridorOwner == R.Id) Old.CorridorOwner = -1;
			Old.Occ.Reset();
		}
		FNpcSpan& S = P.Spans[Idx];
		S.Occupant = R.Id;
		S.Occ = { FVector(S.Y0 - HalfY, S.Y0 + HalfY, (float)R.Id) };
		S.ReservedAt = Now;
		if (bNeedCorridor) S.CorridorOwner = R.Id;
		R.Lane = Idx;
		R.Span = Idx; R.SpanY = S.Y0;
	};

	if (R.Lane >= 0 && P.Spans.IsValidIndex(R.Lane) && CanUse(R.Lane)) { Commit(R.Lane); return true; }
	if (R.HomeLane >= 0 && P.Spans.IsValidIndex(R.HomeLane) && CanUse(R.HomeLane)) { Commit(R.HomeLane); return true; }
	if (R.HomeLane < 0)
	{
		for (int32 i = 0; i < 6; ++i) if (CanUse(i)) { Commit(i); return true; }
	}
	return false;
}

void AIronwakeNpcDirector::ReleaseCorridorOnly(FNpcRec& R)
{
	if (Ports.Num() == 0) return;
	for (FNpcSpan& S : Ports[0].Spans) if (S.CorridorOwner == R.Id) S.CorridorOwner = -1;
}

bool AIronwakeNpcDirector::PickLane(FNpcRec& R)
{
	return ReserveBerthAndCorridor(R, false);
}

void AIronwakeNpcDirector::ReleaseLane(FNpcRec& R)
{
	if (Ports.Num() > 0 && R.Lane >= 0 && Ports[0].Spans.IsValidIndex(R.Lane))
	{
		FNpcSpan& S = Ports[0].Spans[R.Lane];
		if (S.CorridorOwner == R.Id) S.CorridorOwner = -1;
		S.Occ.RemoveAll([&](const FVector& Iv) { return (int32)FMath::RoundToInt(Iv.Z) == R.Id; });
		if (S.Occupant == R.Id) S.Occupant = -1;
	}
	R.Lane = -1; R.Span = -1;
}

bool AIronwakeNpcDirector::MouthFreeFor(const FNpcRec& R, int32 Dir) const
{
	const FVector MouthW = H2W(155.f, 0.f);
	if (MouthOwner >= 0 && MouthOwner != R.Id) return false;
	for (const FNpcRec& O : Recs)
	{
		if (O.Id == R.Id || O.Def->bAir) continue;
		if (O.State == ENpcState::Docked || O.State == ENpcState::Anchored || O.bHold || O.bBasinWait) continue;
		const FVector2D H = W2H(O.Pos);
		const bool bInThroat = (FVector::Dist2D(O.Pos, MouthW) < 26000.f) || (H.X > -35.f && H.X < 260.f && FMath::Abs(H.Y) < 155.f);
		if (bInThroat) return false;
	}
	return true;
}

void AIronwakeNpcDirector::MouthTake(int32 Me, int32 Dir)
{
	if (RecAt(Me))
	{
		MouthOwner = Me;
		MouthDir = Dir;
		MouthReservedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}
}

void AIronwakeNpcDirector::MouthDrop(int32 Me)
{
	if (MouthOwner == Me) { MouthOwner = -1; MouthDir = 0; }
}

void AIronwakeNpcDirector::BuildInboundBasinRoute(FNpcRec& R)
{
	R.Route.Reset(); R.RouteIdx = 0;
	if (R.Lane < 0 || !Ports[0].Spans.IsValidIndex(R.Lane)) return;
	const FNpcSpan& S = Ports[0].Spans[R.Lane];
	const FVector2D HNow = W2H(R.Pos);
	// Inbound starboard mouth lane: Y = -34 m (68 m separated from outbound port lane Y = +46 m). Always enter via outer mouth gate if outside throat!
	if (HNow.X > 225.f || FMath::Abs(HNow.Y) > 95.f) PlanRoute(R.Pos, H2W(280.f, -34.f), R.Route);
	if (R.Lane == 6)
	{
		if (HNow.X > 150.f) R.Route.Add(H2W(145.f, -40.f));
		R.Route.Add(H2W(52.f, S.Y0));
	}
	else if (S.Y0 < 0.f)
	{
		if (HNow.X > 160.f) R.Route.Add(H2W(155.f, -48.f));
		if (HNow.X > 98.f) R.Route.Add(H2W(92.f, S.Y0));
		R.Route.Add(H2W(48.f, S.Y0));
	}
	else if (R.Lane == 7)
	{
		if (HNow.X > 190.f) R.Route.Add(H2W(185.f, 30.f));
		if (HNow.X > 128.f) R.Route.Add(H2W(122.f, 92.f));
		if (HNow.X > 74.f) R.Route.Add(H2W(68.f, S.Y0));
		R.Route.Add(H2W(38.f, S.Y0));
	}
	else
	{
		if (HNow.X > 190.f) R.Route.Add(H2W(185.f, 26.f));
		if (HNow.X > 126.f) R.Route.Add(H2W(120.f, FMath::Min(S.Y0, 96.f)));
		R.Route.Add(H2W(58.f, S.Y0));
	}
}

// ---------------------------------------------------------------------------- state helpers
void AIronwakeNpcDirector::SetState(FNpcRec& R, ENpcState S)
{
	if (R.State == S) return;
	if (R.State == ENpcState::Docked || R.State == ENpcState::Anchored || R.bBasinWait) R.bHold = false;
	R.State = S; R.StateT = 0.f; R.Phase = 0; R.StuckT = 0.f; R.StuckPos = R.Pos;
	if (S == ENpcState::Docked || S == ENpcState::Anchored) { R.bHold = true; R.DesKn = 0.f; R.SpeedKn = 0.f; }
	static const bool bLog = FParse::Param(FCommandLine::Get(), TEXT("iwnpclog"));
	if (bLog) Log(FString::Printf(TEXT("IWNPC state %s -> %s (%s) at (%.0f,%.0f)"), *R.Name, NpcStateName(S), *R.Target, R.Pos.X / 100.f, R.Pos.Y / 100.f));
}

float AIronwakeNpcDirector::HourNowF() const { return HourNow(); }

void AIronwakeNpcDirector::StartTrip(FNpcRec& R, int32 DestPort, ENpcState Then)
{
	// route from here to a port. Greyhaven: reserve outer waiting slot first, then arbitrator grants mouth + corridor into berth.
	R.DestPort = DestPort; R.Route.Reset(); R.RouteIdx = 0; R.bHeld = false; R.bBasinWait = false; R.BasinWaitT = 0.f;
	ReleaseWaitSlot(R);
	R.Target = Ports[DestPort].Name;
	const FVector Start = R.Pos;
	if (DestPort == 0)
	{
		const FVector2D HStart = W2H(Start);
		if (HStart.X < 245.f && FMath::Abs(HStart.Y) < 170.f)
		{
			if (!ReserveBerthAndCorridor(R, true))
			{
				const int32 Slot = ReserveWaitSlot(R);
				R.WaitPt = Slot >= 0 ? H2W(WaitSlots[Slot].H.X, WaitSlots[Slot].H.Y) : H2W(420.f, 0.f);
				PlanRoute(Start, R.WaitPt, R.Route); R.WaitIdx = R.Route.Num() - 1; R.DestKind = 0;
				SetState(R, Then); return;
			}
			MouthTake(R.Id, -1);
			R.WaitIdx = -1;
			BuildInboundBasinRoute(R);
			R.DestKind = 0;
			SetState(R, Then);
			return;
		}
		// Pre-reserve berth if available (or acquire at waiting slot when one frees up)
		PickLane(R);
		const int32 Slot = ReserveWaitSlot(R);
		R.WaitPt = Slot >= 0 ? H2W(WaitSlots[Slot].H.X, WaitSlots[Slot].H.Y) : H2W(440.f + (R.Id % 3) * 30.f, ((R.Id % 5) - 2) * 85.f);
		PlanRoute(Start, R.WaitPt, R.Route);
		R.WaitIdx = R.Route.Num() - 1;
		R.DestKind = 0;
	}
	else
	{
		// If departing Greyhaven outer channel toward SE (Port 1 Kestrel Isle), swing outside the R=465m waiting ring first!
		const FVector2D HStart = W2H(Start);
		if (HStart.X > 150.f && HStart.X < 650.f && FMath::Abs(HStart.Y) < 250.f && DestPort == 1)
		{
			R.Route.Add(H2W(620.f, 70.f));
			R.Route.Add(H2W(680.f, -210.f));
		}
		const FVector Origin = R.Route.Num() ? R.Route.Last() : Start;
		// anchorage slot: spread arrivals on a grid toward the harbour side of the offing so two ships never aim at one point
		const FVector Tow = (HarbourC - Ports[DestPort].Offing).GetSafeNormal2D(); const FVector Lat(-Tow.Y, Tow.X, 0.f);
		const FVector Slot = Ports[DestPort].Offing + Tow * (float)(((R.Id / 4) % 3) * 38000.f / 3.f) + Lat * ((float)(R.Id % 4) - 1.5f) * 10500.f;
		PlanRoute(Origin, Slot, R.Route); R.DestKind = 1;
	}
	SetState(R, Then);
}

bool AIronwakeNpcDirector::FollowRoute(FNpcRec& R, float Kn, float FinalSlowM)
{
	if (R.RouteIdx >= R.Route.Num()) return true;
	const FVector& T = R.Route[R.RouteIdx];
	const bool bLast = R.RouteIdx == R.Route.Num() - 1;
	const FVector2D HT = W2H(T);
	const bool bHarbourPt = (HT.X > -80.f && HT.X < 295.f && FMath::Abs(HT.Y) < 220.f);
	const float Reach = bHarbourPt
		? (bLast ? 1800.f : FMath::Clamp(R.Def->LengthM * 55.f, 1600.f, 2800.f))
		: (bLast ? 2500.f : FMath::Clamp(R.Def->LengthM * 140.f, 3500.f, 12000.f));
	const float D = FVector::Dist2D(R.Pos, T);
	if (D < Reach) { ++R.RouteIdx; if (R.RouteIdx >= R.Route.Num()) return true; }
	const FVector& N = R.Route[R.RouteIdx];
	R.DesYaw = BrgDeg(R.Pos, N);
	R.DesKn = Kn;
	if (R.RouteIdx == R.Route.Num() - 1 && FinalSlowM > 0.f) R.DesKn = FMath::Min(Kn, FMath::Max(0.8f, FVector::Dist2D(R.Pos, N) / 100.f / FinalSlowM * Kn));
	return false;
}

void AIronwakeNpcDirector::DoSailing(FNpcRec& R, const FVector& Target, float& OutYaw)
{
	const float Wf = IronwakeFleet::WindFromYaw(GetWorld());
	const float ToT = BrgDeg(R.Pos, Target);
	const float TWA = FMath::FindDeltaAngleDegrees(Wf, ToT);
	const float Dist = FVector::Dist2D(R.Pos, Target);
	if (Dist < 30000.f || FMath::Abs(TWA) > (R.BeatTack != 0 ? 60.f : 52.f)) { R.BeatTack = 0; OutYaw = ToT; return; }
	if (R.BeatTack == 0) { R.BeatTack = TWA >= 0.f ? 1 : -1; R.BeatFrom = R.Pos; R.TackT = 0.f; }
	const FVector U = (Target - R.BeatFrom).GetSafeNormal2D(); const FVector Nl(-U.Y, U.X, 0.f);
	const float Off = FVector::DotProduct(R.Pos - R.BeatFrom, Nl);
	const float Hc = Wf + R.BeatTack * 47.f;
	const float Moving = FVector::DotProduct(FRotator(0.f, Hc, 0.f).Vector(), Nl);
	const float Corridor = FMath::Max(Dist * 0.14f, 24000.f);
	if (R.TackT > 40.f && Moving * Off > 0.f && FMath::Abs(Off) > Corridor) { R.BeatTack = -R.BeatTack; R.TackT = 0.f; R.BeatFrom = R.Pos; }
	OutYaw = Wf + R.BeatTack * 47.f;
}

float AIronwakeNpcDirector::LandAvoidYaw(const FNpcRec& R, float Des, float LookCm) const
{
	UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()); if (!Co) return Des;
	const FVector2D P(R.Pos.X, R.Pos.Y);
	const float HalfB = R.Def->BeamM * 60.f + 400.f, Draught = FMath::Max(R.Def->DraftM * 100.f, 60.f);
	if (Co->PathClear(P, Des, LookCm, HalfB, Draught) >= LookCm) return Des;
	for (float Off = 15.f; Off <= 150.f; Off += 15.f)
		for (int32 S = -1; S <= 1; S += 2) { const float Y = Des + S * Off; if (Co->PathClear(P, Y, LookCm, HalfB, Draught) >= LookCm) return Y; }
	return Des + 90.f;
}

// COLREGS-lite: steam gives way to sail, moving gives way to stationary/waiting, head-on both alter to starboard, sail bears away rather than pinching into irons.
void AIronwakeNpcDirector::Avoid(FNpcRec& R, float Dt, bool bLand, bool bHarb)
{
	const FNpcDef& D = *R.Def;
	AIronwakeNpcShip* B = R.Body.Get(); if (!B) return;
	const float MyV = B->V;
	const FVector MyDir = FRotator(0.f, R.Yaw, 0.f).Vector();
	const FVector MyVel = MyDir * MyV;
	float Delta = 0.f, Slow = 1.f;
	const float MyLen = D.LengthM * 100.f;
	const bool bMySail = D.bSail && !R.bAux;
	const float Wf = bMySail ? IronwakeFleet::WindFromYaw(GetWorld()) : 0.f;
	for (const FNpcContact& C : Contacts)
	{
		if (C.RecId == R.Id) continue;
		if (C.RecId >= 0 && Recs[C.RecId].bHold && Recs[C.RecId].State == ENpcState::Docked && (bHarb || InHarbour(R.Pos, 45.f))) continue;
		const FNpcRec* OtherRec = C.RecId >= 0 ? &Recs[C.RecId] : nullptr;
		const bool bOtherSail = OtherRec && OtherRec->Def->bSail && !OtherRec->bAux;
		const FVector Rel = C.Pos - R.Pos; const float Dist = Rel.Size2D();
		if (Dist > 130000.f) continue;
		const float RelBrg = FRotator::NormalizeAxis(Rel.Rotation().Yaw - R.Yaw);
		const float OtherCourse = C.Vel.Size2D() > 40.f ? C.Vel.Rotation().Yaw : BrgDeg(FVector::ZeroVector, MyDir) + 180.f;
		const float Cross = FMath::Abs(FRotator::NormalizeAxis(OtherCourse - R.Yaw));
		const bool bStationary = C.bStationary;
		const bool bHeadOn = !bStationary && FMath::Abs(RelBrg) < 25.f && Cross > 140.f;
		// Priority order: legacy/player (<0) > stationary > sail-under-canvas > capital ship > lower RecId; head-on both turn to starboard
		const bool bHigherPrio = (C.RecId < 0) || bStationary || (!bMySail && bOtherSail) || (OtherRec && OtherRec->Def->HullSize > D.HullSize && !bMySail) || (!bMySail == !bOtherSail && C.RecId < R.Id);
		if (!bHigherPrio && !bHeadOn) continue;
		const FVector RV = C.Vel - MyVel;
		const bool bBrigPair = D.Id == TEXT("brig") || (OtherRec && OtherRec->Def->Id == TEXT("brig"));
		const float Safe = (0.75f * (MyLen + C.LenCm) + 3800.f + (C.RecId < 0 ? 4500.f : 0.f)) * (bBrigPair ? 1.55f : 1.f);
		const float RV2 = RV.SizeSquared2D();
		float Tcpa = 0.f, Cpa = Dist;
		if (RV2 > 1.f) { Tcpa = -(Rel.X * RV.X + Rel.Y * RV.Y) / RV2; Cpa = (Rel + RV * FMath::Max(Tcpa, 0.f)).Size2D(); }
		if (Tcpa < 0.f && Dist > Safe) continue;
		if (Tcpa > 125.f || Cpa > Safe) continue;
		const bool bOvertaking = !bStationary && Cross < 45.f && FMath::Abs(RelBrg) < 50.f && MyV > C.Vel.Size2D();
		const bool bGiveWay = bStationary || bHeadOn || bOvertaking || (RelBrg > 0.f && RelBrg < 115.f) || bHigherPrio;
		if (bGiveWay)
		{
			const float CourseRelBrg = FRotator::NormalizeAxis(Rel.Rotation().Yaw - R.DesYaw);
			float Turn = bStationary ? (CourseRelBrg >= 0.f ? -42.f : 42.f) : 36.f;
			if (bMySail && FMath::Abs(FMath::FindDeltaAngleDegrees(Wf, R.DesYaw + Turn)) < 40.f) Turn = -38.f;   // bear away instead of pinching into irons
			Delta += Turn * FMath::Clamp(1.3f - Cpa / Safe, 0.45f, 1.f);
			if (!bMySail)
			{
				if (Tcpa < 35.f || Dist < Safe * 1.15f) Slow = FMath::Min(Slow, 0.6f);
				if (Dist < Safe * 0.7f && Tcpa < 18.f) Slow = FMath::Min(Slow, 0.45f);
			}
		}
		else if (Dist < Safe * 0.85f && !bMySail) Slow = FMath::Min(Slow, 0.65f);
	}
	Delta = FMath::Clamp(Delta, -55.f, 55.f);
	if (FMath::Abs(Delta) > 1.f) { R.AvoidDelta = Delta; R.AvoidHoldT = 6.5f; }
	else { R.AvoidHoldT -= Dt; if (R.AvoidHoldT <= 0.f) R.AvoidDelta *= FMath::Exp(-1.6f * Dt); }
	R.AvoidSlow = Slow;
	if (!bHarb && !InHarbour(R.Pos, 45.f)) R.DesYaw = FRotator::NormalizeAxis(R.DesYaw + R.AvoidDelta);
	if (bLand && R.DesKn > 0.5f && !bHarb && !InHarbour(R.Pos, 20.f))
	{
		const float Look = FMath::Clamp(MyV * 22.f + MyLen * 2.f, 25000.f, 70000.f);
		R.DesYaw = LandAvoidYaw(R, R.DesYaw, Look);
	}
	R.DesKn *= R.AvoidSlow;
}

// ---------------------------------------------------------------------------- ledger-only motion
void AIronwakeNpcDirector::Advance(FNpcRec& R, float DistCm)
{
	while (DistCm > 1.f && R.RouteIdx < R.Route.Num())
	{
		const FVector D = R.Route[R.RouteIdx] - R.Pos; const float L = D.Size2D();
		if (L <= DistCm) { R.Pos = FVector(R.Route[R.RouteIdx].X, R.Route[R.RouteIdx].Y, 0.f); DistCm -= L; ++R.RouteIdx; }
		else { R.Yaw = D.Rotation().Yaw; R.Pos += FVector(D.X, D.Y, 0.f) / L * DistCm; DistCm = 0.f; }
	}
}

void AIronwakeNpcDirector::FarStep(FNpcRec& R, float Dt)
{
	if (R.Def->bAir) { AdvanceAir(R, Dt); return; }
	switch (R.State)
	{
	case ENpcState::Transit: case ENpcState::Patrol: case ENpcState::Depart: case ENpcState::Flee: case ENpcState::Aggressive:
		if (R.State == ENpcState::Depart && R.Phase == 0 && R.Lane >= 0)
		{ DepartPhase1(R); R.RouteIdx = FMath::Min(2, R.Route.Num() - 1); R.Pos = FVector(R.Route[R.RouteIdx].X, R.Route[R.RouteIdx].Y, 0.f); R.Yaw = 45.f; }   // far: skip the reverse-out
		R.SpeedKn = FMath::Max(R.DesKn, 0.f);
		Advance(R, KnToSim(R.SpeedKn) * Dt);
		break;
	case ENpcState::Arrive:
		if (R.DestKind == 0 && R.Lane >= 0)
		{
			FVector P; float Y; BerthPose(R, R.Lane, P, Y); R.Pos = P; R.Yaw = Y; R.SpeedKn = 0.f; ArriveDone(R);
		}
		else if (R.DestKind == 2) { R.SpeedKn = 0.f; ArriveDone(R); }
		break;
	case ENpcState::Escort:
	{
		FNpcRec* L = RecAt(R.Leader);
		if (L) { const FVector Fw = FRotator(0.f, L->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f); R.Pos = L->Pos + Fw * R.Station.X + Rt * R.Station.Y; R.Yaw = L->Yaw; R.SpeedKn = L->SpeedKn; }
		break;
	}
	default: R.SpeedKn = 0.f; break;
	}
}

void AIronwakeNpcDirector::AdvanceAir(FNpcRec& R, float Dt)
{
	if (R.PatrolPts.Num() == 0) return;
	float Dist = KnToSim(R.Def->CruiseKn) * Dt;
	while (Dist > 1.f)
	{
		const FVector T = R.PatrolPts[R.PatrolIdx]; const FVector D = T - R.Pos; const float L = D.Size2D();
		if (L <= Dist) { R.Pos = FVector(T.X, T.Y, 0.f); Dist -= L; R.PatrolIdx = (R.PatrolIdx + 1) % R.PatrolPts.Num(); }
		else { R.Yaw = D.Rotation().Yaw; R.Pos += FVector(D.X, D.Y, 0.f) / L * Dist; Dist = 0.f; }
	}
	R.SpeedKn = R.Def->CruiseKn;
}

// ---------------------------------------------------------------------------- arrival / mooring / departure
void AIronwakeNpcDirector::ArriveDone(FNpcRec& R)
{
	BasinDrop(R.Id);
	MouthDrop(R.Id);
	ReleaseWaitSlot(R);
	ReleaseCorridorOnly(R);
	FNpcPort& P = Ports[FMath::Max(R.DestPort, 0)];
	R.CurPort = R.DestPort; ++P.Arrivals; ++R.TripCount;
	R.Route.Reset(); R.RouteIdx = 0;
	if (R.DestKind == 0 && R.Lane >= 0)
	{
		FVector BP; float BY; BerthPose(R, R.Lane, BP, BY); R.BerthPos = BP; R.BerthYaw = BY;
		R.Pos = BP; R.Yaw = BY;
		const int32 Crates = Rng.RandRange(20, 90); P.Boarded += Crates;
		Log(FString::Printf(TEXT("IWNPC LEDGER %s moored at %s lane %s: %d crates unloaded (port total %d)"), *R.Name, *P.Name, *P.Spans[R.Lane].Name, Crates, P.Boarded));
		SetState(R, ENpcState::Docked);
		R.Target = FString::Printf(TEXT("%s %s"), *P.Name, *P.Spans[R.Lane].Name);
	}
	else if (R.DestKind == 2 && R.State == ENpcState::Arrive && R.Lane < 0 && R.bHomePt) { SetState(R, ENpcState::Anchored); R.Target = TEXT("mooring"); }
	else { SetState(R, ENpcState::Anchored); R.Target = FString::Printf(TEXT("anchored off %s"), *P.Name); }
	R.NextActionT = (float)GetWorld()->GetTimeSeconds() + Dwell(R);
	// helpers go home
	if (R.PilotId >= 0) { if (FNpcRec* Pi = RecAt(R.PilotId)) if (Pi->Leader == R.Id) Pi->Leader = -1; R.PilotId = -1; }
	if (R.TugId >= 0) { if (FNpcRec* Tg = RecAt(R.TugId)) if (Tg->Leader == R.Id) Tg->Leader = -1; R.TugId = -1; }
}

float AIronwakeNpcDirector::Dwell(const FNpcRec& R) const
{
	switch (R.Role)
	{
	case ENpcRole::Merchant: return R.CurPort == 0 ? 135.f + 25.f * (R.Id % 4) : 55.f + 15.f * (R.Id % 3);
	case ENpcRole::Ferry: return 95.f;
	case ENpcRole::Harbour: return 105.f + 20.f * (R.Id % 3);
	case ENpcRole::Taxi: return 35.f + 12.f * (R.Id % 3);
	default: return 60.f;
	}
}

void AIronwakeNpcDirector::DepartPhase1(FNpcRec& R)
{
	const FVector2D H = W2H(R.Pos);
	R.Phase = 1; R.StuckT = 0.f; R.StuckPos = R.Pos;
	const int32 OldLane = R.Lane;
	const float Y0 = OldLane >= 0 ? Ports[0].Spans[OldLane].Y0 : H.Y;
	R.Route.Reset(); R.RouteIdx = 0;
	// Outbound port-chart channel: Y = +34 m (68 m separated from inbound Y = -34 m); never cross ModularPier (X <= 90, Y in [-16, +8]) or jetties
	if (OldLane == 7)
	{
		// Tender (Y=139): already backed past Jetty North (X > 42); curve south of North Breakwater into outbound fairway clear of WaitSlots & SteamBuoy
		R.Route.Add(H2W(68.f, 122.f));
		R.Route.Add(H2W(122.f, 86.f));
		R.Route.Add(H2W(205.f, 44.f));
		PlanRoute(H2W(205.f, 44.f), H2W(575.f, 55.f), R.Route);
	}
	else if (Y0 > 0.f)
	{
		// North basin (N1..N3, Pilot, Gig, Gig2): first point turns bow NORTH (+Y, Yaw +135) away from ModularPier, then out outbound fairway
		R.Route.Add(H2W(H.X + 4.f, FMath::Clamp(Y0 + 22.f, 44.f, 104.f)));
		R.Route.Add(H2W(92.f, FMath::Clamp(Y0 + 18.f, 44.f, 98.f)));
		R.Route.Add(H2W(145.f, 46.f));
		R.Route.Add(H2W(215.f, 44.f));
		PlanRoute(H2W(215.f, 44.f), H2W(575.f, 55.f), R.Route);
	}
	else
	{
		// South basin (S1..S3, Lighter): turn away from South Breakwater while staying well south of ModularPier until X >= 142 m
		R.Route.Add(H2W(H.X + 6.f, FMath::Clamp(Y0 + 20.f, -92.f, -40.f)));
		R.Route.Add(H2W(102.f, FMath::Clamp(Y0 + 24.f, -88.f, -40.f)));
		R.Route.Add(H2W(145.f, -34.f));
		R.Route.Add(H2W(220.f, 42.f));
		PlanRoute(H2W(220.f, 42.f), H2W(575.f, 55.f), R.Route);
	}
	// Berth occupant freed once clear of berth; corridor lock kept until X > 120 m in Depart tick
	if (OldLane >= 0 && Ports[0].Spans.IsValidIndex(OldLane))
	{
		FNpcSpan& S = Ports[0].Spans[OldLane];
		S.Occ.RemoveAll([&](const FVector& Iv) { return (int32)FMath::RoundToInt(Iv.Z) == R.Id; });
		if (S.Occupant == R.Id) S.Occupant = -1;
	}
	R.Target = TEXT("leaving Greyhaven");
}

void AIronwakeNpcDirector::BeginDepartFromLane(FNpcRec& R)
{
	BasinTake(R.Id);
	MouthTake(R.Id, 1);
	if (R.Lane >= 0 && Ports[0].Spans.IsValidIndex(R.Lane))
	{
		Ports[0].Spans[R.Lane].CorridorOwner = R.Id;
		Ports[0].Spans[R.Lane].ReservedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}
	R.Phase = 0; R.bHold = false; R.bAux = true;
	SetState(R, ENpcState::Depart);
}

bool AIronwakeNpcDirector::ShouldShelter(const FNpcRec& R) const
{
	return R.Faction != ENpcFaction::Crown && R.Role != ENpcRole::Airship && R.Role != ENpcRole::Pirate && R.Def->HullSize <= 1 && R.Role != ENpcRole::Pilot && R.Role != ENpcRole::Tug;
}

bool AIronwakeNpcDirector::BasinFree(int32 Me)
{
	const FNpcRec* R = RecAt(Me);
	return R ? MouthFreeFor(*R, -1) : true;
}
void AIronwakeNpcDirector::BasinTake(int32 Me) { if (BasinOwner != Me) BasinT = GetWorld()->GetTimeSeconds(); BasinOwner = Me; }
void AIronwakeNpcDirector::BasinDrop(int32 Me) { if (BasinOwner == Me) BasinOwner = -1; }

// ---------------------------------------------------------------------------- the brain
void AIronwakeNpcDirector::TickBrain(FNpcRec& R, float Dt)
{
	UWorld* W = GetWorld();
	const FNpcDef& D = *R.Def;
	const float Now = (float)W->GetTimeSeconds();
	R.StateT += Dt;
	const bool bNear = R.Body.IsValid();
	const FVector PP = PlayerPos();
	const float DPlayer = FVector::Dist2D(R.Pos, PP);
	const float Hour = HourNow();
	const bool bGale = IsGale();
	const float PrevDesKn = R.DesKn;
	R.DesKn = 0.f; R.bAux = false;
	if (D.bAir) { AirBrain(R, Dt); return; }

	// ---- stuck monitor (only while brain actively orders headway, vessel is not intentionally holding/waiting, and speed is stalled)
	const bool bMakingWay = bNear && FMath::Abs(R.Body->V) > 155.f && R.Body->BlockedT < 0.05f && Now >= R.Body->BackUntil;
	const bool bIntentionalWait = bMakingWay || R.bHold || R.bBasinWait || R.State == ENpcState::Docked || R.State == ENpcState::Anchored || R.State == ENpcState::Drift
		|| (R.State == ENpcState::Depart && R.Phase == 0) || R.State == ENpcState::Arrive || R.State == ENpcState::Fish || R.State == ENpcState::Work || R.State == ENpcState::Moor
		|| (R.State == ENpcState::Escort && R.SpeedKn < 0.3f) || FMath::Abs(PrevDesKn) < 0.45f;
	if (bNear && !bIntentionalWait)
	{
		R.StuckT += Dt;
		const float StuckLimit = (R.AvoidSlow < 0.7f) ? 150.f : 55.f;
		if (R.StuckT >= StuckLimit)
		{
			const float Moved = FVector::Dist2D(R.Pos, R.StuckPos);
			const bool bSailBecalmed = D.bSail && R.Body->SailKn < 0.6f && !R.bAux;
			if (Moved < 2200.f && !bSailBecalmed && R.StateT > 45.f)
			{
				++R.Stucks; ++StatStuck;
				{ const FVector2D Hh = W2H(R.Pos); Log(FString::Printf(TEXT("IWNPC STUCKH %s H(%.0f,%.0f) wait=%d/%d mouthOwner=%d bw=%d dest=%d kind=%d lane=%d sailKn=%.1f twa=%.0f aux=%d tack=%d blockedT=%.1f back=%.0f"), *R.Name, Hh.X, Hh.Y, R.WaitIdx, R.RouteIdx, MouthOwner, R.bBasinWait ? 1 : 0, R.DestPort, R.DestKind, R.Lane, R.Body->SailKn, R.Body->TrueWindAngle, R.bAux ? 1 : 0, R.BeatTack, R.Body->BlockedT, R.Body->BackUntil)); }
				Log(FString::Printf(TEXT("IWNPC STUCK %s state=%s ph=%d at (%.0f,%.0f) m moved %.0f m in %.0f s route %d/%d V=%.1f desKn=%.1f eng=%.1f avoid=%.2f/%.0f yaw=%.0f des=%.0f hold=%d"), *R.Name, NpcStateName(R.State), R.Phase, R.Pos.X / 100.f, R.Pos.Y / 100.f, Moved / 100.f, StuckLimit, R.RouteIdx, R.Route.Num(), R.Body->V / 51.4444f / 1.5f, PrevDesKn, R.Body->EngineKn, R.AvoidSlow, R.AvoidDelta, R.Yaw, R.DesYaw, R.bHold ? 1 : 0));
				ReleaseCorridorOnly(R); MouthDrop(R.Id); ReleaseWaitSlot(R);
				R.Phase = 0; R.PendState = R.State; SetState(R, ENpcState::Drift); R.RouteIdx = 0;
			}
			R.StuckPos = R.Pos; R.StuckT = 0.f;
		}
	}
	else { R.StuckPos = R.Pos; R.StuckT = 0.f; }

	// ---- weather: sheltering
	if (bGale && ShouldShelter(R) && !R.bSheltering && (R.State == ENpcState::Fish || R.State == ENpcState::Transit || R.State == ENpcState::Patrol || R.State == ENpcState::Work))
	{
		R.bSheltering = true;
		if (R.HomeLane >= 0 || R.bHomePt) { R.DestKind = R.HomeLane >= 0 ? 0 : 2; GoHome(R); }
		else StartTrip(R, NearestPort(R.Pos), ENpcState::Transit);
	}
	if (!bGale) R.bSheltering = false;

	switch (R.State)
	{
	// ------------------------------------------------------------------ at rest
	case ENpcState::Docked:
	case ENpcState::Anchored:
	{
		R.bHold = true; R.DesKn = 0.f; R.bBasinWait = false;
		Decide(R);
		break;
	}
	case ENpcState::Drift:
	{
		R.DesKn = 0.f; R.DesYaw = R.Yaw; R.bBasinWait = false;
		if (R.StateT > 20.f)
		{
			const ENpcState S = R.PendState; R.Route.Reset(); R.RouteIdx = 0; R.StuckT = 0.f; R.NextActionT = Now;
			BasinDrop(R.Id); MouthDrop(R.Id); ReleaseCorridorOnly(R); ReleaseWaitSlot(R);
			if (S == ENpcState::Transit || S == ENpcState::Arrive || S == ENpcState::Depart)
			{
				if (R.HomeLane >= 0 && R.Itinerary.Num() == 0) GoHome(R);
				else if (R.DestPort >= 0) StartTrip(R, R.DestPort, ENpcState::Transit);
				else SetState(R, ENpcState::Anchored);
			}
			else SetState(R, ENpcState::Anchored);
		}
		break;
	}
	// ------------------------------------------------------------------ leaving the berth
	case ENpcState::Depart:
	{
		R.bAux = true; R.bBasinWait = false;
		if (R.Phase == 0)
		{
			// back straight out along the reserved lane until bow clears all jetties and neighbor sterns
			FVector BP; float BY = 225.f; if (R.Lane >= 0) BerthPose(R, R.Lane, BP, BY);
			const FVector2D H = W2H(R.Pos);
			const float LY = R.Lane >= 0 ? Ports[0].Spans[R.Lane].Y0 : H.Y;
			R.DesYaw = 225.f - FMath::Clamp((H.Y - LY) * 1.2f, -11.f, 11.f);
			R.DesKn = -2.4f;
			const float XBow = R.Lane >= 0 ? Ports[0].Spans[R.Lane].XBow : -36.f;
			const float MinClear = (R.Lane < 6) ? 52.f : (R.Lane < 8 ? 44.f : 26.f);
			const float XClear = FMath::Max(XBow + 1.38f * D.LengthM + 22.f, MinClear);
			if (H.X > XClear || R.StateT > 110.f)
			{
				static const bool bL = FParse::Param(FCommandLine::Get(), TEXT("iwnpclog"));
				if (bL) Log(FString::Printf(TEXT("IWNPC DEPART1 %s H=(%.1f,%.1f) t=%.0f yaw=%.0f V=%.1f"), *R.Name, H.X, H.Y, R.StateT, R.Yaw, R.Body.IsValid() ? R.Body->V / 77.2f : 0.f));
				DepartPhase1(R);
			}
		}
		else
		{
			R.bAux = true;
			const FVector2D H = W2H(R.Pos);
			if (H.X > 120.f && R.Lane >= 0) ReleaseLane(R);   // release corridor lock once clear of inner basin
			if (H.X > 265.f) { BasinDrop(R.Id); MouthDrop(R.Id); }
			const bool bDone = FollowRoute(R, FMath::Min(D.CruiseKn, 4.2f), 0.f);
			if (bDone)
			{
				ReleaseLane(R); BasinDrop(R.Id); MouthDrop(R.Id);
				if (R.PendDest >= 0) { const int32 Dst = R.PendDest; R.PendDest = -1; R.bAux = false; StartTrip(R, Dst, R.PendState); }
				else if (R.PendState == ENpcState::Work)
				{
					R.Route.Reset(); R.RouteIdx = 0; R.DestKind = 3;
					const FVector DestW = R.WorkPt.IsZero() ? H2W(520.f, 0.f) : R.WorkPt;
					PlanRoute(R.Pos, DestW, R.Route);
					R.Target = TEXT("working the roads");
					SetState(R, ENpcState::Transit);
				}
				else { SetState(R, ENpcState::Anchored); }
			}
			else if (bNear) Avoid(R, Dt, H.X > 220.f, H.X < 240.f);
		}
		break;
	}
	// ------------------------------------------------------------------ passage
	case ENpcState::Transit:
	{
		R.bAux = false;
		float Kn = D.CruiseKn;
		// Harbour-mouth waiting-slot arbitrator: hold on spaced outer slot until both mouth token and berth+corridor are reserved
		if (R.WaitIdx >= 0 && R.DestPort == 0 && R.DestKind == 0)
		{
			const FVector WaitPos = R.Route.IsValidIndex(R.WaitIdx) ? R.Route[R.WaitIdx] : R.WaitPt;
			const float DistWait = FVector::Dist2D(R.Pos, WaitPos);
			if (R.RouteIdx >= R.WaitIdx && DistWait < 5500.f)
			{
				bool bOlderReady = false;
				for (FNpcRec& O : Recs)
				{
					if (O.Id == R.Id || !O.bBasinWait || O.BasinWaitT <= R.BasinWaitT + 1.f) continue;
					if (MouthFreeFor(O, -1) && ReserveBerthAndCorridor(O, false)) { bOlderReady = true; break; }
				}
				if (!bOlderReady && MouthFreeFor(R, -1) && ReserveBerthAndCorridor(R, true))
				{
					BasinTake(R.Id);
					MouthTake(R.Id, -1);
					ReleaseWaitSlot(R);
					R.WaitIdx = -1; R.bBasinWait = false; R.BasinWaitT = 0.f; R.bHold = false;
					R.StuckT = 0.f; R.StuckPos = R.Pos;
					BuildInboundBasinRoute(R);
				}
				else
				{
					R.bBasinWait = true; R.BasinWaitT += Dt; R.bAux = true;
					R.StuckT = 0.f; R.StuckPos = R.Pos;
					if (DistWait > 1800.f)
					{
						R.bHold = false;
						R.DesYaw = BrgDeg(R.Pos, WaitPos);
						R.DesKn = FMath::Clamp(DistWait / 1800.f, 0.8f, 2.2f);
					}
					else
					{
						R.bHold = true;
						R.DesKn = 0.f;
						R.DesYaw = BrgDeg(R.Pos, H2W(160.f, -34.f));
					}
					if (R.BasinWaitT > 360.f)
					{
						ReleaseWaitSlot(R);
						if (R.HomeLane < 0) ReleaseLane(R);
						R.WaitIdx = -1; R.bBasinWait = false; R.BasinWaitT = 0.f;
						R.Route.Reset(); R.RouteIdx = 0; R.NextActionT = Now + 50.f;
						SetState(R, ENpcState::Anchored);
						R.Target = TEXT("anchorage (berths full)");
					}
					break;
				}
			}
			else R.bBasinWait = false;
		}
		else R.bBasinWait = false;

		const bool bDone = FollowRoute(R, Kn, 0.f);
		if (bDone)
		{
			if (R.DestKind == 0 && R.DestPort == 0)
			{
				// Hard rule: never enter Arrive inside the basin without confirmed berth + corridor reservation
				if (ReserveBerthAndCorridor(R, true)) SetState(R, ENpcState::Arrive);
				else StartTrip(R, 0, ENpcState::Transit);
			}
			else if (R.DestKind == 3)
			{
				R.Route.Reset(); R.RouteIdx = 0;
				if (R.Role == ENpcRole::Fisher) { SetState(R, ENpcState::Fish); R.Phase = 0; R.FishPt = R.Zone; }
				else { SetState(R, ENpcState::Work); R.WaitUntil = Now + 55.f + 15.f * (R.Id % 4); }
			}
			else if (R.DestKind == 2) { SetState(R, ENpcState::Arrive); }
			else { R.DestKind = 1; SetState(R, ENpcState::Arrive); }
			break;
		}
		// P25 Sailing Brig Harbour Rule: engage auxiliary steam/tow inside 450 m of harbour mouth so wind shifts never blow into irons
		const float MouthDistCm = FVector::Dist2D(R.Pos, H2W(155.f, 0.f));
		if (D.bSail && (MouthDistCm < 45000.f || InHarbour(R.Pos, 90.f))) { R.bAux = true; R.BeatTack = 0; }
		else if (D.bSail && R.RouteIdx < R.Route.Num()) DoSailing(R, R.Route[R.RouteIdx], R.DesYaw);
		// harbour speed limit
		const FVector2D HNow = W2H(R.Pos);
		const bool bInChannel = (HNow.X > -55.f && HNow.X < 265.f && FMath::Abs(HNow.Y) < 170.f);
		if (bInChannel) { R.DesKn = FMath::Min(R.DesKn, 4.2f); R.bAux = true; }
		if (bNear) Avoid(R, Dt, !bInChannel, bInChannel);
		else R.DesKn = bInChannel ? FMath::Min(Kn, 4.2f) : Kn;
		break;
	}
	case ENpcState::Arrive:
	{
		R.bAux = true; R.bBasinWait = false;
		if (R.DestKind == 0 && R.Lane >= 0)
		{
			// straight in along the reserved lane with a stopping-distance speed profile, cross-track steering
			FVector BP; float BY; BerthPose(R, R.Lane, BP, BY);
			const FVector2D H = W2H(R.Pos), HB = W2H(BP);
			const float DistM = H.X - HB.X;            // metres still to go (heading -X)
			const float Ye = H.Y - HB.Y;               // + = north of the lane
			R.DesYaw = 225.f + FMath::Clamp(Ye * 1.9f, -26.f, 26.f) * (DistM > 18.f ? 1.f : 0.65f);
			const float A = 0.14f;                      // m/s^2 sim deceleration used for the profile
			const float VSim = FMath::Sqrt(2.f * A * FMath::Max(DistM - 2.0f, 0.f));
			float Kn = FMath::Clamp(VSim / (KnCmS / 100.f * Comp), 0.f, 4.2f);
			if (DistM > 1.8f) Kn = FMath::Max(Kn, 0.55f);
			R.DesKn = Kn;
			if (bNear) { R.AvoidSlow = 1.f; Avoid(R, Dt, false, true); }
			if (DistM < 1.5f || (R.StateT > 110.f && DistM < 5.f)) { R.bHold = true; ArriveDone(R); }
			else if (R.StateT > 175.f) { Log(FString::Printf(TEXT("IWNPC ARRIVE timeout %s dist %.1f m ye %.1f"), *R.Name, DistM, Ye)); R.bHold = true; ArriveDone(R); }
		}
		else
		{
			// sea point / anchorage / mooring: slow down onto the target
			FVector Tg = R.Route.Num() ? R.Route.Last() : R.Pos;
			if (R.DestKind == 2 && R.bHomePt) Tg = R.HomePt;
			const float Dm = FVector::Dist2D(R.Pos, Tg) / 100.f;
			R.DesYaw = BrgDeg(R.Pos, Tg);
			if (D.bSail && Dm > 45.f && FVector::Dist2D(R.Pos, H2W(155.f, 0.f)) >= 45000.f) DoSailing(R, Tg, R.DesYaw);
			else if (D.bSail) R.bAux = true;
			const float VSim = FMath::Sqrt(2.f * 0.12f * FMath::Max(Dm - 4.f, 0.f));
			R.DesKn = FMath::Clamp(VSim / (KnCmS / 100.f * Comp), 0.f, D.CruiseKn);
			if (Dm > 4.f) R.DesKn = FMath::Max(R.DesKn, 0.6f);
			if (bNear) Avoid(R, Dt, Dm > 80.f, false);
			if (Dm < 4.5f || R.StateT > 240.f) { R.bHold = true; ArriveDone(R); }
		}
		break;
	}
	// ------------------------------------------------------------------ work
	case ENpcState::Fish:
	{
		R.bBasinWait = false;
		const bool bHome = Hour >= R.HomeHour || bGale;
		if (bHome && !R.bSheltering) { R.DestKind = 2; GoHome(R); break; }
		if (R.Phase == 0)
		{
			if (R.FishPt.IsZero() || FVector::Dist2D(R.Pos, R.FishPt) < 3500.f)
			{
				R.Phase = 1; R.WaitUntil = Now + Rng.FRandRange(22.f, 55.f);
			}
			else
			{
				R.DesKn = D.CruiseKn; R.DesYaw = BrgDeg(R.Pos, R.FishPt);
				if (D.bSail) DoSailing(R, R.FishPt, R.DesYaw);
				if (bNear) Avoid(R, Dt, true, false);
			}
		}
		else
		{
			R.DesKn = 0.f; R.DesYaw = R.Yaw;
			if (Now > R.WaitUntil)
			{
				const float Ang = Rng.FRandRange(0.f, 360.f), Rad = Rng.FRandRange(0.25f, 1.f) * R.ZoneR;
				R.FishPt = PolarPt(R.Zone, Ang, Rad); R.Phase = 0; R.BeatTack = 0;
			}
		}
		break;
	}
	case ENpcState::Work:
	{
		// a working boat on station: heave to (the crane / oars / bell would work here)
		R.bBasinWait = false;
		R.DesKn = 0.f; R.DesYaw = R.Yaw;
		if (Now > R.WaitUntil) { R.DestKind = R.HomeLane >= 0 ? 0 : 2; GoHome(R); }
		break;
	}
	case ENpcState::Patrol:
	{
		if (R.PatrolPts.Num() == 0) { SetState(R, ENpcState::Anchored); break; }
		const bool bNightBerth = ((R.Role == ENpcRole::Cutter || R.Role == ENpcRole::Escort || R.Role == ENpcRole::Gunboat)) && (Hour >= 22.f || Hour < 5.f) && R.RoadPt.Size2D() > 1.f;
		if (bNightBerth) { R.DestKind = 2; R.HomePt = R.RoadPt; R.bHomePt = true; R.Route.Reset(); R.RouteIdx = 0; PlanRoute(R.Pos, R.RoadPt, R.Route); R.Target = TEXT("anchorage"); SetState(R, ENpcState::Transit); break; }
		if (R.Route.Num() == 0 || R.RouteIdx >= R.Route.Num())
		{
			R.PatrolIdx = (R.PatrolIdx + 1) % R.PatrolPts.Num(); R.Route.Reset(); R.RouteIdx = 0;
			PlanRoute(R.Pos, R.PatrolPts[R.PatrolIdx], R.Route);
			R.Target = FString::Printf(TEXT("patrol leg %d"), R.PatrolIdx + 1);
		}
		FollowRoute(R, D.CruiseKn, 0.f);
		if (D.bSail && R.RouteIdx < R.Route.Num()) DoSailing(R, R.Route[R.RouteIdx], R.DesYaw);
		if (bNear)
		{
			Avoid(R, Dt, true, false);
			// the cutter hails a vessel it passes close by
			if (R.Role == ENpcRole::Cutter && DPlayer < 80000.f && Now - R.HailT > 240.f)
			{
				R.HailT = Now;
				if (Voy.IsValid() && DPlayer < 60000.f) Voy->ShowToast(FString::Printf(TEXT("%s hails: \"Ahoy there! Revenue patrol, show your colours.\""), *R.Name), 5.f);
				Log(FString::Printf(TEXT("IWNPC HAIL %s hails the player at %.0f m"), *R.Name, DPlayer / 100.f));
			}
		}
		// the ironclad / gunboat convoy rule is handled by Decide on Escort roles
		break;
	}
	case ENpcState::Escort:
	{
		FNpcRec* L = RecAt(R.Leader);
		if (!L) { R.Leader = -1; GoHome(R); break; }
		const bool bLeaderIn = L->State == ENpcState::Docked || L->State == ENpcState::Anchored || L->State == ENpcState::Moor;
		if (R.bHelper && bLeaderIn) { R.Leader = -1; GoHome(R); break; }
		if (R.Role == ENpcRole::Escort && bLeaderIn) { R.Leader = -1; if (R.RoadPt.Size2D() > 1.f) { R.DestKind = 2; R.bHomePt = true; R.HomePt = R.RoadPt; R.Route.Reset(); R.RouteIdx = 0; PlanRoute(R.Pos, R.RoadPt, R.Route); R.Target = TEXT("convoy anchorage"); SetState(R, ENpcState::Transit); } break; }
		const FVector Fw = FRotator(0.f, L->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
		const FVector Tg = L->Pos + Fw * R.Station.X + Rt * R.Station.Y;
		const float Dist = FVector::Dist2D(R.Pos, Tg);
		const float Ahead = FVector::DotProduct(Tg - R.Pos, Fw);
		const float W2 = FMath::Clamp(Dist / 6000.f, 0.f, 1.f);
		R.DesYaw = R.Yaw + FRotator::NormalizeAxis(FMath::Lerp(L->Yaw, BrgDeg(R.Pos, Tg), W2) - R.Yaw);
		R.DesKn = FMath::Clamp(L->SpeedKn + Ahead / 1500.f, 0.f, D.MaxKn);
		if (L->SpeedKn < 0.3f && Dist < 4000.f) R.DesKn = 0.f;
		R.bAux = true;
		if (bNear) { Avoid(R, Dt, true, R.bHelper && InHarbour(R.Pos, 10.f)); }
		break;
	}
	case ENpcState::Flee:
	{
		const FVector Away = (R.Pos - R.ThreatPos).GetSafeNormal2D();
		R.DesYaw = Away.Rotation().Yaw; R.DesKn = D.MaxKn; R.bAux = true;
		if (bNear) Avoid(R, Dt, true, false);
		if (R.StateT > 150.f || FVector::Dist2D(R.Pos, R.ThreatPos) > 220000.f) { R.DestKind = R.HomeLane >= 0 ? 0 : 2; GoHome(R); }
		break;
	}
	case ENpcState::Aggressive:
	{
		// stalk: close to ~450 m on the player's quarter, hail once, break off
		if (DPlayer > 600000.f || R.StateT > 420.f) { SetState(R, ENpcState::Patrol); R.Route.Reset(); R.RouteIdx = 0; R.Threat = 1; break; }
		const FVector PV = PlayerVel(); const FVector Fw = PV.GetSafeNormal2D().IsNearlyZero() ? FVector(1, 0, 0) : PV.GetSafeNormal2D(); const FVector Rt(-Fw.Y, Fw.X, 0.f);
		const FVector Tg = PP - Fw * 45000.f + Rt * 28000.f;
		const float Dist = FVector::Dist2D(R.Pos, Tg);
		R.DesYaw = BrgDeg(R.Pos, Tg);
		R.DesKn = FMath::Clamp(Dist / 3000.f, 2.f, D.MaxKn);
		if (D.bSail) DoSailing(R, Tg, R.DesYaw);
		if (bNear) Avoid(R, Dt, true, false);
		if (DPlayer < 80000.f && Now - R.HailT > 300.f)
		{
			R.HailT = Now;
			if (Voy.IsValid()) Voy->ShowToast(TEXT("Unknown sloop hails: \"Heave to and show your cargo!\""), 5.f);
			Log(FString::Printf(TEXT("IWNPC HAIL %s (pirate hook) hails the player at %.0f m; no fire (combat parked)"), *R.Name, DPlayer / 100.f));
		}
		break;
	}
	default: break;
	}
	if (bNear && R.State == ENpcState::Docked && R.Lane >= 0) { /* lamp/hold handled by body */ }
}

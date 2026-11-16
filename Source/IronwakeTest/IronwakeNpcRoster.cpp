// P20: roster, timetable (Decide), airship brain, test hooks.
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeTest.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	inline FVector PolarPt(const FVector& C, float AngDeg, float R) { const float A = FMath::DegreesToRadians(AngDeg); return C + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f); }
	bool IsCrownPatrol(const FNpcRec& R) { return R.Role == ENpcRole::Cutter || R.Role == ENpcRole::Gunboat || R.Role == ENpcRole::Escort; }
}

int32 AIronwakeNpcDirector::Add(FName DefId, ENpcRole Role, ENpcFaction F, const FLinearColor& Col)
{
	FNpcDef* D = IronwakeNpc::FindMutable(DefId); if (!D) return -1;
	FNpcRec R; R.Id = Recs.Num(); R.Def = D; R.Role = Role; R.Faction = F; R.FlagColor = Col;
	R.Name = D->Names.Num() ? D->Names[D->NextName++ % D->Names.Num()] : D->ClassName;
	R.Threat = (F == ENpcFaction::Pirate) ? 2 : 0; R.bCanFire = (Role == ENpcRole::Gunboat || Role == ENpcRole::Escort || Role == ENpcRole::Pirate);
	R.LastBrainT = 0.013f * R.Id;
	Recs.Add(R);
	return R.Id;
}

void AIronwakeNpcDirector::BuildRoster()
{
	Recs.Reset();
	const FLinearColor Crown(0.38f, 0.06f, 0.05f), Trade(0.24f, 0.045f, 0.035f), Loc(0.32f, 0.24f, 0.09f), Pir(0.03f, 0.03f, 0.03f);
	auto Berth = [&](int32 Id, int32 Lane, float ActionSec)
	{
		FNpcRec& R = Recs[Id]; R.HomeLane = Lane; Ports[0].Spans[Lane].HomeOf = Id;
		ReserveBerthAndCorridor(R, false);
		FVector P; float Y; BerthPose(R, Lane, P, Y); R.Pos = P; R.Yaw = Y; R.BerthPos = P; R.BerthYaw = Y; R.CurPort = 0;
		R.State = ENpcState::Docked; R.bHold = true; R.Target = FString::Printf(TEXT("Greyhaven %s"), *Ports[0].Spans[Lane].Name);
		R.NextActionT = ActionSec;
	};
	auto Sea = [&](int32 Id, const FVector& P, float Yaw)
	{
		FNpcRec& R = Recs[Id]; R.Pos = P; R.Yaw = Yaw; R.State = ENpcState::Anchored; R.bHold = true; R.HomePt = P; R.bHomePt = true; R.RoadPt = P; R.Target = TEXT("anchorage");
	};

	// ---- 4 coastal merchant steamers, 4 merchant brigs, 1 paddle ferry (berth ALL docked hulls BEFORE calling StartTrip!)
	// P29+P32: connect all 6 working ports (0=Greyhaven, 4=Ironcrag, 5=Stigsby, 6=Kestrel Cove, 7=North Grimscar, 8=Sable Haven)
	const int32 P_Ironcrag = Ports.Num() > 4 ? 4 : 1;
	const int32 P_Stigsby  = Ports.Num() > 5 ? 5 : 2;
	const int32 P_Kestrel  = Ports.Num() > 6 ? 6 : P_Stigsby;
	const int32 P_Grimscar = Ports.Num() > 7 ? 7 : P_Ironcrag;
	const int32 P_Sable    = Ports.Num() > 8 ? 8 : 0;
	const int32 M0 = Add(TEXT("goliath"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Berth(M0, 0, 26.f); Recs[M0].HomeLane = -1; Ports[0].Spans[0].HomeOf = -1; Recs[M0].Itinerary = { P_Ironcrag, 0, P_Stigsby, 0 };
	const int32 M1 = Add(TEXT("goliath"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Recs[M1].Itinerary = { 0, P_Ironcrag, P_Stigsby, 0 };
	const int32 M2 = Add(TEXT("goliath"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Berth(M2, 3, 72.f); Recs[M2].HomeLane = -1; Ports[0].Spans[3].HomeOf = -1; Recs[M2].Itinerary = { P_Stigsby, 0, P_Ironcrag, 0 };
	const int32 M3 = Add(TEXT("goliath"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Recs[M3].Itinerary = { 0, P_Stigsby, P_Ironcrag, 0 };
	{ FNpcRec& R = Recs[M3]; R.Pos = Ports[P_Ironcrag].Offing; R.State = ENpcState::Anchored; R.bHold = true; R.CurPort = P_Ironcrag; R.NextActionT = 40.f; R.Target = FString::Printf(TEXT("anchored off %s"), *Ports[P_Ironcrag].Name); }

	const int32 B0 = Add(TEXT("brig"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Recs[B0].Itinerary = { 1, P_Ironcrag, 0, P_Stigsby };
	const int32 B1 = Add(TEXT("brig"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Berth(B1, 4, 118.f); Recs[B1].HomeLane = -1; Ports[0].Spans[4].HomeOf = -1; Recs[B1].Itinerary = { P_Ironcrag, P_Stigsby, 0, 2 };
	const int32 B2 = Add(TEXT("brig"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Berth(B2, 1, 165.f); Recs[B2].HomeLane = -1; Ports[0].Spans[1].HomeOf = -1; Recs[B2].Itinerary = { P_Stigsby, P_Ironcrag, 0, 3 };
	const int32 B3 = Add(TEXT("brig"), ENpcRole::Merchant, ENpcFaction::Trade, Trade); Recs[B3].Itinerary = { 0, P_Ironcrag, P_Stigsby, 0 };
	{ FNpcRec& R = Recs[B3]; R.Pos = Ports[P_Stigsby].Offing; R.State = ENpcState::Anchored; R.bHold = true; R.CurPort = P_Stigsby; R.NextActionT = 30.f; R.Target = FString::Printf(TEXT("anchored off %s"), *Ports[P_Stigsby].Name); }

	int32 A = Add(TEXT("paddle"), ENpcRole::Ferry, ENpcFaction::Trade, Trade); Berth(A, 5, 6.f); Recs[A].HomeLane = -1; Ports[0].Spans[5].HomeOf = -1; Recs[A].Itinerary = { 1, 0, P_Stigsby, 0 };

	// Now that lanes 0, 1, 3, 4, 5 are occupied (leaving N3 = lane 2 free!), launch M1 & B0 cleanly clear of player at H(778, 141)
	{ FNpcRec& R = Recs[M1]; R.Pos = H2W(1020.f, -180.f); R.Yaw = 205.f; R.CurPort = 2; R.LegSeq = 1; StartTrip(R, 0, ENpcState::Transit); R.SpeedKn = R.Def->CruiseKn; }
	{ FNpcRec& R = Recs[B0]; R.Pos = H2W(640.f, -260.f); R.Yaw = -40.f; R.CurPort = 0; R.LegSeq = 1; StartTrip(R, 1, ENpcState::Transit); R.SpeedKn = 6.5f; }

	// ---- 6 harbour craft: dedicated home moorings + separated outer WorkPt stations (zero shared-box pileups!)
	A = Add(TEXT("lighter"), ENpcRole::Harbour, ENpcFaction::Local, Loc);
	{ FNpcRec& R = Recs[A]; R.HomeLane = 6; Ports[0].Spans[6].HomeOf = A; R.WorkPt = H2W(195.f, -350.f); R.Pos = H2W(260.f, -310.f); R.Yaw = -85.f; R.DestKind = 3; PlanRoute(R.Pos, R.WorkPt, R.Route); R.State = ENpcState::Transit; R.SpeedKn = 3.f; R.Target = TEXT("working the roads"); }
	A = Add(TEXT("tender"), ENpcRole::Harbour, ENpcFaction::Local, Loc); Berth(A, 7, 48.f); Recs[A].WorkPt = H2W(195.f, 350.f);
	A = Add(TEXT("pilot"), ENpcRole::Pilot, ENpcFaction::Local, Loc);
	{ FNpcRec& R = Recs[A]; R.HomeLane = 8; Ports[0].Spans[8].HomeOf = A; R.WorkPt = H2W(660.f, 210.f); R.Pos = H2W(540.f, 190.f); R.Yaw = 55.f; R.DestKind = 3; PlanRoute(R.Pos, R.WorkPt, R.Route); R.State = ENpcState::Transit; R.SpeedKn = 7.f; R.Target = TEXT("pilot boarding ground"); }
	A = Add(TEXT("gig"), ENpcRole::Taxi, ENpcFaction::Local, Loc); Berth(A, 9, 2.f); Recs[A].WorkPt = H2W(160.f, 285.f);
	A = Add(TEXT("gig"), ENpcRole::Taxi, ENpcFaction::Local, Loc);
	{ FNpcRec& R = Recs[A]; R.HomeLane = 10; Ports[0].Spans[10].HomeOf = A; R.WorkPt = H2W(315.f, 385.f); R.Pos = H2W(245.f, 295.f); R.Yaw = 95.f; R.DestKind = 3; PlanRoute(R.Pos, R.WorkPt, R.Route); R.State = ENpcState::Transit; R.SpeedKn = 2.6f; R.Target = TEXT("water taxi"); }
	A = Add(TEXT("tug"), ENpcRole::Tug, ENpcFaction::Local, Loc);
	{ FNpcRec& R = Recs[A]; Sea(A, H2W(540.f, -290.f), 20.f); R.WorkPt = H2W(660.f, -390.f); R.PatrolPts = { H2W(540.f, -290.f), H2W(680.f, -410.f), H2W(520.f, -420.f) }; R.State = ENpcState::Patrol; R.bHold = false; R.SpeedKn = 6.5f; R.Target = TEXT("harbour tug station"); }

	// ---- 8 fishing smacks: lateral NW & far-SE banks (>= 250 m clear of all patrol loops, player spawn & merchant lanes)
	static const FVector2D FishH[8] = {
		{ 500.f,  480.f }, { 680.f,  530.f }, { 540.f,  670.f }, { 740.f,  710.f },
		{ 480.f, -530.f }, { 660.f, -590.f }, { 520.f, -730.f }, { 720.f, -770.f }
	};
	for (int32 i = 0; i < 8; ++i)
	{
		A = Add(TEXT("smack"), ENpcRole::Fisher, ENpcFaction::Local, Loc);
		FNpcRec& R = Recs[A];
		const FVector HomeMooring = H2W(190.f + 16.f * i, 260.f + 18.f * i);
		Sea(A, HomeMooring, 195.f);
		R.Zone = H2W(FishH[i].X, FishH[i].Y); R.ZoneR = 5800.f; R.HomeHour = 17.5f + 0.4f * i; R.NextActionT = 4.f + 8.f * i;
		R.Target = TEXT("moored in the roads");
	}

	// ---- Crown patrol & ironclad escort convoy: clean concentric seaward offing arcs (zero zone overlap!)
	A = Add(TEXT("cutter"), ENpcRole::Cutter, ENpcFaction::Crown, Crown);
	{ FNpcRec& R = Recs[A]; Sea(A, H2W(1080.f, -180.f), 90.f); R.RoadPt = R.Pos; R.PatrolPts = { H2W(1080.f, -180.f), H2W(1320.f, 60.f), H2W(1080.f, 300.f) }; R.NextActionT = 8.f; }
	A = Add(TEXT("ironclad"), ENpcRole::Escort, ENpcFaction::Crown, Crown);
	{ FNpcRec& R = Recs[A]; Sea(A, H2W(1540.f, -360.f), 95.f); R.RoadPt = R.Pos; R.PatrolPts = { H2W(1540.f, -360.f), H2W(1740.f, 60.f), H2W(1540.f, 460.f) }; R.NextActionT = 12.f; }
	const int32 IronId = A;
	A = Add(TEXT("gunboat"), ENpcRole::Gunboat, ENpcFaction::Crown, Crown);
	{ FNpcRec& R = Recs[A]; Sea(A, H2W(1500.f, -430.f), 95.f); R.RoadPt = R.Pos; R.Leader = IronId; R.Station = FVector2D(-26000.f, 14000.f); R.NextActionT = 13.f; }

	// ---- the stranger: far outer sea patrol (separated from Crown convoy)
	if (!bNoPirate)
	{
		A = Add(TEXT("pirate"), ENpcRole::Pirate, ENpcFaction::Pirate, Pir);
		FNpcRec& R = Recs[A]; R.Pos = H2W(2550.f, -850.f); R.Yaw = 90.f; R.State = ENpcState::Patrol; R.SpeedKn = 5.5f;
		R.PatrolPts = { H2W(2550.f, -850.f), H2W(2850.f, -150.f), H2W(2550.f, 550.f) }; R.Target = TEXT("unknown"); R.Threat = 2;
	}

	// ---- P32: Outer Archipelago Packet Schooners, Chapman Dispatch Sloops & Admiralty Flagship Convoy!
	if (Ports.Num() > 8)
	{
		const int32 Sc0 = Add(TEXT("schooner"), ENpcRole::Merchant, ENpcFaction::Trade, Trade);
		if (Sc0 >= 0) { FNpcRec& R = Recs[Sc0]; R.Itinerary = { P_Grimscar, P_Ironcrag, P_Stigsby, P_Kestrel }; R.Pos = Ports[P_Kestrel].Offing; R.State = ENpcState::Anchored; R.bHold = true; R.CurPort = P_Kestrel; R.NextActionT = 18.f; R.Target = FString::Printf(TEXT("anchored off %s"), *Ports[P_Kestrel].Name); }

		const int32 Sc1 = Add(TEXT("schooner"), ENpcRole::Merchant, ENpcFaction::Trade, Trade);
		if (Sc1 >= 0) { FNpcRec& R = Recs[Sc1]; R.Itinerary = { P_Sable, 0, P_Grimscar, P_Ironcrag }; R.Pos = Ports[P_Grimscar].Offing; R.State = ENpcState::Anchored; R.bHold = true; R.CurPort = P_Grimscar; R.NextActionT = 22.f; R.Target = FString::Printf(TEXT("anchored off %s"), *Ports[P_Grimscar].Name); }

		const int32 Sl0 = Add(TEXT("sloop_npc"), ENpcRole::Merchant, ENpcFaction::Trade, Trade);
		if (Sl0 >= 0) { FNpcRec& R = Recs[Sl0]; R.Itinerary = { 0, P_Sable, P_Kestrel, P_Stigsby }; R.Pos = Ports[P_Sable].Offing; R.State = ENpcState::Anchored; R.bHold = true; R.CurPort = P_Sable; R.NextActionT = 15.f; R.Target = FString::Printf(TEXT("anchored off %s"), *Ports[P_Sable].Name); }

		const int32 Imp = Add(TEXT("implacable"), ENpcRole::Escort, ENpcFaction::Crown, Crown);
		if (Imp >= 0)
		{
			FNpcRec& R = Recs[Imp];
			Sea(Imp, Ports[P_Grimscar].Offing + FVector(-45000.f, -35000.f, 0.f), 215.f);
			R.RoadPt = R.Pos;
			R.PatrolPts = { Ports[P_Grimscar].Offing + FVector(-45000.f, -35000.f, 0.f), FVector(520000.f, 380000.f, 0.f), FVector(640000.f, -220000.f, 0.f) };
			R.NextActionT = 14.f;
		}
	}

	// ---- 3 airships
	auto Air = [&](FName Def, ENpcFaction F, const FLinearColor& C, float A0, float Rad, float Dir)
	{
		const int32 Id = Add(Def, ENpcRole::Airship, F, C); if (Id < 0) return;
		FNpcRec& R = Recs[Id]; R.State = ENpcState::Transit; R.Target = TEXT("high route");
		for (int32 k = 0; k < 5; ++k) R.PatrolPts.Add(PolarPt(HarbourC + FVector(15000.f, 15000.f, 0.f), A0 + Dir * k * 72.f, Rad * (k % 2 ? 0.82f : 1.f)));
		R.PatrolIdx = 1; R.Pos = R.PatrolPts[0]; R.Yaw = (R.PatrolPts[1] - R.PatrolPts[0]).Rotation().Yaw; R.SpeedKn = R.Def->CruiseKn; R.bAway = false;
	};
	Air(TEXT("patrol_airship"), ENpcFaction::Crown, Crown, 20.f, 145000.f, 1.f);
	Air(TEXT("cargo_airship"), ENpcFaction::Trade, Trade, 140.f, 195000.f, -1.f);
	Air(TEXT("sky_yacht"), ENpcFaction::Local, Loc, 260.f, 120000.f, 1.f);

	// Push any sea waypoint / work / fishing centre >= 110 m clear of buoys and island obstacles
	auto ClearObs = [&](FVector& Pt)
	{
		for (int32 k = 0; k < 8; ++k)
		{
			bool bMoved = false;
			for (const FObs& O : Obs)
			{
				const float D = FVector::Dist2D(Pt, O.C), Need = O.R + 4500.f;
				if (D < Need) { const FVector Dir = (Pt - O.C).GetSafeNormal2D(); Pt = O.C + (Dir.IsNearlyZero() ? FVector(1, 0, 0) : Dir) * (Need + 1500.f); bMoved = true; }
			}
			if (!bMoved) break;
		}
	};
	for (FNpcRec& R : Recs)
	{
		if (R.Def->bAir) continue;
		if (!R.WorkPt.IsZero()) ClearObs(R.WorkPt);
		if (!R.Zone.IsZero()) ClearObs(R.Zone);
		for (FVector& P : R.PatrolPts) ClearObs(P);
	}
	// ---- daytime initial states: fishers out on the seaward banks (most actively sailing!), patrols underway
	const float H = HourNow();
	int32 FishIdx = 0;
	for (FNpcRec& R : Recs)
	{
		if (R.Role == ENpcRole::Fisher && H > 6.f && H < R.HomeHour - 0.5f)
		{
			const float OffA = 45.f * FishIdx;
			R.Pos = PolarPt(R.Zone, OffA, 2200.f);
			R.FishPt = PolarPt(R.Zone, OffA + 165.f, 5500.f);
			R.Yaw = (R.FishPt - R.Pos).Rotation().Yaw;
			R.State = ENpcState::Fish; R.bHold = false;
			R.Phase = (FishIdx % 3 == 2) ? 1 : 0;
			R.SpeedKn = R.Phase == 0 ? 4.5f : 0.f;
			R.WaitUntil = 8.f + 7.f * FishIdx;
			R.Target = TEXT("fishing ground");
			++FishIdx;
		}
		if (IsCrownPatrol(R) && R.Role != ENpcRole::Gunboat && H > 5.f && H < 22.f && R.PatrolPts.Num() >= 2)
		{
			R.Pos = R.PatrolPts[0]; R.Pos.Z = 0.f;
			R.PatrolIdx = 1; R.Route.Reset(); R.RouteIdx = 0; PlanRoute(R.Pos, R.PatrolPts[1], R.Route);
			R.Yaw = (R.PatrolPts[1] - R.Pos).Rotation().Yaw;
			R.State = ENpcState::Patrol; R.bHold = false; R.SpeedKn = R.Def->CruiseKn; R.Target = TEXT("patrol"); R.Phase = 0;
		}
	}
	for (FNpcRec& R : Recs)
	{
		if (R.Role == ENpcRole::Gunboat)
		{
			if (FNpcRec* L = RecAt(R.Leader); L && L->State == ENpcState::Patrol)
			{
				const FVector Fw = FRotator(0.f, L->Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
				R.State = ENpcState::Escort; R.bHold = false;
				R.Pos = L->Pos + Fw * R.Station.X + Rt * R.Station.Y; R.Yaw = L->Yaw; R.SpeedKn = L->SpeedKn;
				R.Target = TEXT("escorting ") + L->Name;
			}
		}
	}
}

// ---------------------------------------------------------------------------- timetable
void AIronwakeNpcDirector::GoHome(FNpcRec& R)
{
	R.Route.Reset(); R.RouteIdx = 0; R.bHeld = false; R.bBasinWait = false; R.BasinWaitT = 0.f;
	ReleaseWaitSlot(R);
	if (R.HomeLane >= 0)
	{
		R.DestPort = 0; R.DestKind = 0;
		const FVector2D HStart = W2H(R.Pos);
		if (HStart.X < 252.f && FMath::Abs(HStart.Y) < 175.f && ReserveBerthAndCorridor(R, true))
		{
			MouthTake(R.Id, -1);
			R.WaitIdx = -1;
			BuildInboundBasinRoute(R);
		}
		else
		{
			PickLane(R);
			const int32 Slot = ReserveWaitSlot(R);
			R.WaitPt = Slot >= 0 ? H2W(WaitSlots[Slot].H.X, WaitSlots[Slot].H.Y) : H2W(430.f, 0.f);
			PlanRoute(R.Pos, R.WaitPt, R.Route);
			R.WaitIdx = R.Route.Num() - 1;
		}
		R.Target = FString::Printf(TEXT("home to %s"), *Ports[0].Spans[R.HomeLane].Name);
		SetState(R, ENpcState::Transit);
	}
	else if (R.bHomePt)
	{
		R.DestPort = 0; R.DestKind = 2;
		PlanRoute(R.Pos, R.HomePt, R.Route); R.Target = TEXT("home mooring");
		SetState(R, ENpcState::Transit);
	}
	else StartTrip(R, NearestPort(R.Pos), ENpcState::Transit);
}

void AIronwakeNpcDirector::Decide(FNpcRec& R)
{
	const float Now = (float)GetWorld()->GetTimeSeconds();
	if (Now < R.NextActionT) return;
	const float Hour = HourNow();
	const bool bGale = IsGale();
	R.NextActionT = Now + 5.f;
	if (bGale && R.Faction != ENpcFaction::Crown) return;
	auto CanDepartLane = [&](int32 LaneIdx) -> bool
	{
		if (!MouthFreeFor(R, 1)) return false;
		if (LaneIdx < 0 || !Ports[0].Spans.IsValidIndex(LaneIdx)) return true;
		const FNpcSpan& S = Ports[0].Spans[LaneIdx];
		for (int32 j = 0; j < Ports[0].Spans.Num(); ++j)
			if (j != LaneIdx && Ports[0].Spans[j].CorridorOwner >= 0 && Ports[0].Spans[j].CorridorOwner != R.Id && FMath::Abs(Ports[0].Spans[j].Y0 - S.Y0) < 24.f)
				return false;
		return true;
	};
	switch (R.Role)
	{
	case ENpcRole::Merchant: case ENpcRole::Ferry:
	{
		if (Hour < 5.5f || Hour > 21.f) return;         // night: stay put
		if (R.Itinerary.Num() == 0) return;
		const int32 Next = R.Itinerary[R.LegSeq % R.Itinerary.Num()];
		if (Next == R.CurPort) { ++R.LegSeq; return; }
		if (Next == 0 && R.Lane < 0 && !PickLane(R)) { R.NextActionT = Now + 20.f; return; }
		if (R.Lane >= 0 && R.CurPort == 0)
		{
			if (!CanDepartLane(R.Lane)) { R.NextActionT = Now + 5.f; return; }
			++R.LegSeq;
			R.PendDest = Next; R.PendState = ENpcState::Transit;
			BeginDepartFromLane(R);
			R.Target = TEXT("leaving for ") + Ports[Next].Name;
		}
		else
		{
			++R.LegSeq;
			StartTrip(R, Next, ENpcState::Transit);
		}
		break;
	}
	case ENpcRole::Harbour: case ENpcRole::Taxi: case ENpcRole::Pilot:
	{
		if (Hour < 6.f || Hour > 19.f) return;
		if (!CanDepartLane(R.Lane)) { R.NextActionT = Now + 5.f; return; }
		R.PendDest = -1; R.PendState = ENpcState::Work;
		R.WaitUntil = Now + 55.f + 15.f * (R.Id % 4);
		BeginDepartFromLane(R);
		R.Target = TEXT("working the roads");
		break;
	}
	case ENpcRole::Tug:
	{
		if (Hour < 6.f || Hour > 19.f) return;
		R.PatrolIdx = 0; R.Route.Reset(); R.RouteIdx = 0; R.Target = TEXT("harbour tug station"); SetState(R, ENpcState::Patrol);
		break;
	}
	case ENpcRole::Fisher:
	{
		if (Hour < 5.5f || Hour > R.HomeHour - 1.f) return;
		R.Route.Reset(); R.RouteIdx = 0; R.DestKind = 3; R.FishPt = FVector::ZeroVector; R.Phase = 0;
		PlanRoute(R.Pos, R.Zone, R.Route); R.Target = TEXT("to the fishing ground"); SetState(R, ENpcState::Transit);
		break;
	}
	case ENpcRole::Cutter: case ENpcRole::Escort:
	{
		if (Hour < 5.f || Hour > 22.f) return;
		R.PatrolIdx = 0; R.Route.Reset(); R.RouteIdx = 0; R.Target = TEXT("patrol"); SetState(R, ENpcState::Patrol);
		break;
	}
	case ENpcRole::Gunboat:
	{
		if (Hour < 5.f || Hour > 22.f) return;
		if (RecAt(R.Leader)) { R.Target = TEXT("escorting ") + RecAt(R.Leader)->Name; SetState(R, ENpcState::Escort); }
		break;
	}
	default: break;
	}
}

// ---------------------------------------------------------------------------- airships
void AIronwakeNpcDirector::AirBrain(FNpcRec& R, float Dt)
{
	if (R.PatrolPts.Num() == 0) return;
	const FVector& T = R.PatrolPts[R.PatrolIdx];
	if (FVector::Dist2D(R.Pos, T) < 12000.f) R.PatrolIdx = (R.PatrolIdx + 1) % R.PatrolPts.Num();
	R.DesYaw = (R.PatrolPts[R.PatrolIdx] - R.Pos).Rotation().Yaw;
	R.DesKn = R.Def->CruiseKn;
	R.bHold = false; R.bAux = false;
}

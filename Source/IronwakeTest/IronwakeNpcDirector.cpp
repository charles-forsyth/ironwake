// P20: NPC director: ledger, ports, roster, spawning, contacts, HUD. Brain in IronwakeNpcBrain.cpp, ship body in IronwakeNpcShip.cpp.
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeLivingSea.h"
#include "IronwakeShell.h"
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GenericPlatform/GenericPlatformMisc.h"

const TCHAR* NpcStateName(ENpcState S)
{
	switch (S)
	{
	case ENpcState::Docked: return TEXT("DOCKED"); case ENpcState::Depart: return TEXT("DEPART"); case ENpcState::Transit: return TEXT("TRANSIT");
	case ENpcState::Arrive: return TEXT("ARRIVE"); case ENpcState::Moor: return TEXT("MOOR"); case ENpcState::Fish: return TEXT("FISH");
	case ENpcState::Patrol: return TEXT("PATROL"); case ENpcState::Escort: return TEXT("ESCORT"); case ENpcState::Flee: return TEXT("FLEE");
	case ENpcState::Drift: return TEXT("DRIFT"); case ENpcState::Anchored: return TEXT("ANCHORED"); case ENpcState::Aggressive: return TEXT("AGGRESSIVE");
	case ENpcState::Work: return TEXT("WORK");
	}
	return TEXT("?");
}
const TCHAR* NpcRoleName(ENpcRole R)
{
	switch (R)
	{
	case ENpcRole::Fisher: return TEXT("fishing smack"); case ENpcRole::Taxi: return TEXT("water taxi"); case ENpcRole::Ferry: return TEXT("ferry");
	case ENpcRole::Harbour: return TEXT("harbour craft"); case ENpcRole::Merchant: return TEXT("merchant"); case ENpcRole::Cutter: return TEXT("revenue cutter");
	case ENpcRole::Gunboat: return TEXT("gunboat"); case ENpcRole::Escort: return TEXT("ironclad escort"); case ENpcRole::Pilot: return TEXT("pilot cutter");
	case ENpcRole::Tug: return TEXT("tug"); case ENpcRole::Airship: return TEXT("airship"); case ENpcRole::Pirate: return TEXT("unknown sloop");
	}
	return TEXT("?");
}
const TCHAR* NpcFactionName(ENpcFaction F)
{
	switch (F) { case ENpcFaction::Crown: return TEXT("Crown"); case ENpcFaction::Trade: return TEXT("Trade"); case ENpcFaction::Local: return TEXT("Local"); case ENpcFaction::Pirate: return TEXT("Free Captains"); }
	return TEXT("?");
}

// ---------------------------------------------------------------------------- craft definitions
namespace
{
	TArray<FNpcDef> MakeDefs()
	{
		TArray<FNpcDef> A;
		auto Fin = [&](FNpcDef D)
		{
			if (D.FlagCm.IsNearlyZero() && !D.bAir) D.FlagCm = FVector(-0.14f * D.LengthM * 100.f, 0.f, FMath::Clamp(0.42f * D.LengthM * 100.f, 450.f, 2400.f));
			A.Add(D);
		};
		{ FNpcDef D; D.Id = TEXT("smack"); D.ClassName = TEXT("Fishing smack"); D.WorldMesh = TEXT("FishingSmack"); D.LengthM = 15.f; D.BeamM = 4.2f; D.DraftM = 1.4f; D.MaxKn = 7.f; D.CruiseKn = 5.f; D.AccelMS2 = 0.25f; D.TurnRadiusM = 35.f; D.RudderSlew = 30.f; D.YawResponse = 3.5f; D.bSail = true; D.HullSize = 0; D.LampZCm = 520.f;
		  D.Names = { TEXT("Gannet"), TEXT("Brisk Mary"), TEXT("Tern"), TEXT("Good Intent"), TEXT("Pilchard"), TEXT("Kittiwake II"), TEXT("Curlew Lass"), TEXT("Silver Darlings") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("pirate"); D.ClassName = TEXT("Unmarked sloop"); D.WorldMesh = TEXT("FishingSmack"); D.LengthM = 15.f; D.BeamM = 4.2f; D.DraftM = 1.4f; D.MaxKn = 8.f; D.CruiseKn = 6.f; D.AccelMS2 = 0.3f; D.TurnRadiusM = 32.f; D.RudderSlew = 32.f; D.YawResponse = 3.8f; D.bSail = true; D.HullSize = 0; D.LampZCm = 520.f;
		  D.Names = { TEXT("Black Heron") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("gig"); D.ClassName = TEXT("Rowing gig"); D.WorldMesh = TEXT("GigBody"); D.LengthM = 8.f; D.BeamM = 2.6f; D.DraftM = 1.2f; D.MaxKn = 3.5f; D.CruiseKn = 2.8f; D.AccelMS2 = 0.3f; D.TurnRadiusM = 9.f; D.RudderSlew = 40.f; D.YawResponse = 4.f; D.bOars = true; D.HullSize = 0; D.LampZCm = 260.f;
		  D.FlagCm = FVector(-300.f, 0.f, 230.f); D.Names = { TEXT("Wherry Anne"), TEXT("Dolphin") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("paddle"); D.ClassName = TEXT("Paddle steamer"); D.WorldMesh = TEXT("PaddleSteamerV3"); D.HullYaw = 0.f; D.MeshScale = 0.90f; D.BootTopCm = -393.f; D.LengthM = 45.f; D.BeamM = 9.2f; D.DraftM = 2.45f; D.MaxKn = 9.f; D.CruiseKn = 7.5f; D.AccelMS2 = 0.35f; D.TurnRadiusM = 70.f; D.RudderSlew = 22.f; D.YawResponse = 2.4f; D.bSteam = true; D.bPaddle = true; D.HullSize = 1;
		  D.SmokeCm = FVector(190.f, -9.f, 1698.f); D.SmokeScale = 0.18f; D.LampZCm = 1585.f; D.FlagCm = FVector(-2015.f, 0.f, 505.f); D.Names = { TEXT("Duchess of Kestrel") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("lighter"); D.ClassName = TEXT("Coal lighter"); D.WorldMesh = TEXT("CoalLighter"); D.LengthM = 30.f; D.BeamM = 11.f; D.DraftM = 1.1f; D.MaxKn = 4.f; D.CruiseKn = 3.f; D.AccelMS2 = 0.2f; D.TurnRadiusM = 45.f; D.RudderSlew = 18.f; D.YawResponse = 2.f; D.HullSize = 1; D.LampZCm = 900.f;
		  D.Names = { TEXT("Hodman") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("tender"); D.ClassName = TEXT("Salvage tender"); D.WorldMesh = TEXT("SalvageTenderV3"); D.HullYaw = 0.f; D.MeshScale = 0.78f; D.BootTopCm = -304.f; D.LengthM = 29.6f; D.BeamM = 9.8f; D.DraftM = 1.5f; D.MaxKn = 6.f; D.CruiseKn = 4.5f; D.AccelMS2 = 0.3f; D.TurnRadiusM = 40.f; D.RudderSlew = 22.f; D.YawResponse = 2.4f; D.bSteam = true; D.HullSize = 1;
		  D.SmokeCm = FVector(491.f, -5.f, 1024.f); D.SmokeScale = 0.22f; D.LampZCm = 1280.f; D.FlagCm = FVector(1350.f, 0.f, 850.f); D.Names = { TEXT("Dredger's Hope") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("pilot"); D.ClassName = TEXT("Pilot cutter"); D.WorldMesh = TEXT("PilotCutter"); D.LengthM = 14.f; D.BeamM = 3.8f; D.DraftM = 1.6f; D.MaxKn = 10.f; D.CruiseKn = 8.f; D.AccelMS2 = 0.9f; D.TurnRadiusM = 20.f; D.RudderSlew = 40.f; D.YawResponse = 4.5f; D.bSteam = true; D.HullSize = 0;
		  D.SmokeScale = 0.f; D.LampZCm = 1100.f; D.Names = { TEXT("Pathfinder") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("tug"); D.ClassName = TEXT("Harbour tug"); D.WorldMesh = TEXT("HarbourTug"); D.LengthM = 22.f; D.BeamM = 8.6f; D.DraftM = 2.4f; D.MaxKn = 10.f; D.CruiseKn = 7.f; D.AccelMS2 = 1.2f; D.TurnRadiusM = 35.f; D.RudderSlew = 35.f; D.YawResponse = 4.f; D.bSteam = true; D.HullSize = 1;
		  D.SmokeCm = FVector(220.f, 0.f, 1280.f); D.SmokeScale = 0.2f; D.LampZCm = 1300.f; D.Names = { TEXT("Stalwart") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("goliath"); D.ClassName = TEXT("Coastal merchant steamer"); D.WorldMesh = TEXT("CoastalSteamer"); D.MeshScale = 0.91f; D.LengthM = 50.f; D.BeamM = 10.2f; D.DraftM = 2.8f; D.MaxKn = 9.f; D.CruiseKn = 7.5f; D.AccelMS2 = 0.3f; D.TurnRadiusM = 90.f; D.RudderSlew = 20.f; D.YawResponse = 2.3f; D.bSteam = true; D.HullSize = 1;
		  D.SmokeCm = FVector(-1400.f, 0.f, 2100.f); D.SmokeScale = 0.3f; D.LampZCm = 1400.f; D.Names = { TEXT("SS Calloway"), TEXT("SS Marrow Queen"), TEXT("SS Hesper"), TEXT("SS Ironmere") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("brig"); D.ClassName = TEXT("Merchant brig"); D.WorldMesh = TEXT("TallBrig"); D.BootTopCm = -310.f; D.MeshScale = 1.f; D.LengthM = 42.f; D.BeamM = 9.5f; D.DraftM = 3.1f; D.MaxKn = 9.f; D.CruiseKn = 8.f; D.AccelMS2 = 0.25f; D.TurnRadiusM = 70.f; D.RudderSlew = 18.f; D.YawResponse = 2.2f; D.bSail = true; D.bRig = true; D.HullSize = 1; D.LampZCm = 1750.f;
		  D.FlagCm = FVector(-189.f, -4.35f, 2375.f); D.Names = { TEXT("Brig Prosper"), TEXT("Brig Lady Merriweather"), TEXT("Brig Constant"), TEXT("Brig Halcyon") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("schooner"); D.ClassName = TEXT("Topsail schooner"); D.WorldMesh = TEXT("AmphionSchooner"); D.BootTopCm = -265.f; D.MeshScale = 1.f; D.LengthM = 38.f; D.BeamM = 8.4f; D.DraftM = 2.65f; D.MaxKn = 10.5f; D.CruiseKn = 8.5f; D.AccelMS2 = 0.32f; D.TurnRadiusM = 62.f; D.RudderSlew = 22.f; D.YawResponse = 2.6f; D.bSail = true; D.HullSize = 1; D.LampZCm = 1650.f;
		  D.Names = { TEXT("SchoonerSwiftwing"), TEXT("Schooner Sea Swallow"), TEXT("Schooner North Star") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("sloop_npc"); D.ClassName = TEXT("Chapman packet sloop"); D.WorldMesh = TEXT("ChapmanSloop"); D.BootTopCm = -195.f; D.MeshScale = 1.f; D.LengthM = 26.f; D.BeamM = 6.2f; D.DraftM = 1.95f; D.MaxKn = 11.f; D.CruiseKn = 8.8f; D.AccelMS2 = 0.40f; D.TurnRadiusM = 48.f; D.RudderSlew = 28.f; D.YawResponse = 3.2f; D.bSail = true; D.HullSize = 0; D.LampZCm = 1180.f;
		  D.Names = { TEXT("Packet Lapwing"), TEXT("Packet Stormy Petrel") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("implacable"); D.ClassName = TEXT("74-gun flagship"); D.WorldMesh = TEXT("Implacable"); D.BootTopCm = -580.f; D.MeshScale = 1.f; D.LengthM = 74.f; D.BeamM = 16.5f; D.DraftM = 5.8f; D.MaxKn = 8.5f; D.CruiseKn = 6.8f; D.AccelMS2 = 0.22f; D.TurnRadiusM = 195.f; D.RudderSlew = 15.f; D.YawResponse = 1.8f; D.bSail = true; D.HullSize = 2; D.LampZCm = 3200.f;
		  D.Names = { TEXT("HMS Implacable") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("cutter"); D.ClassName = TEXT("Revenue cutter"); D.WorldMesh = TEXT("RevenueCutter"); D.LengthM = 24.f; D.BeamM = 5.6f; D.DraftM = 1.6f; D.MaxKn = 11.f; D.CruiseKn = 8.f; D.AccelMS2 = 0.6f; D.TurnRadiusM = 45.f; D.RudderSlew = 30.f; D.YawResponse = 3.4f; D.bSteam = true; D.HullSize = 1;
		  D.SmokeCm = FVector(150.f, 0.f, 1500.f); D.SmokeScale = 0.2f; D.LampZCm = 1500.f; D.Names = { TEXT("HMRC Vigilant") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("gunboat"); D.ClassName = TEXT("Patrol gunboat"); D.WorldMesh = TEXT("PatrolGunboat"); D.LengthM = 32.f; D.BeamM = 6.f; D.DraftM = 1.1f; D.MaxKn = 12.f; D.CruiseKn = 9.f; D.AccelMS2 = 0.7f; D.TurnRadiusM = 60.f; D.RudderSlew = 28.f; D.YawResponse = 3.f; D.bSteam = true; D.HullSize = 1;
		  D.SmokeCm = FVector(-100.f, 0.f, 800.f); D.SmokeScale = 0.2f; D.LampZCm = 800.f; D.Names = { TEXT("HMS Sentinel") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("ironclad"); D.ClassName = TEXT("Ironclad escort"); D.LibMesh = TEXT("/Game/Ironwake/Ships/Kracken/SM_Kracken_Hull.SM_Kracken_Hull"); D.BootTopCm = 50.f; D.LengthM = 120.f; D.BeamM = 20.f; D.DraftM = 6.f; D.MaxKn = 12.f; D.CruiseKn = 9.f; D.AccelMS2 = 0.5f; D.TurnRadiusM = 280.f; D.RudderSlew = 16.f; D.YawResponse = 2.f; D.bSteam = true; D.HullSize = 2;
		  D.SmokeCm = FVector(-600.f, 0.f, 3600.f); D.SmokeScale = 0.4f; D.LampZCm = 3500.f; D.Names = { TEXT("HMS Cataract") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("patrol_airship"); D.ClassName = TEXT("Patrol airship"); D.WorldMesh = TEXT("PatrolAirshipV4"); D.HullYaw = 0.f; D.LengthM = 70.f; D.BeamM = 35.f; D.bAir = true; D.AltCm = 24000.f; D.MaxKn = 22.f; D.CruiseKn = 16.f; D.HullSize = 1; D.LampZCm = 0.f;
		  D.Names = { TEXT("Aurora Watch") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("cargo_airship"); D.ClassName = TEXT("Cargo airship"); D.WorldMesh = TEXT("CargoAirshipBody"); D.LengthM = 120.f; D.BeamM = 80.f; D.bAir = true; D.AltCm = 31000.f; D.MaxKn = 14.f; D.CruiseKn = 10.f; D.HullSize = 2;
		  D.Names = { TEXT("Long Haul") }; Fin(D); }
		{ FNpcDef D; D.Id = TEXT("sky_yacht"); D.ClassName = TEXT("Sky yacht"); D.WorldMesh = TEXT("SkyYacht"); D.LengthM = 45.f; D.BeamM = 37.f; D.bAir = true; D.AltCm = 18000.f; D.MaxKn = 18.f; D.CruiseKn = 12.f; D.HullSize = 1;
		  D.Names = { TEXT("Lady Zephyr") }; Fin(D); }
		return A;
	}
}

namespace IronwakeNpc
{
	static TArray<FNpcDef>& DefsMut() { static TArray<FNpcDef> D = MakeDefs(); return D; }
	const TArray<FNpcDef>& Defs() { return DefsMut(); }
	FNpcDef* FindMutable(FName Id) { for (FNpcDef& D : DefsMut()) if (D.Id == Id) return &D; return nullptr; }
	const FNpcDef* Find(FName Id) { return FindMutable(Id); }
}

// ---------------------------------------------------------------------------- director
TArray<TWeakObjectPtr<USceneComponent>> AIronwakeNpcDirector::Windmills;

AIronwakeNpcDirector::AIronwakeNpcDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Rng.Initialize(2026);
}

AIronwakeNpcDirector* AIronwakeNpcDirector::Get(const UWorld* W)
{
	if (!W) return nullptr;
	TActorIterator<AIronwakeNpcDirector> It(const_cast<UWorld*>(W));
	return It ? *It : nullptr;
}

void AIronwakeNpcDirector::BeginPlay()
{
	Super::BeginPlay();
	Voy = AIronwakeVoyage::Get(GetWorld());
	if (Voy.IsValid()) { HarbourC = Voy->HarbourC; OwnHour = Voy->ClockHour; }
	HourAbs = OwnHour;
	if (FParse::Param(FCommandLine::Get(), TEXT("iwnonpc"))) SetActorTickEnabled(false);
	{ float V; if (FParse::Value(FCommandLine::Get(), TEXT("iwnpcmax="), V)) MaxLive = (int32)V; }
	{ float V; if (FParse::Value(FCommandLine::Get(), TEXT("iwnpcseed="), V)) Rng.Initialize((int32)V); }
	bHud = FParse::Param(FCommandLine::Get(), TEXT("iwnpchud"));
	bNoPirate = FParse::Param(FCommandLine::Get(), TEXT("iwnopirate"));
	{ float V; if (FParse::Value(FCommandLine::Get(), TEXT("iwnpcts="), V)) { SoakScale = V; UGameplayStatics::SetGlobalTimeDilation(this, V); } }
	{ float V; if (FParse::Value(FCommandLine::Get(), TEXT("iwnpcquit="), V)) QuitAfter = V; }
}

FVector AIronwakeNpcDirector::H2W(float XM, float YM) const
{
	const FVector W = HarbourC + FRotator(0.f, 45.f, 0.f).RotateVector(FVector(XM * 100.f, YM * 100.f, 0.f));
	return FVector(W.X, W.Y, 0.f);
}
FVector2D AIronwakeNpcDirector::W2H(const FVector& W) const
{
	const FVector L = FRotator(0.f, 45.f, 0.f).UnrotateVector(W - HarbourC) / 100.f;
	return FVector2D(L.X, L.Y);
}

int32 AIronwakeNpcDirector::LiveCount() const { int32 N = 0; for (const FNpcRec& R : Recs) if (R.Body.IsValid()) ++N; return N; }
FNpcRec* AIronwakeNpcDirector::FindByDef(FName DefId, int32 Nth) { for (FNpcRec& R : Recs) if (R.Def && R.Def->Id == DefId && Nth-- == 0) return &R; return nullptr; }

void AIronwakeNpcDirector::Log(const FString& S) { UE_LOG(LogIronwake, Display, TEXT("%s"), *S); }

FVector AIronwakeNpcDirector::PlayerPos() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) if (APawn* P = PC->GetPawn()) return P->GetActorLocation();
	return FVector(1e9f, 1e9f, 0.f);
}
FVector AIronwakeNpcDirector::ViewPos() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager) return PC->PlayerCameraManager->GetCameraLocation();
		if (APawn* P = PC->GetPawn()) return P->GetActorLocation();
	}
	return FVector(1e9f, 1e9f, 0.f);
}
FVector AIronwakeNpcDirector::PlayerVel() const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(PC->GetPawn())) { const FVector V = P->GetHullVelocity(); return FVector(V.X, V.Y, 0.f); }
	return FVector::ZeroVector;
}

float AIronwakeNpcDirector::HourNow() const { return (float)FMath::Fmod(HourAbs, 24.0); }
bool AIronwakeNpcDirector::IsGale() const { return IronwakeSea::GetSeaState() >= 2.6f || (Voy.IsValid() && Voy->bLiving && Voy->LiveState >= 5); }

void AIronwakeNpcDirector::BuildContacts()
{
	Contacts.Reset();
	const FVector PP = PlayerPos();
	if (PP.X < 1e8f) Contacts.Add({ PP, PlayerVel(), PlayerLenCm, -1, PlayerVel().Size2D() < 30.f, 0.f });
	for (TActorIterator<AIronwakeMerchant> It(GetWorld()); It; ++It)
	{
		const AIronwakeMerchant* M = *It;
		if (!IsValid(M) || M->Route.Num() < 2) continue;
		const FVector Fw = FRotator(0.f, M->Yaw, 0.f).Vector();
		Contacts.Add({ M->GetActorLocation(), Fw * (M->BlockedS > 0.2f ? 0.f : M->SpeedCmS), M->LengthCm, -2, M->BlockedS > 0.2f, M->Yaw });
	}
	for (const FNpcRec& R : Recs)
	{
		if (!R.Body.IsValid() || R.Def->bAir) continue;
		const AIronwakeNpcShip* B = R.Body.Get();
		const FVector Fw = FRotator(0.f, R.Yaw, 0.f).Vector();
		const float Sp = B->V;
		const bool bStat = R.State == ENpcState::Docked || R.State == ENpcState::Anchored || R.State == ENpcState::Work || R.bHold || R.bBasinWait || FMath::Abs(Sp) < 40.f;
		Contacts.Add({ R.Pos, Fw * Sp, R.Def->LengthM * 100.f, R.Id, bStat, R.Yaw });
	}
}

void AIronwakeNpcDirector::TeleportRec(FNpcRec& R, const FVector& P, float Yaw, ENpcState S, float Kn)
{
	ReleaseWaitSlot(R);
	ReleaseCorridorOnly(R);
	MouthDrop(R.Id);
	if (R.Body.IsValid()) DespawnBody(R);
	R.Pos = FVector(P.X, P.Y, 0.f); R.Yaw = Yaw; R.SpeedKn = Kn; R.State = S; R.StateT = 0.f; R.bWarp = false; R.bHold = false; R.bBasinWait = false; R.StuckT = 0.f; R.StuckPos = R.Pos;
}

void AIronwakeNpcDirector::TestRoute(FNpcRec& R, bool bHome)
{
	R.Route.Reset(); R.RouteIdx = 0; R.bHold = false;
	if (bHome && R.HomeLane >= 0) { GoHome(R); } else StartTrip(R, 0, ENpcState::Transit);
}

void AIronwakeNpcDirector::MarkAllKnown() { for (FNpcRec& R : Recs) R.bKnown = true; }

void AIronwakeNpcDirector::SpawnBody(FNpcRec& R)
{
	if (R.Body.IsValid()) return;
	UWorld* W = GetWorld();
	FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; SP.Owner = this;
	AIronwakeNpcShip* B = W->SpawnActor<AIronwakeNpcShip>(AIronwakeNpcShip::StaticClass(), FTransform(FRotator(0.f, R.Yaw, 0.f), R.Pos + FVector(0, 0, 0)), SP);
	if (!B) return;
	R.Body = B;
	B->Init(this, R.Id);
	if (R.Def->bAir) return;
	if (R.State == ENpcState::Docked || R.State == ENpcState::Anchored || R.bHold) { /* body starts at rest */ }
	else B->V = R.SpeedKn * 51.4444f * 1.5f;
}

void AIronwakeNpcDirector::DespawnBody(FNpcRec& R)
{
	if (AIronwakeNpcShip* B = R.Body.Get()) { B->Destroy(); }
	R.Body = nullptr;
}

int32 AIronwakeNpcDirector::CountMovingParts() const
{
	int32 N = 0; for (const FNpcRec& R : Recs) if (const AIronwakeNpcShip* B = R.Body.Get()) N += B->MovingParts; return N;
}

void AIronwakeNpcDirector::UpdateEmbodiment(float Dt)
{
	const FVector PP = PlayerPos();
	const FVector VP = ViewPos();
	const bool bPlayer = PP.X < 1e8f;
	const bool bView = VP.X < 1e8f;
	auto NearDist = [&](const FVector& Pos) -> float
	{
		const float DP = bPlayer ? FVector::Dist2D(Pos, PP) : 1e9f;
		const float DV = bView ? FVector::Dist2D(Pos, VP) : 1e9f;
		return (bPlayer || bView) ? FMath::Min(DP, DV) : 0.f;
	};
	auto CamDist = [&](const FVector& Pos) -> float { return bView ? FVector::Dist2D(Pos, VP) : (bPlayer ? FVector::Dist2D(Pos, PP) : 0.f); };
	// spawn / despawn with hysteresis: nearest to player or active camera first
	TArray<int32> Order; for (const FNpcRec& R : Recs) Order.Add(R.Id);
	Order.Sort([&](int32 A, int32 B) { return CamDist(Recs[A].Pos) < CamDist(Recs[B].Pos); });
	int32 Live = LiveCount();
	for (int32 Id : Order)
	{
		FNpcRec& R = Recs[Id];
		const float D = NearDist(R.Pos);
		if (R.Body.IsValid())
		{
			if (!bForceLive && D > DespawnRadiusCm) { DespawnBody(R); --Live; }
		}
		else if ((bForceLive || D < SpawnRadiusCm) && Live < MaxLive && !R.bAway)
		{
			// do not spawn on top of something
			bool bOk = true;
			if (!R.Def->bAir) bOk = ValidPose(*R.Def, R.Pos, R.Yaw, false);
			if (bOk) { SpawnBody(R); ++Live; }
		}
	}
	// wake FX and lamps measured from active camera (ViewPos) so Photo-Mode/aerial harbour shots render crisp wakes even when player hull is offshore!
	int32 Fx = 0, Lamps = 0;
	const float Lamp = (HourNow() > 19.5f || HourNow() < 5.5f) ? 1.f : 0.f;
	TSet<int32> KeepFx;
	for (int32 Id : Order)
	{
		FNpcRec& R = Recs[Id]; AIronwakeNpcShip* B = R.Body.Get(); if (!B || !B->bFx) continue;
		const float D = CamDist(R.Pos);
		if (R.SpeedKn > 0.4f && !R.Def->bAir && D < FxRadiusCm * 1.6f) { KeepFx.Add(Id); ++Fx; }
	}
	for (int32 Id : Order)
	{
		FNpcRec& R = Recs[Id]; AIronwakeNpcShip* B = R.Body.Get(); if (!B || KeepFx.Contains(Id) || B->bFx) continue;
		const float D = CamDist(R.Pos);
		if (R.SpeedKn > 0.8f && !R.Def->bAir && D < FxRadiusCm && Fx < MaxFx) { KeepFx.Add(Id); ++Fx; }
	}
	for (int32 Id : Order)
	{
		FNpcRec& R = Recs[Id]; AIronwakeNpcShip* B = R.Body.Get(); if (!B) continue;
		const float D = CamDist(R.Pos);
		const bool bWantFx = KeepFx.Contains(Id);
		if (bWantFx != B->bFx) { B->SetFx(bWantFx); if (FParse::Param(FCommandLine::Get(), TEXT("iwfxlog"))) UE_LOG(LogIronwake, Display, TEXT("IWNPC fx %s %s d=%.0f m kn=%.1f"), bWantFx ? TEXT("ON") : TEXT("OFF"), *R.Name, D / 100.f, R.SpeedKn); }
		const bool bWantLamp = Lamp > 0.3f && !R.Def->bAir && D < 250000.f && Lamps < MaxLamps + 10;
		if (bWantLamp != B->bLampOn) B->SetLamp(bWantLamp);
		if (bWantLamp) ++Lamps;
	}
}

// ---------------------------------------------------------------------------- tick
void AIronwakeNpcDirector::Tick(float DtIn)
{
	Super::Tick(DtIn);
	UWorld* W = GetWorld();
	if (!W || W->IsPaused()) return;
	if (!Voy.IsValid()) Voy = AIronwakeVoyage::Get(W);
	const float Dt = FMath::Min(DtIn, 0.5f);
	// ---- game clock (own days counter on top of the voyage clock)
	if (Voy.IsValid() && Voy->bLiving)
	{
		const float H = Voy->ClockHour;
		if (ClockPrev < 0.f) { OwnHour = H; ClockPrev = H; }
		double Delta = H - ClockPrev; if (Delta < -12.0) Delta += 24.0; if (Delta < 0.0) Delta = 0.0;
		OwnHour += Delta; ClockPrev = H;
	}
	else OwnHour += Dt * (24.0 / (48.0 * 60.0)) * (double)FMath::Max(UGameplayStatics::GetGlobalTimeDilation(this), 0.01f) / FMath::Max(UGameplayStatics::GetGlobalTimeDilation(this), 0.01f);
	HourAbs = OwnHour;
	if (!bInit)
	{
		InitTime += Dt;
		if (InitTime < 1.5) return;   // let the harbour / islands register their solids
		BuildPorts(); BuildObstacles(); BuildRoster();
		bInit = true;
		Log(FString::Printf(TEXT("IWNPC init: %d ships (%d ports, %d obstacles), hour %.1f"), Recs.Num(), Ports.Num(), Obs.Num(), HourNow()));
	}
	{
		// windmill sails: turn rate follows the wind (slow turn in calm, ~0.5 rev/s cap)
		const float Wk = IronwakeFleet::WindKn(W);
		const float Rev = FMath::Clamp(0.015f + Wk * 0.011f, 0.02f, 0.35f);
		WindAngle = FMath::Fmod(WindAngle + Rev * 360.f * Dt, 360.f);
		for (TWeakObjectPtr<USceneComponent>& Wm : Windmills) if (USceneComponent* C = Wm.Get()) C->SetRelativeRotation(FRotator(0.f, 0.f, WindAngle));
	}
	EmbT += Dt; ContactT += Dt;
	// player length for CPA
	if (APlayerController* PC = W->GetFirstPlayerController()) if (AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(PC->GetPawn())) PlayerLenCm = P->HullLengthM * 100.f;
	if (ContactT >= 0.1f) { ContactT = 0.f; BuildContacts(); ExpireStaleReservations(W->GetTimeSeconds()); }
	// ---- brains: near ones at 10 Hz (all of them are staggered by a per-rec phase), far ones at 1 Hz
	const float Now = (float)W->GetTimeSeconds();
	const FVector PP = PlayerPos();
	for (FNpcRec& R : Recs)
	{
		const bool bNear = R.Body.IsValid();
		const float Period = bNear ? 0.1f : 1.0f;
		if (Now - R.LastBrainT < Period) continue;
		const float Step = FMath::Min(Now - R.LastBrainT, 5.f);
		R.LastBrainT = Now - 0.013f * (R.Id % 7);
		if (bNear) TickBrain(R, Step);
		else { TickBrain(R, Step); FarStep(R, Step); }
	}
	if (EmbT >= 0.5f) { EmbT = 0.f; UpdateEmbodiment(0.5f); }
	SoakStats(Dt);
	if (QuitAfter > 0.f && Now > QuitAfter)
	{
		int32 Moving = 0, Arr = 0; for (const FNpcRec& R : Recs) { if (R.SpeedKn > 0.8f) ++Moving; Arr += R.TripCount; }
		Log(FString::Printf(TEXT("IWNPC SOAK DONE t=%.0f hour=%.2f trips=%d crates=%d moving=%d"), Now, HourNow(), Arr, Ports.Num() ? Ports[0].Boarded : 0, Moving));
		Log(FString::Printf(TEXT("IWNPC collisions=%d stuck=%d groundings=%d"), StatCollisions, StatStuck, StatGroundings));
		QuitAfter = 0.f;
		UKismetSystemLibrary::QuitGame(this, W->GetFirstPlayerController(), EQuitPreference::Quit, false);
		FGenericPlatformMisc::RequestExit(false);
	}
	(void)PP;
}

void AIronwakeNpcDirector::SoakStats(float Dt)
{
	LogT += Dt;
	if (LogT < 60.f) return;
	LogT = 0.f;
	int32 Moving = 0; for (const FNpcRec& R : Recs) if (R.SpeedKn > 0.8f) ++Moving;
	Log(FString::Printf(TEXT("IWNPC t=%.0f hour=%.2f live=%d moving=%d collisions=%d stuck=%d groundings=%d moving_parts=%d gale=%d"), (float)GetWorld()->GetTimeSeconds(), HourNow(), LiveCount(), Moving, StatCollisions, StatStuck, StatGroundings, CountMovingParts(), IsGale() ? 1 : 0));
}

FString AIronwakeNpcDirector::SummaryLine() const
{
	TMap<FString, int32> N;
	for (const FNpcRec& R : Recs) N.FindOrAdd(NpcStateName(R.State))++;
	FString S;
	for (const TPair<FString, int32>& P : N) S += FString::Printf(TEXT("%s=%d "), *P.Key, P.Value);
	return S;
}

// ---------------------------------------------------------------------------- HUD (F3 list), scope tooltip
FString AIronwakeNpcDirector::HudLine(const FNpcRec& R, const FVector& Player) const
{
	const float D = FVector::Dist2D(R.Pos, Player);
	const FString Tg = R.Target.IsEmpty() ? FString(TEXT("-")) : R.Target;
	return FString::Printf(TEXT("%-22s %-15s %-10s %5.1f kn  %5.0f m  %s%s"), *R.Name, NpcRoleName(R.Role), NpcStateName(R.State), R.SpeedKn, D, *Tg, R.Body.IsValid() ? TEXT("") : TEXT("  (ledger)"));
}

void AIronwakeNpcDirector::DrawNpcHud(AHUD* H, UCanvas* Cv, UFont* Font, float S, const FVector& Player)
{
	if (!bHud || !bInit) return;
	float Y = 60.f * S; const float X = 24.f * S;
	H->DrawRect(FLinearColor(0.02f, 0.03f, 0.035f, 0.72f), X - 8.f * S, Y - 8.f * S, 800.f * S, (Recs.Num() + 3) * 17.f * S + 16.f);
	H->DrawText(FString::Printf(TEXT("NPC TRAFFIC  %d ships, %d live   %s   %s"), Recs.Num(), LiveCount(), *FString::Printf(TEXT("%02d:%02d"), (int32)HourNow(), (int32)(FMath::Frac(HourNow()) * 60.f)), IsGale() ? TEXT("GALE: shelter") : TEXT("")), FLinearColor(0.86f, 0.74f, 0.47f), X, Y, Font, 0.8f * S);
	Y += 20.f * S;
	TArray<const FNpcRec*> Sorted; for (const FNpcRec& R : Recs) Sorted.Add(&R);
	Sorted.Sort([&](const FNpcRec& A, const FNpcRec& B) { return FVector::DistSquared2D(A.Pos, Player) < FVector::DistSquared2D(B.Pos, Player); });
	for (const FNpcRec* R : Sorted)
	{
		H->DrawText(HudLine(*R, Player), R->Body.IsValid() ? FLinearColor(0.85f, 0.88f, 0.9f) : FLinearColor(0.55f, 0.58f, 0.6f), X, Y, Font, 0.62f * S);
		Y += 16.f * S;
	}
}

const FNpcRec* AIronwakeNpcDirector::ScopeTarget(const FVector& Eye, float AimYaw, float MaxRangeCm, float& OutRangeCm)
{
	const FNpcRec* Best = nullptr; float BestScore = 1e9f;
	for (const FNpcRec& R : Recs)
	{
		const FVector P = R.Def->bAir ? FVector(R.Pos.X, R.Pos.Y, R.Def->AltCm) : R.Pos;
		const FVector D = P - Eye; const float Rg = D.Size2D(); if (Rg > MaxRangeCm || Rg < 500.f) continue;
		const float Brg = FMath::Abs(FRotator::NormalizeAxis(D.Rotation().Yaw - AimYaw));
		const float Tol = FMath::RadiansToDegrees(FMath::Atan2(FMath::Max(R.Def->LengthM * 100.f * 0.6f, 600.f), Rg)) + 1.5f;
		if (Brg < Tol && Brg < BestScore) { BestScore = Brg; Best = &R; OutRangeCm = Rg; }
	}
	if (Best) const_cast<FNpcRec*>(Best)->bKnown = true;
	return Best;
}

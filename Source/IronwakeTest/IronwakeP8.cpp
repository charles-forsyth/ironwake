#include "IronwakeP8.h"
#include "IronwakeNpc.h"
#include "IronwakeShipWave.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "IronwakeTest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"
#include "IronwakeMoonTable.inc"

AIronwakeNightSky::AIronwakeNightSky()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	Dome = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Dome"));
	SetRootComponent(Dome);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sph(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sph.Succeeded()) Dome->SetStaticMesh(Sph.Object);
	Dome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dome->SetCastShadow(false);
	Dome->bAffectDistanceFieldLighting = false;
	Dome->bAffectDynamicIndirectLighting = false;
	Dome->SetCanEverAffectNavigation(false);
	Dome->SetRelativeScale3D(FVector(800000.f)); // radius 400 km: beyond the 300 km ocean
	Dome->SetVisibility(false);
}

void AIronwakeNightSky::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Sky/M_IW_NightSky.M_IW_NightSky")))
	{
		MID = UMaterialInstanceDynamic::Create(M, this);
		Dome->SetMaterial(0, MID);
	}
	else UE_LOG(LogIronwake, Warning, TEXT("IWP8 night sky material missing"));
	for (int32 i = 0; i < 8; ++i)
		MoonTex.Add(LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Sky/T_IW_MoonPhase%d.T_IW_MoonPhase%d"), i, i)));
	DayIdx = FMath::Clamp(FDateTime::Now().GetDayOfYear() - 1, 0, 364);
	float V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwskyday="), V)) DayIdx = FMath::Clamp((int32)V, 0, 364);
	if (FParse::Value(FCommandLine::Get(), TEXT("iwskyhour="), V)) { Hour = V; bHold = true; }
	if (FParse::Value(FCommandLine::Get(), TEXT("iwskyrate="), V)) ClockRate = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwstargain="), V)) StarGain = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwmoongain="), V)) MoonGain = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwptth="), V)) PtThresh = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwskygamma="), V)) SkyGamma = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwptgain="), V)) PtGain = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwglow="), V)) GlowGain = V;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwglowmip="), V)) GlowMip = V;
	UE_LOG(LogIronwake, Display, TEXT("IWP8 night sky up: day=%d (moon phase %.0f deg) hour=%.1f lat=%.0f mid=%d"), DayIdx, GMoonPhaseDeg[DayIdx], Hour, LatDeg, MID != nullptr);
}

FVector AIronwakeNightSky::DirOf(float RaDeg, float DecDeg) const
{
	const double a = FMath::DegreesToRadians(RaDeg), d = FMath::DegreesToRadians(DecDeg);
	const double qx = FMath::Cos(d) * FMath::Cos(a), qy = FMath::Cos(d) * FMath::Sin(a), qz = FMath::Sin(d);
	return FVector(LR0.R * qx + LR1.R * qy + LR2.R * qz, LR0.G * qx + LR1.G * qy + LR2.G * qz, LR0.B * qx + LR1.B * qy + LR2.B * qz).GetSafeNormal();
}

FString AIronwakeNightSky::Describe() const
{
	return FString::Printf(TEXT("hour=%.2f day=%d phase=%.0f sprite=%d fade=%.2f moonNEU=(%.2f,%.2f,%.2f) alt=%.1f"), Hour, DayIdx, MoonPhaseDeg, MoonSprite, Fade,
		MoonDirNEU.X, MoonDirNEU.Y, MoonDirNEU.Z, FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(MoonDirNEU.Z, -1.0, 1.0))));
}

void AIronwakeNightSky::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bHold) Hour = FMath::Fmod(Hour + Dt * ClockRate / 3600.f, 24.f);
	// stars fade out as the weather blends toward day, and with cloud/overcast (Clear 0)
	Fade = FMath::Clamp((Night - 0.5f) / 0.4f, 0.f, 1.f) * Clear;
	const bool bVis = Fade > 0.003f && MID;
	if (Dome->GetVisibleFlag() != bVis) Dome->SetVisibility(bVis);
	if (!bVis)
	{
		// P13: the moon's direction/phase stays current while the dome is hidden (moonlight under cloud, twilight)
		const double D2R = PI / 180.0, Phi = LatDeg * D2R;
		const double LamS = 360.0 * (((DayIdx + 1) - 80.0) / 365.25) * D2R, Eps = 23.44 * D2R;
		const double RaS = FMath::Atan2(FMath::Sin(LamS) * FMath::Cos(Eps), FMath::Cos(LamS));
		const double Theta = RaS + (Hour - 12.0) * 15.0 * D2R;
		MoonPhaseDeg = GMoonPhaseDeg[DayIdx];
		const double LamM = LamS + MoonPhaseDeg * D2R;
		const double RaM = FMath::Atan2(FMath::Sin(LamM) * FMath::Cos(Eps), FMath::Cos(LamM));
		const double DecM = FMath::Asin(FMath::Sin(LamM) * FMath::Sin(Eps));
		const double H = Theta - RaM, cP = FMath::Cos(Phi), sP = FMath::Sin(Phi);
		MoonDirNEU = FVector(cP * FMath::Sin(DecM) - sP * FMath::Cos(DecM) * FMath::Cos(H), -FMath::Cos(DecM) * FMath::Sin(H), sP * FMath::Sin(DecM) + cP * FMath::Cos(DecM) * FMath::Cos(H));
		return;
	}
	if (APlayerCameraManager* CM = UGameplayStatics::GetPlayerCameraManager(this, 0)) SetActorLocation(CM->GetCameraLocation());

	// ---- sky orientation. World axes: +X north, +Y east, +Z up. Observer at latitude Phi, local sidereal time Theta.
	const double D2R = PI / 180.0, Phi = LatDeg * D2R;
	const int32 Doy = DayIdx + 1;
	const double LamS = 360.0 * ((Doy - 80.0) / 365.25) * D2R;      // sun ecliptic longitude (vernal equinox ~ day 80)
	const double Eps = 23.44 * D2R;
	const double RaS = FMath::Atan2(FMath::Sin(LamS) * FMath::Cos(Eps), FMath::Cos(LamS));
	const double Theta = RaS + (Hour - 12.0) * 15.0 * D2R;           // LST = sun RA + sun hour angle
	const double cT = FMath::Cos(Theta), sT = FMath::Sin(Theta), cP = FMath::Cos(Phi), sP = FMath::Sin(Phi);
	const FLinearColor R0((float)(-cT * sP), (float)(-sT), (float)(cT * cP), 0.f);
	const FLinearColor R1((float)(-sT * sP), (float)(cT), (float)(sT * cP), 0.f);
	const FLinearColor R2((float)cP, 0.f, (float)sP, 0.f);
	// ---- moon: elongation = phase angle east of the sun along the ecliptic (inclination ignored), real daily phase
	MoonPhaseDeg = GMoonPhaseDeg[DayIdx];
	const double LamM = LamS + MoonPhaseDeg * D2R;
	const double RaM = FMath::Atan2(FMath::Sin(LamM) * FMath::Cos(Eps), FMath::Cos(LamM));
	const double DecM = FMath::Asin(FMath::Sin(LamM) * FMath::Sin(Eps));
	const double H = Theta - RaM;
	MoonDirNEU = FVector(cP * FMath::Sin(DecM) - sP * FMath::Cos(DecM) * FMath::Cos(H), -FMath::Cos(DecM) * FMath::Sin(H),
		sP * FMath::Sin(DecM) + cP * FMath::Cos(DecM) * FMath::Cos(H));
	MoonSprite = ((int32)FMath::RoundToInt(MoonPhaseDeg / 45.f)) & 7;
	LR0 = R0; LR1 = R1; LR2 = R2;
	MID->SetVectorParameterValue(TEXT("R0"), R0);
	MID->SetVectorParameterValue(TEXT("R1"), R1);
	MID->SetVectorParameterValue(TEXT("R2"), R2);
	MID->SetVectorParameterValue(TEXT("MoonDir"), FLinearColor((float)MoonDirNEU.X, (float)MoonDirNEU.Y, (float)MoonDirNEU.Z, 0.f));
	MID->SetScalarParameterValue(TEXT("StarGain"), StarGain);
	MID->SetScalarParameterValue(TEXT("Gamma"), SkyGamma); MID->SetScalarParameterValue(TEXT("PtGain"), PtGain);
	MID->SetScalarParameterValue(TEXT("GlowGain"), GlowGain); MID->SetScalarParameterValue(TEXT("GlowMip"), GlowMip);
	MID->SetScalarParameterValue(TEXT("StarFade"), Fade);
	MID->SetScalarParameterValue(TEXT("MoonFade"), Fade);
	MID->SetScalarParameterValue(TEXT("MoonGain"), MoonGain);
	MID->SetScalarParameterValue(TEXT("MoonCos"), (float)FMath::Cos(MoonRadiusDeg * D2R));
	if (MoonTex.IsValidIndex(MoonSprite) && MoonTex[MoonSprite]) MID->SetTextureParameterValue(TEXT("MoonTex"), MoonTex[MoonSprite]);
}

// ---------------------------------------------------------------------------- NPC craft + harbour town
#include "IronwakeVoyage.h"
#include "IronwakeShell.h"
#include "IronwakeWorld.h"
#include "IronwakeCollision.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AIronwakeNpcCraft::AIronwakeNpcCraft()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(R);
}

void AIronwakeNpcCraft::Setup(const TCHAR* MeshName, const FVector& CentreCm, float RadA, float RadB, float PeriodS, float AltCm, float KeelCm, float Phase, bool bCW, bool bAir)
{
	Body = IronwakeWorld::Place(this, RootComponent, MeshName, FVector::ZeroVector, FRotator::ZeroRotator, FVector(1.f), false, false);
	Centre = CentreCm; A = RadA; B = RadB; Period = PeriodS; Alt = AltCm; Keel = KeelCm; Ph = Phase; Sgn = bCW ? 1.f : -1.f; bAirship = bAir;
	T = Phase * Period;
	if (!bAir && Body && Body->GetStaticMesh())
		if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()))
		{
			const FBox Bx = Body->GetStaticMesh()->GetBoundingBox();
			TArray<FVector2D> Poly; UIronwakeColl::OutlineRect(Bx.Min.X - 40.f, Bx.Max.X + 40.f, FMath::Max(Bx.Max.Y, -Bx.Min.Y) + 40.f, Poly);
			CollShape = UIronwakeColl::MakeShape(Poly, FMath::Clamp(-Bx.Min.Z * 0.5f, 150.f, 400.f));
			Co->RegisterHull(this, CollShape, false);
			bCollReg = true;
			TArray<FVector> Loop;
			for (int32 i = 0; i < 72; ++i) { const float Ta = 2.f * PI * i / 72.f; Loop.Add(FVector(Centre.X + A * FMath::Cos(Ta), Centre.Y + B * FMath::Sin(Ta), 0.f)); }
		}
	Tick(0.f);
}

void AIronwakeNpcCraft::Tick(float Dt)
{
	Super::Tick(Dt);
	const float SlowK = (GetWorld() && GetWorld()->GetTimeSeconds() < SlowUntil) ? 0.3f : 1.f;
	const float T0 = T;
	T += Dt * SlowK;
	const float th = Sgn * 2.f * PI * T / Period;
	const FVector P(Centre.X + A * FMath::Cos(th), Centre.Y + B * FMath::Sin(th), 0.f);
	const FVector Tan(-A * FMath::Sin(th) * Sgn, B * FMath::Cos(th) * Sgn, 0.f);
	const float Yaw = Tan.Rotation().Yaw;
	UWorld* W = GetWorld();
	if (bAirship)
	{
		const float Z = Alt + 2500.f * FMath::Sin(T * 0.05f);
		Cur = FRotator(1.5f * FMath::Sin(T * 0.21f), Yaw, 2.f * FMath::Sin(T * 0.17f));
		SetActorLocationAndRotation(FVector(P.X, P.Y, Z), Cur);
		return;
	}
	if (bCollReg && Dt > 0.f)
	{
		if (UIronwakeColl* Co = UIronwakeColl::Get(W))
		{
			if (!bAudited)
			{
				bAudited = true;
				TArray<FVector> Loop;
				for (int32 i = 0; i < 72; ++i) { const float Ta = 2.f * PI * i / 72.f; Loop.Add(FVector(Centre.X + A * FMath::Cos(Ta), Centre.Y + B * FMath::Sin(Ta), 0.f)); }
				Co->AuditPath(GetName(), Loop, true, CollShape.R * 0.2f, CollShape.DraughtCm);
			}
			const FVector2D PN(P.X, P.Y);
			const FIwHit H = Co->TestPose(this, CollShape, PN, Yaw);
			if (H.bHit || Co->MinDepthAt(CollShape, PN, Yaw) < CollShape.DraughtCm) { T = T0; return; }   // hold position this tick
		}
	}
	const FVector Fw = FRotator(0.f, Yaw, 0.f).Vector();
	const float L = 1200.f;
	const float Zc = AIronwakeShell::WaterZAtWaves(W, P), Zb = AIronwakeShell::WaterZAtWaves(W, P + Fw * L), Zs = AIronwakeShell::WaterZAtWaves(W, P - Fw * L);
	const FVector Rt = FRotator(0.f, Yaw + 90.f, 0.f).Vector();
	const float Zp = AIronwakeShell::WaterZAtWaves(W, P + Rt * 400.f), Zq = AIronwakeShell::WaterZAtWaves(W, P - Rt * 400.f);
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Zb - Zs, 2.f * L)) * 0.8f;
	const float Roll = FMath::RadiansToDegrees(FMath::Atan2(Zp - Zq, 800.f)) * 0.6f;
	const FRotator Want(Pitch, Yaw, Roll);
	Cur = (Dt > 0.f) ? FMath::RInterpTo(Cur, Want, Dt, 1.5f) : Want;
	SetActorLocationAndRotation(FVector(P.X, P.Y, Zc + AIronwakeShell::RenderedSeaOffsetCm + Keel), Cur);
	// P19: NPC hulls make waves too (speed from the loop, hull from the mesh bounds); paddle steamer and gunboat are the steamers.
	if (Body && Body->GetStaticMesh() && Dt > 0.f)
	{
		const FBox Bx = Body->GetStaticMesh()->GetBoundingBox();
		FIwShipWaveSrc Sw; Sw.PosCm = FVector2D(P.X, P.Y); const FVector Fv = FRotator(0.f, Yaw, 0.f).Vector(); Sw.CosH = Fv.X; Sw.SinH = Fv.Y;
		Sw.V = 2.f * PI * FMath::Sqrt(0.5f * (A * A + B * B)) / FMath::Max(Period, 1.f) * 0.01f;
		Sw.L = (Bx.Max.X - Bx.Min.X) * 0.01f; Sw.X1 = Bx.Max.X * 0.01f; Sw.HalfBeam = FMath::Max(Bx.Max.Y, -Bx.Min.Y) * 0.01f;
		Sw.Draft = FMath::Clamp(-Bx.Min.Z * 0.01f * 0.5f, 1.f, 6.f); Sw.Freeboard = FMath::Clamp(Bx.Max.Z * 0.01f * 0.3f, 1.f, 4.f); Sw.Bluff = 0.5f;
		IronwakeSea::RegisterShipWave(this, Sw);
	}
}

void AIronwakeVoyage::BuildTownP8(USceneComponent* Hs)
{
	// P10: a real town. Harbour frame: +X seaward, +Y along the quay, quay top Q = 2 m, rendered-sea zero. Land x -196..-50, y -135..165.
	// Square on the quay (x -90..-50), main street y = 22 running up the hill (-X) from the square to the chapel at x ~ -183,
	// lanes A (x = -100) and B (x = -140) crossing it, terraces of cottages/townhouses/timber houses facing the streets.
	// Ground climbs from Q at x = -90 to Q + 5 at x = -142 (pitched boxes follow it).
	const float Q = 2.0f;
	const auto M = [](float X, float Y, float Z) { return FVector(X * 100.f, Y * 100.f, Z * 100.f); };
	BuildTownP12(Hs);   // P12: hillside town (IronwakeP12.cpp) replaces the P10 flat layout
	// Moored boats at the quay (ride the swell): pilot launch and rowing gig.
	auto Fl = [&](const TCHAR* N, float X, float Y, float Keel, float Yaw)
	{
		if (UStaticMeshComponent* C = IronwakeWorld::Place(this, Hs, N, M(X, Y, Keel), FRotator(0.f, Yaw, 0.f)))
		{ Floaters.Add(C); FloaterBase.Add(FVector(Keel * 100.f, Yaw, Floaters.Num() * 1.9f)); }
	};
	const bool bOldNpc = FParse::Param(FCommandLine::Get(), TEXT("iwoldnpc"));
	if (bOldNpc) { Fl(TEXT("PilotLaunch"), -40, 103, -0.7f, 90); Fl(TEXT("RowingGig"), -44, 78, -0.25f, 82); }
	// NPC craft on slow loops outside the harbour mouth (seaward = harbour yaw 45), airships on high loops.
	const FVector Out = FVector(0.7071f, 0.7071f, 0.f);
	const FVector Side = FVector(-0.7071f, 0.7071f, 0.f);
	const FVector C0 = HarbourC;
	auto Npc = [&](const TCHAR* N, const FVector& Ctr, float Ra, float Rb, float Period, float Alt, float Keel, float Ph, bool CW, bool Air)
	{
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; SP.Owner = this;
		if (AIronwakeNpcCraft* A = GetWorld()->SpawnActor<AIronwakeNpcCraft>(AIronwakeNpcCraft::StaticClass(), FTransform(Ctr), SP)) A->Setup(N, Ctr, Ra, Rb, Period, Alt, Keel, Ph, CW, Air);
	};
	if (!bOldNpc)
	{
		FActorSpawnParameters SP; SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; SP.Owner = this;
		GetWorld()->SpawnActor<AIronwakeNpcDirector>(AIronwakeNpcDirector::StaticClass(), FTransform(C0), SP);   // P20
	}
	else {
	Npc(TEXT("PaddleSteamer"), C0 + Out * 90000.f + Side * 15000.f, 40000.f, 22000.f, 1500.f, 0.f, -150.f, 0.1f, true, false);
	Npc(TEXT("SteamDrifter"), C0 + Out * 62000.f - Side * 30000.f, 26000.f, 15000.f, 1100.f, 0.f, -130.f, 0.55f, false, false);
	Npc(TEXT("PatrolGunboat"), C0 + Out * 120000.f - Side * 10000.f, 52000.f, 30000.f, 1300.f, 0.f, -110.f, 0.8f, true, false);
	Npc(TEXT("PatrolAirship"), C0 + Out * 100000.f + Side * 25000.f, 70000.f, 45000.f, 1500.f, 24000.f, 0.f, 0.3f, true, true);
	Npc(TEXT("SkyYacht"), C0 + Out * 70000.f - Side * 40000.f, 45000.f, 30000.f, 1100.f, 17000.f, 0.f, 0.75f, false, true);
	Npc(TEXT("CargoAirshipLong"), C0 + Out * 85000.f - Side * 8000.f, 55000.f, 30000.f, 1700.f, 21000.f, 0.f, 0.5f, true, true);   // P17
	Npc(TEXT("CoalLighter"), C0 + Out * 48000.f + Side * 22000.f, 18000.f, 11000.f, 900.f, 0.f, -110.f, 0.2f, true, false);   // P17
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP8 town built: %d floaters"), Floaters.Num());
}

// ---------------------------------------------------------------------------- P8 capture director
// -iwp8=shots [-iwp8quit]: day + night views toward the port from the sea (chase + high wide), then a clear-night sky shot.
// -iwp8=bench: hold the day harbour chase view (CSV capture).  -iwp8=sky: only the sky shots.
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
void AIronwakeVoyage::TickP8Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	P8T += RealDt;
	const float Since = P8T - P8Mark;
	const FVector Town = HarbourC + FRotator(0.f, 45.f, 0.f).RotateVector(FVector(-9000.f, 0.f, 0.f)); // town centre, 90 m inland of the quay edge
	auto Stand = [&](float StandM, float FromYaw)
	{
		const FVector D = FRotator(0.f, FromYaw, 0.f).Vector();
		const FVector L = FVector(Town.X, Town.Y, 0.f) + D * StandM * 100.f;
		P->SetActorLocationAndRotation(FVector(L.X, L.Y, P->GetActorLocation().Z), FRotator(0.f, FromYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(2);
		P->ResetCameraToHorizon();
	};
	struct FStep { const TCHAR* Shot; int32 Wx; float StandM; float FromYaw; bool bHigh; float Hour; float SkyPitch; float SkyYaw; };
	static const FStep Steps[] = {
		{ TEXT("P8_day_chase"), 0, 450.f, 55.f, false, -1.f, 0.f, 0.f },
		{ TEXT("P8_day_chase2"), 0, 280.f, 40.f, false, -1.f, 0.f, 0.f },
		{ TEXT("P8_day_high"), 0, 700.f, 50.f, true, -1.f, 0.f, 0.f },
		{ TEXT("P8_night_chase"), 5, 450.f, 55.f, false, 23.f, 0.f, 0.f },
		{ TEXT("P8_night_high"), 5, 700.f, 50.f, true, 23.f, 0.f, 0.f },
		{ TEXT("P8_sky_south"), 5, 450.f, 55.f, true, 23.f, 35.f, 180.f },
		{ TEXT("P8_sky_wide"), 5, 450.f, 55.f, true, 0.5f, 50.f, 90.f },
		{ TEXT("P8_sky_orion"), 5, 450.f, 55.f, true, 23.f, -998.f, 83.8f },
		{ TEXT("P8_sky_cygnus"), 5, 450.f, 55.f, true, 21.f, -997.f, 305.f },
		{ TEXT("P8_sky_moon"), 5, 450.f, 55.f, true, 23.f, -999.f, 0.f },
		{ TEXT("P8_town_close_a"), 0, 230.f, 25.f, true, -1.f, -5.f, 0.f },
		{ TEXT("P8_town_close_b"), 0, 230.f, 70.f, true, -1.f, -5.f, 0.f },
		{ TEXT("P8_town_top"), 0, 120.f, 45.f, true, -1.f, -6.f, 0.f },
		{ TEXT("P8_town_down"), 0, 10.f, 45.f, true, -1.f, -7.f, 0.f },
		{ TEXT("P8_town_close_night"), 5, 230.f, 45.f, true, 23.f, -5.f, 0.f },
	};
	int32 NS = UE_ARRAY_COUNT(Steps);
	if (P8Mode == TEXT("sky")) NS = 10;
	if (P8Mode == TEXT("skyq")) NS = 9;
	if (P8Mode == TEXT("town")) NS = 15; // first 10 steps include the sky shots (day shots 1-3 are cheap)
	if (P8Mode == TEXT("bench"))
	{
		if (P8Step == 0 && P8T >= 3.f) { if (!C->bStarted) C->StartGame(true); Stand(330.f, 60.f); P8Step = 1; P8Mark = P8T; }
		if (P8Step == 1 && Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		return;
	}
	if (P8Step == 0)
	{
		if (P8T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		P8Step = (P8Mode == TEXT("town")) ? 11 : (P8Mode == TEXT("skyq")) ? 6 : (P8Mode == TEXT("sky")) ? 4 : 1; P8Mark = P8T; SetWeather(Steps[P8Step - 1].Wx, 0.1f); Stand(Steps[P8Step - 1].StandM, Steps[P8Step - 1].FromYaw); return;
	}
	if (P8Step >= 1 && P8Step <= NS)
	{
		const FStep& S = Steps[P8Step - 1];
		if (Since > 2.f && Since < 2.3f && !S.bHigh) P->ResetCameraToHorizon();
		if (Since > 3.f && Since < 3.3f && S.bHigh)
		{
			if (!bPhoto) TogglePhoto();
			const FVector D = FRotator(0.f, S.FromYaw, 0.f).Vector();
			const bool bSkyShot = S.SkyPitch > 0.f || S.SkyPitch < -900.f;
			const bool bClose = S.SkyPitch == -5.f || S.SkyPitch == -6.f || S.SkyPitch == -7.f;
			const bool bTop = S.SkyPitch == -6.f; const bool bDown = S.SkyPitch == -7.f;
			const FVector Eye = FVector(Town.X, Town.Y, 0.f) + D * S.StandM * 100.f + FVector(0, 0, bSkyShot ? 7000.f : bDown ? 22000.f : bTop ? 14000.f : bClose ? 2500.f : 6500.f);
			if ((S.SkyPitch == -998.f || S.SkyPitch == -997.f) && NightSky)
			{
				const FVector Dd = NightSky->DirOf(S.SkyYaw, S.SkyPitch == -998.f ? -1.2f : 40.f);
				{ float Po = 0.f; FParse::Value(FCommandLine::Get(), TEXT("iwskypo="), Po); FRotator Rr = Dd.Rotation(); float Yo = 0.f; FParse::Value(FCommandLine::Get(), TEXT("iwskyyo="), Yo); Rr.Pitch += Po; Rr.Yaw += Yo; SetPhotoView(Eye + FVector(0, 0, 8000.f), Rr); }
				UE_LOG(LogIronwake, Display, TEXT("IWP10 aim hour=%.2f dir=(%.3f,%.3f,%.3f) pitch=%.1f yaw=%.1f"), NightSky->Hour, Dd.X, Dd.Y, Dd.Z, Dd.Rotation().Pitch, Dd.Rotation().Yaw);
			}
			else if (S.SkyPitch < -900.f && NightSky) SetPhotoView(Eye, FRotator(FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(NightSky->MoonDirNEU.Z, -1.0, 1.0))) * 0.9f, FMath::RadiansToDegrees(FMath::Atan2(NightSky->MoonDirNEU.Y, NightSky->MoonDirNEU.X)), 0.f));
			else if (bDown)
			{
				const FVector Loc = HarbourC + FRotator(0.f, 45.f, 0.f).RotateVector(FVector(-11000.f, 0.f, 0.f)) + FVector(0, 0, 24000.f);
				SetPhotoView(Loc, FRotator(-89.f, 45.f, 0.f));
			}
			else if (bClose) SetPhotoView(Eye, (Town + FVector(0.f, 0.f, 600.f) - Eye).Rotation());
			else if (S.SkyPitch > 0.f) SetPhotoView(Eye, FRotator(S.SkyPitch, S.SkyYaw, 0.f));
			else SetPhotoView(Eye, (Town + FVector(0, 0, 1500.f) - Eye).Rotation());
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(S.SkyPitch == -7.f ? 70.f : S.SkyPitch > 0.f ? 75.f : S.SkyPitch < -996.f && S.SkyPitch > -999.f ? 90.f : S.SkyPitch < -900.f ? 40.f : 55.f);
		}
		if (NightSky && S.Hour >= 0.f && Since > 0.5f) NightSky->SetClock(S.Hour);
		if (Since >= 12.f && !bP8Shot)
		{
			C->IWShot(S.Shot); bP8Shot = true;
			UE_LOG(LogIronwake, Display, TEXT("IWP8 shot %s wx=%d sky: %s"), S.Shot, S.Wx, NightSky ? *NightSky->Describe() : TEXT("-"));
		}
		if (Since >= 13.5f)
		{
			bP8Shot = false; ++P8Step; P8Mark = P8T;
			if (bPhoto) TogglePhoto();
			if (P8Step <= NS)
			{
				const FStep& N = Steps[P8Step - 1];
				if (N.Wx != WeatherTarget) SetWeather(N.Wx, 0.1f);
				Stand(N.StandM, N.FromYaw);
			}
		}
		return;
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP8 DONE"));
	P8Mode.Empty();
	if (FParse::Param(FCommandLine::Get(), TEXT("iwp8quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
}

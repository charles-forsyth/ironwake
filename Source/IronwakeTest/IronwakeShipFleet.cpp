// S2: roster hulls on the player pawn (cutter / frigate / ironclad), sail drive from the wind, auxiliary steam.
#include "IronwakeShipPawn.h"
#include "IronwakeFleet.h"
#include "IronwakeShell.h"
#include "IronwakeWorld.h"
#include "IronwakeTest.h"
#include "BuoyancyComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

FString AIronwakeShipPawn::GetShipName() const
{
	return ShipDef ? ShipDef->Name : (bIsNPC ? FString(TEXT("Kracken")) : FString(TEXT("Devastation")));
}

void AIronwakeShipPawn::ApplyFleetHull()
{
	// Called from ApplyHullDef (before Super::BeginPlay) for the non-ironclad roster hulls.
	const FIronwakeShipDef& D = *ShipDef;
	UStaticMesh* M = ShipId == TEXT("frigate") ? MeshFrigateHull.Get() : (ShipId == TEXT("cutter") ? MeshCutterHull.Get() : nullptr);
	if (!M) M = LoadObject<UStaticMesh>(nullptr, D.HullMesh);
	HullMesh->SetStaticMesh(M);
	// No turrets, screws or rudder mesh from the Devastation on these hulls (the combat code keeps its invisible pivots).
	TArray<USceneComponent*> Kids;
	TurretFore->GetChildrenComponents(true, Kids);
	TArray<USceneComponent*> Kids2;
	TurretAft->GetChildrenComponents(true, Kids2);
	Kids.Append(Kids2);
	for (USceneComponent* K : Kids) K->SetVisibility(false);
	PropPort->SetVisibility(false); PropStbd->SetVisibility(false); RudderMesh->SetVisibility(false);
	const float Lw = D.Profile ? D.Profile().Length() : D.LengthM;
	TurretFore->SetRelativeLocation(FVector(Lw * 25.f, 0.f, 300.f));
	TurretAft->SetRelativeLocation(FVector(-Lw * 25.f, 0.f, 300.f));

	HullProf = D.Profile ? D.Profile() : FIronwakeHullProfile::Aglae();
	HullLengthM = D.LengthM;
	MassKg = D.MassKg;
	if (D.TelegraphKnots.Num() == 7) TelegraphKnots = D.TelegraphKnots;
	MaxAccelMS2 = D.MaxAccelMS2; SpeedTimeConstant = D.SpeedTimeConstant; MinTurnRadiusM = D.MinTurnRadiusM;
	YawResponse = D.YawResponse; LateralDrag = D.LateralDrag; RudderSlewDegPerSec = D.RudderSlew;
	HeavePeriod = D.HeavePeriod; HeaveZeta = D.HeaveZeta; PitchPeriod = D.PitchPeriod; PitchZeta = D.PitchZeta;
	RollPeriod = D.RollPeriod; RollZeta = D.RollZeta;
	SeaFollowHeave = D.SeaFollowHeave; SeaFollowPitch = D.SeaFollowPitch; SeaFollowRoll = D.SeaFollowRoll; FitSpan = D.FitSpan;
	DraftBelowBootTopCm = D.DraftBelowBootTopCm;
	HullBody->SetBoxExtent(FVector(HullProf.Length() * 0.5f * 100.f, D.BeamM * 0.5f * 100.f, FMath::Clamp(D.LengthM * 5.f, 150.f, 700.f)));
	// Pontoons stay at the ironclad layout's depth (W1 waterplane hydrostatics replaced them; they only set the static
	// sea level on the body = PontoonZCm + 0.1 R = -70 cm, which the waterline rule below relies on).
	HullMeshZ = -150.f + 0.1f * 800.f + AIronwakeShell::RenderedSeaOffsetCm - DraftBelowBootTopCm;
	// Centre the hull's waterplane on the body origin (the frigate's waterline is aft of its bbox centre: bowsprit).
	const float Xc = 0.5f * (HullProf.X0 + HullProf.X1) * 100.f;
	HullMesh->SetRelativeLocation(FVector(-Xc, 0.f, HullMeshZ));
	HullProf.X0 -= Xc / 100.f; HullProf.X1 -= Xc / 100.f;
	// Camera: frigate masts are 57 m; keep the default zoom out a step more for small hulls.
	if (ShipId == TEXT("cutter")) for (float& Z : ZoomStepsL) Z *= 1.8f;   // 30 m hull: same metres as a bigger ship
	if (ShipId == TEXT("brig")) ZoomStepsL = { 0.44f, 0.56f, 0.72f, 0.92f, 1.18f, 1.5f, 2.0f, 2.8f, 3.9f, 5.4f }; // 42 m brig, 27 m masts: closest ~18.5 m
	if (ShipId == TEXT("frigate")) ZoomStepsL = { 0.36f, 0.45f, 0.58f, 0.75f, 0.95f, 1.25f, 1.7f, 2.5f, 3.6f, 5.2f }; // 57 m masts, 26 m yards: closest ~32 m, outside the sails
	UE_LOG(LogIronwake, Display, TEXT("IWS2 hull %s (%s) L=%.0f m mass=%.0f t turn R=%.0f m tel=[%.0f..%.0f kn] meshZ=%.0f xc=%.0f"),
		*D.Name, *ShipId.ToString(), D.LengthM, MassKg / 1000.f, MinTurnRadiusM, TelegraphKnots[0], TelegraphKnots.Last(), HullMeshZ, Xc);
}

void AIronwakeShipPawn::FleetBeginPlay()
{
	if (bIsNPC || !ShipDef) return;
	if (ShipDef->bSail)
	{
		SailRig = NewObject<UIronwakeSailRig>(this, TEXT("SailRig"));
		SailRig->SetupAttachment(HullMesh);
		SailRig->bTallBrig = (ShipId == TEXT("brig"));
		SailRig->ClothMat = ClothMaterial;
		SailRig->RegisterComponent();
		SailRig->Build();
	}
	if (ShipDef->bFunnelSmoke)
	{
		if (UNiagaraSystem* Smoke = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_FunnelSmoke.NS_IW_FunnelSmoke")))
		{
			FunnelSmoke = UNiagaraFunctionLibrary::SpawnSystemAttached(Smoke, HullMesh, NAME_None, ShipDef->FunnelTop, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
			if (FunnelSmoke) FunnelSmoke->SetWorldScale3D(FVector(ShipDef->SmokeScale));
		}
	}
	// P28: 1870s oilskin helmsman (SM_SailorCrewStand) + spoked mahogany/brass wheel on the quarterdeck (350 m cull distance)
	if (UStaticMesh* StandM = IronwakeWorld::Mesh(TEXT("SailorCrewStand")))
	{
		FVector DeckCm(-2500.f, 0.f, 452.f); // ironclad aft bridge superstructure deck
		if      (ShipId == TEXT("frigate")) DeckCm = FVector(-3344.f, 0.f, 736.f);
		else if (ShipId == TEXT("brig"))    DeckCm = FVector(-1596.f, 0.f, 905.f);
		else if (ShipId == TEXT("cutter"))  DeckCm = FVector(-1140.f, 0.f,  62.f);

		HelmsmanPivot = NewObject<USceneComponent>(this, TEXT("HelmsmanPivot"));
		HelmsmanPivot->SetupAttachment(HullMesh);
		HelmsmanPivot->SetRelativeLocation(DeckCm);
		HelmsmanPivot->RegisterComponent();

		UStaticMeshComponent* Hm = NewObject<UStaticMeshComponent>(this, TEXT("HelmsmanMesh"));
		Hm->SetupAttachment(HelmsmanPivot);
		Hm->SetStaticMesh(StandM);
		Hm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hm->SetGenerateOverlapEvents(false);
		Hm->SetCastShadow(true);
		Hm->SetCullDistance(35000.f);
		Hm->RegisterComponent();

		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		UMaterialInterface* Paint = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Ships/Materials/M_IW_BrigPaint.M_IW_BrigPaint"));
		if (!Paint) Paint = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		auto MakeTint = [&](const FLinearColor& C, float Rgh, float Met) -> UMaterialInstanceDynamic*
		{
			if (!Paint) return nullptr;
			UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Paint, this);
			D->SetVectorParameterValue(TEXT("Color"), C);
			D->SetScalarParameterValue(TEXT("Roughness"), Rgh);
			D->SetScalarParameterValue(TEXT("Metallic"), Met);
			return D;
		};
		UMaterialInstanceDynamic* WoodM = MakeTint(FLinearColor(0.14f, 0.07f, 0.035f), 0.58f, 0.0f);
		UMaterialInstanceDynamic* BrassM = MakeTint(FLinearColor(0.68f, 0.48f, 0.18f), 0.34f, 0.85f);

		if (Cube && Cyl)
		{
			UStaticMeshComponent* Binnacle = NewObject<UStaticMeshComponent>(this, TEXT("HelmPedestal"));
			Binnacle->SetupAttachment(HullMesh);
			Binnacle->SetStaticMesh(Cube);
			Binnacle->SetRelativeLocation(DeckCm + FVector(52.f, 0.f, 52.f));
			Binnacle->SetRelativeScale3D(FVector(0.18f, 0.24f, 1.04f));
			Binnacle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Binnacle->SetCullDistance(35000.f);
			if (WoodM) Binnacle->SetMaterial(0, WoodM);
			Binnacle->RegisterComponent();

			HelmWheelPivot = NewObject<USceneComponent>(this, TEXT("HelmWheelPivot"));
			HelmWheelPivot->SetupAttachment(HullMesh);
			HelmWheelPivot->SetRelativeLocation(DeckCm + FVector(41.f, 0.f, 102.f));
			HelmWheelPivot->RegisterComponent();

			for (int32 sp = 0; sp < 4; ++sp)
			{
				UStaticMeshComponent* Spoke = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("HelmSpoke_%d"), sp));
				Spoke->SetupAttachment(HelmWheelPivot);
				Spoke->SetStaticMesh(Cyl);
				Spoke->SetRelativeRotation(FRotator(0.f, 0.f, 45.f * sp));
				Spoke->SetRelativeScale3D(FVector(0.038f, 0.038f, 0.96f));
				Spoke->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Spoke->SetCullDistance(35000.f);
				if (BrassM) Spoke->SetMaterial(0, BrassM);
				Spoke->RegisterComponent();
			}
		}
	}
}

void AIronwakeShipPawn::TickSails(float Dt)
{
	if (!SailRig) return;
	UWorld* W = GetWorld();
	const float From = IronwakeFleet::WindFromYaw(W);
	const float WindKn = IronwakeFleet::WindKn(W);
	TrueWindAngle = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, From); // + = wind from starboard
	const float Polar = IronwakeFleet::Polar(TrueWindAngle);
	// Sail plan by telegraph: 3 topsails, 4 + courses/spanker, 5 + topgallants/jibs, 6 + staysails (all sail).
	int32 Set = TelegraphIndex;
	// W2: auto-reef. In a GALE (> 32 kn) she carries no more than courses + topsails, in a STORM (> 42 kn) close-reefed
	// topsails only, unless the player overrides by ordering more sail again within 3 s of the reef (bReefOverride).
	{
		// W3: decide on a slow wind (gusts below 42 kn must not shake the topsails back out): fast to reef, slow to shake out.
		ReefWindKn = WindKn > ReefWindKn ? FMath::FInterpTo(ReefWindKn, WindKn, Dt, 2.f) : FMath::FInterpTo(ReefWindKn, WindKn, Dt, 0.05f);
		const int32 Cap = ReefWindKn > 40.f ? 3 : ReefWindKn > 30.f ? 4 : 6;
		if (!bReefOverride && Set > Cap && Set >= 3)
		{
			if (ReefedTo != Cap) { ReefedTo = Cap; ReefAt = W->GetTimeSeconds(); UE_LOG(LogIronwake, Display, TEXT("IWW2 auto-reef to sail plan %d at %.0f kn wind"), Cap, WindKn); }
			Set = Cap;
		}
		if (Cap >= 6) { ReefedTo = -1; bReefOverride = false; }
	}
	static const float PlanFrac[] = { 0.f, 0.f, 0.f, 0.30f, 0.62f, 0.90f, 1.f };
	const float Plan = PlanFrac[FMath::Clamp(Set, 0, 6)];
	SailPlanFrac = Plan;
	// Hull speed ~ 0.72 x true wind on a broad reach with all sail (16 kn wind -> ~11.5 kn), less with a smaller plan.
	// Diminishing returns in strong wind (hull speed ~14 kn for 67 m waterline at game scale).
	const float Raw = 0.72f * WindKn * Polar * FMath::Sqrt(Plan);
	SailKn = 14.f * (1.f - FMath::Exp(-Raw / 14.f)) * 1.25f;
	SailKn = FMath::Min(SailKn, Raw);
	// Apparent wind for the HUD (boat speed adds a headwind component).
	{
		const float Bs = FMath::Max(GetSpeedKnots(), 0.f);
		const float Tr = FMath::DegreesToRadians(TrueWindAngle);
		const FVector2D A(WindKn * FMath::Cos(Tr) + Bs, WindKn * FMath::Sin(Tr));
		ApparentKn = A.Size();
		ApparentAngle = FMath::RadiansToDegrees(FMath::Atan2(A.Y, A.X));
	}
	SailFill = Plan > 0.f ? FMath::Clamp(Polar * FMath::Clamp(WindKn / 16.f, 0.3f, 1.2f), 0.f, 1.f) : 0.f;
	SailRig->TickRig(Dt, TrueWindAngle, Set, SailFill, WindKn, ApparentAngle);
}

void AIronwakeShipPawn::ToggleSteam()
{
	if (!SailRig) return;
	bAuxSteam = !bAuxSteam;
	if (!FunnelSmoke && bAuxSteam)
	{
		// Auxiliary engine vents through a short stack between fore and main masts.
		const FVector StackLoc = (ShipId == TEXT("brig")) ? FVector(-260.f, 0.f, 1280.f) : FVector(-2300.f, 0.f, 2300.f);
		if (UNiagaraSystem* Smoke = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_FunnelSmoke.NS_IW_FunnelSmoke")))
			FunnelSmoke = UNiagaraFunctionLibrary::SpawnSystemAttached(Smoke, HullMesh, NAME_None, StackLoc, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		if (FunnelSmoke) FunnelSmoke->SetWorldScale3D(FVector(ShipId == TEXT("brig") ? 0.55f : 0.9f));
	}
	if (FunnelSmoke) FunnelSmoke->SetActive(bAuxSteam || !SailRig, true);
	UE_LOG(LogIronwake, Display, TEXT("IWS2 auxiliary steam %s"), bAuxSteam ? TEXT("ON") : TEXT("OFF"));
}

void AIronwakeShipPawn::IWSteam(int32 On) { if ((On != 0) != bAuxSteam) ToggleSteam(); }
void AIronwakeShipPawn::OnSteamAction(const FInputActionValue& V) { ToggleSteam(); }

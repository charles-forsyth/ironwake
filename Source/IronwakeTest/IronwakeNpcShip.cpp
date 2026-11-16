// P20: NPC ship embodiment (IronwakeNpc.h). Same dynamics model as the player's ship (engine telegraph with accel limit, rudder with slew,
// yaw rate = V / turn radius x rudder, heave/pitch/roll springs on the sea, collision registry, grounding), driven by a steering layer
// (the brain sets DesYaw / DesKn; this class turns them into rudder + telegraph exactly like a helmsman).
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeShell.h"
#include "IronwakeShipWave.h"
#include "IronwakeWorld.h"
#include "IronwakeTest.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	constexpr float KnCmS = 51.4444f;
	constexpr float Comp = 1.5f;   // speed compression, like the player's ships

	UMaterialInstanceDynamic* TintMat(UObject* Outer, const FLinearColor& C)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, Outer);
		M->SetVectorParameterValue(TEXT("Color"), C);
		return M;
	}
	UStaticMesh* BasicMesh(const TCHAR* N) { return LoadObject<UStaticMesh>(nullptr, N); }
}

FIronwakeHullProfile AIronwakeNpcShip::ProfileFor(const FNpcDef& D, float& Xc)
{
	FIronwakeHullProfile P; Xc = 0.f;
	if (D.Id == TEXT("ironclad")) P = FIronwakeHullProfile::Kracken();
	else if (D.Id == TEXT("brig"))
	{
		P = FIronwakeHullProfile::TallBrig();
		Xc = 0.5f * (P.X0 + P.X1) * 100.f; P.X0 -= Xc / 100.f; P.X1 -= Xc / 100.f;
		for (float& B : P.HalfB) B *= D.MeshScale;
		P.X0 *= D.MeshScale; P.X1 *= D.MeshScale;
		return P;
	}
	else return GenericProfile(D.LengthM, D.BeamM);
	return P;
}

FIronwakeHullProfile AIronwakeNpcShip::GenericProfile(float L, float B)
{
	FIronwakeHullProfile P; P.X0 = -0.5f * L; P.X1 = 0.5f * L;
	const int32 N = 25;
	for (int32 i = 0; i < N; ++i)
	{
		const float x = -1.f + 2.f * (float)i / (N - 1);
		const float h = x > 0.f ? FMath::Pow(FMath::Max(1.f - FMath::Pow(x, 2.2f), 0.f), 0.65f) : FMath::Pow(FMath::Max(1.f - FMath::Pow(-x, 3.5f), 0.f), 0.55f);
		P.HalfB.Add(FMath::Max(0.04f * B, 0.5f * B * h));
	}
	P.BowDeckM = FMath::Clamp(0.1f * L, 1.2f, 4.f); P.KeelM = FMath::Clamp(0.05f * L, 0.6f, 6.f);
	return P;
}

AIronwakeNpcShip::AIronwakeNpcShip()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	HullPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HullPivot"));
	HullPivot->SetupAttachment(Root);
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(HullPivot);
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hull->SetGenerateOverlapEvents(false);
}

FNpcRec* AIronwakeNpcShip::Rec() const { AIronwakeNpcDirector* D = Dir.Get(); return D ? D->RecById(RecId) : nullptr; }

float AIronwakeNpcShip::ThrustFrac() const
{
	const FNpcRec* R = Rec(); if (!R || !R->Def) return 0.f;
	return FMath::Clamp(EngineKn / FMath::Max(R->Def->MaxKn, 1.f), 0.f, 1.2f);
}

void AIronwakeNpcShip::Init(AIronwakeNpcDirector* D, int32 InRecId)
{
	Dir = D; RecId = InRecId;
	FNpcRec* R = Rec(); if (!R || !R->Def) return;
	const FNpcDef& Df = *R->Def;
	Pos2 = FVector2D(R->Pos.X, R->Pos.Y); Yaw = R->Yaw;
	V = R->SpeedKn * KnCmS * Comp; EngineKn = R->SpeedKn;
	BuildVisuals();
	if (Df.bAir)
	{
		SetActorLocationAndRotation(FVector(Pos2.X, Pos2.Y, Df.AltCm), FRotator(0.f, Yaw, 0.f));
		return;
	}
	DraughtCm = FMath::Max(Df.DraftM * 100.f, 60.f);
	TArray<FVector2D> Poly;
	UIronwakeColl::OutlineFromProfile(Prof, Poly, FMath::Clamp(0.012f * Df.LengthM * 100.f, 40.f, 120.f));
	Shape = UIronwakeColl::MakeShape(Poly, DraughtCm);
	if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld())) { Co->RegisterHull(this, Shape, false); bShapeReg = true; }
	const float Z0 = AIronwakeShell::WaterZAtWaves(GetWorld(), FVector(Pos2.X, Pos2.Y, 0.f));
	HeaveZ = Z0;
	SetActorLocationAndRotation(FVector(Pos2.X, Pos2.Y, Z0), FRotator(0.f, Yaw, 0.f));
	bInit = false;
}

void AIronwakeNpcShip::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld())) Co->UnregisterHull(this);
	IronwakeSea::UnregisterShipWave(this);
	Super::EndPlay(Reason);
}

void AIronwakeNpcShip::BuildVisuals()
{
	FNpcRec* R = Rec(); if (!R || !R->Def) return;
	const FNpcDef& D = *R->Def;
	UStaticMesh* M = D.WorldMesh ? IronwakeWorld::Mesh(D.WorldMesh) : LoadObject<UStaticMesh>(nullptr, D.LibMesh);
	if (M) Hull->SetStaticMesh(M); else UE_LOG(LogIronwake, Warning, TEXT("IWNPC missing mesh for %s"), *D.Id.ToString());
	Hull->SetCastShadow(true);
	float Xc = 0.f;
	Prof = ProfileFor(D, Xc);
	const float ZOff = (D.WorldMesh && FMath::Abs(D.BootTopCm) > 0.5f) ? D.BootTopCm : (D.WorldMesh ? -D.DraftM * 100.f : D.BootTopCm);
	HullPivot->SetRelativeLocation(FVector(-Xc * D.MeshScale, 0.f, D.bAir ? 0.f : ZOff));
	HullPivot->SetRelativeRotation(FRotator(0.f, D.HullYaw, 0.f));
	HullPivot->SetRelativeScale3D(FVector(D.MeshScale));
	UStaticMesh* Cube = BasicMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Plane = BasicMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cyl = BasicMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto NewSM = [&](USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Sc, UMaterialInterface* Mat) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh); C->SetupAttachment(Parent);
		C->SetRelativeLocation(Loc); C->SetRelativeRotation(Rot); C->SetRelativeScale3D(Sc);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetGenerateOverlapEvents(false); C->SetCastShadow(false);
		if (Mat) C->SetMaterial(0, Mat);
		C->RegisterComponent(); ExtraMeshes.Add(C);
		return C;
	};
	auto NewPivot = [&](USceneComponent* Parent, const FVector& Loc, const FRotator& Rot) -> USceneComponent*
	{
		USceneComponent* P = NewObject<USceneComponent>(this);
		P->SetupAttachment(Parent); P->SetRelativeLocation(Loc); P->SetRelativeRotation(Rot); P->RegisterComponent();
		return P;
	};
	UMaterialInstanceDynamic* Brass = TintMat(this, FLinearColor(0.34f, 0.22f, 0.07f));
	UMaterialInstanceDynamic* Dark = TintMat(this, FLinearColor(0.035f, 0.03f, 0.028f));
	UMaterialInstanceDynamic* Red = TintMat(this, FLinearColor(0.20f, 0.03f, 0.02f));

	// ---- moving parts -------------------------------------------------------------------------------------------------------------
	// propeller helper: spin pivot about its local X (roll), blades radial; Orient aligns local X to the wanted axis.
	UMaterialInstanceDynamic* DiscMat = nullptr;
	{ UMaterialInterface* Sm = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Town/M_IW_Smk.M_IW_Smk")); if (Sm) { DiscMat = UMaterialInstanceDynamic::Create(Sm, this); DiscMat->SetScalarParameterValue(TEXT("Op"), 0.055f); DiscMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.48f, 0.50f)); } }
	// mesh prop: blades component + faint disc (normal along the spin axis) under the spinning pivot
	auto AddMeshProp = [&](USceneComponent* Or, UStaticMeshComponent* Blades, int32 Axis, float Rev, float Ph, const TCHAR* Tag)
	{
		FNpcPart Pt; Pt.Pivot = Or; Pt.Kind = 0; Pt.RevPerSec = Rev; Pt.Axis = Axis; Pt.Phase = Ph; Pt.Idx = Parts.Num(); Pt.Blades = Blades;
		const FBoxSphereBounds Bb = Blades->GetStaticMesh()->GetBounds();
		const float Rad = Axis == 0 ? FMath::Max(Bb.BoxExtent.Y, Bb.BoxExtent.Z) : FMath::Max(Bb.BoxExtent.X, Bb.BoxExtent.Z);
		UStaticMeshComponent* Dc = NewSM(Or, Cyl, FVector::ZeroVector, Axis == 0 ? FRotator(90.f, 0.f, 0.f) : FRotator(0.f, 0.f, 90.f), FVector(Rad * 2.f / 100.f, Rad * 2.f / 100.f, 0.02f), DiscMat);
		Dc->SetVisibility(false); Pt.Disc = Dc;
		UE_LOG(LogIronwake, Display, TEXT("IWNPC prop_init %s %s axis=%d radius=%.0f cm rev/s=%.2f"), *D.Id.ToString(), Tag, Axis, Rad, Rev);
		Parts.Add(Pt);
	};
	auto AddProp = [&](USceneComponent* Parent, const FVector& Loc, const FRotator& Orient, float RadiusCm, int32 Blades, float Rev, float Ph)
	{
		USceneComponent* Or = NewPivot(Parent, Loc, Orient);
		USceneComponent* Sp = NewPivot(Or, FVector::ZeroVector, FRotator::ZeroRotator);
		USceneComponent* Bl = NewPivot(Sp, FVector::ZeroVector, FRotator::ZeroRotator);
		for (int32 b = 0; b < Blades; ++b)
		{
			USceneComponent* Bp = NewPivot(Bl, FVector::ZeroVector, FRotator(0.f, 0.f, 360.f * b / Blades));
			NewSM(Bp, Cube, FVector(0.f, 0.f, RadiusCm * 0.5f), FRotator(0.f, 0.f, 18.f), FVector(0.05f, FMath::Max(0.10f, RadiusCm * 0.0016f), RadiusCm / 100.f), Brass);
		}
		NewSM(Sp, Cyl, FVector::ZeroVector, FRotator(0.f, 0.f, 90.f), FVector(RadiusCm * 0.0022f, RadiusCm * 0.0022f, 0.25f), Dark);   // hub
		FNpcPart P; P.Pivot = Sp; P.Kind = 0; P.RevPerSec = Rev; P.Phase = Ph; P.Idx = Parts.Num(); P.Blades = Bl;
		if (DiscMat) { UStaticMeshComponent* Dc = NewSM(Sp, Cyl, FVector::ZeroVector, FRotator(90.f, 0.f, 0.f), FVector(RadiusCm * 2.f / 100.f, RadiusCm * 2.f / 100.f, 0.02f), DiscMat); Dc->SetVisibility(false); P.Disc = Dc; }
		UE_LOG(LogIronwake, Display, TEXT("IWNPC prop_init %s procedural radius=%.0f cm blades=%d rev/s=%.2f"), *D.Id.ToString(), RadiusCm, Blades, Rev);
		Parts.Add(P);
	};
	if (D.Id == TEXT("patrol_airship"))
	{
		// P28: clean bladeless-pod PatrolAirshipV4 + 3 Ursa-prepped 4-blade brass/mahogany propellers (X_ue=+100*X_b, Y_ue=-100*Y_b, Z_ue=+100*Z_b).
		// Cruise rev/s ~2.5 (> 1.7 rev/s activates translucent swept motion-blur disc while blades spin ~135 deg per 0.15 s).
		struct FPr4 { const TCHAR* Mesh; FVector PivotCm; float Rev; float Ph; };
		static const FPr4 Pr4[] = {
			{ TEXT("PatrolAirshipV4_prop_port"),  FVector( 1305.3f, -1557.97f,  992.14f),  2.48f, 0.0f },
			{ TEXT("PatrolAirshipV4_prop_stbd"),  FVector( 1303.6f,  1575.93f,  992.27f), -2.54f, 0.7f },
			{ TEXT("PatrolAirshipV4_prop_stern"), FVector(-3595.0f,     0.00f, 3468.52f),  2.62f, 1.4f },
		};
		for (const FPr4& P : Pr4)
		{
			UStaticMesh* Pm = IronwakeWorld::Mesh(P.Mesh); if (!Pm) continue;
			USceneComponent* Or = NewPivot(HullPivot, P.PivotCm, FRotator::ZeroRotator);
			UStaticMeshComponent* Sc = NewObject<UStaticMeshComponent>(this);
			Sc->SetStaticMesh(Pm); Sc->SetupAttachment(Or); Sc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sc->SetGenerateOverlapEvents(false); Sc->RegisterComponent();
			ExtraMeshes.Add(Sc);
			AddMeshProp(Or, Sc, 0, P.Rev, P.Ph, P.Mesh);
		}
	}
	else if (D.Id == TEXT("cargo_airship"))
	{
		// P21: four real propeller meshes (axle along the hull X axis), pivots measured in the hull-mesh frame (cm)
		struct FPr { const TCHAR* Mesh; FVector PivotCm; float Rev; };
		static const FPr Pr[] = {
			{ TEXT("CargoAirshipBody_prop_fwd_port"), FVector(-1274.f, -4132.f, 4209.f), 1.3f }, { TEXT("CargoAirshipBody_prop_fwd_stbd"), FVector(-1274.f, 4198.f, 4209.f), -1.3f },
			{ TEXT("CargoAirshipBody_prop_aft_port"), FVector(409.f, -4132.f, 4209.f), 1.3f }, { TEXT("CargoAirshipBody_prop_aft_stbd"), FVector(409.f, 4228.f, 4209.f), -1.3f } };
		for (const FPr& P : Pr)
		{
			UStaticMesh* Pm = IronwakeWorld::Mesh(P.Mesh); if (!Pm) continue;
			USceneComponent* Or = NewPivot(HullPivot, P.PivotCm, FRotator::ZeroRotator);
			UStaticMeshComponent* Sc = NewObject<UStaticMeshComponent>(this);
			Sc->SetStaticMesh(Pm); Sc->SetupAttachment(Or); Sc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sc->SetGenerateOverlapEvents(false); Sc->RegisterComponent();
			ExtraMeshes.Add(Sc);
			AddMeshProp(Or, Sc, 0, P.Rev * (1.f + 0.17f * Parts.Num()), 0.9f * Parts.Num(), P.Mesh);
		}
	}
	else if (D.Id == TEXT("sky_yacht"))
	{
		AddProp(HullPivot, FVector(-2330.f, 0.f, 800.f), FRotator::ZeroRotator, 380.f, 4, 3.f, 0.f);
	}
	if (D.bPaddle)
	{
		// P28: real PaddleSteamerV3 sponson wheels (scaled 0.86x, tucked inside exterior sponson guard rail with 0 clipping) + wheel-box water churn
		struct FPw { const TCHAR* Mesh; FVector PivotCm; };
		static const FPw Wheels[2] = {
			{ TEXT("PaddleSteamerV3_wheel_port"), FVector(-132.12f, -634.0f, 452.0f) },
			{ TEXT("PaddleSteamerV3_wheel_stbd"), FVector(-132.12f,  622.0f, 452.0f) },
		};
		for (int32 w = 0; w < 2; ++w)
		{
			UStaticMesh* Wm = IronwakeWorld::Mesh(Wheels[w].Mesh);
			USceneComponent* Or = NewPivot(HullPivot, Wheels[w].PivotCm, FRotator::ZeroRotator);
			USceneComponent* Sp = NewPivot(Or, FVector::ZeroVector, FRotator::ZeroRotator);
			if (Wm)
			{
				UStaticMeshComponent* Wc = NewObject<UStaticMeshComponent>(this);
				Wc->SetStaticMesh(Wm); Wc->SetupAttachment(Sp);
				Wc->SetRelativeScale3D(FVector(0.86f, 0.86f, 0.86f));
				Wc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Wc->SetGenerateOverlapEvents(false); Wc->SetCastShadow(true);
				Wc->RegisterComponent(); ExtraMeshes.Add(Wc);
			}
			FNpcPart P; P.Pivot = Sp; P.Kind = 1; P.Axis = 1; P.RevPerSec = 0.35f; P.Sign = 1.f; P.Idx = w; Parts.Add(P);
		}
		if (UMaterialInterface* FoamBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/FX/M_IW_BowFoam.M_IW_BowFoam")))
		{
			PaddleChurnMID = UMaterialInstanceDynamic::Create(FoamBase, this);
			PaddleChurnMID->SetScalarParameterValue(TEXT("Opacity"), 0.f);
			PaddleChurnMID->SetScalarParameterValue(TEXT("Foam Boost"), 2.8f);
			const float Ys[2] = { -575.f, 565.f };
			for (int32 s = 0; s < 2; ++s)
			{
				UStaticMeshComponent* Pad = NewSM(Root, Plane, FVector(-175.f, Ys[s], 8.f), FRotator(0.f, s == 0 ? -4.f : 4.f, 0.f), FVector(5.4f, 1.85f, 1.f), PaddleChurnMID);
				Pad->SetTranslucentSortPriority(11);
				Pad->SetVisibility(false);
				PaddleChurnPads.Add(Pad);
			}
		}
	}
	if (D.Id == TEXT("tender"))
	{
		// P28: SalvageTenderV3 articulated slewing A-frame crane (Kind 6, axis Z) + child pendulum diving bell (Kind 7) clearing stern rail by +77 cm
		if (UStaticMesh* Cm = IronwakeWorld::Mesh(TEXT("SalvageTenderV3_crane")))
		{
			USceneComponent* CranePiv = NewPivot(HullPivot, FVector(-465.0f, -5.22f, 956.54f), FRotator::ZeroRotator);
			UStaticMeshComponent* Cc = NewObject<UStaticMeshComponent>(this);
			Cc->SetStaticMesh(Cm); Cc->SetupAttachment(CranePiv);
			Cc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Cc->SetGenerateOverlapEvents(false); Cc->SetCastShadow(true);
			Cc->RegisterComponent(); ExtraMeshes.Add(Cc);
			FNpcPart Pc; Pc.Pivot = CranePiv; Pc.Kind = 6; Pc.Phase = RecId * 0.7f; Pc.Blades = Cc; Parts.Add(Pc);

			if (UStaticMesh* Bm = IronwakeWorld::Mesh(TEXT("SalvageTenderV3_bell")))
			{
				USceneComponent* BellPiv = NewPivot(CranePiv, FVector(-1676.54f, 0.0f, 824.94f), FRotator::ZeroRotator);
				UStaticMeshComponent* Bc = NewObject<UStaticMeshComponent>(this);
				Bc->SetStaticMesh(Bm); Bc->SetupAttachment(BellPiv);
				Bc->SetRelativeScale3D(FVector(0.76f));
				Bc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Bc->SetGenerateOverlapEvents(false); Bc->SetCastShadow(true);
				Bc->RegisterComponent(); ExtraMeshes.Add(Bc);
				FNpcPart Pb; Pb.Pivot = BellPiv; Pb.Kind = 7; Pb.Phase = RecId * 0.7f; Pb.Blades = Bc; Parts.Add(Pb);
			}
		}
	}
	if (D.bOars)
	{
		// P28: six oars mounted on brass U-crutches in the flat gunwale saddles 60 cm aft of each sculpted thwart (+104, -16, -126 cm; Z=198 > gunwale 186.5 cm)
		// + three real 1870s oilskin SailorCrewRower figures seated flush on the wooden thwarts + stern coxswain.
		static const struct { const TCHAR* Mesh; FVector PivotCm; float Side; float Ph; } Oa[] = {
			{ TEXT("GigBody_oar_0_port"), FVector(  42.f,  154.f, 198.f),  1.f, 0.00f }, { TEXT("GigBody_oar_0_stbd"), FVector(  42.f, -154.f, 198.f), -1.f, 0.00f },
			{ TEXT("GigBody_oar_1_port"), FVector( -76.f,  152.f, 198.f),  1.f, 0.03f }, { TEXT("GigBody_oar_1_stbd"), FVector( -76.f, -152.f, 198.f), -1.f, 0.03f },
			{ TEXT("GigBody_oar_2_port"), FVector(-184.f,  144.f, 198.f),  1.f, 0.06f }, { TEXT("GigBody_oar_2_stbd"), FVector(-184.f, -144.f, 198.f), -1.f, 0.06f } };
		for (const auto& O : Oa)
		{
			UStaticMesh* Om = IronwakeWorld::Mesh(O.Mesh); if (!Om) continue;
			NewSM(HullPivot, Cyl, O.PivotCm + FVector(0.f, 0.f, -6.5f), FRotator::ZeroRotator, FVector(0.055f, 0.055f, 0.14f), Brass);
			NewSM(HullPivot, Cube, O.PivotCm + FVector(5.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.03f, 0.06f, 0.09f), Brass);
			NewSM(HullPivot, Cube, O.PivotCm + FVector(-5.f, 0.f, 0.f), FRotator::ZeroRotator, FVector(0.03f, 0.06f, 0.09f), Brass);
			USceneComponent* Pv = NewPivot(HullPivot, O.PivotCm, FRotator::ZeroRotator);
			UStaticMeshComponent* Sc = NewObject<UStaticMeshComponent>(this);
			Sc->SetStaticMesh(Om); Sc->SetupAttachment(Pv); Sc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sc->SetGenerateOverlapEvents(false); Sc->SetCastShadow(true); Sc->RegisterComponent();
			ExtraMeshes.Add(Sc);
			FNpcPart P; P.Pivot = Pv; P.Kind = 4; P.Sign = O.Side; P.Phase = O.Ph; P.Blades = Sc; Parts.Add(P);
		}
		UStaticMesh* RowerMesh = IronwakeWorld::Mesh(TEXT("SailorCrewRower"));
		const float ThwartXs[3] = { 104.f, -16.f, -126.f };
		for (int32 i = 0; i < 3; ++i)
		{
			USceneComponent* Hip = NewPivot(HullPivot, FVector(ThwartXs[i], (i == 1 ? 5.f : -5.f), 152.f), FRotator::ZeroRotator);
			if (RowerMesh)
			{
				UStaticMeshComponent* Rc = NewSM(Hip, RowerMesh, FVector(0.f, 0.f, -45.f), FRotator(0.f, 180.f, 0.f), FVector(1.08f), nullptr);
				Rc->SetCastShadow(true); Rc->SetCullDistance(35000.f);
			}
			FNpcPart Rp; Rp.Pivot = Hip; Rp.Kind = 5; Rp.Phase = Oa[i * 2].Ph; Parts.Add(Rp);
		}
		if (RowerMesh)
		{
			// Coxswain seated on the stern sheets facing forward (+X)
			UStaticMeshComponent* Cox = NewSM(HullPivot, RowerMesh, FVector(-252.f, 0.f, 108.f), FRotator::ZeroRotator, FVector(1.06f), nullptr);
			Cox->SetCastShadow(true); Cox->SetCullDistance(35000.f);
		}
	}
	// P28: quarterdeck helmsman / deck officer (SM_SailorCrewStand, tight 350 m cull distance) + spoked steering wheel on surface vessels
	if (!D.bAir && !D.bOars)
	{
		if (UStaticMesh* StandM = IronwakeWorld::Mesh(TEXT("SailorCrewStand")))
		{
			FVector DeckCm = FVector::ZeroVector; float CrewYaw = 0.f; bool bAddHelm = true;
			if      (D.Id == TEXT("brig"))    { DeckCm = FVector(-1596.f, 0.f,  905.f); }
			else if (D.Id == TEXT("paddle"))  { DeckCm = FVector(-1700.f, 0.f,  833.f); }
			else if (D.Id == TEXT("tender"))  { DeckCm = FVector(-1180.f, -150.f, 890.f); CrewYaw = 165.f; } // salvage deck riggers facing aft crane
			else if (D.Id == TEXT("goliath")) { DeckCm = FVector(-1870.f, 0.f, 1027.f); }
			else if (D.Id == TEXT("tug"))     { DeckCm = FVector( -660.f, 0.f,  518.f); }
			else bAddHelm = false;
			if (bAddHelm)
			{
				UStaticMeshComponent* Hm = NewSM(HullPivot, StandM, DeckCm, FRotator(0.f, CrewYaw, 0.f), FVector(1.0f), nullptr);
				Hm->SetCastShadow(true); Hm->SetCullDistance(35000.f);
				if (D.Id == TEXT("tender"))
				{
					UStaticMeshComponent* Hm2 = NewSM(HullPivot, StandM, FVector(-1410.f, 175.f, 888.f), FRotator(0.f, 165.f, 0.f), FVector(0.98f), nullptr);
					Hm2->SetCastShadow(true); Hm2->SetCullDistance(35000.f);
				}
				if (CrewYaw == 0.f)
				{
					NewSM(HullPivot, Cube, DeckCm + FVector(52.f, 0.f, 52.f), FRotator::ZeroRotator, FVector(0.18f, 0.22f, 1.04f), Dark)->SetCullDistance(35000.f);
					HelmWheelPivot = NewPivot(HullPivot, DeckCm + FVector(42.f, 0.f, 102.f), FRotator::ZeroRotator);
					for (int32 sp = 0; sp < 4; ++sp)
						NewSM(HelmWheelPivot, Cyl, FVector::ZeroVector, FRotator(0.f, 0.f, 45.f * sp), FVector(0.04f, 0.04f, 0.92f), Brass)->SetCullDistance(35000.f);
				}
			}
		}
	}
	// flag / ensign on raked wooden jackstaff + live occupied-berth-only mooring hawsers
	if (!D.bAir && !D.bRig)
	{
		NewSM(Root, Cyl, D.FlagCm + FVector(18.f, 0.f, -85.f), FRotator(-14.f, 0.f, 0.f), FVector(0.045f, 0.045f, 1.85f), Dark)->SetCullDistance(45000.f);
		USceneComponent* Fp = NewPivot(Root, D.FlagCm, FRotator::ZeroRotator);
		UMaterialInstanceDynamic* Fm = TintMat(this, R->FlagColor);
		const float Fl = FMath::Clamp(D.LengthM * 0.022f, 0.55f, 1.25f);
		for (int32 k = 0; k < 2; ++k)
		{
			UStaticMeshComponent* Fc = NewSM(Fp, Plane, FVector(Fl * 50.f, 0.f, 0.f), FRotator(0.f, 0.f, k ? -90.f : 90.f), FVector(Fl, Fl * 0.58f, 1.f), Fm);
			Fc->SetCullDistance(45000.f);
		}
		FNpcPart P; P.Pivot = Fp; P.Kind = 3; P.Phase = (RecId * 1.7f); P.RevPerSec = 1.4f; Parts.Add(P);
	}
	if (!D.bAir)
	{
		UMaterialInstanceDynamic* RopeMat = TintMat(this, FLinearColor(0.14f, 0.11f, 0.068f));
		for (int32 ml = 0; ml < 2; ++ml)
		{
			UStaticMeshComponent* Rope = NewSM(Root, Cyl, FVector::ZeroVector, FRotator::ZeroRotator, FVector(0.06f, 0.06f, 1.f), RopeMat);
			Rope->SetUsingAbsoluteLocation(true); Rope->SetUsingAbsoluteRotation(true); Rope->SetUsingAbsoluteScale(true);
			Rope->SetVisibility(false); Rope->SetCullDistance(45000.f);
			MooringLines.Add(Rope);
		}
	}
	MovingParts = Parts.Num();
	// sail rig (brig only)
	if (D.bRig)
	{
		Rig = NewObject<UIronwakeSailRig>(this, TEXT("SailRig"));
		Rig->SetupAttachment(Hull);
		Rig->bTallBrig = (D.Id == TEXT("brig"));
		Rig->ClothMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Ships/Materials/M_IW_Cloth.M_IW_Cloth"));
		Rig->RegisterComponent();
		Rig->Build();
		MovingParts += Rig->BrigParts.Num();
	}
	// funnel smoke
	if (!D.SmokeCm.IsNearlyZero())
	{
		if (UNiagaraSystem* S = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_FunnelSmoke.NS_IW_FunnelSmoke")))
		{
			Smoke = UNiagaraFunctionLibrary::SpawnSystemAttached(S, Root, NAME_None, D.SmokeCm, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
			if (Smoke) Smoke->SetWorldScale3D(FVector(D.SmokeScale));
		}
	}
}

void AIronwakeNpcShip::SetLamp(bool bOn)
{
	FNpcRec* R = Rec(); if (!R || !R->Def) return;
	if (bOn && !Lamp)
	{
		Lamp = NewObject<UPointLightComponent>(this, TEXT("MastLamp"));
		Lamp->SetupAttachment(Root); Lamp->SetRelativeLocation(FVector(0.f, 0.f, R->Def->LampZCm));
		Lamp->SetIntensityUnits(ELightUnits::Candelas); Lamp->SetIntensity(0.f); Lamp->SetAttenuationRadius(6000.f);
		Lamp->SetLightColor(FLinearColor(1.f, 0.72f, 0.4f)); Lamp->SetCastShadows(false); Lamp->RegisterComponent();
	}
	if (Lamp) { Lamp->SetIntensity(bOn ? 5000.f : 0.f); Lamp->SetVisibility(bOn); }
	bLampOn = bOn;
}

void AIronwakeNpcShip::SetFx(bool bOn)
{
	FNpcRec* R = Rec(); if (!R || !R->Def || R->Def->bAir) return;
	if (bOn && !Wake3)
	{
		float MaxB = 0.f; for (float B : Prof.HalfB) MaxB = FMath::Max(MaxB, B);
		BowWave = NewObject<UIronwakeBowWave>(this, TEXT("BowWave")); BowWave->SetupAttachment(Root); BowWave->RegisterComponent();
		BowWave->Init(Prof, FMath::Clamp(R->Def->LengthM / 125.f, 0.08f, 1.f));
		Wake3 = NewObject<UIronwakeWake>(this, TEXT("Wake3")); Wake3->SetupAttachment(Root); Wake3->RegisterComponent();
		Wake3->Init(R->Def->LengthM, MaxB, R->Def->bSteam || R->Def->bPaddle);
	}
	bFx = bOn;
	if (BowWave) BowWave->SetVisibility(bOn);
	if (Wake3) Wake3->SetVisibility(bOn);
}

// ---------------------------------------------------------------------------- dynamics
void AIronwakeNpcShip::StepAir(float Dt)
{
	FNpcRec* R = Rec(); if (!R) return;
	const FNpcDef& D = *R->Def;
	UWorld* W = GetWorld();
	EngineKn = FMath::FInterpConstantTo(EngineKn, R->DesKn, Dt, 1.5f);
	const float Target = EngineKn * KnCmS * Comp;
	V += (Target - V) * FMath::Clamp(Dt / 6.f, 0.f, 1.f);
	const float Err = FRotator::NormalizeAxis(R->DesYaw - Yaw);
	const float Want = FMath::Clamp(Err * 0.6f, -2.2f, 2.2f);   // deg/s, gentle
	YawRate += (Want - YawRate) * (1.f - FMath::Exp(-1.2f * Dt));
	Yaw = FRotator::NormalizeAxis(Yaw + YawRate * Dt);
	const FVector Fw = FRotator(0.f, Yaw, 0.f).Vector();
	Pos2 += FVector2D(Fw.X, Fw.Y) * V * Dt;
	const float T = (float)W->GetTimeSeconds();
	const float Ph = RecId * 1.3f;
	const float Z = D.AltCm + 900.f * FMath::Sin(T * 0.11f + Ph);
	const float Pit = 1.4f * FMath::Sin(T * 0.23f + Ph) + (Z > GetActorLocation().Z ? 0.6f : -0.6f) * 0.f;
	const float Rol = FMath::Clamp(-YawRate * 2.2f, -6.f, 6.f) + 1.2f * FMath::Sin(T * 0.19f + Ph * 2.f);
	SetActorLocationAndRotation(FVector(Pos2.X, Pos2.Y, Z), FRotator(Pit, Yaw, Rol));
	R->Pos = FVector(Pos2.X, Pos2.Y, 0.f); R->Yaw = Yaw; R->SpeedKn = V / KnCmS / Comp;
}

void AIronwakeNpcShip::StepHold(float Dt)
{
	FNpcRec* R = Rec(); if (!R) return;
	// moored / anchored: the hull stays at its berth pose and only rides the swell
	Pos2 = FVector2D(R->Pos.X, R->Pos.Y); Yaw = R->Yaw;
	V = 0.f; YawRate = 0.f; EngineKn = FMath::FInterpConstantTo(EngineKn, 0.f, Dt, 3.f); RudderDeg = FMath::FInterpConstantTo(RudderDeg, 0.f, Dt, 20.f);
	R->SpeedKn = 0.f;
}

void AIronwakeNpcShip::StepWarp(float Dt)
{
	FNpcRec* R = Rec(); if (!R) return;
	const float Now = (float)GetWorld()->GetTimeSeconds();
	const float U = FMath::Clamp((Now - R->WarpT0) / FMath::Max(R->WarpDur, 1.f), 0.f, 1.f);
	const float S = U * U * (3.f - 2.f * U);
	const FVector P = FMath::Lerp(R->WarpFrom, R->WarpTo, S);
	Pos2 = FVector2D(P.X, P.Y);
	Yaw = FRotator::NormalizeAxis(R->WarpYawFrom + FRotator::NormalizeAxis(R->WarpYawTo - R->WarpYawFrom) * S);
	const float Speed = (R->WarpTo - R->WarpFrom).Size2D() / FMath::Max(R->WarpDur, 1.f) * 6.f * U * (1.f - U);   // cm/s along the move
	V = Speed * 0.f; EngineKn = FMath::FInterpConstantTo(EngineKn, 0.f, Dt, 3.f);
	YawRate = 0.f; RudderDeg = FMath::FInterpConstantTo(RudderDeg, 0.f, Dt, 20.f);
	R->SpeedKn = Speed / KnCmS / Comp;
	R->Pos = FVector(Pos2.X, Pos2.Y, 0.f); R->Yaw = Yaw;
}

void AIronwakeNpcShip::StepDynamics(float Dt)
{
	FNpcRec* R = Rec(); if (!R) return;
	const FNpcDef& D = *R->Def;
	UWorld* W = GetWorld();
	UIronwakeColl* Co = UIronwakeColl::Get(W);
	// ---- steering layer: heading error -> rudder; speed order -> engine telegraph
	float DesKn = R->DesKn;
	if (D.bSail)
	{
		const float From = IronwakeFleet::WindFromYaw(W), WindKn = IronwakeFleet::WindKn(W);
		TrueWindAngle = FMath::FindDeltaAngleDegrees(Yaw, From);
		const float Polar = IronwakeFleet::Polar(FMath::Abs(TrueWindAngle));
		const float Raw = 0.72f * WindKn * Polar * (D.bRig ? 1.f : 0.85f);
		SailKn = FMath::Min(14.f * (1.f - FMath::Exp(-Raw / 14.f)) * 1.25f, Raw);
		SailFill = DesKn > 0.5f ? FMath::Clamp(Polar * FMath::Clamp(WindKn / 16.f, 0.3f, 1.2f), 0.f, 1.f) : 0.f;
		if (!R->bAux)
		{
			// no engine: the wind sets the speed. Becalmed (or in irons) a working sailer kicks over its auxiliary / sweeps, so nothing sits dead for minutes.
			if (DesKn > 0.5f && R->State != ENpcState::Fish && R->State != ENpcState::Work && SailKn < 2.6f) DesKn = FMath::Min(DesKn, FMath::Max(SailKn, 2.8f));
			else DesKn = FMath::Min(DesKn, SailKn);
		}
	}
	if ((float)W->GetTimeSeconds() < BackUntil) { DesKn = -2.2f; R->bAux = true; }   // backing off a head-on / blocked meeting
	EngineKn = FMath::FInterpConstantTo(EngineKn, DesKn, Dt, FMath::Max(2.f, D.AccelMS2 * 6.f));
	const float TargetV = EngineKn * KnCmS * Comp;
	const float MaxDv = D.AccelMS2 * 100.f * Dt;
	const float Tc = FMath::Clamp(1.2f + D.LengthM / 25.f, 1.2f, 6.f);
	V += FMath::Clamp((TargetV - V) * FMath::Clamp(Dt / Tc, 0.f, 1.f), -1.3f * MaxDv, MaxDv);
	const float Err = FRotator::NormalizeAxis(R->DesYaw - Yaw);
	const float RudCmd = FMath::Clamp(Err * 1.1f, -35.f, 35.f) * (V < -15.f ? -1.f : 1.f);   // astern: the helm works the other way
	RudderDeg = FMath::FInterpConstantTo(RudderDeg, RudCmd, Dt, D.RudderSlew);
	const float SpeedMS = V / 100.f;
	float Rad = (D.bSail && D.bRig && !R->bAux) ? D.TurnRadiusM * 1.55f : D.TurnRadiusM;   // P25: square-rigged brigs turn wide (1.55x) outside the roads
	const bool bBasin = R->bAux && (R->State == ENpcState::Depart || R->State == ENpcState::Transit || R->State == ENpcState::Arrive) && Dir->InHarbour(FVector(Pos2.X, Pos2.Y, 0.f), 85.f);
	if (bBasin) Rad *= 0.36f;   // tug-assisted / prop-wash steerage in the basin (including Arrive!)
	const bool bPropWash = bBasin || ((D.bSteam || R->bAux || D.bOars) && (FMath::Abs(EngineKn) > 0.35f || FMath::Abs(R->DesKn) > 0.35f)) || (D.bSail && R->DesKn > 0.5f && FMath::Abs(Err) > 22.f);
	const float SteerV = bPropWash ? FMath::Max(FMath::Abs(SpeedMS), 2.2f) * (V < -15.f ? -1.f : 1.f) : SpeedMS;
	float DesYR = FMath::RadiansToDegrees(SteerV / FMath::Max(Rad, 1.f)) * (RudderDeg / 35.f);
	if (D.bSail && D.bRig && !R->bAux && FMath::Abs(TrueWindAngle) < 40.f && FMath::Abs(SpeedMS) < 2.5f)
		DesYR += -FMath::Sign(TrueWindAngle == 0.f ? 1.f : TrueWindAngle) * 0.8f * (1.f - FMath::Abs(SpeedMS) / 2.5f) + 0.6f * (RudderDeg / 35.f);
	YawRate += (DesYR - YawRate) * (1.f - FMath::Exp(-D.YawResponse * Dt));
	// ---- planned motion
	float NewYaw = Yaw + YawRate * Dt;
	const FVector Fw = FRotator(0.f, Yaw, 0.f).Vector();
	FVector2D Vel(Fw.X * V, Fw.Y * V);
	if (D.bSail && !R->bAux && SailFill > 0.01f)   // leeway only when sailing without auxiliary power
	{
		const float Twa = FMath::Abs(TrueWindAngle);
		const float Lee = 6.f * FMath::Clamp((110.f - Twa) / 70.f, 0.f, 1.f) * FMath::Clamp(SailFill * 1.5f, 0.f, 1.f);
		const FVector Rt(-Fw.Y, Fw.X, 0.f);
		const float Vl = -FMath::Sign(TrueWindAngle) * FMath::Tan(FMath::DegreesToRadians(Lee)) * FMath::Max(V, 0.f);
		Vel += FVector2D(Rt.X, Rt.Y) * Vl;
	}
	FVector2D NewPos = Pos2 + Vel * Dt;
	// ---- look-ahead emergency brake: a hull (NPC, legacy merchant or the player's) on the 2.0-4.2 s track ahead -> stop below 30 cm/s before contact
	if (Co && bShapeReg && V > 20.f)
	{
		const float Tla = FMath::Clamp(2.0f + V / 320.f, 2.0f, 4.2f);
		const FVector2D Ahead = Pos2 + Vel * Tla;
		const FIwHit Ha = Co->TestPose(this, Shape, Ahead, Yaw + YawRate * Tla * 0.5f, UIronwakeColl::MaskAll);
		const AIronwakeNpcShip* OtherNpc = Cast<AIronwakeNpcShip>(Ha.Other.Get());
		const FNpcRec* OtherRec = OtherNpc ? Dir->RecById(OtherNpc->RecId) : nullptr;
		const bool bOtherHeld = OtherRec && (OtherRec->bHold || OtherRec->bBasinWait || OtherRec->State == ENpcState::Docked || OtherRec->State == ENpcState::Anchored || OtherRec->State == ENpcState::Work || FMath::Abs(OtherNpc->V) < 40.f);
		const bool bOtherSail = OtherRec && OtherRec->Def->bSail && !OtherRec->bAux;
		const bool bMySail = D.bSail && !R->bAux;
		const bool bOtherYields = OtherNpc && !bOtherHeld && !(bOtherSail && !bMySail) && ((bMySail && !bOtherSail) || OtherNpc->RecId > RecId);
		const bool bImminent = Co->TestPose(this, Shape, Pos2 + Vel * 0.75f, Yaw, UIronwakeColl::MaskAll).bHit;
		if ((Ha.bHit && !bOtherYields && Ha.Other.IsValid()) || bImminent)
		{
			EngineKn = FMath::Min(EngineKn, 0.2f);
			V = FMath::Min(V * FMath::Exp(-2.5f * Dt), 22.f);
			NewPos = Pos2 + FVector2D(Fw.X, Fw.Y) * V * Dt;
			bBraking = true;
		}
		else bBraking = false;
	}
	// ---- collision: solids, other hulls, seabed. NPC never moves into an overlap; contact slows it.
	bool bBlocked = false; const float NowT0 = (float)W->GetTimeSeconds();
	if (Co && bShapeReg)
	{
		const FIwHit Hit = Co->TestPose(this, Shape, NewPos, NewYaw);
		const bool bShoal = Co->MinDepthAt(Shape, NewPos, NewYaw) < DraughtCm;
		FIwHit Now; bool bNowShoal = false; bool bEase = false;
		if (Hit.bHit || bShoal) { Now = Co->TestPose(this, Shape, Pos2, Yaw); bNowShoal = Co->MinDepthAt(Shape, Pos2, Yaw) < DraughtCm; bEase = Now.bHit && Hit.bHit && Hit.Pen <= Now.Pen + 0.5f && !(bShoal && !bNowShoal); }
		if ((Hit.bHit || bShoal) && !bEase)
		{
			bBlocked = true;
			if (NowT0 - LastBlockLogT > 20.f && FMath::Abs(V) < 30.f && R->State == ENpcState::Depart) { LastBlockLogT = NowT0; const FVector2D Hh = Dir->W2H(FVector(Pos2.X, Pos2.Y, 0.f)); Dir->Log(FString::Printf(TEXT("IWNPC BLOCKED %s at H(%.1f,%.1f) by %s shoal=%d now=%d/%d pen=%.0f"), *R->Name, Hh.X, Hh.Y, Hit.bHit ? *Hit.Tag : TEXT("-"), bShoal ? 1 : 0, Now.bHit ? 1 : 0, bNowShoal ? 1 : 0, Hit.Pen)); }
			const float NowT = (float)W->GetTimeSeconds();
			const bool bPlayerOther = Cast<APawn>(Hit.Other.Get()) != nullptr;
			if (NowT - LastImpactT > 4.f && !Now.bHit && !bNowShoal && V > 30.f)
			{
				LastImpactT = NowT;
				if (!bPlayerOther) { ++R->Collisions; ++Dir->StatCollisions; if (bShoal) ++Dir->StatGroundings; }
				{ const FVector2D Hh = Dir->W2H(FVector(Pos2.X, Pos2.Y, 0.f)); FString OtherS; if (const AIronwakeNpcShip* Os = Cast<AIronwakeNpcShip>(Hit.Other.Get())) { if (const FNpcRec* Or = Dir->RecById(Os->RecId)) OtherS = FString::Printf(TEXT("[%s %s hold=%d V=%.1f]"), *Or->Name, NpcStateName(Or->State), Or->bHold ? 1 : 0, Os->V / 77.2f); }
				Dir->Log(FString::Printf(TEXT("IWNPC IMPACT %s with %s%s%s at H(%.1f,%.1f) v=%.1f kn state=%s ph=%d yaw=%.0f(H%.0f) rud=%.0f"), *R->Name, Hit.bHit ? *Hit.Tag : TEXT("-"), *OtherS, bShoal ? TEXT(" +shoal") : TEXT(""), Hh.X, Hh.Y, V / KnCmS / Comp, NpcStateName(R->State), R->Phase, Yaw, Yaw - 45.f, RudderDeg)); }
			}
			BlockedT += Dt;
			if (const AIronwakeNpcShip* Os = Cast<AIronwakeNpcShip>(Hit.Other.Get()))
			{
				const FNpcRec* Or = Dir->RecById(Os->RecId);
				const bool bOtherHeld = Or && (Or->bHold || Or->bBasinWait || Or->State == ENpcState::Docked || Or->State == ENpcState::Anchored || Or->State == ENpcState::Work);
				if (BlockedT > 4.5f && V > -20.f && (Os->RecId < RecId || bOtherHeld) && !(R->State == ENpcState::Depart && R->Phase == 0)) { BackUntil = NowT0 + 9.f + (RecId % 5); BlockedT = 0.f; }
			}
			else if (BlockedT > 4.5f && V > -20.f && !(R->State == ENpcState::Depart && R->Phase == 0))
			{
				BackUntil = NowT0 + 8.f + (RecId % 4); BlockedT = 0.f;
			}
			// do not enter: keep the old pose, bleed speed, allow turning away
			NewPos = Pos2; V *= FMath::Exp(-3.f * Dt);
			if (Now.bHit && Now.Pen > 20.f)
			{
				// already overlapping (rammed by something): ease out along the contact normal
				NewPos = Pos2 + Now.N.GetSafeNormal() * FMath::Min(Now.Pen, 60.f) * Dt * 2.f;
			}
		}
	}
	if (!bBlocked) BlockedT = FMath::Max(0.f, BlockedT - Dt);
	Pos2 = NewPos; Yaw = FRotator::NormalizeAxis(NewYaw);
	R->Pos = FVector(Pos2.X, Pos2.Y, 0.f); R->Yaw = Yaw; R->SpeedKn = V / KnCmS / Comp;
	LastDesYR = DesYR;
}

void AIronwakeNpcShip::StepHydro(float Dt, float DesYR)
{
	FNpcRec* R = Rec(); if (!R) return;
	const FNpcDef& D = *R->Def;
	UWorld* W = GetWorld();
	const FVector P(Pos2.X, Pos2.Y, 0.f);
	const FVector Fw = FRotator(0.f, Yaw, 0.f).Vector(), Rt(-Fw.Y, Fw.X, 0.f);
	const float L = Prof.Length() * 100.f, HB = FMath::Max(Prof.HalfBreadthAt(0.f), 0.5f) * 100.f * 0.8f;
	auto Zs = [&](const FVector& Q) { return IronwakeSea::FastZ(W, Q) + IronwakeSea::ShipWaveZ(Q, this) * 0.5f; };
	const float Zc = Zs(P), Zb = Zs(P + Fw * L * 0.32f), Zst = Zs(P - Fw * L * 0.32f), Zp = Zs(P - Rt * HB), Zsb = Zs(P + Rt * HB);
	const float SeaZ = (2.f * Zc + Zb + Zst + Zp + Zsb) / 6.f;
	const float SeaPitch = FMath::RadiansToDegrees(FMath::Atan2(Zb - Zst, L * 0.64f));
	const float SeaRoll = -FMath::RadiansToDegrees(FMath::Atan2(Zsb - Zp, 2.f * HB));
	if (!bInit)
	{
		bInit = true; HeaveZ = SeaZ; HeaveVel = 0.f; PitchDeg = SeaPitch; RollDeg = SeaRoll; PitchRate = RollRate = 0.f;
	}
	const float Lm = D.LengthM;
	const float Th = FMath::Clamp(2.4f + 0.036f * Lm, 3.f, 7.f);
	const float Follow = D.HullSize >= 2 ? 0.8f : 1.f;
	auto Spring = [Dt](float& X, float& Vv, float Target, float Period, float Zeta)
	{
		const float Wn = 2.f * PI / FMath::Max(Period, 0.5f);
		const int32 N = FMath::Clamp(FMath::CeilToInt(Dt / 0.02f), 1, 8); const float H = Dt / N;
		for (int32 k = 0; k < N; ++k) { const float A = Wn * Wn * (Target - X) - 2.f * Zeta * Wn * Vv; Vv += A * H; X += Vv * H; }
	};
	Spring(HeaveZ, HeaveVel, SeaZ, Th, 0.55f);
	float Heel = DesYR * 1.1f;
	if (D.bSail && D.bRig)
	{
		const float Side = FMath::Sin(FMath::DegreesToRadians(FMath::Clamp(FMath::Abs(TrueWindAngle), 0.f, 180.f)));
		SailHeel = FMath::Clamp(7.f * SailFill * Side * FMath::Min(FMath::Square(IronwakeFleet::WindKn(W) / 16.f), 2.2f), 0.f, 8.f);
		Heel += -FMath::Sign(TrueWindAngle) * SailHeel;
	}
	Spring(PitchDeg, PitchRate, SeaPitch * Follow, Th * 0.95f, 0.55f);
	Spring(RollDeg, RollRate, SeaRoll * Follow + Heel, Th * 1.5f, 0.4f);
	PitchDeg = FMath::Clamp(PitchDeg, -9.f, 9.f); RollDeg = FMath::Clamp(RollDeg, -12.f, 12.f);
	SetActorLocationAndRotation(FVector(Pos2.X, Pos2.Y, HeaveZ), FRotator(PitchDeg, Yaw, RollDeg));
	// wave source (P27: matches KeepFx 0.45 kn hysteresis + screw thrust & roll clearance)
	const float Kn = V / KnCmS / Comp;
	const bool bNpcSteam = D.bSteam || D.bPaddle || R->bAux;
	const float NpcThrust = bNpcSteam ? FMath::Clamp(FMath::Max(ThrustFrac(), Kn / FMath::Max(D.MaxKn, 6.f) * 0.85f), 0.f, 1.1f) : 0.f;
	if (Kn > 0.45f)
	{
		FIwShipWaveSrc Sw; Sw.PosCm = Pos2; Sw.CosH = Fw.X; Sw.SinH = Fw.Y;
		Sw.V = V / 100.f; Sw.L = Prof.Length(); Sw.X1 = Prof.X1;
		float MaxB = 0.f; for (float B : Prof.HalfB) MaxB = FMath::Max(MaxB, B);
		Sw.HalfBeam = MaxB; Sw.Draft = FMath::Clamp(D.DraftM, 0.5f, 8.f); Sw.Freeboard = FMath::Clamp(D.LengthM * 0.06f, 0.8f, 4.f);
		Sw.CrestK = D.HullSize == 0 ? 0.22f : 0.30f; Sw.TroughK = D.HullSize == 0 ? 0.f : 0.08f; Sw.QuarterK = 0.55f; Sw.WedgeK = 1.0f; Sw.Bluff = D.bSail ? 0.3f : 0.5f;
		Sw.bPlayer = false;
		Sw.bTwinScrew = (D.Id == TEXT("ironclad"));
		Sw.ThrustSteam = NpcThrust;
		Sw.RollOutM = FMath::Sin(FMath::DegreesToRadians(FMath::Abs(RollDeg))) * Sw.Freeboard * 0.65f;
		IronwakeSea::RegisterShipWave(this, Sw);
	}
	else IronwakeSea::UnregisterShipWave(this);
	if (bFx && Wake3 && BowWave)
	{
		const float Frac = FMath::Clamp(Kn / 13.f, 0.f, 1.2f);
		const float SpeedMS = V / 100.f;
		BowWave->UpdateWave(GetActorTransform(), Frac, SpeedMS, 0.f, 0.f);
		Wake3->SetPropWash(bNpcSteam && Kn > 0.6f, D.Id == TEXT("ironclad"), NpcThrust);
		Wake3->UpdateWake(Dt, GetActorTransform(), Prof.X0 + 1.f, Prof.X1 - 1.f, SpeedMS, Frac, IronwakeSea::GetSeaState());
	}
}

void AIronwakeNpcShip::StepParts(float Dt)
{
	FNpcRec* R = Rec(); if (!R) return;
	const FNpcDef& D = *R->Def;
	const float T = (float)GetWorld()->GetTimeSeconds();
	const float Thrust = D.bAir ? FMath::Clamp(EngineKn / FMath::Max(D.MaxKn, 1.f), 0.f, 1.f) : ThrustFrac();
	const float SpeedMS = V / 100.f;
	const float WindTo = IronwakeFleet::WindFromYaw(GetWorld()) + 180.f;
	for (FNpcPart& P : Parts)
	{
		USceneComponent* C = P.Pivot.Get(); if (!C) continue;
		switch (P.Kind)
		{
		case 0:   // propeller: rev follows thrust; above 1.7 rev/s activates translucent swept motion-blur disc around spinning blades
		{
			const float Rev = P.RevPerSec * (0.12f + 0.88f * Thrust);
			P.Rate = Rev;
			if (P.Disc.IsValid())
			{
				const bool bD = FMath::Abs(Rev) > 1.7f;
				if (bD != P.bDisc)
				{
					P.bDisc = bD;
					if (USceneComponent* Bl = P.Blades.Get()) Bl->SetVisibility(true, true);
					P.Disc->SetVisibility(bD);
				}
			}
			P.Angle = FMath::Fmod(P.Angle + Rev * 360.f * Dt, 360.f);
			C->SetRelativeRotation(P.Axis == 1 ? FRotator(P.Angle, 0.f, 0.f) : FRotator(0.f, 0.f, P.Angle));
			break;
		}
		case 1:   // paddle wheel: surface speed ~ ship speed (spins around lateral Y inside sponson wheel-well)
		{
			const float KnNow = FMath::Abs(V) / KnCmS / Comp;
			const float Rev = SpeedMS / (2.f * PI * 4.2f) * 1.35f + (KnNow > 0.2f ? 0.04f * Thrust : 0.f);
			P.Rate = Rev;
			P.Angle = FMath::Fmod(P.Angle - Rev * 360.f * Dt, 360.f);
			C->SetRelativeRotation(FRotator(P.Angle, 0.f, 0.f));
			break;
		}
		case 2:   // oar stroke while under way
		{
			const float Stroke = Thrust > 0.05f ? 1.f : 0.f;
			P.Angle += P.RevPerSec * 2.f * PI * Dt * (0.3f + 0.7f * Stroke);
			const float Ph = P.Angle + P.Phase;
			C->SetRelativeRotation(FRotator(-14.f + 11.f * FMath::Sin(Ph - 1.2f) * Stroke, P.BaseRot.Y + P.Sign * 26.f * FMath::Cos(Ph) * Stroke, 0.f));
			break;
		}
		case 4:   // P28 clamped 4-phase rowing stroke (1.5 s cycle): Catch -> Drive -> Release -> Recover; clears thwarts (Z=151.4) & gunwales (Z=186.5) 100%
		{
			const bool bRow = V > 25.f && !R->bHold;
			if (bRow) P.Angle = FMath::Fmod(P.Angle + Dt / 1.50f, 1.f);
			const float U = FMath::Frac(P.Angle + P.Phase);
			float Sweep, Dip;     // Sweep centered at +16 deg (compensates -18.3 deg mesh angle); Dip + = blade down into sea (Z<120 cm)
			auto Sm = [](float x) { x = FMath::Clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); };
			if (!bRow) { Sweep = 14.f; Dip = -4.f; }
			else if (U < 0.12f) { Sweep = 32.f; Dip = FMath::Lerp(-3.f, 13.5f, Sm(U / 0.12f)); }                          // 1. Catch (blade forward, lowers into water)
			else if (U < 0.48f) { Sweep = FMath::Lerp(32.f, -4.f, Sm((U - 0.12f) / 0.36f)); Dip = 13.5f; }                // 2. Drive (blade sweeps aft submerged)
			else if (U < 0.60f) { Sweep = -4.f; Dip = FMath::Lerp(13.5f, -3.f, Sm((U - 0.48f) / 0.12f)); }                // 3. Release (lifts cleanly out of water)
			else                { Sweep = FMath::Lerp(-4.f, 32.f, Sm((U - 0.60f) / 0.40f)); Dip = -3.f; }                 // 4. Recover (returns forward well above water & thwarts)
			C->SetRelativeRotation(FRotator(0.f, P.Sign * Sweep, P.Sign * Dip));
			if (FParse::Param(FCommandLine::Get(), TEXT("iwoarlog")) && P.Blades.IsValid() && (int32)(T * 4.f) != (int32)((T - Dt) * 4.f))
			{
				UStaticMeshComponent* Oc = Cast<UStaticMeshComponent>(P.Blades.Get()); const FBoxSphereBounds Bb = Oc->GetStaticMesh()->GetBounds();
				const float Sg = Bb.Origin.Y > 0.f ? 1.f : -1.f;
				const FTransform Hi = HullPivot->GetComponentTransform();
				const FVector Blade = Hi.InverseTransformPosition(Oc->GetComponentTransform().TransformPosition(FVector(Bb.Origin.X, Bb.Origin.Y + Sg * Bb.BoxExtent.Y, Bb.Origin.Z)));
				const FVector Grip = Hi.InverseTransformPosition(Oc->GetComponentTransform().TransformPosition(FVector(Bb.Origin.X, Bb.Origin.Y - Sg * Bb.BoxExtent.Y, Bb.Origin.Z)));
				UE_LOG(LogIronwake, Display, TEXT("IWNPC oar side=%+.0f u=%.2f blade_z=%.0f grip_z=%.0f thwart_clear=+%.0f gunwale_clear=+%.0f cm sweep=%.1f dip=%.1f"),
					P.Sign, U, Blade.Z, Grip.Z, Grip.Z - 151.4f, FMath::Min(Grip.Z, 198.f) - 186.5f, Sweep, Dip);
			}
			break;
		}
		case 5:   // SailorCrewRower rocks in phase with the 1.5 s stroke (+Pitch = lean aft toward stern at Catch, -Pitch = lay back toward bow on Drive)
		{
			const bool bRow = V > 25.f && !R->bHold;
			if (bRow) P.Angle = FMath::Fmod(P.Angle + Dt / 1.50f, 1.f);
			const float U = FMath::Frac(P.Angle + P.Phase);
			C->SetRelativeRotation(FRotator(bRow ? 17.0f * FMath::Cos((U - 0.08f) * 2.f * PI) : 0.f, 0.f, 0.f));
			break;
		}
		case 6:   // SalvageTenderV3 A-frame crane slews around Z (+/- 26 deg salvage cycle during Work/moored, +/- 4.5 deg underway)
		{
			const bool bWork = (R->State == ENpcState::Work || R->State == ENpcState::Docked || R->State == ENpcState::Anchored || FMath::Abs(V) < 80.f);
			const float Amp = bWork ? 26.f : 4.5f, Om = bWork ? 1.15f : 0.28f;
			P.Angle = Amp * FMath::Sin(T * Om + P.Phase);
			P.Rate = Amp * Om * FMath::Cos(T * Om + P.Phase);
			C->SetRelativeRotation(FRotator(0.f, P.Angle, 0.f));
			break;
		}
		case 7:   // SalvageTenderV3 diving bell pendulum (child of crane head sheave, clears stern counter)
		{
			const bool bWork = (R->State == ENpcState::Work || R->State == ENpcState::Docked || R->State == ENpcState::Anchored || FMath::Abs(V) < 80.f);
			const float Om = bWork ? 1.15f : 0.28f;
			const float PendRoll = (bWork ? 8.5f : 2.8f) * FMath::Sin(T * Om * 1.45f + P.Phase - 0.85f) - RollDeg * 0.45f;
			const float PendPitch = (bWork ? 5.2f : 2.0f) * FMath::Cos(T * Om * 1.20f + P.Phase + 0.45f) - PitchDeg * 0.45f;
			C->SetRelativeRotation(FRotator(PendPitch, 0.f, PendRoll));
			break;
		}
		case 3:   // ensign streams downwind and flutters
		{
			const float Rel = FRotator::NormalizeAxis(WindTo - GetActorRotation().Yaw);
			C->SetRelativeRotation(FRotator(0.f, Rel + 9.f * FMath::Sin(T * P.RevPerSec * 4.1f + P.Phase), 7.f * FMath::Sin(T * P.RevPerSec * 3.3f + P.Phase * 2.f)));
			break;
		}
		default: break;
		}
	}
	if (HelmWheelPivot) HelmWheelPivot->SetRelativeRotation(FRotator(0.f, 0.f, RudderDeg * 4.2f));
	if (PaddleChurnPads.Num() > 0 && PaddleChurnMID)
	{
		const float KnNow = FMath::Abs(V) / KnCmS / Comp;
		const bool bChurn = KnNow > 0.45f;
		if (bChurn)
		{
			PaddleChurnMID->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp((KnNow - 0.45f) / 6.0f, 0.f, 0.44f));
			PaddleChurnMID->SetScalarParameterValue(TEXT("Foam Boost"), FMath::Lerp(1.6f, 3.2f, FMath::Clamp(KnNow / 7.5f, 0.f, 1.f)));
		}
		for (int32 s = 0; s < PaddleChurnPads.Num(); ++s)
			if (UStaticMeshComponent* Pad = PaddleChurnPads[s])
			{
				if (Pad->IsVisible() != bChurn) Pad->SetVisibility(bChurn);
				if (bChurn) Pad->SetRelativeLocation(FVector(-175.f + 14.f * FMath::Sin(T * 5.2f + s * 1.7f), s == 0 ? -575.f : 565.f, 8.f + 3.f * FMath::Sin(T * 6.8f + s)));
			}
	}
	// Live mooring hawsers: visible ONLY when this ship is ACTUALLY docked at a Greyhaven quay berth (zero ropes into empty water!)
	if (MooringLines.Num() == 2 && Dir.IsValid())
	{
		const FVector2D HNow = Dir->W2H(GetActorLocation());
		const bool bQuayDocked = (R->State == ENpcState::Docked && FMath::Abs(R->SpeedKn) < 0.25f && HNow.X > -52.f && HNow.X < 6.f && HNow.Y > -142.f && HNow.Y < 152.f);
		if (bQuayDocked)
		{
			const float SeaBaseZ = AIronwakeShell::WaterZAt(GetWorld(), Dir->HarbourC) + AIronwakeShell::RenderedSeaOffsetCm;
			const FTransform HfT(FRotator(0.f, 45.f, 0.f), FVector(Dir->HarbourC.X, Dir->HarbourC.Y, SeaBaseZ));
			static const float RealBerthYs[] = { -129.f, -109.f, -89.f, -42.f, 25.f, 46.f, 139.f };
			float NearBy = HNow.Y; float BestDy = 999.f;
			for (float By : RealBerthYs) if (FMath::Abs(HNow.Y - By) < BestDy) { BestDy = FMath::Abs(HNow.Y - By); NearBy = By; }
			const FVector Fw = GetActorForwardVector(), Rt = GetActorRightVector();
			const float BowReachCm = FMath::Max(D.LengthM * 35.f, 260.f);
			const float BeamOffCm  = FMath::Clamp(D.BeamM * 24.f, 80.f, 260.f);
			const float DeckZCm    = FMath::Clamp(D.LengthM * 4.6f, 85.f, 220.f);
			for (int32 ml = 0; ml < 2; ++ml)
			{
				const float Sgn = ml == 0 ? -1.f : 1.f;
				const FVector BollardW = HfT.TransformPosition(FVector(-5130.f, (NearBy + Sgn * 4.5f) * 100.f, 426.f));
				const FVector FairleadW = GetActorLocation() + Fw * BowReachCm + Rt * (Sgn * BeamOffCm) + FVector(0.f, 0.f, DeckZCm);
				const FVector Delta = FairleadW - BollardW; const float Len = Delta.Size();
				if (UStaticMeshComponent* Rope = MooringLines[ml])
				{
					if (BestDy < 8.f && Len < 1150.f)
					{
						Rope->SetWorldLocationAndRotation((BollardW + FairleadW) * 0.5f, FRotationMatrix::MakeFromZ(Delta / FMath::Max(Len, 1.f)).Rotator());
						Rope->SetWorldScale3D(FVector(0.055f, 0.055f, Len / 100.f));
						if (!Rope->IsVisible()) Rope->SetVisibility(true);
					}
					else if (Rope->IsVisible()) Rope->SetVisibility(false);
				}
			}
		}
		else
		{
			for (UStaticMeshComponent* Rope : MooringLines) if (Rope && Rope->IsVisible()) Rope->SetVisibility(false);
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("iwproplog")) && (int32)(T * 2.f) != (int32)((T - Dt) * 2.f))
		for (const FNpcPart& P : Parts) if (P.Kind == 0 && P.Disc.IsValid()) UE_LOG(LogIronwake, Display, TEXT("IWNPC prop_rate=%.2f idx=%d disc=%d angle=%.0f %s"), P.Rate, P.Idx, P.bDisc ? 1 : 0, P.Angle, *D.Id.ToString());
	if (Smoke && D.SmokeScale > 0.f)
	{
		// P28: bank fires when docked/anchored/working/held so zero stationary white steam pillar rises over the central harbour; 50% gentler scale underway
		const bool bFires = R->State != ENpcState::Docked && R->State != ENpcState::Anchored && R->State != ENpcState::Work && !R->bHold && FMath::Abs(R->SpeedKn) > 0.8f;
		if (Smoke->IsActive() != bFires) Smoke->SetActive(bFires, true);
		if (bFires) Smoke->SetWorldScale3D(FVector(D.SmokeScale * 0.52f * (0.42f + 0.58f * FMath::Clamp(Thrust, 0.f, 1.f))));
	}
	if (Rig)
	{
		const float WindKn = IronwakeFleet::WindKn(GetWorld());
		const float Bs = FMath::Max(R->SpeedKn, 0.f);
		const float Tr = FMath::DegreesToRadians(TrueWindAngle);
		const float AppAngle = WindKn > 0.5f ? FMath::RadiansToDegrees(FMath::Atan2(WindKn * FMath::Sin(Tr), WindKn * FMath::Cos(Tr) + Bs)) : TrueWindAngle;
		const FVector2D HPos = Dir.IsValid() ? Dir->W2H(GetActorLocation()) : FVector2D(999.f, 0.f);
		const bool bMoored = (R->State == ENpcState::Docked) || (R->State == ENpcState::Anchored) || R->bHold || (R->State == ENpcState::Depart && R->Phase == 0) || (R->State == ENpcState::Arrive && HPos.X < 60.f);
		int32 StepLvl = 2;
		if (!bMoored && R->DesKn > 0.3f)
		{
			// Under auxiliary power (within 450 m of harbour mouth / basin Transit / Arrive / Depart) or gale: reef down to topsails+jib only (courses furled!)
			if (R->bAux || WindKn > 36.f || (HPos.X < 450.f && FMath::Abs(HPos.Y) < 260.f)) StepLvl = 3;
			else StepLvl = 6;                                   // full sail at open sea
		}
		const float UseFill = (StepLvl > 2) ? FMath::Max(SailFill, R->bAux ? 0.50f : 0.f) : 0.f;
		Rig->TickRig(Dt, TrueWindAngle, StepLvl, UseFill, WindKn, AppAngle);
	}
}

void AIronwakeNpcShip::Tick(float DtIn)
{
	Super::Tick(DtIn);
	FNpcRec* R = Rec(); AIronwakeNpcDirector* Dr = Dir.Get();
	if (!R || !R->Def || !Dr) { Destroy(); return; }
	if (GetWorld()->IsPaused()) return;
	const float Dt = FMath::Min(DtIn, 0.5f);
	if (Dt <= 0.f) return;
	const FNpcDef& D = *R->Def;
	if (D.bAir)
	{
		// sub-step the air dynamics at <= 0.1 s so time dilation does not change the flight path
		float Left = Dt; while (Left > 1e-4f) { const float S = FMath::Min(Left, 0.1f); StepAir(S); Left -= S; }
		StepParts(Dt);
		return;
	}
	if (R->bWarp) { StepWarp(Dt); }
	else if (R->bHold) { StepHold(Dt); }
	else
	{
		float Left = Dt; while (Left > 1e-4f) { const float S = FMath::Min(Left, 0.1f); StepDynamics(S); Left -= S; }
	}
	StepHydro(Dt, LastDesYR);
	StepParts(Dt);
}

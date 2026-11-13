// P12: the harbour town rebuilt as a terraced hillside that reads from the sea. Deterministic placement only.
// Harbour frame (metres): +X seaward, +Y along the quay, quay top Q = 2 above the rendered sea. Four terraces climb inland (-X) in 3.3 m steps,
// each with a front street, a front row of houses facing the sea and (on the lower three) a taller rear row, stair-ramps staggered in Y so they wind.
#include "IronwakeNpc.h"
#include "IronwakeVoyage.h"
#include "IronwakeP15.h"
#include "IronwakeP23.h"
#include "IronwakeP8.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "IronwakeWorld.h"
#include "IronwakeCollision.h"
#include "IronwakeTest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

TWeakObjectPtr<UStaticMeshComponent> GBasinSlab;
void IronwakeSetBasinCalm(bool bOn) { if (UStaticMeshComponent* B = GBasinSlab.Get()) if (B->IsVisible() != bOn) B->SetVisibility(bOn); }

namespace
{
	struct FTerr { float X0, X1, Z; };
	struct FHouseT { const TCHAR* Name; float W, D, H, Ridge; };
	UMaterialInstanceDynamic* VTintP12(UObject* Outer, const FLinearColor& C)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* Mi = UMaterialInstanceDynamic::Create(Base, Outer);
		Mi->SetVectorParameterValue(TEXT("Color"), C);
		return Mi;
	}
}

void AIronwakeVoyage::BuildTownP12(USceneComponent* Hs)
{
	const float Q = 2.0f;
	const auto M = [](float X, float Y, float Z) { return FVector(X * 100.f, Y * 100.f, Z * 100.f); };
	const auto MatOf = [](const TCHAR* N) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Town/%s.%s"), N, N)); };
	static float GroundK = -1.f; if (GroundK < 0.f) { GroundK = 3.0f; FParse::Value(FCommandLine::Get(), TEXT("iwground="), GroundK); }
	const auto SetM = [&](UStaticMeshComponent* C, const TCHAR* N)
	{
		if (!C) return;
		UMaterialInterface* Mi = MatOf(N); if (!Mi) return;
		// P14: the earth/scree under the town and the plateau read near-black from the sea at noon; lift the ground tint (not the quay setts or cobbles).
		if (FCString::Strcmp(N, TEXT("MI_IW_Ground")) == 0)
		{
			static TWeakObjectPtr<UMaterialInstanceDynamic> Gm;
			if (!Gm.IsValid() || Gm->GetOuter() != this) { UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Mi, this); FLinearColor T(1.f, 1.f, 1.f, 1.f); Mi->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), T); D->SetVectorParameterValue(TEXT("Tint"), FLinearColor(T.R * GroundK, T.G * GroundK, T.B * GroundK, 1.f)); Gm = D; }
			C->SetMaterial(0, Gm.Get()); return;
		}
		C->SetMaterial(0, Mi);
	};
	// axis-aligned block between corners (metres)
	const auto Block = [&](float X0, float X1, float Y0, float Y1, float Z0, float Z1, const TCHAR* Mat)
	{
		UStaticMeshComponent* C = Mesh(Hs, CubeMesh, M((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, (Z0 + Z1) * 0.5f), FRotator::ZeroRotator, FVector(FMath::Abs(X1 - X0), FMath::Abs(Y1 - Y0), FMath::Abs(Z1 - Z0)));
		SetM(C, Mat); return C;
	};
	// pitched slab rising toward -X from (Xa, Za) to (Xb, Zb): top surface along the line
	const auto Ramp = [&](float Xa, float Xb, float Za, float Zb, float Ya, float Yb, float Th, const TCHAR* Mat)
	{
		const float Dx = Xa - Xb, Dz = Zb - Za, L = FMath::Sqrt(Dx * Dx + Dz * Dz);
		const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(Dz, Dx));
		UStaticMeshComponent* C = Mesh(Hs, CubeMesh, M((Xa + Xb) * 0.5f, (Ya + Yb) * 0.5f, (Za + Zb) * 0.5f - Th * 0.5f / FMath::Cos(FMath::Atan2(Dz, Dx))), FRotator(Pitch, 0.f, 0.f), FVector(L, FMath::Abs(Yb - Ya), Th));
		SetM(C, Mat); return C;
	};
	FRandomStream Rg(1912);
	static const int32 NT = 5;
	static const FTerr T[5] = { { -90.f, -112.f, Q + 5.5f }, { -112.f, -134.f, Q + 11.f }, { -134.f, -156.f, Q + 16.5f }, { -156.f, -178.f, Q + 22.f }, { -178.f, -200.f, Q + 27.5f } };
	const float Ylo = -100.f, Yhi = 150.f;
	const float RampTopX = -200.f, PlatZ = Q + 56.f, RampEndX = -262.f;
	const auto Zr = [&](float X) { return FMath::Lerp(Q + 27.5f, PlatZ, FMath::Clamp((RampTopX - X) / (RampTopX - RampEndX), 0.f, 1.f)); };

	const float GapY[6] = { 18.f, 44.f, -22.f, 74.f, -48.f, 0.f };
	// ---- P15 ground: one natural hillside heightfield (IronwakeP15.cpp) replaces the P12 wedge: pads cut under the houses, road corridors,
	// cut banks between terraces (stone retaining walls only where houses stand), rock outcrops, gullies, a rounded crest.
	FIwHill Hill;
	const bool bP23 = !FParse::Param(FCommandLine::Get(), TEXT("iwoldtown"));
	const float ChapelZ = bP23 ? Hill.Base(-187.f, 22.f) : T[NT - 1].Z;
	FP23Plan Plan23; int32 NBase23 = 0;
	{
		FRandomStream Rw(1915);
		// P17: apron in five pieces so the seaward edge is a mix: quay wall (B, D), a cove with a beach + slipway (C), flank revetment (A, E)
		Hill.Pads.Add({ -92.f, -60.f, -256.f, -135.f, Q + 0.12f, 0.4f });
		Hill.Pads.Add({ -92.f, -50.f, -135.f, 62.f, Q + 0.12f, 0.4f });
		Hill.Pads.Add({ -92.f, -74.f, 62.f, 112.f, Q + 0.12f, 0.4f });
		Hill.Pads.Add({ -92.f, -50.f, 112.f, 166.f, Q + 0.12f, 0.4f });
		Hill.Pads.Add({ -92.f, -62.f, 166.f, 266.f, Q + 0.12f, 0.4f });
		const float GasZ = Hill.Base(-150.f, -144.f);   // gasometer bench at the industrial end
		Hill.Pads.Add({ -166.f, -134.f, -160.f, -128.f, GasZ, 2.f });
		if (!bP23) for (int32 i = 0; i < NT; ++i)
			Hill.Pads.Add({ T[i].X1 - 0.3f, T[i].X0 - 0.05f, -91.f - 4.f * (i % 3), 133.f + 5.f * ((i + 1) % 3), T[i].Z, 2.f });
		const float Ra = 2.9f;
		const auto Z0 = [&](int32 i) { return T[i].Z; };
		if (bP23) { NBase23 = IwP23::AddRoads(Hill); IwP23::AddVerges(Hill, NBase23); }
		else
		{
			{
				FIwRoad Mr; Mr.HalfW = Ra; Mr.Lift = 0.07f;
				Mr.Nodes.Add(FVector(-76.f, GapY[0], Q + 0.2f));
				Mr.Nodes.Add(FVector(-92.f, GapY[0], Z0(0) + 0.05f));
				for (int32 i = 0; i < NT - 1; ++i)
				{
					const float Xs = T[i].X0 - 1.9f, Gy = GapY[i + 1];
					Mr.Nodes.Add(FVector(Xs, GapY[i] + (Gy > GapY[i] ? 8.f : -8.f), Z0(i) + 0.05f));
					Mr.Nodes.Add(FVector(Xs, Gy, Z0(i) + 0.05f));
					Mr.Nodes.Add(FVector(T[i].X0 - 4.f, Gy, Z0(i) + 0.05f));
					Mr.Nodes.Add(FVector(T[i + 1].X0 - 0.3f, Gy, Z0(i + 1) + 0.05f));
					Mr.Nodes.Add(FVector(T[i + 1].X0 - 1.9f, Gy, Z0(i + 1) + 0.05f));
				}
				Mr.Nodes.Add(FVector(T[NT - 1].X0 - 1.9f, -22.f, Z0(NT - 1) + 0.05f));
				Hill.Roads.Add(Mr);
			}
			for (int32 i = 0; i < NT; ++i)   // terrace lanes (cobble, worn edges) in front of the house rows
			{
				FIwRoad L; L.HalfW = 2.1f; L.Lift = 0.04f;
				for (float Y = -90.f; Y <= 134.f; Y += 28.f) L.Nodes.Add(FVector(T[i].X0 - 1.9f + 0.5f * FMath::Sin(Y * 0.06f + i), Y, T[i].Z + 0.05f));
				Hill.Roads.Add(L);
			}
		}
		{
			FIwRoad Fp; Fp.HalfW = 2.0f; Fp.Lift = 0.05f; Fp.bFollow = true;   // chapel terrace -> crest, switchbacking up the slope
			const FVector2D Pts[] = { { -200.f, -40.f }, { -214.f, -78.f }, { -238.f, -66.f }, { -254.f, -14.f }, { -276.f, 42.f }, { -300.f, 20.f }, { -322.f, -30.f }, { -337.f, -48.f } };
			for (const FVector2D& P : Pts) Fp.Nodes.Add(FVector(P.X, P.Y, 0.f));
			Hill.Roads.Add(Fp);
			FIwRoad Cr; Cr.HalfW = 1.5f; Cr.Lift = 0.05f; Cr.bFollow = true;   // crest footpath
			const FVector2D Cp[] = { { -334.f, -175.f }, { -339.f, -118.f }, { -337.f, -48.f }, { -335.f, 10.f }, { -340.f, 66.f }, { -336.f, 124.f }, { -338.f, 186.f } };
			for (const FVector2D& P : Cp) Cr.Nodes.Add(FVector(P.X, P.Y, 0.f));
			Hill.Roads.Add(Cr);
		}
		if (!bP23)
		{
			// P17: light stone kerbs along both edges of the main road (thin roads in the same list: same design Z, built as ribbons below)
			const FIwRoad Main = Hill.Roads[0];
			for (int32 Sd = -1; Sd <= 1; Sd += 2)
			{
				FIwRoad K; K.HalfW = 0.3f; K.Lift = 0.10f;
				for (int32 n = 0; n < Main.Nodes.Num(); ++n)
				{
					const FVector A = Main.Nodes[FMath::Max(n - 1, 0)], B = Main.Nodes[FMath::Min(n + 1, Main.Nodes.Num() - 1)];
					const FVector2D Tg = (FVector2D(B) - FVector2D(A)).GetSafeNormal(), Nl(-Tg.Y, Tg.X);
					K.Nodes.Add(FVector(Main.Nodes[n].X + Nl.X * Sd * (Main.HalfW + 0.1f), Main.Nodes[n].Y + Nl.Y * Sd * (Main.HalfW + 0.1f), Main.Nodes[n].Z));
				}
				Hill.Roads.Add(K);
			}
		}
		Hill.BakeRoads();
		if (bP23)
		{
			IwP23::Plan(Hill, Plan23, 2323, NBase23);
			Hill.Pads.Add({ -204.f, -176.f, -14.f, 46.f, ChapelZ, 1.5f });   // chapel + bell tower bench
			for (const FP23House& H : Plan23.Houses)
			{
				const float Rad = FMath::DegreesToRadians(H.Phi), Ex = FMath::Abs(FMath::Cos(Rad)) * H.D + FMath::Abs(FMath::Sin(Rad)) * H.W, Ey = FMath::Abs(FMath::Sin(Rad)) * H.D + FMath::Abs(FMath::Cos(Rad)) * H.W;
				Hill.Pads.Add({ H.X - Ex * 0.5f - 0.7f, H.X + Ex * 0.5f + 0.7f, H.Y - Ey * 0.5f - 0.7f, H.Y + Ey * 0.5f + 0.7f, H.Z, 1.2f });
			}
		}
		UMaterialInterface* HillM = MatOf(TEXT("MI_IW_Hill"));
		// P17: no lime lawn: duller, yellower grass, warmer earth
		if (HillM) if (UMaterialInstanceDynamic* Hd = UMaterialInstanceDynamic::Create(HillM, this))
		{
			Hd->SetVectorParameterValue(TEXT("GrassTint"), FLinearColor(0.52f, 0.62f, 0.33f, 1.f));
			Hd->SetVectorParameterValue(TEXT("EarthTint"), FLinearColor(0.92f, 0.78f, 0.60f, 1.f));
			HillM = Hd;
		}
		Hill.BuildTerrain(this, Hs, HillM);
		const int32 NRd = Hill.Roads.Num();
		// P17: main road pale and wide (reads against the olive slope), lanes dark with worn edges, footpaths tan
		UMaterialInstanceDynamic* MainRd = nullptr; UMaterialInstanceDynamic* LaneRd = nullptr; UMaterialInstanceDynamic* VergeRd = nullptr;
		if (UMaterialInterface* Rm = MatOf(TEXT("MI_IW_Road")))
		{
			MainRd = UMaterialInstanceDynamic::Create(Rm, this); MainRd->SetVectorParameterValue(TEXT("Tint"), FLinearColor(2.6f, 2.45f, 2.15f, 1.f)); MainRd->SetScalarParameterValue(TEXT("DirtAmt"), 0.f);
			LaneRd = UMaterialInstanceDynamic::Create(Rm, this); LaneRd->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.5f, 0.47f, 0.44f, 1.f));
			VergeRd = UMaterialInstanceDynamic::Create(Rm, this); VergeRd->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.22f, 0.16f, 0.11f, 1.f));
		}
		for (int32 k = 0; k < NRd; ++k)
		{
			if (bP23)
			{
				UMaterialInterface* M23 = k == 0 ? (MainRd ? (UMaterialInterface*)MainRd : MatOf(TEXT("MI_IW_Road"))) : k < NBase23 ? (MainRd ? (UMaterialInterface*)MainRd : MatOf(TEXT("MI_IW_Road")))
					: k < 2 * NBase23 ? (VergeRd ? (UMaterialInterface*)VergeRd : MatOf(TEXT("MI_IW_Path"))) : MatOf(TEXT("MI_IW_Path"));
				if (k >= NBase23 && k < 2 * NBase23) { /* verge first-built order is fine: lower lift */ }
				Hill.BuildRibbon(this, Hs, M23, Hill.Roads[k]); continue;
			}
			UMaterialInterface* Mr = k == 0 ? (MainRd ? (UMaterialInterface*)MainRd : MatOf(TEXT("MI_IW_Road"))) : k < 1 + NT ? (LaneRd ? (UMaterialInterface*)LaneRd : MatOf(TEXT("MI_IW_Road")))
				: k >= NRd - 2 ? MatOf(TEXT("MI_IW_Setts")) : (MainRd ? (UMaterialInterface*)MainRd : MatOf(TEXT("MI_IW_Path")));
			Hill.BuildRibbon(this, Hs, Mr, Hill.Roads[k]);
		}
		// ship collision: the terrain is land above -2.6 m: register its footprint as solids (merged per 8 m column)
		if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()))
		{
			TArray<FVector4> Rc; Hill.LandRects(Rc);
			const FTransform Ht = Hs->GetComponentTransform();
			for (const FVector4& R : Rc)
			{
				const FVector Wc = Ht.TransformPosition(M((R.X + R.Y) * 0.5f, (R.Z + R.W) * 0.5f, 0.f));
				Co->AddObb(FVector2D(Wc.X, Wc.Y), Ht.Rotator().Yaw, FVector2D(FMath::Abs(R.Y - R.X) * 50.f + 30.f, FMath::Abs(R.W - R.Z) * 50.f + 30.f), Wc.Z - 3000.f, Wc.Z + 6000.f, false, TEXT("quay"));
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP15 land solids: %d"), Rc.Num());
		}
		Block(-58.f, -90.f, -135.f, 62.f, Q + 0.12f, Q + 0.17f, TEXT("MI_IW_Setts"));   // apron + square (P17: split around the cove)
		Block(-58.f, -90.f, 112.f, 165.f, Q + 0.12f, Q + 0.17f, TEXT("MI_IW_Setts"));
		Block(-74.f, -90.f, 62.f, 112.f, Q + 0.12f, Q + 0.17f, TEXT("MI_IW_Setts"));    // cove plaza
		Block(-50.2f, -58.f, -135.f, 62.f, Q + 0.12f, Q + 0.19f, TEXT("MI_IW_Deck"));   // planked edge
		Block(-50.2f, -58.f, 112.f, 165.f, Q + 0.12f, Q + 0.19f, TEXT("MI_IW_Deck"));
		// stone retaining walls on each cut, only in stretches (house fronts), with gaps where the bank shows and the ramps cross
		float PrevZ = Q + 0.12f;
		if (!bP23) for (int32 w = 0; w < NT; ++w)
		{
			const float Wx = T[w].X0, Wy0 = -90.f - 4.f * (w % 3), Wy1 = 132.f + 5.f * ((w + 1) % 3);
			float Y = Wy0 + Rw.FRandRange(0.f, 6.f);
			while (Y < Wy1 - 6.f)
			{
				const float Len = Rw.FRandRange(9.f, 19.f);
				float Ya = Y, Yb = FMath::Min(Y + Len, Wy1 - 2.f);
				const float Gy = GapY[w];
				if (Yb > Gy - 4.5f && Ya < Gy + 4.5f) { if (Ya < Gy - 4.5f) Yb = Gy - 4.5f; else { Y = Gy + 4.5f; continue; } }
				if (Yb - Ya > 2.f)
				{
					// P17: every stretch differs: height (full / 55-80 %), setback jitter, material, a lean (batter), some stretches left as an earth bank
					const float Kv = Rw.FRand();
					if (Kv < 0.18f) { Y = Yb + Rw.FRandRange(3.f, 7.f); continue; }
					const float Hk = Kv < 0.5f ? 1.f : Rw.FRandRange(0.55f, 0.8f);
					const float Jx = Rw.FRandRange(-1.6f, 1.6f);
					const TCHAR* Wm = Kv < 0.35f ? TEXT("MI_IW_Setts") : Kv < 0.75f ? TEXT("MI_IW_Cobble") : TEXT("MI_IW_Ground");
					const float Z0w = PrevZ - 0.05f, Z1w = Z0w + (T[w].Z - Z0w) * Hk;
					UStaticMeshComponent* Wc = Block(Wx + Jx, Wx + Jx + 0.9f + 0.4f * Rw.FRand(), Ya, Yb, Z0w, Z1w, Wm);
					if (Wc) Wc->SetRelativeRotation(FRotator(0.f, Rw.FRandRange(-4.f, 4.f), 0.f));
				}
				Y = Yb + Rw.FRandRange(3.f, 7.f);
			}
			PrevZ = T[w].Z;
		}
	}
	// ---- P14: sheltered basin: a glassy slab just above the mean sea inside the breakwaters. It hides the open-sea chop when the sea is
	// moderate or worse, so the basin reads calm next to the full state outside (the ocean is one global spectrum). Visibility via IronwakeSetBasinCalm.
	{
		if (UStaticMeshComponent* Bs = Block(-48.f, 140.f, -150.f, 158.f, 0.05f, 0.42f, TEXT("MI_IW_Setts")))
		{
			// the ocean's own material on a flat slab: same colour, ripple normals and sky reflection, but no Gerstner height or crest foam
			UMaterialInterface* Wm = MatOf(TEXT("MI_IW_WaterInst")); if (!Wm) Wm = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Ironwake/Water/MI_IW_WaterInst.MI_IW_WaterInst"));
			if (Wm) Bs->SetMaterial(0, Wm);
			else { UMaterialInstanceDynamic* Mi = VTintP12(this, FLinearColor(0.004f, 0.012f, 0.022f)); if (Mi) Bs->SetMaterial(0, Mi); }
			Bs->SetCollisionEnabled(ECollisionEnabled::NoCollision); Bs->SetCastShadow(false); Bs->SetVisibility(false);
			GBasinSlab = Bs;
		}
	}
	// ---- glow: one dynamic material for windows + lamp bulbs (On = night factor); brighter than P10 so it reads at 700 m
	if (UMaterialInterface* GlowM = MatOf(TEXT("MI_IW_Window")))
	{
		WinMID = UMaterialInstanceDynamic::Create(GlowM, this);
		WinMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.4f, 1.15f, 0.38f));
		const auto MakeIsm = [&](UStaticMesh* Sm) -> UInstancedStaticMeshComponent*
		{
			UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
			I->SetStaticMesh(Sm); I->SetupAttachment(Hs); I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
			I->SetMaterial(0, WinMID); I->RegisterComponent(); AddInstanceComponent(I); return I;
		};
		WinISM = MakeIsm(CubeMesh); BulbISM = MakeIsm(SphereMesh);
	}
	// ---- instanced chimneys (brick) and vegetation
	UInstancedStaticMeshComponent* ChimIsm = NewObject<UInstancedStaticMeshComponent>(this);
	ChimIsm->SetStaticMesh(CubeMesh); ChimIsm->SetupAttachment(Hs); ChimIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	{ const FLinearColor Brick(0.20f, 0.085f, 0.06f); if (UMaterialInstanceDynamic* Mi = VTintP12(this, Brick)) ChimIsm->SetMaterial(0, Mi); }
	ChimIsm->RegisterComponent(); AddInstanceComponent(ChimIsm);
	TMap<FString, UInstancedStaticMeshComponent*> VegIsm;
	const auto Veg = [&](const TCHAR* Name, const FVector& LocM, float Yaw, float S, float Pitch = 0.f, float Roll = 0.f, float Sz = 1.f, float LeanY = 0.f)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("iwnoveg"))) return;
		UInstancedStaticMeshComponent*& I = VegIsm.FindOrAdd(Name);
		if (!I)
		{
			UStaticMesh* Sm = IronwakeWorld::Mesh(Name); if (!Sm) return;
			I = NewObject<UInstancedStaticMeshComponent>(this);
			I->SetStaticMesh(Sm); I->SetupAttachment(Hs); I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
			I->RegisterComponent(); AddInstanceComponent(I);
		}
		if (LeanY != 0.f) I->AddInstance(FTransform(FQuat(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(-LeanY)) * FQuat(FRotator(0.f, Yaw, 0.f)), M(LocM.X, LocM.Y, LocM.Z), FVector(S, S, S * Sz)));
		else I->AddInstance(FTransform(FRotator(Pitch, Yaw, Roll), M(LocM.X, LocM.Y, LocM.Z), FVector(S, S, S * Sz)));
	};
	UInstancedStaticMeshComponent* SmkIsm = NewObject<UInstancedStaticMeshComponent>(this);
	SmkIsm->SetStaticMesh(SphereMesh); SmkIsm->SetupAttachment(Hs); SmkIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision); SmkIsm->SetCastShadow(false);
	if (UMaterialInterface* Sm = MatOf(TEXT("M_IW_Smk"))) SmkIsm->SetMaterial(0, Sm);
	SmkIsm->RegisterComponent(); AddInstanceComponent(SmkIsm);
	int32 NHouses = 0, NWin = 0, NChim = 0, NSmoke = 0;

	// ---- houses
	static const FHouseT Types[3] = { { TEXT("TownRowCottages"), 18.f, 8.26f, 11.f, 8.6f }, { TEXT("TownStoneTownhouse"), 8.f, 8.46f, 12.f, 11.4f }, { TEXT("TownTimberHouse"), 7.18f, 7.74f, 9.f, 7.6f } };
	// P24: weathered coastal North-Atlantic limewash/stone (linear 0.78-1.28): whitewash, cream, buff sandstone, pale slate, ochre, brick, grey-green
	const FLinearColor Tints[7] = { FLinearColor(1.18f, 1.16f, 1.08f), FLinearColor(1.20f, 1.08f, 0.82f), FLinearColor(1.05f, 0.90f, 0.68f), FLinearColor(0.84f, 0.88f, 0.92f),
		FLinearColor(1.10f, 0.88f, 0.52f), FLinearColor(0.92f, 0.62f, 0.50f), FLinearColor(0.80f, 0.92f, 0.82f) };
	int32 LastTint = -1;
	// stair gaps (y centre, half-width) per terrace i: the ramp that climbs from terrace i to i+1 (i=-1: quay -> terrace 0)
	const auto House = [&](int32 Ti, float Cx, float Cy, float S, float YawJ) -> UStaticMeshComponent*
	{
		const FHouseT& H = Types[Ti];
		const float Yaw = -90.f + YawJ;
		UStaticMeshComponent* C = IronwakeWorld::Place(this, Hs, H.Name, M(Cx, Cy, 0.f), FRotator(0.f, Yaw, 0.f), FVector(S), true);
		if (!C) return nullptr;
		int32 Tn; do { Tn = Rg.RandRange(0, 6); } while (Tn == LastTint); LastTint = Tn;
		IronwakeWorld::TintAll(C, Tints[Tn]);
		++NHouses;
		return C;
	};
	for (int32 Row = 0; Row < (bP23 ? 0 : 2); ++Row)
		for (int32 i = 0; i < NT; ++i)
		{
			if (Row == 1 && i == NT - 1) continue;
			const FTerr& Tr = T[i];
			const float Front = Tr.X0 - (Row == 0 ? 4.6f : 15.2f);
			float Y = -86.f;
			const float YEnd = i == 0 ? 124.f : 128.f;
			const float YStart = i == 0 ? -64.f : -86.f;
			Y = YStart;
			int32 Guard = 0;
			while (Y < YEnd && Guard++ < 60)
			{
				if (Row == 1 && Rg.FRand() < 0.4f) { Y += Rg.FRandRange(5.f, 12.f); continue; }
				int32 Ti = 1; float R = Rg.FRand();
				if (Row == 0) Ti = R < 0.25f ? 0 : R < 0.62f ? 1 : 2;
				const float S = Row == 1 ? Rg.FRandRange(1.0f, 1.25f) : Rg.FRandRange(0.85f, 1.22f);
				const FHouseT& H = Types[Ti];
				const float W = H.W * S, D = H.D * S;
				// stair gap at the ramps that cross this terrace (ramp i climbs out of terrace i-1... keep both neighbours clear)
				bool bGap = false;
				if (i == NT - 1 && Y + W > -4.f && Y < 34.f) { Y = 34.f; continue; }   // chapel + bell tower plot
				for (int32 g = i - 1; g <= i; ++g)
				{
					if (g < -1 || g > NT - 2) continue;
					const float Gy = GapY[g + 1];
					if (Y + W > Gy - 3.5f && Y < Gy + 3.5f) { Y = Gy + 3.8f; bGap = true; break; }
				}
				if (bGap) continue;
				if (Y + W > YEnd) break;
				const float Cx = Front - D * 0.5f, Cy = Y + W * 0.5f;
				if (Row == 1 && Cx - D * 0.5f < Tr.X1 - 0.5f) { Y += W + 1.f; continue; }
				UStaticMeshComponent* C = House(Ti, Cx, Cy, S, Rg.FRandRange(-3.f, 3.f));
				// pad under the house (terrace is flat; fill any rounding) -- house base on the terrace top
				if (C) C->SetRelativeLocation(M(Cx, Cy, Tr.Z - 0.12f));
				const float Base = Tr.Z;
				// windows on the sea-facing facade (seaward = +X in this frame, yaw ~ -90)
				if (WinISM)
				{
					const float Hh = H.H * S;
					const int32 Cols = FMath::Max(1, FMath::FloorToInt(W / 2.8f)), Rows = FMath::Clamp(FMath::FloorToInt((Hh * 0.66f - 1.5f) / 3.f) + 1, 1, 4);
					for (int32 r = 0; r < Rows; ++r)
						for (int32 c = 0; c < Cols; ++c)
						{
							if (Rg.FRand() < 0.38f) continue;
							const float V = Cy + (c + 0.5f - Cols * 0.5f) * (W / Cols);
							WinISM->AddInstance(FTransform(FRotator(0.f, -90.f, 0.f), M(Cx + D * 0.5f + 0.05f, V, Base + 1.9f + 3.f * r), FVector(0.9f, 0.12f, 1.35f)));
							++NWin;
						}
				}
				// chimney(s) on the ridge
				{
					const int32 NC = Ti == 0 ? 1 : (Rg.FRand() < 0.5f ? 2 : 1);
					for (int32 k = 0; k < NC; ++k)
					{
						const float Lx = (k ? -1.f : 1.f) * Rg.FRandRange(0.18f, 0.32f) * W;
						const float Top = Base + H.Ridge * S + Rg.FRandRange(1.4f, 2.6f);
						const float Bot = Base + H.Ridge * S - 1.6f;
						ChimIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx - D * 0.12f, Cy + Lx, (Top + Bot) * 0.5f), FVector(1.1f, 1.1f, Top - Bot)));
						ChimIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx - D * 0.12f, Cy + Lx, Top + 0.1f), FVector(1.5f, 1.5f, 0.25f)));
						++NChim;
						if ((NChim % 5) == 2 && SmkIsm)
						{
							// thin drifting plume: 7 translucent spheres, growing and leaning downwind (+Y)
							const int32 Nq = 4 + (NChim % 3);
							for (int32 q = 0; q < Nq; ++q)
							{
								const float Hq = Top + 1.0f + 2.2f * q, Rq = 0.3f + 0.16f * q;
								SmkIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx - D * 0.12f - 0.4f * q, Cy + Lx + 0.9f * q, Hq), FVector(Rq * 0.5f, Rq * 0.5f, Rq * 0.6f)));
							}
							++NSmoke;
						}
					}
				}
				Y += W + Rg.FRandRange(0.4f, 2.4f);
			}
		}
	// ---- P23: stone retaining walls on the downhill edge of every road (vertical faces read from the sea where the flat ribbon is foreshortened)
	if (bP23)
	{
		int32 Nw = 0;
		for (int32 Ri = 0; Ri < NBase23; ++Ri)
		{
			const FIwRoad& Rd = Hill.Roads[Ri];
			for (int32 I = 3; I + 4 < Rd.P.Num(); I += 5)
			{
				const FVector2D Pc(Rd.P[I]); const FVector2D Tg = (FVector2D(Rd.P[I + 3]) - FVector2D(Rd.P[I - 3])).GetSafeNormal(), Nl(-Tg.Y, Tg.X);
				const float Sd = Hill.Base(Pc.X + Nl.X * 6.f, Pc.Y + Nl.Y * 6.f) < Hill.Base(Pc.X - Nl.X * 6.f, Pc.Y - Nl.Y * 6.f) ? 1.f : -1.f;
				const FVector2D Ed = Pc + Nl * Sd * (Rd.HalfW + 0.5f);
				const float Zrw = Rd.P[I].Z, Zg = Hill.Base(Ed.X + Nl.X * Sd * 2.2f, Ed.Y + Nl.Y * Sd * 2.2f);
				const float Hh = Zrw - Zg;
				if (Zrw < 3.f) continue;
				{ const float Yk = FMath::RadiansToDegrees(FMath::Atan2(Tg.Y, Tg.X));
				  UStaticMeshComponent* Kb = Mesh(Hs, CubeMesh, M(Ed.X, Ed.Y, Zrw + 0.22f), FRotator(0.f, Yk, 0.f), FVector(7.8f, 0.58f, 0.52f));
				  SetM(Kb, TEXT("MI_IW_DarkStone")); ++Nw; }
				if (Hh < 0.9f) continue;
				const float Ht = FMath::Min(Hh + 0.6f, 3.8f), Yaw = FMath::RadiansToDegrees(FMath::Atan2(Tg.Y, Tg.X));
				UStaticMeshComponent* W = Mesh(Hs, CubeMesh, M(Ed.X, Ed.Y, Zrw + 0.25f - Ht * 0.5f), FRotator(0.f, Yaw, 0.f), FVector(8.0f, 0.9f, Ht));
				SetM(W, TEXT("MI_IW_DarkStone")); ++Nw;
			}
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP23 retaining wall segments: %d"), Nw);
	}
	// ---- P23: clustered houses along the roads (planner in IronwakeP23.cpp): bench plinth, yaw follows the contour, per-house scale/height
	if (bP23)
	{
		int32 NPl = 0, P24Win = 0, P24Rej = 0;
		for (const FP23House& H : Plan23.Houses)
		{
			const FHouseT& Ht = Types[H.Type];
			const float Yaw = H.Phi - 90.f + 180.f * 0.f;   // mesh faces +X at yaw -90 in the old layout: facade normal = Phi
			const FVector Sc(H.S, H.S, H.S * H.Sz);
			UStaticMeshComponent* C = IronwakeWorld::Place(this, Hs, Ht.Name, M(H.X, H.Y, H.Z - 0.12f), FRotator(0.f, H.Phi - 90.f + 0.f, 0.f), Sc, true);
			(void)Yaw;
			if (!C) continue;
			{ IronwakeWorld::TintAll(C, Tints[H.Tint] * (1.8f * Rg.FRandRange(0.92f, 1.06f))); }
			++NHouses; ++NPl;
			FVector2D Nn(FMath::Cos(FMath::DegreesToRadians(H.Phi)), FMath::Sin(FMath::DegreesToRadians(H.Phi))), Tt(-Nn.Y, Nn.X);
			// plinth / retaining wall where the natural ground falls away below the bench
			float Zlow = H.Z;
			for (int32 a = -1; a <= 1; a += 2) for (int32 b = -1; b <= 1; b += 2) Zlow = FMath::Min(Zlow, Hill.Base(H.X + Nn.X * a * H.D * 0.5f + Tt.X * b * H.W * 0.5f, H.Y + Nn.Y * a * H.D * 0.5f + Tt.Y * b * H.W * 0.5f));
			if (H.Z - Zlow > 0.35f)
			{
				UStaticMeshComponent* Pl = Mesh(Hs, CubeMesh, M(H.X, H.Y, (H.Z + Zlow - 2.2f) * 0.5f), FRotator(0.f, H.Phi, 0.f), FVector(H.D + 1.1f, H.W + 1.1f, H.Z - Zlow + 2.3f));
				SetM(Pl, TEXT("MI_IW_DarkStone"));
				UStaticMeshComponent* Cp = Mesh(Hs, CubeMesh, M(H.X + Nn.X * (H.D * 0.5f + 0.55f), H.Y + Nn.Y * (H.D * 0.5f + 0.55f), H.Z + 0.05f), FRotator(0.f, H.Phi, 0.f), FVector(0.55f, H.W + 1.3f, 0.35f));
				SetM(Cp, TEXT("MI_IW_DarkStone"));
			}
			const float Base = H.Z;
			if (WinISM)
			{
				// P24: each window is line-traced onto the house mesh; kept only on a near-vertical front facade (rejects roofs, gables, side walls)
				C->UpdateComponentToWorld(); C->RecreatePhysicsState();
				const FTransform HsT = Hs->GetComponentTransform();
				const FVector FrontW = HsT.TransformVectorNoScale(FVector(Nn.X, Nn.Y, 0.f)).GetSafeNormal();
				const int32 Cols = FMath::Max(1, FMath::FloorToInt(H.W / 2.8f)), Rows = FMath::Clamp(FMath::FloorToInt((H.H * 0.66f - 1.5f) / 3.f) + 1, 1, 4);
				for (int32 r = 0; r < Rows; ++r)
					for (int32 c = 0; c < Cols; ++c)
					{
						if (Rg.FRand() < 0.30f) continue;
						const float V = (c + 0.5f - Cols * 0.5f) * (H.W / Cols);
						const float Zw = Base + 1.9f + 3.f * r;
						if (Zw > Base + H.Ridge - 1.2f) { ++P24Rej; continue; }
						const FVector Ctr = C->Bounds.Origin;   // real house centre in world space (the mesh pivot is not its centre)
						const FVector TanW = HsT.TransformVectorNoScale(FVector(Tt.X, Tt.Y, 0.f)).GetSafeNormal();
						const FVector Lat = Ctr + TanW * V * 100.f;
						const FVector S0 = FVector(Lat.X, Lat.Y, HsT.GetLocation().Z + Zw * 100.f) + FrontW * 2500.f, E0 = FVector(Lat.X, Lat.Y, HsT.GetLocation().Z + Zw * 100.f);
						FHitResult Hit; FCollisionQueryParams Qp(NAME_None, true);
						const bool bHit = C->LineTraceComponent(Hit, S0, E0, Qp);
						if (!bHit) { ++P24Rej; continue; }
						if (FVector::DotProduct(Hit.ImpactNormal, FrontW) < 0.82f || FMath::Abs(Hit.ImpactNormal.Z) > 0.22f) { ++P24Rej; continue; }
						const FVector Wp = FVector(Hit.ImpactPoint) + Hit.ImpactNormal * 3.f;
						const float Yw = FMath::RadiansToDegrees(FMath::Atan2(Hit.ImpactNormal.Y, Hit.ImpactNormal.X));
						const FTransform Wt(FRotator(0.f, Yw, 0.f), Wp, FVector(0.04f, 0.75f, 1.15f));
						WinISM->AddInstance(Wt * HsT.Inverse()); ++NWin; ++P24Win;
					}
			}
			const int32 NC = H.Type == 0 ? 1 : (Rg.FRand() < 0.5f ? 2 : 1);
			for (int32 k = 0; k < NC; ++k)
			{
				const float Lx = (k ? -1.f : 1.f) * Rg.FRandRange(0.18f, 0.32f) * H.W;
				const float Top = Base + H.Ridge + Rg.FRandRange(1.4f, 2.6f), Bot = Base + H.Ridge - 1.6f;
				const float Cx = H.X - Nn.X * H.D * 0.12f + Tt.X * Lx, Cy = H.Y - Nn.Y * H.D * 0.12f + Tt.Y * Lx;
				ChimIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx, Cy, (Top + Bot) * 0.5f), FVector(1.1f, 1.1f, Top - Bot)));
				ChimIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx, Cy, Top + 0.1f), FVector(1.5f, 1.5f, 0.25f)));
				++NChim;
				if ((NChim % 4) == 1 && SmkIsm)
				{
					const int32 Nq = 5 + (NChim % 3);
					for (int32 q = 0; q < Nq; ++q)
					{
						const float Hq = Top + 1.0f + 2.0f * q, Rq = 0.3f + 0.15f * q;
						SmkIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx - 0.4f * q, Cy + 1.0f * q, Hq), FVector(Rq * 0.5f, Rq * 0.5f, Rq * 0.6f)));
					}
					++NSmoke;
				}
			}
		}
		for (const FP23Veg& V : Plan23.Veg)
		{
			const float Z = Hill.Final(V.X, V.Y);
			if (V.Kind == 0) Veg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(V.X, V.Y, Z - 0.1f), Rg.FRandRange(0.f, 360.f), V.S);
			else if (V.Kind == 4) Veg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(V.X, V.Y, Z - 0.1f), 90.f + Rg.FRandRange(-25.f, 25.f), V.S, Rg.FRandRange(6.f, 14.f), 0.f, 0.65f);
			else if (V.Kind == 1) Veg(TEXT("FirTree01"), FVector(V.X, V.Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), V.S);
			else if (V.Kind == 2) Veg(TEXT("IslandTree01"), FVector(V.X, V.Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), V.S * 1.4f);
			else IronwakeWorld::Place(this, Hs, Rg.FRand() < 0.5f ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03"), M(V.X, V.Y, Z - 0.5f - 1.6f * V.S), FRotator(Rg.FRandRange(-8.f, 8.f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(-8.f, 8.f)), FVector(V.S, V.S * Rg.FRandRange(0.8f, 1.3f), V.S * Rg.FRandRange(0.7f, 1.2f)), false, false);
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP23 houses %d in %d clusters, veg %d"), NPl, Plan23.Clusters, Plan23.Veg.Num());
		UE_LOG(LogIronwake, Display, TEXT("IWP24 windows placed=%d trace_rejected=%d"), P24Win, P24Rej);
	}
	// ---- P15: stone steps up the steepest crest link (the roads themselves are ribbons built with the ground)
	{
		const auto Steps = [&](const FVector2D& A, const FVector2D& B, float W)
		{
			const float Len = FVector2D::Distance(A, B), Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
			const int32 N = FMath::CeilToInt(Len / 0.55f);
			for (int32 k = 0; k < N; ++k)
			{
				const FVector2D P = FMath::Lerp(A, B, (k + 0.5f) / N);
				const float Zt = Hill.Final(P.X, P.Y) + 0.08f;
				UStaticMeshComponent* C = Mesh(Hs, CubeMesh, M(P.X, P.Y, Zt - 0.2f), FRotator(0.f, Yaw, 0.f), FVector(Len / N + 0.05f, W, 0.4f));
				SetM(C, TEXT("MI_IW_Cobble"));
			}
		};
		Steps(FVector2D(-300.f, 20.f), FVector2D(-322.f, -30.f), 2.0f);
		Steps(FVector2D(-254.f, -14.f), FVector2D(-276.f, 42.f), 2.0f);
	}
	// ---- landmarks: customs house (dome) on the quay, chapel + bell tower with a tall spire on the top terrace
	{
		const auto Land = [&](const TCHAR* N, float X, float Y, float Yaw, float Z)
		{
			if (UStaticMesh* Sm = IronwakeWorld::Mesh(N))
			{
				const FVector E = Sm->GetBounds().BoxExtent * 0.01f;
				const FVector R = FRotator(0.f, Yaw, 0.f).RotateVector(FVector(E.X, E.Y, 0.f));
				if (Z > Q + 0.1f) Block(X - FMath::Abs(R.X) - 0.3f, X + FMath::Abs(R.X) + 0.3f, Y - FMath::Abs(R.Y) - 0.3f, Y + FMath::Abs(R.Y) + 0.3f, Q, Z + 0.3f, TEXT("MI_IW_Cobble"));
			}
			return IronwakeWorld::Place(this, Hs, N, M(X, Y, Z - 0.1f), FRotator(0.f, Yaw, 0.f), FVector(1.f), true);
		};
		Land(TEXT("TownCustomsHouse"), -66.f, 48.f, -90.f, Q + 0.3f);
		Land(TEXT("TownChapel"), -187.f, 22.f, -90.f, ChapelZ + 0.1f);
		Land(TEXT("TownBandstand"), -72.f, 2.f, 0.f, Q + 0.3f);
		for (int32 i = 0; i < 4; ++i) Land(TEXT("TownMarketStall"), -69.5f, -14.f + 4.8f * i, -90.f + (i % 2 ? 3.f : -3.f), Q + 0.3f);

		// ---- P28: Batch-4 upgraded buildings + painted Victorian shop signs ----
		// 1) Half-timbered Anchor Tavern facing the harbour square (+X) right beside the main street (Y=18), with SignAnchorTavern + brass wall lamps
		IronwakeWorld::Place(this, Hs, TEXT("TownTavernV4"), M(-84.0f, 30.5f, Q + 0.16f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true);
		IronwakeWorld::Place(this, Hs, TEXT("SignAnchorTavern"), M(-79.58f, 30.5f, Q + 3.64f), FRotator(0.f, 0.f, 0.f), FVector(1.f), false, false);
		IronwakeWorld::Place(this, Hs, TEXT("WallLamp"), M(-79.55f, 28.1f, Q + 3.3f), FRotator::ZeroRotator, FVector(1.f), false, false);
		IronwakeWorld::Place(this, Hs, TEXT("WallLamp"), M(-79.55f, 32.9f, Q + 3.3f), FRotator::ZeroRotator, FVector(1.f), false, false);
		Lamp(Hs, M(-79.0f, 30.5f, Q + 3.8f), FLinearColor(1.f, 0.72f, 0.38f), 4200.f, 2400.f);

		// 2) Quay Merchants' Row: TownChandleryV4 nestled tightly (0 m flank gap!) between two stone townhouses so its flat flanks are 100% hidden,
		//    with crisp SignShipChandlery fascia board mounted over the shopfront lintel.
		if (UStaticMeshComponent* FlankL = IronwakeWorld::Place(this, Hs, TEXT("TownStoneTownhouse"), M(-84.2f, -34.7f, Q + 0.16f), FRotator(0.f, -90.f, 0.f), FVector(1.02f), true))
			IronwakeWorld::TintAll(FlankL, FLinearColor(1.08f, 0.96f, 0.78f));
		IronwakeWorld::Place(this, Hs, TEXT("TownChandleryV4"), M(-84.0f, -26.3f, Q + 0.16f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true);
		IronwakeWorld::Place(this, Hs, TEXT("SignShipChandlery"), M(-79.92f, -26.3f, Q + 3.56f), FRotator(0.f, 0.f, 0.f), FVector(1.f), false, false);
		if (UStaticMeshComponent* FlankR = IronwakeWorld::Place(this, Hs, TEXT("TownStoneTownhouse"), M(-84.2f, -17.9f, Q + 0.16f), FRotator(0.f, -90.f, 0.f), FVector(0.98f, 1.02f, 0.95f), true))
			IronwakeWorld::TintAll(FlankR, FLinearColor(0.88f, 0.91f, 0.94f));
		IronwakeWorld::Place(this, Hs, TEXT("WallLamp"), M(-79.85f, -23.8f, Q + 3.3f), FRotator::ZeroRotator, FVector(1.f), false, false);
		IronwakeWorld::Place(this, Hs, TEXT("WallLamp"), M(-79.85f, -28.8f, Q + 3.3f), FRotator::ZeroRotator, FVector(1.f), false, false);
		Lamp(Hs, M(-79.2f, -26.3f, Q + 3.8f), FLinearColor(1.f, 0.74f, 0.42f), 4200.f, 2400.f);

		// 3) Red-brick clerestory TownFoundryV4 on the industrial south wharf with subtle pale stack plume
		IronwakeWorld::Place(this, Hs, TEXT("TownFoundryV4"), M(-98.0f, -122.0f, Q + 0.15f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true);
		if (SmkIsm)
		{
			for (int32 q = 0; q < 6; ++q)
			{
				const float Hq = Q + 15.6f + 2.3f * q, Rq = 0.42f + 0.20f * q;
				SmkIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(-97.3f - 0.4f * q, -131.8f + 1.0f * q, Hq), FVector(Rq * 0.55f, Rq * 0.55f, Rq * 0.65f)));
			}
		}
		// bell tower beside the chapel: stone shaft, belfry louvres (dark), slate spire (cone) ~44 m above the terrace
		const float Bx = -187.f, By = 8.f, Zt = ChapelZ;
		Block(Bx - 3.f, Bx + 3.f, By - 3.f, By + 3.f, Zt - 0.2f, Zt + 24.f, TEXT("MI_IW_Setts"));
		Block(Bx - 3.3f, Bx + 3.3f, By - 3.3f, By + 3.3f, Zt + 24.f, Zt + 25.f, TEXT("MI_IW_Cobble"));
		Block(Bx + 2.9f, Bx + 3.1f, By - 1.1f, By + 1.1f, Zt + 19.f, Zt + 22.5f, TEXT("MI_IW_Deck"));
		Lamp(Hs, M(Bx + 5.f, By, Zt + 9.f), FLinearColor(1.f, 0.72f, 0.4f), 5000.f, 3000.f);      // P24 uplight on the bell tower
		Lamp(Hs, M(Bx + 4.f, By, Zt + 21.f), FLinearColor(1.f, 0.7f, 0.35f), 6000.f, 2500.f);      // belfry lantern
		Lamp(Hs, M(Bx - 4.f, By + 14.f, Zt + 6.f), FLinearColor(1.f, 0.72f, 0.4f), 4000.f, 2500.f);   // chapel forecourt
		const FLinearColor Slate(0.045f, 0.05f, 0.06f);
		Mesh(Hs, ConeMesh, M(Bx, By, Zt + 25.f + 13.f), FRotator::ZeroRotator, FVector(7.f, 7.f, 26.f), &Slate);
		if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld())) { (void)Co; }
		// customs house + chapel lit windows on the quay level are in the model textures; add a clock-face glow at night
		if (BulbISM) BulbISM->AddInstance(FTransform(FRotator::ZeroRotator, M(Bx + 3.1f, By, Zt + 21.f), FVector(0.6f)));
	}
	// ---- P24: headland lighthouse on the south-west (-Y) bluff, wind-bent broadleaf trees, quay-front variety (wet band, fender piles, stilt huts)
	if (bP23)
	{
		float Lx = -158.f, Ly = -265.f, Lz = -99.f;
		for (float X = -130.f; X >= -190.f; X -= 3.f) for (float Y = -235.f; Y >= -300.f; Y -= 3.f) { const float Z = Hill.Final(X, Y); if (Z > Lz) { Lz = Z; Lx = X; Ly = Y; } }
		UE_LOG(LogIronwake, Display, TEXT("IWP24 lighthouse site (%.0f, %.0f) ground Z %.1f"), Lx, Ly, Lz);
		if (Lz > 1.5f)
		{
			const FLinearColor Lime(0.82f, 0.80f, 0.74f), Ox(0.30f, 0.05f, 0.04f), Iron(0.03f, 0.03f, 0.035f), Glass(6.f, 4.2f, 1.6f);
			const float Zb = Lz - 1.f;
			Block(Lx - 6.f, Lx + 6.f, Ly - 6.f, Ly + 6.f, Zb - 3.f, Zb + 1.8f, TEXT("MI_IW_Cobble"));
			Mesh(Hs, CylMesh, M(Lx, Ly, Zb + 1.8f + 12.f), FRotator::ZeroRotator, FVector(5.4f, 5.4f, 24.f), &Lime);
			Mesh(Hs, CylMesh, M(Lx, Ly, Zb + 1.8f + 9.f), FRotator::ZeroRotator, FVector(5.5f, 5.5f, 4.f), &Ox);
			Mesh(Hs, CylMesh, M(Lx, Ly, Zb + 1.8f + 24.4f), FRotator::ZeroRotator, FVector(6.6f, 6.6f, 0.8f), &Iron);
			Mesh(Hs, CylMesh, M(Lx, Ly, Zb + 1.8f + 26.f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 2.4f), &Glass);
			Mesh(Hs, ConeMesh, M(Lx, Ly, Zb + 1.8f + 28.9f), FRotator::ZeroRotator, FVector(3.4f, 3.4f, 3.4f), &Iron);
			Lamp(Hs, M(Lx, Ly, Zb + 1.8f + 26.f), FLinearColor(1.f, 0.78f, 0.45f), 60000.f, 60000.f);
			if (BulbISM) BulbISM->AddInstance(FTransform(FRotator::ZeroRotator, M(Lx, Ly, Zb + 1.8f + 26.f), FVector(2.f)));
			Lamp(Hs, M(Lx + 6.f, Ly, Zb + 3.f), FLinearColor(1.f, 0.72f, 0.4f), 9000.f, 3500.f);
			IronwakeWorld::Place(this, Hs, TEXT("TownFisherCottage"), M(Lx + 14.f, Ly + 8.f, Zb + 0.5f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true, false);
			if (UIronwakeColl* Co = UIronwakeColl::Get(GetWorld()))
			{ const FTransform Ht2 = Hs->GetComponentTransform(); const FVector Wc = Ht2.TransformPosition(M(Lx, Ly, 0.f)); Co->AddObb(FVector2D(Wc.X, Wc.Y), Ht2.Rotator().Yaw, FVector2D(750.f, 750.f), Wc.Z - 3000.f, Wc.Z + 6000.f, true, TEXT("lighthouse")); }
		}
		// wind-bent coastal broadleaf trees (lean downwind +Y) in the cluster gaps, along the town flanks, and denser along the upper ridge (P28)
		{
			int32 Nt = 0; FRandomStream Rt(2424);
			for (int32 k = 0; k < 11000 && Nt < 280; ++k)
			{
				const float X = Rt.FRandRange(-100.f, -315.f), Y = Rt.FRandRange(-185.f, 225.f);
				const float Z = Hill.Final(X, Y);
				if (Z < 3.5f || Hill.RoadDist(X, Y) < 3.f || Hill.PadDist(X, Y) < 4.2f || Hill.SlopeDeg(X, Y) > 31.f) continue;
				const float MinNz = (X < -195.f) ? 0.24f : 0.34f;
				if (Hill.Noise(X / 20.f, Y / 20.f, 227, 2) < MinNz) continue;
				const TCHAR* Tn[] = { TEXT("IslandTree01"), TEXT("IslandTree02"), TEXT("IslandTree03") };
				bool bNearH = false; for (const FP23House& H : Plan23.Houses) if (FVector2D::DistSquared(FVector2D(X, Y), FVector2D(H.X, H.Y)) < FMath::Square(0.6f * FMath::Max(H.W, H.D) + 2.f)) { bNearH = true; break; }
				if (bNearH) continue;
				Veg(Tn[Rt.RandRange(0, 2)], FVector(X, Y, Z - 0.15f), Rt.FRandRange(0.f, 360.f), Rt.FRandRange(1.3f, 2.15f), 0.f, 0.f, Rt.FRandRange(0.75f, 0.92f), Rt.FRandRange(10.f, 20.f)); ++Nt;
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP24 wind-bent trees: %d"), Nt);
		}
		// quay-front: tidal wet band on the quay wall faces + timber fender piles; 3 stilt huts in the cove / flank shallows
		{
			const FLinearColor Wet(0.012f, 0.018f, 0.016f), Pile(0.05f, 0.035f, 0.025f);
			int32 Nf = 0;
			for (float Y = -134.f; Y < 164.f; Y += 8.f)
			{
				if (Y > 58.f && Y < 116.f) continue;
				if (FMath::Abs(Y + 72.f) < 5.f || FMath::Abs(Y - 122.f) < 5.f) continue;   // jetty roots
				if (FMath::Fmod(FMath::Abs(Y), 16.f) < 8.f) Mesh(Hs, CylMesh, M(-50.35f, Y, 0.f), FRotator::ZeroRotator, FVector(0.42f, 0.42f, 6.2f), &Pile), ++Nf;
			}
			Mesh(Hs, CubeMesh, M(-49.95f, -135.f + (58.f + 135.f) * 0.5f - 0.f, 0.55f), FRotator::ZeroRotator, FVector(0.12f, 193.f, 2.1f), &Wet);
			Mesh(Hs, CubeMesh, M(-49.95f, 116.f + 24.f, 0.55f), FRotator::ZeroRotator, FVector(0.12f, 48.f, 2.1f), &Wet);
			struct FSt { float X, Y, Yaw; const TCHAR* N; }; static const FSt St[] = { { -62.f, 62.f, 0.f, TEXT("TownFisherCottage") }, { -80.f, 114.f, 0.f, TEXT("TownBoathouse") }, { -92.f, -150.f, 0.f, TEXT("TownFisherCottage") } };
			int32 Ns = 0;
			for (const FSt& S2 : St)
			{
				const float Zg2 = Hill.Final(S2.X, S2.Y);
				if (Zg2 > 1.f) continue;   // only where the ground is in the shallows
				const float Zp = Q + 0.6f;
				Block(S2.X - 5.f, S2.X + 5.f, S2.Y - 5.f, S2.Y + 5.f, Zp - 0.3f, Zp, TEXT("MI_IW_Deck"));
				for (int32 a = -1; a <= 1; a += 2) for (int32 b = -1; b <= 1; b += 2) Mesh(Hs, CylMesh, M(S2.X + a * 4.6f, S2.Y + b * 4.6f, Zp - 3.f), FRotator::ZeroRotator, FVector(0.4f, 0.4f, 6.f), &Pile);
				IronwakeWorld::Place(this, Hs, S2.N, M(S2.X, S2.Y, Zp), FRotator(0.f, S2.Yaw, 0.f), FVector(0.9f), true, false); ++Ns;
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP24 quay variety: fender piles %d, stilt huts %d"), Nf, Ns);
		}
	}
	// ---- quay props: dense along the front (crates, barrels, carts, bollards), crane already placed by BuildHarbour
	{
		const auto Prop = [&](const TCHAR* N, float X, float Y, float Yaw, float S = 1.f) { return IronwakeWorld::Place(this, Hs, N, M(X, Y, Q + 0.2f), FRotator(0.f, Yaw, 0.f), FVector(S), true, false); };
		const TCHAR* Names[] = { TEXT("TownCrateStack"), TEXT("WoodenBarrels"), TEXT("WoodenBarrels"), TEXT("TownHandcart"), TEXT("MilitaryCrate"), TEXT("TownCrateStack"), TEXT("MilitaryCrate") };
		for (float Y = -62.f; Y < 126.f; Y += Rg.FRandRange(4.5f, 8.f))
		{
			if (FMath::Abs(Y - 48.f) < 14.f) continue;      // customs house forecourt
			if (Y > 56.f && Y < 118.f) continue;            // P17: cove (beach + slipway), no quay props
			if (FMath::Abs(Y - GapY[0]) < 5.f) continue;
			const int32 Pi = Rg.RandRange(0, 6);
			const float X = Rg.FRandRange(-62.f, -56.f);
			Prop(Names[Pi], X, Y, Rg.FRandRange(-40.f, 40.f), (Pi == 1 || Pi == 2) ? 1.35f : 1.f);
		}
		for (float Y = -60.f; Y < 126.f; Y += 17.f) { if (Y > 56.f && Y < 118.f) continue; Prop(TEXT("BollardCapstan"), -51.4f, Y, 90.f); }
		for (float Y = -70.f; Y < 126.f; Y += 22.f) { if (Y + 9.f > -92.f && Y + 9.f < -34.f) continue; if (Y + 9.f > 56.f && Y + 9.f < 118.f) continue; Prop(TEXT("TownCrateStack"), -91.f, Y + 9.f, Rg.FRandRange(0.f, 90.f)); }
	}
	// ---- street lamps: along the terrace streets and the quay (warm point light + emissive bulb at night)
	{
		const auto Lamp1 = [&](float X, float Y, float Z, float Cd = 1100.f, float Bulb = 0.2f)
		{
			IronwakeWorld::Place(this, Hs, TEXT("GasLamp"), M(X, Y, Z), FRotator::ZeroRotator, FVector(1.f), false, false);
			Lamp(Hs, M(X, Y, Z + 4.4f), FLinearColor(1.f, 0.6f, 0.26f), Cd, 1800.f);
			if (BulbISM) BulbISM->AddInstance(FTransform(FRotator::ZeroRotator, M(X, Y, Z + 4.3f), FVector(Bulb)));
		};
		for (float Y = -70.f; Y <= 125.f; Y += 38.f) { if (Y > 56.f && Y < 118.f) continue; Lamp1(-75.f, Y, Q + 0.2f); }
		if (!bP23) for (int32 i = 0; i < NT; ++i)
			for (float Y = -60.f + 12.f * i; Y <= 125.f; Y += 40.f) Lamp1(T[i].X0 - 1.6f, Y, T[i].Z + 0.05f);
		if (bP23)
			for (int32 Ri = 0; Ri < NBase23; ++Ri)
			{
				const FIwRoad& Rd = Hill.Roads[Ri];
				for (int32 I = 8; I + 1 < Rd.P.Num(); I += (Ri == 0 ? 7 : 11))
				{
					const FVector2D Tg = (FVector2D(Rd.P[I + 1]) - FVector2D(Rd.P[I - 1])).GetSafeNormal(), Nl(-Tg.Y, Tg.X);
					{ const float Lx = Rd.P[I].X + Nl.X * (Rd.HalfW + 1.2f), Ly = Rd.P[I].Y + Nl.Y * (Rd.HalfW + 1.2f), Lz = Hill.Final(Lx, Ly); const FLinearColor Cob(0.5f, 0.48f, 0.44f); Mesh(Hs, CubeMesh, M(Lx, Ly, Lz - 0.2f), FRotator::ZeroRotator, FVector(0.9f, 0.9f, 0.7f), &Cob); Lamp1(Lx, Ly, Lz + 0.15f, 5200.f, 0.45f); }
				}
			}
	}
	// ---- P15 vegetation + rocks: scattered over the heightfield by its layer weights (grass slopes: shrubs/grass/few trees; scree and
	// outcrops: boulders; gardens behind each terrace: trees and shrubs). No scatter on roads, pads or the sea margin.
	{
		const auto Rock = [&](float X, float Y, float Z, float S)
		{ IronwakeWorld::Place(this, Hs, Rg.FRand() < 0.5f ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03"), M(X, Y, Z), FRotator(Rg.FRandRange(-8.f, 8.f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(-8.f, 8.f)), FVector(S, S * Rg.FRandRange(0.8f, 1.3f), S * Rg.FRandRange(0.7f, 1.3f)), false, false); };
		int32 NV = 0, NR = 0;
		for (int32 k = 0; k < 2600; ++k)
		{
			const float X = Rg.FRandRange(-440.f, -96.f), Y = Rg.FRandRange(-250.f, 300.f);
			const float Z = Hill.Final(X, Y);
			if (Z < 1.f || Hill.RoadDist(X, Y) < 2.0f || Hill.PadDist(X, Y) < 3.5f) continue;
			const float Sl = Hill.SlopeDeg(X, Y); const FLinearColor Wt = Hill.Weights(X, Y, Z, Sl);
			const float R = Rg.FRand();
			if (Wt.A > 0.45f || Wt.B > 0.5f) { if (R < 0.07f) { { const float Rs = Rg.FRandRange(1.4f, 3.4f); Rock(X, Y, Z - 0.5f - 1.6f * Rs, Rs); }; ++NR; } continue; }
			if (Sl > 30.f) continue;
			const bool bNearTown = X > -205.f;
			if (R < (bNearTown ? 0.10f : 0.07f) * Wt.R) { Veg(TEXT("FirTree01"), FVector(X, Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(0.9f, 1.6f)); ++NV; }
			else if (R < 0.45f * Wt.R + 0.05f) { Veg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(X, Y, Z - 0.1f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.3f, 2.8f)); ++NV; }
			else if (R < 0.55f * Wt.R + 0.08f) { Veg(TEXT("GrassMedium01"), FVector(X, Y, Z - 0.05f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(2.f, 3.5f)); ++NV; }
		}
		// gardens: a few trees and shrubs on the back strip of every terrace, and a couple of boulders on each bank
		if (!bP23) for (int32 i = 0; i < NT; ++i)
			for (int32 k = 0; k < 14; ++k)
			{
				const float Y = Rg.FRandRange(-84.f, 126.f);
				if (FMath::Abs(Y - GapY[i + 1 > 5 ? 5 : i + 1]) < 4.f) continue;
				const float X = T[i].X1 + Rg.FRandRange(0.5f, 2.2f);
				if (Rg.FRand() < 0.5f) Veg(i < 2 ? TEXT("IslandTree01") : TEXT("FirTree01"), FVector(X, Y, T[i].Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(0.5f, 0.85f) * (i < 2 ? 1.6f : 1.f));
				else Veg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(X, Y, T[i].Z - 0.1f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.4f, 2.6f));
			}
		// hedges/shrubs along the top edge of each retaining wall and boulders at the bank toe
		if (!bP23) for (int32 i = 0; i < NT; ++i)
			for (float Y = -88.f; Y < 132.f; Y += Rg.FRandRange(2.5f, 5.f))
			{
				if (FMath::Abs(Y - GapY[i]) < 5.5f) continue;
				const float Zt = (i == 0 ? Q + 0.12f : T[i - 1].Z);
				Veg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(T[i].X0 + 1.4f, Y, T[i].Z - 0.1f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.0f, 1.8f)); (void)Zt;
			}
		// boulders along both town flanks (blend the heightfield into the cliff scans)
		for (int32 k = 0; k < 46; ++k)
		{
			const int32 Sd = k & 1; const float X = -96.f - 8.f * (k >> 1), Y = (Sd ? 155.f : -105.f) + (Sd ? 1.f : -1.f) * Rg.FRandRange(0.f, 12.f);
			const float Z = Hill.Final(X, Y); if (Z > 0.5f) { const float Rs = Rg.FRandRange(2.6f, 4.8f); Rock(X, Y, Z - 0.6f - 1.0f * Rs, Rs); };
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP15 scatter: %d veg, %d rocks"), NV, NR);
	}

	// ======================================================================== P17: seaward edge, jetties, slipway, batch-2 models, shrub belt
	{
		const FLinearColor Iron(0.03f, 0.03f, 0.035f), Pale(0.62f, 0.60f, 0.54f);
		UIronwakeColl* Co = UIronwakeColl::Get(GetWorld());
		const FTransform Ht = Hs->GetComponentTransform();
		const auto Solid = [&](float Xc, float Yc, float Hx, float Hy, const TCHAR* Tag)
		{ if (Co) { const FVector Wc = Ht.TransformPosition(M(Xc, Yc, 0.f)); Co->AddObb(FVector2D(Wc.X, Wc.Y), Ht.Rotator().Yaw, FVector2D(Hx * 100.f, Hy * 100.f), Wc.Z - 3000.f, Wc.Z + 6000.f, false, Tag); } };
		// two short stone jetties (rubble core, setts deck, iron bollards, ladders on the end face)
		const auto Jetty = [&](float Yc, float X1)
		{
			const float X0 = -50.f, W = 3.2f;
			Block(X0, X1, Yc - W, Yc + W, -4.5f, Q + 0.1f, TEXT("MI_IW_Cobble"));
			Block(X0, X1, Yc - W - 0.3f, Yc + W + 0.3f, Q - 0.2f, Q + 0.35f, TEXT("MI_IW_Setts"));       // coping
			Block(X0, X1, Yc - W + 0.4f, Yc + W - 0.4f, Q + 0.1f, Q + 0.2f, TEXT("MI_IW_Deck"));
			for (float X = X0 + 6.f; X < X1 - 2.f; X += 11.f) for (int32 Sd = -1; Sd <= 1; Sd += 2) IronwakeWorld::Place(this, Hs, TEXT("BollardCapstan"), M(X, Yc + Sd * (W - 0.5f), Q + 0.2f), FRotator(0.f, 90.f, 0.f), FVector(1.f), true, false);
			for (int32 Sd = -1; Sd <= 1; Sd += 2) for (float Dy = -0.3f; Dy <= 0.3f; Dy += 0.6f) Block(X1 + 0.02f, X1 + 0.14f, Yc + Sd * 1.4f + Dy - 0.05f, Yc + Sd * 1.4f + Dy + 0.05f, -2.f, Q + 0.9f, TEXT("MI_IW_Deck"));
			Mesh(Hs, CubeMesh, M((X0 + X1) * 0.5f, Yc, 2.f), FRotator::ZeroRotator, FVector(0.01f), &Iron);
			Solid((X0 + X1) * 0.5f, Yc, (X1 - X0) * 0.5f + 0.5f, W + 0.5f, TEXT("jetty"));
		};
		Jetty(-72.f, -6.f);
		Jetty(122.f, 2.f);
		// quay ladders on the straight wall stretches (vertical iron rails + rungs), every ~35 m
		for (float Y = -120.f; Y < 55.f; Y += 34.f)
		{
			if (FMath::Abs(Y + 72.f) < 6.f || FMath::Abs(Y) < 5.f) continue;
			for (float Dy = -0.3f; Dy <= 0.3f; Dy += 0.6f) Mesh(Hs, CubeMesh, M(-49.98f, Y + Dy, Q - 1.2f), FRotator::ZeroRotator, FVector(0.1f, 0.1f, 3.6f), &Iron);
			for (float Z = -1.4f; Z < Q + 0.8f; Z += 0.5f) Mesh(Hs, CubeMesh, M(-49.95f, Y, Z), FRotator::ZeroRotator, FVector(0.1f, 0.7f, 0.07f), &Iron);
		}
		// slipway: stone ramp from the cove plaza down into the shallows, flanked by low stone walls; boathouse on the plaza
		{
			if (UStaticMeshComponent* R = Ramp(-30.f, -74.f, -2.4f, Q + 0.1f, 84.f, 94.f, 0.6f, TEXT("MI_IW_Cobble"))) (void)R;
			Block(-72.f, -34.f, 83.4f, 84.2f, -1.f, Q + 0.2f, TEXT("MI_IW_Setts"));
			Block(-72.f, -34.f, 93.8f, 94.6f, -1.f, Q + 0.2f, TEXT("MI_IW_Setts"));
			IronwakeWorld::Place(this, Hs, TEXT("TownBoathouse"), M(-83.f, 106.f, Q + 0.1f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true, false);
			IronwakeWorld::Place(this, Hs, TEXT("TownFisherCottage"), M(-83.f, 68.f, Q + 0.1f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true, false);
			IronwakeWorld::Place(this, Hs, TEXT("TownFishRacks"), M(-80.f, 119.f, Q + 0.1f), FRotator(0.f, 0.f, 0.f), FVector(1.f), true, false);
		}
		// boulders and a pale rubble toe along the cove edges and the flank shorelines
		{
			const auto Boulder = [&](float X, float Y, float S)
			{
				const float Z = Hill.Final(X, Y);
				IronwakeWorld::Place(this, Hs, Rg.FRand() < 0.5f ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03"), M(X, Y, Z - 0.5f - 1.6f * S), FRotator(Rg.FRandRange(-8.f, 8.f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(-8.f, 8.f)), FVector(S, S * Rg.FRandRange(0.8f, 1.3f), S * Rg.FRandRange(0.8f, 1.1f)), false, false);
			};
			int32 Nb = 0;
			for (float Y = -300.f; Y <= 330.f; Y += 7.f)
			{
				const bool bQuayStretch = Y > -134.f && Y < 164.f && !(Y > 58.f && Y < 116.f);
				if (bQuayStretch) continue;
				if (Y > -250.f && Y < 60.f ? false : false) continue;
				float Xs = 1e9f;
				for (float X = -160.f; X < -10.f; X += 1.f) { const float Z = Hill.Final(X, Y); if (Z < 0.2f) { Xs = X; break; } }
				if (Xs > 1e8f) continue;
				const int32 N = (Y > 56.f && Y < 118.f) ? 1 : 2;
				for (int32 k = 0; k < N; ++k) { Boulder(Xs - Rg.FRandRange(0.f, 2.f), Y + Rg.FRandRange(-2.5f, 2.5f), Rg.FRandRange(1.0f, 2.6f)); ++Nb; }
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP17 shore boulders: %d"), Nb);
		}
		// white-stone steps on every other main-road link between terraces (the road climbs 5.5 m over ~18 m, so steps show on the slope)
		{
			int32 Ns = 0;
			if (!bP23) for (int32 i = 0; i < NT - 1; i += 2)
			{
				const float Gy = GapY[i + 1], Xa = T[i].X0 - 4.f, Xb = T[i + 1].X0 - 0.3f;
				const int32 N = FMath::CeilToInt(FMath::Abs(Xb - Xa) / 1.5f);
				for (int32 k = 0; k < N; ++k)
				{
					const float X = Xa + (Xb - Xa) * (k + 0.5f) / N;
					const float Zt = Hill.Final(X, Gy) + 0.06f;
					Mesh(Hs, CubeMesh, M(X, Gy, Zt - 0.12f), FRotator::ZeroRotator, FVector(1.55f, 5.4f, 0.3f), &Pale); ++Ns;
				}
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP17 white steps: %d"), Ns);
		}
		// batch-2 models + P28 busy quay apron HISM clutter (crates, barrels, timber baulk piles, coal heaps, mooring hawsers) & dockworker groups (350 m cull)
		{
			const auto Pl = [&](const TCHAR* N, float X, float Y, float Z, float Yaw) { return IronwakeWorld::Place(this, Hs, N, M(X, Y, Z), FRotator(0.f, Yaw, 0.f), FVector(1.f), true, false); };
			Pl(TEXT("TownWaterPump"), -70.5f, 24.5f, Q + 0.15f, 0.f);
			Pl(TEXT("TownMarketHall"), -80.f, -56.f, Q + 0.15f, 0.f);
			Pl(TEXT("TownFishRacks"), -56.f, -82.f, Q + 0.15f, 0.f);
			Pl(TEXT("TownFishRacks"), -56.f, -122.f, Q + 0.15f, 180.f);
			Pl(TEXT("TownHorseWagon"), -63.f, -18.f, Q + 0.15f, 25.f);

			// P28: HISM cargo clutter, timber piles, coal heaps, mooring hawsers, and SailorCrewStand dockhands (tight 350 m cull distance!)
			auto MakeWorldHism = [&](const TCHAR* MeshName, int32 StartCullCm, int32 EndCullCm) -> UHierarchicalInstancedStaticMeshComponent*
			{
				UStaticMesh* Sm = IronwakeWorld::Mesh(MeshName);
				if (!Sm) return nullptr;
				UHierarchicalInstancedStaticMeshComponent* Hi = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
				Hi->SetStaticMesh(Sm); Hi->SetupAttachment(Hs);
				Hi->SetCollisionEnabled(ECollisionEnabled::NoCollision); Hi->SetGenerateOverlapEvents(false);
				Hi->SetCastShadow(EndCullCm <= 40000);
				Hi->SetCullDistances(StartCullCm, EndCullCm);
				Hi->RegisterComponent(); AddInstanceComponent(Hi);
				return Hi;
			};
			UHierarchicalInstancedStaticMeshComponent* CrewHism = MakeWorldHism(TEXT("SailorCrewStand"), 28000, 35000);
			UHierarchicalInstancedStaticMeshComponent* CrateHism = MakeWorldHism(TEXT("TownCrateStack"), 50000, 75000);
			UHierarchicalInstancedStaticMeshComponent* MilCrateHism = MakeWorldHism(TEXT("MilitaryCrate"), 50000, 75000);
			UHierarchicalInstancedStaticMeshComponent* BarrelGrpHism = MakeWorldHism(TEXT("WoodenBarrels"), 50000, 75000);

			// Sawn timber piles + anthracite coal heaps (ISM primitives, tight cull; mooring hawsers live on AIronwakeNpcShip ONLY when a ship is docked!)
			auto MakePrimIsm = [&](UStaticMesh* Sm, const FLinearColor& Col, int32 EndCullCm) -> UInstancedStaticMeshComponent*
			{
				UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(this);
				I->SetStaticMesh(Sm); I->SetupAttachment(Hs);
				I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
				I->SetCullDistances((int32)(EndCullCm * 0.78f), EndCullCm);
				if (UMaterialInstanceDynamic* Mi = VTintP12(this, Col)) I->SetMaterial(0, Mi);
				I->RegisterComponent(); AddInstanceComponent(I);
				return I;
			};
			UInstancedStaticMeshComponent* TimberIsm = MakePrimIsm(CubeMesh, FLinearColor(0.32f, 0.19f, 0.085f), 75000);
			UInstancedStaticMeshComponent* CoalIsm   = MakePrimIsm(SphereMesh, FLinearColor(0.016f, 0.016f, 0.019f), 80000);

			auto AddTimberPile = [&](float Xm, float Ym, float YawDeg)
			{
				if (!TimberIsm) return;
				const FQuat Qr(FRotator(0.f, YawDeg, 0.f));
				for (int32 tier = 0; tier < 3; ++tier)
					for (int32 col = -2; col <= 2; ++col)
					{
						const FVector Off = Qr.RotateVector(M(0.f, col * 0.34f, 0.24f + tier * 0.26f));
						TimberIsm->AddInstance(FTransform(Qr, M(Xm, Ym, Q + 0.17f) + Off, FVector(4.4f - 0.25f * tier, 0.30f, 0.22f)));
					}
			};
			auto AddCoalHeap = [&](float Xm, float Ym, float ScaleM)
			{
				if (!CoalIsm) return;
				CoalIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Xm, Ym, Q - 0.15f * ScaleM), FVector(ScaleM * 1.45f, ScaleM * 1.15f, ScaleM * 0.58f)));
				CoalIsm->AddInstance(FTransform(FRotator(0.f, 35.f, 0.f), M(Xm - 0.9f * ScaleM, Ym + 0.7f * ScaleM, Q - 0.18f * ScaleM), FVector(ScaleM * 0.95f, ScaleM * 0.85f, ScaleM * 0.44f)));
			};
			auto AddCrewGroup = [&](float Xm, float Ym, float Zm, int32 Count, float SeedYaw)
			{
				if (!CrewHism) return;
				for (int32 c = 0; c < Count; ++c)
				{
					const float Ang = FMath::DegreesToRadians(SeedYaw + c * (360.f / FMath::Max(Count, 1)));
					const float R = (Count == 1) ? 0.f : 0.82f;
					const float Cx = Xm + FMath::Cos(Ang) * R, Cy = Ym + FMath::Sin(Ang) * R;
					const float FaceYaw = (Count == 1) ? SeedYaw : (FMath::RadiansToDegrees(Ang) + 180.f);
					CrewHism->AddInstance(FTransform(FRotator(0.f, FaceYaw, 0.f), M(Cx, Cy, Zm), FVector(0.96f + 0.04f * (c % 3))));
				}
			};

			// Populate south & north quay aprons with cargo stacks, timber baulk piles & coal heaps (no static empty-water ropes!)
			AddTimberPile(-56.5f, -96.f, 88.f); AddTimberPile(-58.5f, -68.f, 4.f);
			AddTimberPile(-56.2f,  15.f, 92.f); AddTimberPile(-57.0f, 128.f, 86.f);
			AddCoalHeap(-59.5f, 134.f, 3.2f);   AddCoalHeap(-58.0f, 147.f, 2.7f);
			AddCoalHeap(-55.5f, -43.f, 2.2f); // coal lighter berth heap

			static const float BerthYs[] = { -129.f, -109.f, -89.f, -42.f, 25.f, 46.f, 139.f };
			for (int32 bi = 0; bi < UE_ARRAY_COUNT(BerthYs); ++bi)
			{
				const float By = BerthYs[bi];
				IronwakeWorld::Place(this, Hs, TEXT("BollardCapstan"), M(-51.3f, By - 4.5f, Q + 0.19f), FRotator(0.f, 90.f, 0.f), FVector(1.f), false, false);
				IronwakeWorld::Place(this, Hs, TEXT("BollardCapstan"), M(-51.3f, By + 4.5f, Q + 0.19f), FRotator(0.f, 90.f, 0.f), FVector(1.f), false, false);
				if (CrateHism) CrateHism->AddInstance(FTransform(FRotator(0.f, 27.f * bi + 11.f, 0.f), M(-55.4f, By - 2.4f, Q + 0.18f), FVector(1.08f)));
				if (BarrelGrpHism) BarrelGrpHism->AddInstance(FTransform(FRotator(0.f, 43.f * bi + 7.f, 0.f), M(-54.9f, By + 2.5f, Q + 0.18f), FVector(1.30f)));
				if (MilCrateHism)
				{
					MilCrateHism->AddInstance(FTransform(FRotator(0.f, 19.f * bi + 6.f, 0.f), M(-57.3f, By, Q + 0.18f), FVector(1.22f)));
					MilCrateHism->AddInstance(FTransform(FRotator(0.f, 37.f * bi + 19.f, 0.f), M(-57.4f, By + 0.2f, Q + 0.72f), FVector(1.12f)));
				}
			}
			// Weathered wooden oak casks & crates outside Anchor Tavern & Ship Chandlery (zero modern blue drums!)
			if (BarrelGrpHism)
			{
				BarrelGrpHism->AddInstance(FTransform(FRotator(0.f, 22.f, 0.f), M(-78.6f, 27.8f, Q + 0.17f), FVector(1.22f)));
				BarrelGrpHism->AddInstance(FTransform(FRotator(0.f, 65.f, 0.f), M(-79.0f, -23.3f, Q + 0.17f), FVector(1.18f)));
			}
			if (CrateHism)
			{
				CrateHism->AddInstance(FTransform(FRotator(0.f, -14.f, 0.f), M(-78.9f, 33.4f, Q + 0.17f), FVector(0.88f)));
				CrateHism->AddInstance(FTransform(FRotator(0.f, 18.f, 0.f), M(-79.1f, -29.2f, Q + 0.17f), FVector(0.92f)));
			}
			// Dockhands & mariners conversing along the quay, outside the Anchor Tavern & Ship Chandlery, and by the customs house
			AddCrewGroup(-77.8f,  31.2f, Q + 0.18f, 3, 25.f);  // Anchor Tavern patrons
			AddCrewGroup(-78.0f, -25.8f, Q + 0.18f, 2, 70.f);  // Ship Chandlery customers
			AddCrewGroup(-62.5f,  46.0f, Q + 0.18f, 2, 10.f);  // Customs House steps
			AddCrewGroup(-53.2f,  19.8f, Q + 0.18f, 3, 45.f);  // N1 promenade stevedores (unobstructed in front of crates)
			AddCrewGroup(-53.2f,  32.5f, Q + 0.18f, 2, -20.f); // N2 promenade dockhands
			AddCrewGroup(-53.5f, -41.5f, Q + 0.18f, 2, 110.f); // Coal lighter gang
			AddCrewGroup(-54.0f, -88.5f, Q + 0.18f, 3, 60.f);  // S1 berth cargo crew
			AddCrewGroup(-54.2f, -108.f, Q + 0.18f, 2, -35.f); // S2 berth riggers
			AddCrewGroup(-53.8f, 136.0f, Q + 0.18f, 3, 85.f);  // Coaling & salvage tender berth crew
			AddCrewGroup(-68.0f,  15.5f, Q + 0.18f, 2, 0.f);   // Harbour square watch
			UE_LOG(LogIronwake, Display, TEXT("IWP28 quay clutter built: dockhands=%d timber=%d coal=%d"),
				CrewHism ? CrewHism->GetInstanceCount() : 0, TimberIsm ? TimberIsm->GetInstanceCount() : 0,
				CoalIsm ? CoalIsm->GetInstanceCount() : 0);
			if (!bP23) Pl(TEXT("TownHorseWagon"), T[1].X0 - 1.9f, -40.f, T[1].Z + 0.1f, 90.f);
			const float Zc = Hill.Final(-338.f, 96.f);
			// P21: windmill with a separate sail cross (axle +X in the mesh frame, pivot 805,-9,1641 cm), turned by the NPC director with the wind
			if (UStaticMeshComponent* Wb = Pl(TEXT("WindmillBody"), -338.f, 96.f, Zc - 0.2f, 0.f))
			{
				UStaticMesh* Sm = IronwakeWorld::Mesh(TEXT("WindmillBody_sails"));
				USceneComponent* Pv = NewObject<USceneComponent>(this); Pv->SetupAttachment(Wb); Pv->SetRelativeLocation(FVector(805.f, -9.f, 1641.f)); Pv->RegisterComponent(); AddInstanceComponent(Pv);
				if (Sm) { UStaticMeshComponent* Sc = NewObject<UStaticMeshComponent>(this); Sc->SetStaticMesh(Sm); Sc->SetupAttachment(Pv); Sc->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sc->RegisterComponent(); AddInstanceComponent(Sc); }
				AIronwakeNpcDirector::Windmills.Add(Pv);
			}
			const float Zg = Hill.Final(-150.f, -144.f);
			Pl(TEXT("TownGasometer"), -150.f, -144.f, Zg - 0.1f, 0.f);
			// moored craft that ride the swell, one on each jetty
			const auto Moor = [&](const TCHAR* N, float X, float Y, float Keel, float Yaw)
			{
				if (UStaticMeshComponent* C = IronwakeWorld::Place(this, Hs, N, M(X, Y, Keel), FRotator(0.f, Yaw, 0.f)))
				{ Floaters.Add(C); FloaterBase.Add(FVector(Keel * 100.f, Yaw, Floaters.Num() * 1.9f)); }
			};
			if (FParse::Param(FCommandLine::Get(), TEXT("iwoldnpc")))   // P20: the NPC lighter and tender are real ships now
			{
				Solid(-22.f, -62.f, 15.5f, 6.5f, TEXT("lighter")); Solid(-14.f, 132.f, 14.5f, 6.5f, TEXT("tender"));
				Moor(TEXT("CoalLighter"), -22.f, -62.f, -1.1f, 0.f);
				Moor(TEXT("SalvageTender"), -14.f, 132.f, -1.3f, 180.f);
			}
		}
		// scattered fisher cottages / timber houses beyond the terrace ends and up the flanks so the town frays out instead of stopping at a rectangle
		{
			struct FSc { float X, Y, Yaw; bool Fish; };
			static const FSc Sc[] = { { -104.f, -150.f, 0.f, true }, { -112.f, -168.f, 20.f, true }, { -126.f, -186.f, 8.f, false }, { -150.f, -176.f, -10.f, true }, { -170.f, -150.f, 15.f, false },
				{ -118.f, 150.f, -5.f, true }, { -128.f, 170.f, 12.f, true }, { -146.f, 190.f, 0.f, false }, { -168.f, 168.f, -18.f, true }, { -196.f, 150.f, 5.f, false }, { -214.f, -120.f, 10.f, true }, { -230.f, 130.f, -8.f, true } };
			int32 Nc = 0;
			for (const FSc& C : Sc)
			{
				const float Hx = C.Fish ? 4.6f : 4.f;
				float Zmin = 1e9f, Zmax = -1e9f;
				for (int32 a = -1; a <= 1; a += 2) for (int32 b = -1; b <= 1; b += 2) { const float Z = Hill.Final(C.X + a * Hx, C.Y + b * Hx); Zmin = FMath::Min(Zmin, Z); Zmax = FMath::Max(Zmax, Z); }
				if (Zmin < 0.8f || Zmax - Zmin > 4.6f || Hill.RoadDist(C.X, C.Y) < 4.f || Hill.PadDist(C.X, C.Y) < 2.f) continue;
				const float Zb = Zmin + 0.6f * (Zmax - Zmin);
				if (Zb - Zmin > 0.2f) Block(C.X - Hx, C.X + Hx, C.Y - Hx, C.Y + Hx, Zmin - 1.5f, Zb + 0.15f, TEXT("MI_IW_Cobble"));
				UStaticMeshComponent* Ch = IronwakeWorld::Place(this, Hs, C.Fish ? TEXT("TownFisherCottage") : TEXT("TownTimberHouse"), M(C.X, C.Y, Zb + 0.1f), FRotator(0.f, -90.f + C.Yaw + (C.Fish ? 90.f : 0.f), 0.f), FVector(1.f), true, false);
				if (Ch && !C.Fish) IronwakeWorld::TintAll(Ch, Tints[Nc % 7]);
				++Nc;
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP17 fringe houses: %d"), Nc);
		}
		// wind-bent shrub belt below the crest and along the upper flanks: leans downwind (+Y), flattened
		{
			int32 Nw = 0;
			FRandomStream Rb(1717);
			for (int32 k = 0; k < 900 && Nw < 380; ++k)
			{
				const float X = Rb.FRandRange(-345.f, -225.f), Y = Rb.FRandRange(-260.f, 320.f);
				const float Z = Hill.Final(X, Y);
				if (Hill.RoadDist(X, Y) < 2.5f || Hill.SlopeDeg(X, Y) > 32.f) continue;
				const float Dens = Hill.Noise(X / 24.f, Y / 24.f, 81, 2);
				if (Dens < 0.45f) continue;
				Veg(Rb.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(X, Y, Z - 0.1f), 90.f + Rb.FRandRange(-25.f, 25.f), Rb.FRandRange(1.6f, 3.4f), Rb.FRandRange(6.f, 14.f), 0.f, 0.65f); ++Nw;
			}
			// extra big outcrops on the rock-weighted upper slope
			int32 Nr = 0;
			for (int32 k = 0; k < 1500 && Nr < 46; ++k)
			{
				const float X = Rb.FRandRange(-330.f, -215.f), Y = Rb.FRandRange(-250.f, 300.f);
				const float Z = Hill.Final(X, Y);
				if (Hill.RoadDist(X, Y) < 4.f || Hill.PadDist(X, Y) < 3.f) continue;
				if (Hill.Weights(X, Y, Z, Hill.SlopeDeg(X, Y)).A < 0.35f) continue;
				const float Sc = Rb.FRandRange(3.5f, 7.f);
				IronwakeWorld::Place(this, Hs, Rb.FRand() < 0.5f ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03"), M(X, Y, Z - 1.2f - 1.6f * Sc), FRotator(Rb.FRandRange(-10.f, 10.f), Rb.FRandRange(0.f, 360.f), Rb.FRandRange(-10.f, 10.f)), FVector(Sc, Sc * Rb.FRandRange(0.8f, 1.3f), Sc * Rb.FRandRange(0.7f, 1.0f)), false, false); ++Nr;
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP17 shrub belt %d, outcrops %d"), Nw, Nr);
		}
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP12 town built: %d houses, %d lit windows, %d chimneys, %d smoking, %d trees/shrubs types=%d"), NHouses, NWin, NChim, NSmoke, 0, VegIsm.Num());
}

// ---------------------------------------------------------------------------- P12 capture director
// -iwp12=shots|day|night [-iwp12quit]: unobstructed sea cameras (photo cam 8 m over the water) at 400 m / 900 m from the quay, day and night, plus chase views.
#include "IronwakePlayerController.h"
#include "IronwakeShipPawn.h"
#include "IronwakeP8.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
void AIronwakeVoyage::TickP12Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	P12T += RealDt;
	const float Since = P12T - P12Mark;
	const FVector Q0 = HarbourC;   // quay-edge centre; harbour frame +X = world yaw 45
	const FVector Town = HarbourC + FRotator(0.f, 45.f, 0.f).RotateVector(FVector(-14000.f, 2000.f, 0.f));
	struct FStep { const TCHAR* Shot; float Hour; int32 Wx; float DistM; float YawOff; float EyeM; float AimM; float Fov; bool bChase; bool bHf = false; FVector He = FVector::ZeroVector, Ha = FVector::ZeroVector; };
	static const FStep All[] = {
		{ TEXT("P12_day_400"), 9.4f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P12_day_900"), 9.4f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P12_day_400b"), 9.4f, 0, 400.f, 28.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P12_day_700s"), 9.4f, 0, 700.f, -25.f, 8.f, 14.f, 55.f, false },
		{ TEXT("P12_day_chase"), 9.4f, 0, 450.f, 8.f, 0.f, 0.f, 0.f, true },
		{ TEXT("P12_night_400"), 22.5f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P12_night_900"), 22.5f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P12_night_chase"), 22.5f, 0, 450.f, 8.f, 0.f, 0.f, 0.f, true },
		{ TEXT("P12_dusk_400"), 17.3f, 1, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P12_close_a"), 9.4f, 0, 150.f, 18.f, 14.f, 14.f, 60.f, false },
		{ TEXT("P12_top"), 9.4f, 0, 260.f, 0.f, 150.f, 8.f, 70.f, false },
		{ TEXT("P15_noon_400"), 12.f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P15_noon_900"), 12.f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P15_dusk_400"), 17.3f, 1, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P15_night_400"), 22.5f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P15_top"), 12.f, 0, 300.f, 0.f, 170.f, 20.f, 75.f, false },
		{ TEXT("P15_street"), 12.f, 0, -72.f, 0.f, 5.f, 18.f, 75.f, false },
		{ TEXT("P15_flank"), 12.f, 0, 330.f, -50.f, 30.f, 20.f, 60.f, false },
		{ TEXT("P15_crest"), 12.f, 0, 120.f, 20.f, 40.f, 40.f, 70.f, false },
		{ TEXT("P17_noon_400"), 12.f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P17_noon_900"), 12.f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P17_dusk_400"), 17.3f, 1, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P17_night_400"), 22.5f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P17_top"), 12.f, 0, 300.f, 0.f, 170.f, 20.f, 75.f, false },
		{ TEXT("P17_street"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 75.f, false, true, FVector(-78.f, 18.f, 5.4f), FVector(-200.f, 30.f, 26.f) },
		{ TEXT("P17_flank"), 12.f, 0, 330.f, -50.f, 30.f, 20.f, 60.f, false },
		{ TEXT("P17_cove"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 65.f, false, true, FVector(10.f, 86.f, 7.f), FVector(-90.f, 92.f, 7.f) },
		{ TEXT("P17_quay"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 70.f, false, true, FVector(110.f, -60.f, 9.f), FVector(-70.f, -62.f, 6.f) },
		{ TEXT("P17_noon_150"), 12.f, 0, 150.f, 18.f, 14.f, 14.f, 60.f, false },
		{ TEXT("P23_noon_400"), 12.f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P23_noon_900"), 12.f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P23_dusk_400"), 17.3f, 1, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P23_night_400"), 22.5f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P23_top"), 12.f, 0, 300.f, 0.f, 170.f, 20.f, 75.f, false },
		{ TEXT("P23_street"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 75.f, false, true, FVector(-92.f, 14.f, 6.f), FVector(-140.f, 60.f, 12.f) },
		{ TEXT("P23_flank"), 12.f, 0, 330.f, -50.f, 30.f, 20.f, 60.f, false },
		{ TEXT("P23_cove"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 65.f, false, true, FVector(10.f, 86.f, 7.f), FVector(-90.f, 92.f, 7.f) },
		{ TEXT("P23_quay"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 70.f, false, true, FVector(110.f, -60.f, 9.f), FVector(-70.f, -62.f, 6.f) },
		{ TEXT("P23_noon_150"), 12.f, 0, 150.f, 18.f, 14.f, 14.f, 60.f, false },
		{ TEXT("P23_mouth"), 12.f, 0, 0.f, 0.f, 0.f, 0.f, 70.f, false, true, FVector(260.f, -60.f, 14.f), FVector(-120.f, 20.f, 25.f) },
		{ TEXT("P24_noon_400"), 12.f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P24_noon_900"), 12.f, 0, 900.f, 0.f, 8.f, 14.f, 50.f, false },
		{ TEXT("P24_dusk_400"), 17.3f, 1, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P24_night_400"), 22.5f, 0, 400.f, 0.f, 8.f, 14.f, 60.f, false },
		{ TEXT("P24_top"), 12.f, 0, 300.f, 0.f, 170.f, 20.f, 75.f, false },
		{ TEXT("P24_street"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 75.f, false, true, FVector(-92.f, 14.f, 6.f), FVector(-140.f, 60.f, 12.f) },
		{ TEXT("P24_street_dusk"), 17.6f, 1, 400.f, 0.f, 0.f, 0.f, 75.f, false, true, FVector(-92.f, 14.f, 6.f), FVector(-140.f, 60.f, 12.f) },
		{ TEXT("P24_flank"), 12.f, 0, 330.f, -50.f, 30.f, 20.f, 60.f, false },
		{ TEXT("P24_cove"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 65.f, false, true, FVector(10.f, 86.f, 7.f), FVector(-90.f, 92.f, 7.f) },
		{ TEXT("P24_quay"), 12.f, 0, 400.f, 0.f, 0.f, 0.f, 70.f, false, true, FVector(110.f, -60.f, 9.f), FVector(-70.f, -62.f, 6.f) },
		{ TEXT("P24_noon_150"), 12.f, 0, 150.f, 18.f, 14.f, 14.f, 60.f, false },
		{ TEXT("P24_mouth"), 12.f, 0, 0.f, 0.f, 0.f, 0.f, 70.f, false, true, FVector(260.f, -60.f, 14.f), FVector(-120.f, 20.f, 25.f) },
	};
	int32 NS = UE_ARRAY_COUNT(All);
	int32 First = 1;
	if (P12Mode == TEXT("day")) NS = 4;
	if (P12Mode == TEXT("night")) { First = 6; NS = 8; }
	if (P12Mode == TEXT("p15")) { First = 12; NS = 19; }
	if (P12Mode == TEXT("p17")) { First = 20; NS = 30; }
	if (P12Mode == TEXT("p23")) { First = 30; NS = 40; }
	if (P12Mode == TEXT("p24")) { First = 41; NS = 52; }
	if (P12Mode == TEXT("bench"))
	{
		if (P12Step == 0 && P12T >= 3.f) { if (!C->bStarted) C->StartGame(true); bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(12.f); ForceWeatherState(1, 0.05f); P12Step = 1; P12Mark = P12T;
			const FVector D = FRotator(0.f, 53.f, 0.f).Vector(); const FVector L = FVector(Town.X, Town.Y, 0.f) + D * 42000.f;
			P->SetActorLocationAndRotation(FVector(L.X, L.Y, P->GetActorLocation().Z), FRotator(0.f, 233.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics); P->IWTelegraph(2); }
		if (P12Step == 1 && Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		return;
	}
	if (P12Mode == TEXT("coll"))
	{
		// P15: drive the frigate at the new hillside from 150 m out at several points; it must stop at the shore (log harbour-frame X,Y).
		struct FRun { float X, Y, HeadDeg; };   // start (harbour frame m), heading relative to harbour +X (180 = toward the hill)
		static const FRun Runs[] = { { 40.f, 60.f, 180.f }, { 40.f, -60.f, 180.f }, { 30.f, 210.f, 200.f }, { 30.f, -230.f, 160.f }, { 20.f, 330.f, 180.f }, { 20.f, -330.f, 180.f }, { 60.f, 120.f, 215.f } };
		const int32 NR = UE_ARRAY_COUNT(Runs);
		const FTransform HT(FRotator(0.f, 45.f, 0.f), FVector(HarbourC.X, HarbourC.Y, 0.f));
		if (P12Step == 0)
		{
			if (P12T < 3.f) return;
			if (!C->bStarted) C->StartGame(true);
			P12Step = 1; P12Mark = P12T; bP12Shot = false;
		}
		const int32 R = P12Step - 1;
		if (R < NR)
		{
			const FRun& Rn = Runs[R];
			if (!bP12Shot)
			{
				const FVector W = HT.TransformPosition(FVector(Rn.X * 100.f, Rn.Y * 100.f, 0.f));
				P->SetActorLocationAndRotation(FVector(W.X, W.Y, P->GetActorLocation().Z), FRotator(0.f, 45.f + Rn.HeadDeg, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
				if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
				P->IWRudder(0); P->IWTelegraph(4); bP12Shot = true; P12Mark = P12T;
				return;
			}
			if (Since > 70.f)
			{
				const FVector L = HT.InverseTransformPosition(P->GetActorLocation()) / 100.f;
				UE_LOG(LogIronwake, Display, TEXT("IWP15 COLL run %d start (%.0f,%.0f) head %.0f -> stopped at harbour X=%.1f Y=%.1f speed %.2f kn"), R, Rn.X, Rn.Y, Rn.HeadDeg, L.X, L.Y, P->GetVelocity().Size() / 51.4f);
				bP12Shot = false; ++P12Step; P12Mark = P12T;
			}
			return;
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP12 DONE"));
		P12Mode.Empty();
		UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		return;
	}
	auto Apply = [&](const FStep& S)
	{
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(S.Hour); SeaOverride = 0.f;
		ForceWeatherState(S.Wx, 0.05f);
		const FVector D = FRotator(0.f, 45.f + S.YawOff, 0.f).Vector();
		const FVector L = FVector(Q0.X, Q0.Y, 0.f) + D * (S.DistM + (S.bChase ? 0.f : 160.f)) * 100.f;
		P->SetActorLocationAndRotation(FVector(L.X, L.Y, P->GetActorLocation().Z), FRotator(0.f, 225.f + S.YawOff, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(2);
		P->ResetCameraToHorizon();
	};
	if (P12Step == 0)
	{
		if (P12T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		P12Step = First; P12Mark = P12T; Apply(All[First - 1]); return;
	}
	if (P12Step >= First && P12Step <= NS)
	{
		const FStep& S = All[P12Step - 1];
		if (Since > 2.f && Since < 2.3f && !S.bChase) { P->ResetCameraToHorizon(); }
		if (Since > 3.f && Since < 11.5f && !S.bChase)
		{
			if (!bPhoto) TogglePhoto();
			if (!bPhoto) TogglePhoto();
			const FVector D = FRotator(0.f, 45.f + S.YawOff, 0.f).Vector();
			const FTransform HfT(FRotator(0.f, 45.f, 0.f), FVector(Q0.X, Q0.Y, 0.f));
			const FVector Eye = S.bHf ? HfT.TransformPosition(S.He * 100.f) : FVector(Q0.X, Q0.Y, 0.f) + D * S.DistM * 100.f + FVector(0.f, 0.f, S.EyeM * 100.f);
			const FVector Aim = S.bHf ? HfT.TransformPosition(S.Ha * 100.f) : Town + FVector(0.f, 0.f, S.AimM * 100.f);
			SetPhotoView(Eye, (Aim - Eye).Rotation());
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(S.Fov);
		}
		if (Since >= 12.f && !bP12Shot)
		{
			C->IWShot(S.Shot); bP12Shot = true;
			UE_LOG(LogIronwake, Display, TEXT("IWP12 shot %s hour=%.1f wx=%s dist=%.0f lamp=%.2f"), S.Shot, ClockHour, *LiveWeatherName(), S.DistM, LiveLamp);
		}
		if (Since >= 13.5f)
		{
			bP12Shot = false; ++P12Step; P12Mark = P12T;
			if (bPhoto) TogglePhoto();
			if (P12Step <= NS) Apply(All[P12Step - 1]);
		}
		return;
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP12 DONE"));
	P12Mode.Empty();
	if (FParse::Param(FCommandLine::Get(), TEXT("iwp12quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
}

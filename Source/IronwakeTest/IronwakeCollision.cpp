#include "IronwakeCollision.h"
#include "IronwakeTest.h"
#include "IronwakeArchipelago.h"
#include "IronwakeHydro.h"
#include "IronwakeShell.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UIronwakeColl::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && W->IsGameWorld();
}

UIronwakeColl* UIronwakeColl::Get(const UWorld* W) { return W ? W->GetSubsystem<UIronwakeColl>() : nullptr; }

static FVector2D Rot2(const FVector2D& V, float YawDeg)
{
	const float S = FMath::Sin(FMath::DegreesToRadians(YawDeg)), C = FMath::Cos(FMath::DegreesToRadians(YawDeg));
	return FVector2D(V.X * C - V.Y * S, V.X * S + V.Y * C);
}

void UIronwakeColl::ToWorld(const TArray<FVector2D>& Local, const FVector2D& Pos, float YawDeg, TArray<FVector2D>& Out)
{
	const float S = FMath::Sin(FMath::DegreesToRadians(YawDeg)), C = FMath::Cos(FMath::DegreesToRadians(YawDeg));
	Out.SetNum(Local.Num(), EAllowShrinking::No);
	for (int32 i = 0; i < Local.Num(); ++i) Out[i] = Pos + FVector2D(Local[i].X * C - Local[i].Y * S, Local[i].X * S + Local[i].Y * C);
}

void UIronwakeColl::OutlineFromProfile(const FIronwakeHullProfile& P, TArray<FVector2D>& Out, float Pad)
{
	// Waterline outline: starboard side stern -> stem (HalfB evenly spaced), a pointed stem, then back along port.
	Out.Reset();
	const int32 N = P.HalfB.Num();
	if (N < 2) { OutlineRect(P.X0 * 100.f, P.X1 * 100.f, 1500.f, Out); return; }
	for (int32 i = 0; i < N; ++i)
	{
		const float X = FMath::Lerp(P.X0, P.X1, (float)i / (N - 1)) * 100.f;
		Out.Add(FVector2D(X, P.HalfB[i] * 100.f + Pad));
	}
	Out.Add(FVector2D(P.X1 * 100.f + Pad + 0.025f * P.Length() * 100.f, 0.f));   // bow flare / forecastle overhang
	for (int32 i = N - 1; i >= 0; --i)
	{
		const float X = FMath::Lerp(P.X0, P.X1, (float)i / (N - 1)) * 100.f;
		Out.Add(FVector2D(X, -(P.HalfB[i] * 100.f + Pad)));
	}
	Out.Add(FVector2D(P.X0 * 100.f - Pad, 0.f));
}

void UIronwakeColl::OutlineRect(float X0, float X1, float HalfB, TArray<FVector2D>& Out)
{
	Out.Reset();
	const float Shoulder = X1 - FMath::Min(HalfB * 2.5f, (X1 - X0) * 0.3f);
	Out.Add(FVector2D(X0, HalfB * 0.7f)); Out.Add(FVector2D(Shoulder, HalfB)); Out.Add(FVector2D(X1, 0.f));
	Out.Add(FVector2D(Shoulder, -HalfB)); Out.Add(FVector2D(X0, -HalfB * 0.7f));
}

void UIronwakeColl::Densify(const TArray<FVector2D>& Poly, float Spacing, TArray<FVector2D>& Out)
{
	Out.Reset();
	for (int32 i = 0; i < Poly.Num(); ++i)
	{
		const FVector2D A = Poly[i], B = Poly[(i + 1) % Poly.Num()];
		const int32 N = FMath::Max(1, FMath::CeilToInt((B - A).Size() / Spacing));
		for (int32 k = 0; k < N; ++k) Out.Add(FMath::Lerp(A, B, (float)k / N));
	}
}

bool UIronwakeColl::InPoly(const FVector2D& P, const TArray<FVector2D>& Poly)
{
	bool bIn = false;
	for (int32 i = 0, j = Poly.Num() - 1; i < Poly.Num(); j = i++)
	{
		if (((Poly[i].Y > P.Y) != (Poly[j].Y > P.Y)) && (P.X < (Poly[j].X - Poly[i].X) * (P.Y - Poly[i].Y) / (Poly[j].Y - Poly[i].Y) + Poly[i].X)) bIn = !bIn;
	}
	return bIn;
}

static float DistToPoly(const FVector2D& P, const TArray<FVector2D>& Poly, FVector2D* OutNearest = nullptr)
{
	float Best = 1e12f;
	for (int32 i = 0; i < Poly.Num(); ++i)
	{
		const FVector2D A = Poly[i], B = Poly[(i + 1) % Poly.Num()];
		const FVector2D AB = B - A;
		const float T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-3f), 0.f, 1.f);
		const FVector2D Q = A + AB * T;
		const float D = (P - Q).Size();
		if (D < Best) { Best = D; if (OutNearest) *OutNearest = Q; }
	}
	return Best;
}

void UIronwakeColl::AddObb(const FVector2D& Centre, float YawDeg, const FVector2D& Half, float ZMin, float ZMax, bool bEllipse, const FString& Tag)
{
	FObb O;
	O.C = Centre; O.U = Rot2(FVector2D(1, 0), YawDeg); O.Half = Half; O.ZMin = ZMin; O.ZMax = ZMax; O.bEllipse = bEllipse; O.Tag = Tag;
	O.R = Half.Size();
	Obbs.Add(O);
}

void UIronwakeColl::AddMeshBounds(const UStaticMesh* Mesh, const FTransform& T, const FString& Tag, bool bEllipse, float MinHalfCm)
{
	if (!Mesh) return;
	const FBoxSphereBounds B = Mesh->GetBounds();
	const FVector S = T.GetScale3D().GetAbs();
	const FRotator R = T.Rotator();
	FVector2D Half(B.BoxExtent.X * S.X, B.BoxExtent.Y * S.Y);
	const FVector Ctr = T.TransformPosition(B.Origin);
	float Yaw = R.Yaw;
	float ZMin, ZMax;
	if (FMath::Abs(R.Pitch) > 20.f || FMath::Abs(R.Roll) > 20.f)
	{
		// Heavily tilted: use the world AABB of the oriented box instead.
		const FBox WB = FBox(B.GetBox()).TransformBy(T);
		Half = FVector2D(WB.GetExtent().X, WB.GetExtent().Y); Yaw = 0.f;
		ZMin = WB.Min.Z; ZMax = WB.Max.Z;
	}
	else
	{
		const FBox WB = FBox(B.GetBox()).TransformBy(T);
		ZMin = WB.Min.Z; ZMax = WB.Max.Z;
	}
	if (FMath::Max(Half.X, Half.Y) < MinHalfCm) return;
	if (bEllipse) Half *= 1.18f;   // inscribed ellipse of a rock's bounds under-covers its lumps: grow it toward the box
	if (ZMax - ZMin > 1500.f && ZMax > SeaZ() + 1500.f)
	{
		// Tall rock/stack: the hull's flared bow and rails overhang the waterline outline, so use the full box plus a margin.
		bEllipse = false; Half += FVector2D(350.f, 350.f);
	}
	AddObb(FVector2D(Ctr.X, Ctr.Y), Yaw, Half, ZMin, ZMax, bEllipse, Tag);
}

void UIronwakeColl::AddComponent(UStaticMeshComponent* C, const FString& Tag, bool bEllipse, float MinHalfCm)
{
	if (!C || !C->GetStaticMesh()) return;
	FPending P; P.C = C; P.Tag = Tag; P.bEllipse = bEllipse; P.MinHalf = MinHalfCm;
	Pending.Add(P);
}

void UIronwakeColl::Bake() const
{
	if (Pending.Num() == 0) return;
	UIronwakeColl* Self = const_cast<UIronwakeColl*>(this);
	TArray<FPending> Work = MoveTemp(Self->Pending);
	for (const FPending& P : Work)
		if (UStaticMeshComponent* C = P.C.Get())
			Self->AddMeshBounds(C->GetStaticMesh(), C->GetComponentTransform(), P.Tag, P.bEllipse, P.MinHalf);
	UE_LOG(LogIronwake, Display, TEXT("IWP9 collision baked %d components -> %d solids total"), Work.Num(), Obbs.Num());
}

FIwShape UIronwakeColl::MakeShape(const TArray<FVector2D>& LocalPoly, float DraughtCm)
{
	FIwShape S; S.Poly = LocalPoly; S.DraughtCm = DraughtCm;
	Densify(LocalPoly, 300.f, S.Dense);
	for (const FVector2D& P : LocalPoly) S.R = FMath::Max(S.R, P.Size());
	return S;
}

void UIronwakeColl::RegisterHull(AActor* A, const FIwShape& Shape, bool bPawn)
{
	if (!A) return;
	UnregisterHull(A);
	FHullEnt H; H.A = A; H.Sh = Shape; H.bPawn = bPawn;
	Hulls.Add(H);
}

void UIronwakeColl::UnregisterHull(AActor* A)
{
	for (int32 i = Hulls.Num() - 1; i >= 0; --i) if (!Hulls[i].A.IsValid() || Hulls[i].A.Get() == A) Hulls.RemoveAtSwap(i);
}

FIwHit UIronwakeColl::TestPose(const AActor* Self, const FIwShape& S, const FVector2D& Pos, float YawDeg, uint8 Mask) const
{
	TArray<FVector2D> PW, DW;
	ToWorld(S.Poly, Pos, YawDeg, PW);
	ToWorld(S.Dense, Pos, YawDeg, DW);
	return TestHull(DW, PW, Pos, S.R, S.DraughtCm, Self, Mask);
}

float UIronwakeColl::MinDepthAt(const FIwShape& S, const FVector2D& Pos, float YawDeg) const
{
	float Best = 1e7f;
	TArray<FVector2D> DW;
	ToWorld(S.Dense, Pos, YawDeg, DW);
	for (const FVector2D& P : DW) Best = FMath::Min(Best, DepthAt(P));
	return Best;
}

float UIronwakeColl::SeaZ() const
{
	if (!bSea) { CachedSea = AIronwakeShell::WaterZAt(GetWorld(), FVector::ZeroVector); bSea = true; }
	return CachedSea;
}

AIronwakeArchipelago* UIronwakeColl::Archipelago() const
{
	if (!Arch.IsValid()) Arch = AIronwakeArchipelago::Get(GetWorld());
	return Arch.Get();
}

bool UIronwakeColl::SeabedAt(const FVector2D& P, float& OutH) const
{
	// Terrain islands: Height(local) is cm over the sea reference; beyond rho 1.25 the sea is open (deep).
	AIronwakeArchipelago* Ar = Archipelago();
	if (!Ar) return false;
	bool bAny = false; float Best = 1e9f;
	for (int32 IsleIdx = 0; IsleIdx < Ar->Isles.Num(); ++IsleIdx)
	{
		if (Ar->HostedPortIsles.Contains(IsleIdx)) continue;
		const FIsle& I = Ar->Isles[IsleIdx];
		if (!I.bTerrain) continue;
		const float Dx = P.X - I.C.X, Dy = P.Y - I.C.Y;
		if (Dx * Dx + Dy * Dy > FMath::Square(I.R * 2.2f)) continue;
		const float Th = FMath::Atan2(Dy, Dx);
		const float Rs = FMath::Max(I.RTheta(Th), 1.f);
		const float Rho = FMath::Sqrt(Dx * Dx + Dy * Dy) / Rs;
		if (Rho >= 1.25f) continue;
		const float H = I.Height(Dx, Dy);
		if (!bAny || H > Best) { Best = H; bAny = true; }
	}
	if (bAny) OutH = Best;
	return bAny;
}

float UIronwakeColl::DepthAt(const FVector2D& P) const
{
	float H;
	if (!SeabedAt(P, H)) return 1e7f;
	return -H;
}

void UIronwakeColl::BuildGrid() const
{
	Grid.Reset();
	Stamp.Init(0, Obbs.Num()); StampN = 0;
	for (int32 i = 0; i < Obbs.Num(); ++i)
	{
		const FObb& O = Obbs[i];
		const int32 X0 = FMath::FloorToInt((O.C.X - O.R) / CellCm), X1 = FMath::FloorToInt((O.C.X + O.R) / CellCm);
		const int32 Y0 = FMath::FloorToInt((O.C.Y - O.R) / CellCm), Y1 = FMath::FloorToInt((O.C.Y + O.R) / CellCm);
		for (int32 Cx = X0; Cx <= X1; ++Cx) for (int32 Cy = Y0; Cy <= Y1; ++Cy) Grid.FindOrAdd(CellKey(Cx, Cy)).Add(i);
	}
	GridCount = Obbs.Num();
}

FIwHit UIronwakeColl::TestHull(const TArray<FVector2D>& DenseW, const TArray<FVector2D>& PolyW, const FVector2D& Centre, float RadiusCm,
	float DraughtCm, const AActor* Self, uint8 Mask) const
{
	Bake();
	FIwHit Best;
	const float Sea = SeaZ();
	auto Consider = [&](float Pen, const FVector2D& N, bool bLand, const FString& Tag, AActor* Other)
	{
		if (Pen > Best.Pen) { Best.bHit = true; Best.Pen = Pen; Best.N = N; Best.bLand = bLand; Best.Tag = Tag; Best.Other = Other; }
	};
	if (Mask & MaskSolids) ForEachNear(Centre, RadiusCm, [&](const FObb& O)
	{
		const float Dx = Centre.X - O.C.X, Dy = Centre.Y - O.C.Y;
		if (Dx * Dx + Dy * Dy > FMath::Square(O.R + RadiusCm)) return;
		if (O.ZMax < Sea - DraughtCm || O.ZMin > Sea + 1500.f) return;
		const FVector2D V(-O.U.Y, O.U.X);
		// hull outline points inside the solid
		for (const FVector2D& P : DenseW)
		{
			const FVector2D D = P - O.C;
			const float Lx = FVector2D::DotProduct(D, O.U), Ly = FVector2D::DotProduct(D, V);
			if (O.bEllipse)
			{
				const float Q = FMath::Square(Lx / O.Half.X) + FMath::Square(Ly / O.Half.Y);
				if (Q >= 1.f) continue;
				const float Rr = FMath::Sqrt(FMath::Max(Q, 1e-6f));
				const float Pen = FVector2D(Lx, Ly).Size() * (1.f / Rr - 1.f);
				FVector2D Nl(Lx / (O.Half.X * O.Half.X), Ly / (O.Half.Y * O.Half.Y));
				Nl = Nl.GetSafeNormal();
				Consider(Pen, (O.U * Nl.X + V * Nl.Y).GetSafeNormal(), false, O.Tag, nullptr);
			}
			else
			{
				if (FMath::Abs(Lx) >= O.Half.X || FMath::Abs(Ly) >= O.Half.Y) continue;
				const float Px = O.Half.X - FMath::Abs(Lx), Py = O.Half.Y - FMath::Abs(Ly);
				if (Px < Py) Consider(Px, O.U * FMath::Sign(Lx), false, O.Tag, nullptr);
				else Consider(Py, V * FMath::Sign(Ly), false, O.Tag, nullptr);
			}
		}
		// solid's own sample points inside the hull (poles / corners poking into the hull side)
		FVector2D Samp[9]; int32 NS = 0;
		Samp[NS++] = O.C;
		if (O.bEllipse) { for (int32 k = 0; k < 8; ++k) { const float A = k * PI / 4.f; Samp[NS++] = O.C + O.U * (FMath::Cos(A) * O.Half.X) + V * (FMath::Sin(A) * O.Half.Y); } }
		else { for (int32 sx = -1; sx <= 1; ++sx) for (int32 sy = -1; sy <= 1; ++sy) if (sx || sy) Samp[NS++] = O.C + O.U * (sx * O.Half.X) + V * (sy * O.Half.Y); }
		for (int32 k = 0; k < NS; ++k)
			if (InPoly(Samp[k], PolyW))
			{
				const float Pen = DistToPoly(Samp[k], PolyW) + 30.f;
				Consider(Pen, (Centre - Samp[k]).GetSafeNormal(), false, O.Tag, nullptr);
			}
	});
	if (Mask & (MaskPawns | MaskOthers))
	{
		for (int32 hi = Hulls.Num() - 1; hi >= 0; --hi)
		{
			FHullEnt& H = Hulls[hi];
			AActor* A = H.A.Get();
			if (!A) { Hulls.RemoveAtSwap(hi); continue; }
			if (A == Self) continue;
			if (!(Mask & (H.bPawn ? MaskPawns : MaskOthers))) continue;
			const FVector L = A->GetActorLocation();
			const FVector2D OC(L.X, L.Y);
			if (FVector2D::DistSquared(OC, Centre) > FMath::Square(H.Sh.R + RadiusCm)) continue;
			TArray<FVector2D> PW, DW;
			ToWorld(H.Sh.Poly, OC, A->GetActorRotation().Yaw, PW);
			ToWorld(H.Sh.Dense, OC, A->GetActorRotation().Yaw, DW);
			for (const FVector2D& P : DenseW)
				if (InPoly(P, PW)) Consider(DistToPoly(P, PW) + 20.f, (Centre - OC).GetSafeNormal(), false, A->GetName(), A);
			for (const FVector2D& P : DW)
				if (InPoly(P, PolyW)) Consider(DistToPoly(P, PolyW) + 20.f, (Centre - OC).GetSafeNormal(), false, A->GetName(), A);
		}
	}
	return Best;
}

float UIronwakeColl::PathClear(const FVector2D& Start, float YawDeg, float LenCm, float HalfBeamCm, float DraughtCm) const
{
	Bake();
	const float Sea = SeaZ();
	const FVector2D Dir = Rot2(FVector2D(1, 0), YawDeg), Side(-Dir.Y, Dir.X);
	const float Margin = 500.f;
	for (float D = 0.f; D <= LenCm; D += 1000.f)
		for (int32 Lat = -1; Lat <= 1; ++Lat)
		{
			const FVector2D P = Start + Dir * D + Side * (Lat * HalfBeamCm);
			if (DepthAt(P) < DraughtCm * 1.5f) return D;
			bool bBlocked = false;
			ForEachNear(P, Margin + 200.f, [&](const FObb& O)
			{
				if (bBlocked) return;
				if (O.ZMax < Sea - DraughtCm || O.ZMin > Sea + 1500.f) return;
				const FVector2D Dd = P - O.C;
				const FVector2D V(-O.U.Y, O.U.X);
				const float Lx = FVector2D::DotProduct(Dd, O.U), Ly = FVector2D::DotProduct(Dd, V);
				if (O.bEllipse ? (FMath::Square(Lx / (O.Half.X + Margin)) + FMath::Square(Ly / (O.Half.Y + Margin)) < 1.f)
					: (FMath::Abs(Lx) < O.Half.X + Margin && FMath::Abs(Ly) < O.Half.Y + Margin)) bBlocked = true;
			});
			if (bBlocked) return D;
		}
	return LenCm;
}

bool UIronwakeColl::AuditPath(const FString& Name, const TArray<FVector>& Pts, bool bClosed, float HalfBeamCm, float DraughtCm) const
{
	const int32 N = Pts.Num();
	if (N < 2) return true;
	const int32 Segs = bClosed ? N : N - 1;
	for (int32 i = 0; i < Segs; ++i)
	{
		const FVector2D A(Pts[i].X, Pts[i].Y), B(Pts[(i + 1) % N].X, Pts[(i + 1) % N].Y);
		const float L = (B - A).Size();
		if (L < 1.f) continue;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
		const float Clear = PathClear(A, Yaw, L, HalfBeamCm, DraughtCm);
		if (Clear < L)
		{
			const FVector2D P = A + (B - A).GetSafeNormal() * Clear;
			UE_LOG(LogIronwake, Warning, TEXT("IWP9 AUDIT %s: segment %d blocked after %.0f m at (%.0f,%.0f) m"), *Name, i, Clear / 100.f, P.X / 100.f, P.Y / 100.f);
			return false;
		}
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP9 AUDIT %s: path clear (%d segments)"), *Name, Segs);
	return true;
}

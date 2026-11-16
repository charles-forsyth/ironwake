// P15: hillside heightfield, roads and terrain mesh for the Greyhaven town. See IronwakeP15.h.
#include "IronwakeP15.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"

namespace
{
	float SS(float A, float B, float X) { const float T = FMath::Clamp((X - A) / (B - A), 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	float H2(int32 X, int32 Y, int32 S)
	{
		uint32 H = (uint32)X * 374761393u + (uint32)Y * 668265263u + (uint32)S * 2246822519u;
		H = (H ^ (H >> 13)) * 1274126177u; H ^= H >> 16;
		return (H & 0xFFFFFFu) / 16777215.f;
	}
	float VN(float X, float Y, int32 S)
	{
		const int32 Xi = FMath::FloorToInt(X), Yi = FMath::FloorToInt(Y);
		float Fx = X - Xi, Fy = Y - Yi; Fx = Fx * Fx * (3.f - 2.f * Fx); Fy = Fy * Fy * (3.f - 2.f * Fy);
		return FMath::Lerp(FMath::Lerp(H2(Xi, Yi, S), H2(Xi + 1, Yi, S), Fx), FMath::Lerp(H2(Xi, Yi + 1, S), H2(Xi + 1, Yi + 1, S), Fx), Fy);
	}
}

float FIwHill::Noise(float X, float Y, int32 Seed, int32 Oct)
{
	float A = 0.5f, F = 1.f, S = 0.f, N = 0.f;
	for (int32 O = 0; O < Oct; ++O) { S += A * VN(X * F + O * 17.3f, Y * F + O * 9.1f, Seed + O); N += A; A *= 0.5f; F *= 2.03f; }
	return S / N;
}

float FIwHill::OutcropMask(float X, float Y) { return SS(0.60f, 0.76f, Noise(X / 30.f, Y / 30.f, 41, 3)); }

float FIwHill::Prof(float U)
{
	if (U <= 200.f) return 0.25f * U - 17.75f;
	static const float PU[] = { 200.f, 250.f, 300.f, 345.f, 400.f, 450.f, 500.f, 545.f };
	static const float PZ[] = { 32.25f, 45.f, 55.f, 58.5f, 53.f, 38.f, 14.f, -9.f };
	static const float PM[] = { 0.25f, 0.25f, 0.15f, 0.0f, -0.22f, -0.42f, -0.5f, -0.3f };
	if (U >= 545.f) return -9.f;
	int32 K = 0; while (K < 6 && U > PU[K + 1]) ++K;
	const float H = PU[K + 1] - PU[K], T = (U - PU[K]) / H, T2 = T * T, T3 = T2 * T;
	return FMath::Max(-9.f, (2 * T3 - 3 * T2 + 1) * PZ[K] + (T3 - 2 * T2 + T) * H * PM[K] + (-2 * T3 + 3 * T2) * PZ[K + 1] + (T3 - T2) * H * PM[K + 1]);
}

float FIwHill::Base(float X, float Y) const
{
	const float Sc = FMath::Max(SpanScale, 0.25f);
	const float Xc = (Sc == 1.f) ? X : (-50.f + (X + 50.f) / Sc);
	const float Yc = (Sc == 1.f) ? Y : (Y / Sc);
	const float U = -Xc;
	const float Aw = FMath::Abs(Yc - 25.f) + 38.f * (Noise(Xc / 110.f + 3.f, Yc / 110.f, 11 + SeedOff, 2) - 0.5f) * 2.f;
	const float Env = 1.f - SS(170.f, 300.f, Aw);
	const float Hc = FMath::Lerp(2.f, Prof(FMath::Max(U, 90.f)), SS(78.f, 112.f, U));
	float Z = FMath::Lerp(-9.f, Hc, Env);
	const float Land = SS(-1.f, 5.f, Z);
	// relief: gentle in the town zone, bolder on the slopes behind it
	const float Amp = (0.45f + 4.5f * SS(170.f, 300.f, U)) * Land;
	Z += (Noise(Xc / 55.f, Yc / 55.f, 21 + SeedOff, 4) - 0.5f) * 2.f * Amp;
	Z += (1.f - FMath::Abs(2.f * Noise(Xc / 150.f, Yc / 65.f, 31 + SeedOff, 2) - 1.f)) * 3.2f * SS(120.f, 260.f, U) * Land;   // ridges running downslope
	for (int32 G = 0; G < 2; ++G)                                                                                 // two gullies
	{
		const float Yg = (G ? 215.f : -178.f) + 16.f * FMath::Sin(U * 0.028f + G * 2.1f);
		const float D = (Yc - Yg) / 10.f;
		Z -= 5.5f * FMath::Exp(-D * D) * SS(110.f, 190.f, U) * SS(470.f, 380.f, U) * Land;
	}
	Z += 4.2f * SS(0.60f, 0.76f, Noise(Xc / 30.f, Yc / 30.f, 41 + SeedOff, 3)) * SS(150.f, 260.f, U) * Land;
	// P17 shoreline: stone quay wall (Y -135..62 and 112..166), a sheltered cove with a gentle beach (Y 62..112, slipway + boathouse),
	// then revetment/beach flanks. ShoreX moves landward in the cove and on the flanks; slope 3 = quay wall, 0.14 = beach.
	const float AY = FMath::Abs(Yc);
	const float Cove = bSolidFront ? 0.f : (30.f * (SS(63.f, 76.f, Yc) - SS(100.f, 112.f, Yc)));
	const float Fl = SS(-128.f, -150.f, Yc) + SS(168.f, 190.f, Yc);
	const float ShoreXc = -50.f - Cove - 7.f * Fl - (AY > 170.f ? 0.0025f * FMath::Square(AY - 170.f) : 0.f) - 5.f * Fl * (Noise(Yc / 40.f, 5.f, 91 + SeedOff, 2) - 0.5f) * 2.f;
	const float ShoreX = -50.f + (ShoreXc + 50.f) * Sc;
	const float D = X - ShoreX;
	if (HeightScale != 1.f && Z > 2.f) Z = 2.f + (Z - 2.f) * HeightScale;
		if (D > 0.f)
		{
			const float Tb = FMath::Max(SS(0.5f, 10.f, Cove), SS(135.f, 175.f, AY));
			const float Slope = FMath::Lerp(3.0f, FMath::Lerp(0.28f, 0.36f, SS(190.f, 230.f, AY)), Tb);
			// P34: plunge underwater beach/cove floor cleanly below Z = -1.6 m once past the surf line so SingleLayerWater never Z-fights shallow triangles!
			const float SurfDrop = (Z < 0.15f) ? 1.35f * SS(0.15f, -0.35f, Z) : 0.f;
			Z = FMath::Min(Z, 2.f - Slope * D - SurfDrop);
		}
	return FMath::Max(Z, -9.5f);
}

void FIwHill::BakeRoads()
{
	for (FIwRoad& R : Roads)
	{
		R.P.Reset();
		if (R.Nodes.Num() < 2) continue;
		R.Mn = FVector2D(1e9f, 1e9f); R.Mx = FVector2D(-1e9f, -1e9f);
		for (int32 I = 0; I + 1 < R.Nodes.Num(); ++I)
		{
			const FVector A = R.Nodes[I], B = R.Nodes[I + 1];
			const int32 N = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(FVector2D(A), FVector2D(B)) / 1.5f));
			for (int32 K = 0; K < N; ++K) R.P.Add(FMath::Lerp(A, B, (float)K / N));
		}
		R.P.Add(R.Nodes.Last());
		if (R.bFollow)
		{
			for (FVector& V : R.P) V.Z = Base(V.X, V.Y) + 0.05f;
			for (int32 It = 0; It < 6; ++It)
			{
				TArray<float> Zs; for (const FVector& V : R.P) Zs.Add(V.Z);
				for (int32 I = 0; I < R.P.Num(); ++I) { float S = 0; int32 N = 0; for (int32 K = -4; K <= 4; ++K) { const int32 J = I + K; if (J >= 0 && J < R.P.Num()) { S += Zs[J]; ++N; } } R.P[I].Z = S / N; }
			}
		}
		for (const FVector& V : R.P) { R.Mn.X = FMath::Min(R.Mn.X, V.X); R.Mn.Y = FMath::Min(R.Mn.Y, V.Y); R.Mx.X = FMath::Max(R.Mx.X, V.X); R.Mx.Y = FMath::Max(R.Mx.Y, V.Y); }
	}
}

float FIwHill::RoadDist(float X, float Y, float* OutZ) const
{
	float Best = 1e9f, BestZ = 0.f;
	for (const FIwRoad& R : Roads)
	{
		const float M = R.HalfW + 4.f;
		if (X < R.Mn.X - M || X > R.Mx.X + M || Y < R.Mn.Y - M || Y > R.Mx.Y + M) continue;
		for (int32 I = 0; I + 1 < R.P.Num(); ++I)
		{
			const FVector A = R.P[I], B = R.P[I + 1];
			const float Dx = B.X - A.X, Dy = B.Y - A.Y, L2 = Dx * Dx + Dy * Dy;
			const float T = L2 > 1e-6f ? FMath::Clamp(((X - A.X) * Dx + (Y - A.Y) * Dy) / L2, 0.f, 1.f) : 0.f;
			const float Px = A.X + Dx * T - X, Py = A.Y + Dy * T - Y;
			const float D = FMath::Sqrt(Px * Px + Py * Py) - R.HalfW;
			if (D < Best) { Best = D; BestZ = FMath::Lerp(A.Z, B.Z, T); }
		}
	}
	if (OutZ) *OutZ = BestZ;
	return Best;
}

float FIwHill::PadDist(float X, float Y) const
{
	float Best = 1e9f;
	for (const FIwPad& P : Pads)
	{
		const float Dx = FMath::Max(0.f, FMath::Max(P.Xu - X, X - P.Xs)), Dy = FMath::Max(0.f, FMath::Max(P.Y0 - Y, Y - P.Y1));
		Best = FMath::Min(Best, FMath::Sqrt(Dx * Dx + Dy * Dy));
	}
	return Best;
}

float FIwHill::Final(float X, float Y) const
{
	float Z = Base(X, Y);
	float Prod = 1.f, Sw = 0.f, Sz = 0.f;
	for (const FIwPad& P : Pads)
	{
		if (X < P.Xu - 1.4f * P.Bl - 0.5f || X > P.Xs + 3.5f * P.Bl + 0.5f || Y < P.Y0 - 3.f * P.Bl - 0.5f || Y > P.Y1 + 3.f * P.Bl + 0.5f) continue;
		const float Du = FMath::Max(0.f, P.Xu - X) / (1.4f * P.Bl), Ds = FMath::Max(0.f, X - P.Xs) / (3.5f * P.Bl), Dy = FMath::Max(0.f, FMath::Max(P.Y0 - Y, Y - P.Y1)) / (3.f * P.Bl);
		const float W = SS(0.f, 1.f, FMath::Sqrt(Du * Du + Ds * Ds + Dy * Dy));
		if (W >= 1.f) continue;
		const float Wi = FMath::Square(1.f - W) + 1e-4f;   // P24: weighted blend of every overlapping bench skirt (no bisector tear)
		Prod *= W; Sw += Wi; Sz += Wi * P.Z;
	}
	if (Sw > 0.f) Z = FMath::Lerp(Z, Sz / Sw, 1.f - Prod);
	float Rz = 0.f; const float Dr = RoadDist(X, Y, &Rz);
	if (Dr < 3.4f) Z = FMath::Lerp(Rz, Z, SS(0.3f, 3.2f, Dr));
	return Z;
}

float FIwHill::SlopeDeg(float X, float Y) const
{
	const float E = 1.2f;
	const float Dx = (Final(X + E, Y) - Final(X - E, Y)) / (2.f * E), Dy = (Final(X, Y + E) - Final(X, Y - E)) / (2.f * E);
	return FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(Dx * Dx + Dy * Dy)));
}

FLinearColor FIwHill::Weights(float X, float Y, float Z, float Sl) const
{
	float Rock = SS(37.f, 50.f, Sl);
	Rock = FMath::Max(Rock, OutcropMask(X, Y) * 0.85f * SS(8.f, 20.f, Z));
	float Scree = SS(24.f, 35.f, Sl) * 0.8f + SS(2.6f, 0.4f, Z) * 0.9f;
	const float Pt = Noise(X / 14.f, Y / 14.f, 51, 3);
	const float Pm = Noise(X / 55.f, Y / 55.f, 53, 3);
	float Earth = FMath::Max(SS(0.54f, 0.70f, Pt) * 0.8f, SS(0.50f, 0.62f, Pm) * 0.85f);
	Rock = FMath::Max(Rock, SS(0.64f, 0.74f, Noise(X / 38.f, Y / 38.f, 57, 3)) * 0.8f * SS(10.f, 30.f, Z));
	const float Rd = RoadDist(X, Y);
	if (Rd < 3.5f) { Earth = FMath::Max(Earth, 0.9f * (1.f - SS(0.3f, 3.5f, Rd))); Rock = FMath::Max(Rock, 0.3f * (1.f - SS(0.3f, 3.5f, Rd))); }
	Scree = FMath::Clamp(Scree, 0.f, 1.f) * (1.f - Rock);
	Earth = FMath::Clamp(Earth, 0.f, 1.f) * FMath::Max(0.f, 1.f - Rock - Scree);
	// quay apron and flank shelves at sea level: packed gravel/earth, not lawn
	const float Yard = SS(4.5f, 2.6f, Z) * SS(-100.f, -60.f, X);
	if (Yard > 0.f) { Earth = FMath::Lerp(Earth, 0.55f, Yard); Scree = FMath::Lerp(Scree, 0.45f, Yard); Rock *= 1.f - Yard; }
	const float Grass = FMath::Max(0.f, 1.f - Rock - Scree - Earth);
	return FLinearColor(Grass, Earth, Scree, Rock);
}

UProceduralMeshComponent* FIwHill::BuildTerrain(AActor* Owner, USceneComponent* Parent, UMaterialInterface* Mat) const
{
	const float Sc = FMath::Max(SpanScale, 0.25f);
	const float XMid1 = -50.f - 50.f * Sc, XMid2 = -50.f - 165.f * Sc, XEnd = -50.f - 495.f * Sc;
	const float YLo = -440.f * Sc, YMid1 = -190.f * Sc, YMid2 = 240.f * Sc, YHi = 490.f * Sc;
	TArray<float> Xs, Ys;
	for (float X = -44.f; X > XMid1; X -= 2.5f) Xs.Add(X);
	for (float X = XMid1; X > XMid2; X -= 1.5f) Xs.Add(X);
	for (float X = XMid2; X >= XEnd; X -= 5.f) Xs.Add(X);
	for (float Y = YLo; Y < YMid1; Y += 5.f) Ys.Add(Y);
	for (float Y = YMid1; Y < YMid2; Y += 1.5f) Ys.Add(Y);
	for (float Y = YMid2; Y <= YHi; Y += 5.f) Ys.Add(Y);
	const int32 NX = Xs.Num(), NY = Ys.Num();
	TArray<float> Zs; Zs.SetNumUninitialized(NX * NY);
	for (int32 I = 0; I < NX; ++I) for (int32 J = 0; J < NY; ++J) Zs[I * NY + J] = Final(Xs[I], Ys[J]);
	TArray<int32> Remap; Remap.Init(-1, NX * NY);
	TArray<FVector> V; TArray<int32> Tri; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	auto Keep = [&](int32 I, int32 J) { return I >= 0 && I < NX && J >= 0 && J < NY; };
	for (int32 I = 0; I + 1 < NX; ++I)
		for (int32 J = 0; J + 1 < NY; ++J)
		{
			const float Mn = FMath::Min(FMath::Min(Zs[I * NY + J], Zs[I * NY + J + 1]), FMath::Min(Zs[(I + 1) * NY + J], Zs[(I + 1) * NY + J + 1]));
			if (Mn < -2.8f) continue;
			const int32 Id[4] = { I * NY + J, I * NY + J + 1, (I + 1) * NY + J, (I + 1) * NY + J + 1 };
			for (int32 K = 0; K < 4; ++K) if (Remap[Id[K]] < 0)
			{
				const int32 Ii = Id[K] / NY, Jj = Id[K] % NY;
				Remap[Id[K]] = V.Num();
				const float X = Xs[Ii], Y = Ys[Jj], Z = Zs[Id[K]];
				V.Add(FVector(X * 100.f, Y * 100.f, Z * 100.f));
				const int32 Ia = FMath::Max(Ii - 1, 0), Ib = FMath::Min(Ii + 1, NX - 1), Ja = FMath::Max(Jj - 1, 0), Jb = FMath::Min(Jj + 1, NY - 1);
				const float Dzx = (Zs[Ib * NY + Jj] - Zs[Ia * NY + Jj]) / FMath::Max(Xs[Ia] - Xs[Ib], 1e-3f) * -1.f;   // dZ/dX (Xs descends)
				const float Dzy = (Zs[Ii * NY + Jb] - Zs[Ii * NY + Ja]) / FMath::Max(Ys[Jb] - Ys[Ja], 1e-3f);
				N.Add(FVector(-Dzx, -Dzy, 1.f).GetSafeNormal());
				UV.Add(FVector2D(X / 50.f, Y / 50.f));
				Col.Add(Weights(X, Y, Z, FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(Dzx * Dzx + Dzy * Dzy)))));
				Tan.Add(FProcMeshTangent(FVector(0.7071f, -0.7071f, 0.f), false));
			}
			const int32 A = Remap[Id[0]], B = Remap[Id[1]], C = Remap[Id[2]], D = Remap[Id[3]];
			Tri.Append({ A, C, B, B, C, D });
		}
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(Owner);
	PM->SetupAttachment(Parent); PM->RegisterComponent(); Owner->AddInstanceComponent(PM);
	PM->CreateMeshSection_LinearColor(0, V, Tri, N, UV, Col, Tan, false, false);
	if (Mat) PM->SetMaterial(0, Mat);
	PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PM->SetCastShadow(true);
	UE_LOG(LogTemp, Display, TEXT("IWP15 terrain: %d verts, %d tris (grid %dx%d)"), V.Num(), Tri.Num() / 3, NX, NY);
	return PM;
}

UProceduralMeshComponent* FIwHill::BuildRibbon(AActor* Owner, USceneComponent* Parent, UMaterialInterface* Mat, const FIwRoad& R) const
{
	if (R.P.Num() < 2) return nullptr;
	const int32 NA = R.P.Num(), NB = 7;
	TArray<FVector> V; TArray<int32> Tri; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> Col; TArray<FProcMeshTangent> Tan;
	for (int32 A = 0; A < NA; ++A)
	{
		const FVector P = R.P[A];
		const FVector2D T = (FVector2D(R.P[FMath::Min(A + 1, NA - 1)]) - FVector2D(R.P[FMath::Max(A - 1, 0)])).GetSafeNormal();
		const FVector2D Nl(-T.Y, T.X);
		const float Hw = R.HalfW * (1.f + 0.10f * (Noise(P.X / 9.f, P.Y / 9.f, 61, 2) - 0.5f) * 2.f);
		for (int32 B = 0; B < NB; ++B)
		{
			const float S = ((float)B / (NB - 1)) * 2.f - 1.f;
			const float X = P.X + Nl.X * S * Hw, Y = P.Y + Nl.Y * S * Hw;
			const float Z = Final(X, Y) + R.Lift;
			V.Add(FVector(X * 100.f, Y * 100.f, Z * 100.f));
			const float E = 0.8f;
			const float Dx = (Final(X + E, Y) - Final(X - E, Y)) / (2 * E), Dy = (Final(X, Y + E) - Final(X, Y - E)) / (2 * E);
			N.Add(FVector(-Dx, -Dy, 1.f).GetSafeNormal());
			UV.Add(FVector2D(A * 0.1f, B * 0.1f));
			const float Wear = FMath::Clamp(FMath::Pow(FMath::Abs(S), 3.f) * 1.4f + (Noise(X / 2.2f, Y / 2.2f, 71, 2) - 0.5f) * 0.7f, 0.f, 1.f);
			Col.Add(FLinearColor(Wear, 0.f, 0.f, 1.f));
			Tan.Add(FProcMeshTangent(FVector(0.7071f, -0.7071f, 0.f), false));
		}
	}
	for (int32 A = 0; A + 1 < NA; ++A)
		for (int32 B = 0; B + 1 < NB; ++B)
		{
			const int32 I0 = A * NB + B, I1 = A * NB + B + 1, I2 = (A + 1) * NB + B, I3 = (A + 1) * NB + B + 1;
			auto Add = [&](int32 P, int32 Q, int32 W)
			{
				const FVector C = FVector::CrossProduct(V[Q] - V[P], V[W] - V[P]);
				if (C.Z > 0.f) Tri.Append({ P, W, Q }); else Tri.Append({ P, Q, W });
			};
			Add(I0, I1, I2); Add(I1, I3, I2);
		}
	UProceduralMeshComponent* PM = NewObject<UProceduralMeshComponent>(Owner);
	PM->SetupAttachment(Parent); PM->RegisterComponent(); Owner->AddInstanceComponent(PM);
	PM->CreateMeshSection_LinearColor(0, V, Tri, N, UV, Col, Tan, false, false);
	if (Mat) PM->SetMaterial(0, Mat);
	PM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PM->SetCastShadow(false);
	return PM;
}

void FIwHill::LandRects(TArray<FVector4>& Out) const
{
	const float Cell = 8.f;
	for (float X0 = -50.f; X0 > -560.f; X0 -= Cell)
	{
		const float X1 = X0 - Cell;
		float RunStart = 0.f; bool bRun = false;
		for (float Y = -460.f; Y <= 500.f + Cell; Y += Cell)
		{
			bool bLand = false;
			if (Y <= 500.f)
				bLand = Final(X0 - Cell * 0.5f, Y + Cell * 0.5f) > -2.6f || Final(X0, Y) > -2.6f || Final(X1, Y) > -2.6f || Final(X0, Y + Cell) > -2.6f || Final(X1, Y + Cell) > -2.6f;
			if (bLand && !bRun) { bRun = true; RunStart = Y; }
			if (!bLand && bRun) { bRun = false; Out.Add(FVector4(X1, X0, RunStart, Y)); }
		}
	}
}

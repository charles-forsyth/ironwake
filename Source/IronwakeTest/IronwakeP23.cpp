// P23: town placement planner (see IronwakeP23.h).
#include "IronwakeP23.h"
#include "IronwakeTest.h"

namespace IwP23
{
const float TypeW[3] = { 18.f, 8.f, 7.18f }, TypeD[3] = { 8.26f, 8.46f, 7.74f }, TypeH[3] = { 11.f, 12.f, 9.f }, TypeRidge[3] = { 8.6f, 11.4f, 7.6f };
}

namespace
{
	float SS(float A, float B, float X) { const float T = FMath::Clamp((X - A) / (B - A), 0.f, 1.f); return T * T * (3.f - 2.f * T); }

	TArray<FVector2D> Chaikin(const TArray<FVector2D>& In, int32 Iter)
	{
		TArray<FVector2D> P = In;
		for (int32 K = 0; K < Iter; ++K)
		{
			TArray<FVector2D> Q; Q.Add(P[0]);
			for (int32 I = 0; I + 1 < P.Num(); ++I) { Q.Add(FMath::Lerp(P[I], P[I + 1], 0.25f)); Q.Add(FMath::Lerp(P[I], P[I + 1], 0.75f)); }
			Q.Add(P.Last()); P = Q;
		}
		return P;
	}
	FIwRoad MakeRoad(const TArray<FVector2D>& Poly, float HalfW, float Lift, int32 Iter)
	{
		FIwRoad R; R.HalfW = HalfW; R.Lift = Lift; R.bFollow = true;
		for (const FVector2D& P : Chaikin(Poly, Iter)) R.Nodes.Add(FVector(P.X, P.Y, 0.f));
		return R;
	}

	void Axes(float PhiDeg, FVector2D& N, FVector2D& T)
	{
		const float R = FMath::DegreesToRadians(PhiDeg);
		N = FVector2D(FMath::Cos(R), FMath::Sin(R)); T = FVector2D(-N.Y, N.X);
	}
	// OBB overlap (separating axis), margin in m
	bool Over(const FP23House& A, const FP23House& B, float M)
	{
		if (FVector2D::DistSquared(FVector2D(A.X, A.Y), FVector2D(B.X, B.Y)) > FMath::Square(0.5f * (FMath::Max(A.W, A.D) + FMath::Max(B.W, B.D)) * 1.45f + M + 1.f)) return false;
		FVector2D Na, Ta, Nb, Tb; Axes(A.Phi, Na, Ta); Axes(B.Phi, Nb, Tb);
		const FVector2D Ax[4] = { Na, Ta, Nb, Tb };
		const FVector2D D(B.X - A.X, B.Y - A.Y);
		for (const FVector2D& L : Ax)
		{
			const float Ra = 0.5f * (FMath::Abs(FVector2D::DotProduct(Na, L)) * A.D + FMath::Abs(FVector2D::DotProduct(Ta, L)) * A.W);
			const float Rb = 0.5f * (FMath::Abs(FVector2D::DotProduct(Nb, L)) * B.D + FMath::Abs(FVector2D::DotProduct(Tb, L)) * B.W);
			if (FMath::Abs(FVector2D::DotProduct(D, L)) > Ra + Rb + M) return false;
		}
		return true;
	}
}

int32 IwP23::AddRoads(FIwHill& Hill)
{
	// main road: quay square -> three hairpins -> chapel front. Corner cutting (Chaikin) rounds the U-turns (radius ~8 m).
	{
		TArray<FVector2D> P = {
			{ -62.f, 18.f }, { -90.f, 18.f }, { -98.f, -66.f },
			{ -103.f, -77.f }, { -112.f, -79.f }, { -117.f, -68.f },
			{ -130.f, 94.f },
			{ -135.f, 105.f }, { -144.f, 106.f }, { -148.f, 94.f },
			{ -153.f, -56.f },
			{ -158.f, -66.f }, { -166.f, -66.f }, { -169.f, -54.f },
			{ -172.f, 22.f } };
		Hill.Roads.Add(MakeRoad(P, 3.6f, 0.07f, 3));
	}
	// contour lanes (3.5 m): lower town toward the cove; out from each hairpin apex along the slope
	Hill.Roads.Add(MakeRoad({ { -92.f, 18.f }, { -96.f, 52.f }, { -101.f, 88.f }, { -105.f, 124.f } }, 2.4f, 0.07f, 2));
	Hill.Roads.Add(MakeRoad({ { -108.f, -79.f }, { -110.f, -92.f }, { -114.f, -106.f } }, 2.4f, 0.07f, 2));
	Hill.Roads.Add(MakeRoad({ { -139.f, 106.f }, { -141.f, 128.f }, { -146.f, 152.f }, { -151.f, 172.f } }, 2.4f, 0.07f, 2));
	Hill.Roads.Add(MakeRoad({ { -117.f, -68.f }, { -121.f, -40.f }, { -125.f, -10.f }, { -128.f, 22.f }, { -130.f, 60.f } }, 2.4f, 0.07f, 2));   // mid-slope lane
	Hill.Roads.Add(MakeRoad({ { -148.f, 94.f }, { -150.f, 60.f }, { -153.f, 20.f }, { -154.f, -20.f } }, 2.4f, 0.07f, 2));
	Hill.Roads.Add(MakeRoad({ { -162.f, -66.f }, { -165.f, -88.f }, { -171.f, -110.f }, { -178.f, -128.f } }, 2.4f, 0.07f, 2));
	// long wavy contour lanes across the whole slope (3.5 m): the town grows along these
	{
		const float Xs[] = { -179.f };
		const float Y0[] = { -100.f }, Y1[] = { 125.f };
		for (int32 L = 0; L < 1; ++L)
		{
			TArray<FVector2D> P;
			for (float Y = Y0[L]; Y <= Y1[L]; Y += 22.f) { P.Add(FVector2D(Xs[L] + 4.f * FMath::Sin(Y * 0.045f + L * 1.7f) + 3.f * (FIwHill::Noise(Y / 40.f, L * 7.f, 301, 2) - 0.5f), Y)); }
			Hill.Roads.Add(MakeRoad(P, 2.4f, 0.07f, 2));
		}
	}
	return Hill.Roads.Num();
}

void IwP23::AddVerges(FIwHill& Hill, int32 NumBase)
{
	for (int32 I = 0; I < NumBase; ++I)
	{
		FIwRoad V = Hill.Roads[I]; V.HalfW += 1.1f; V.Lift = 0.015f; V.P.Reset();
		Hill.Roads.Add(V);
	}
}

void IwP23::Plan(const FIwHill& Hill, FP23Plan& Out, int32 Seed, int32 NBase)
{
	FRandomStream R(Seed);
	const auto Env = [&](float X, float Y)
	{
		const float U = -X, T = SS(95.f, 190.f, U);
		// P28: slightly wider upper-slope bands 3-6 envelope & softer gully mask so house count rises cleanly from ~67 to ~85
		const float Hl = FMath::Lerp(138.f, 124.f, T) + 16.f * (FIwHill::Noise(U / 45.f + 2.f, 1.f, 201, 2) - 0.5f) * 2.f;
		const float Hr = FMath::Lerp(152.f, 126.f, T) + 16.f * (FIwHill::Noise(U / 45.f + 9.f, 4.f, 203, 2) - 0.5f) * 2.f;
		const float Nz = 1.f - 0.42f * SS(0.32f, 0.72f, FIwHill::Noise(X / 24.f + 1.f, Y / 24.f, 207, 2));
		const float Gl = FIwHill::Noise(Y / 32.f + 3.f, X / 60.f, 223, 2);
		if (Gl < 0.04f + 0.05f * T) return false;
		return Y > 18.f - Hl * (0.64f + 0.36f * Nz) && Y < 18.f + Hr * (0.64f + 0.36f * Nz);
	};
	const auto Excl = [&](float X, float Y)
	{
		if (X > -93.f) return true;                                                       // quay, square, cove plaza, P28 tavern & chandlery row
		if (X < -168.f && X > -206.f && Y > -16.f && Y < 48.f) return true;               // chapel + bell tower bench
		if (FVector2D::Distance(FVector2D(X, Y), FVector2D(-150.f, -144.f)) < 26.f) return true;   // gasometer
		if (FVector2D::Distance(FVector2D(X, Y), FVector2D(-128.f, -102.f)) < 30.f) return true;   // factory
		if (FVector2D::Distance(FVector2D(X, Y), FVector2D(-98.f, -122.f)) < 18.f) return true;    // P28 foundry
		if (FVector2D::Distance(FVector2D(X, Y), FVector2D(-112.f, 146.f)) < 24.f) return true;    // warehouse
		return false;
	};
	const auto Slope = [&](float X, float Y)
	{
		const float E = 4.f;
		const float Dx = (Hill.Base(X + E, Y) - Hill.Base(X - E, Y)) / (2 * E), Dy = (Hill.Base(X, Y + E) - Hill.Base(X, Y - E)) / (2 * E);
		return FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(Dx * Dx + Dy * Dy)));
	};
	const auto Down = [&](float X, float Y)
	{
		const float E = 6.f;
		const float Gx = (Hill.Base(X + E, Y) - Hill.Base(X - E, Y)), Gy = (Hill.Base(X, Y + E) - Hill.Base(X, Y - E));
		return FMath::RadiansToDegrees(FMath::Atan2(-Gy, -Gx));
	};
	int32 ClusterId = 0;
	int32 Rj[8] = { 0 };
	const auto Accept = [&](const FP23House& H) -> bool
	{
		if (Excl(H.X, H.Y)) { ++Rj[0]; return false; }
		if (!Env(H.X, H.Y)) { ++Rj[1]; return false; }
		const float NotchMin = (H.X < -130.f) ? 0.21f : 0.28f;
		if (FIwHill::Noise(H.X / 17.f + 5.f, H.Y / 17.f, 211, 3) < NotchMin) { ++Rj[2]; return false; }   // notches
		if (Slope(H.X, H.Y) > 35.5f || FIwHill::OutcropMask(H.X, H.Y) > 0.58f) { ++Rj[3]; return false; }
		FVector2D N, T; Axes(H.Phi, N, T);
		float Zmin = 1e9f, Zmax = -1e9f;
		for (int32 A = -1; A <= 1; ++A) for (int32 B = -1; B <= 1; ++B)
		{
			const float Wd = (H.W * 0.5f + 0.2f) * A, Dd = (H.D * 0.5f + 0.2f) * B;
			const float X = H.X + N.X * Dd + T.X * Wd, Y = H.Y + N.Y * Dd + T.Y * Wd;
			if (Excl(X, Y) && X > -93.f) { ++Rj[4]; return false; }
			if (Hill.RoadDist(X, Y) < -0.3f) { ++Rj[5]; return false; }
			const float Z = Hill.Base(X, Y); Zmin = FMath::Min(Zmin, Z); Zmax = FMath::Max(Zmax, Z);
		}
		if (Zmin < 3.2f || Zmax - Zmin > 9.5f) { ++Rj[6]; return false; }
		for (const FP23House& O : Out.Houses) if (Over(H, O, O.Cluster == ClusterId ? 0.08f : 0.80f)) { ++Rj[7]; return false; }
		return true;
	};

	// P24/P28: stepped benches by elevation band (not by road). 7 bands of ~4 m climb from the quay apron to the chapel, explicit quota per
	// band tapering upward (~85 houses total); clusters of 2-5 houses along the contour (yaw follows Down +/- 14 deg).
	static const float BandLo[7] = { 4.5f, 8.5f, 12.5f, 16.5f, 20.5f, 24.5f, 28.5f }, BandHi[7] = { 8.5f, 12.5f, 16.5f, 20.5f, 24.5f, 28.5f, 35.f };
	static const int32 Quota[7] = { 24, 22, 20, 18, 16, 12, 6 };
	const auto MakeHouse = [&](float Phi) -> FP23House
	{
		FP23House H; const float RTy = R.FRand();
		H.Type = RTy < 0.22f ? 0 : RTy < 0.60f ? 1 : 2;
		H.S = R.FRandRange(0.8f, 1.25f); H.Sz = R.FRandRange(0.80f, 1.45f);
		H.W = IwP23::TypeW[H.Type] * H.S; H.D = IwP23::TypeD[H.Type] * H.S;
		H.H = IwP23::TypeH[H.Type] * H.S * H.Sz; H.Ridge = IwP23::TypeRidge[H.Type] * H.S * H.Sz;
		H.Phi = Phi; H.Tint = R.RandRange(0, 6); H.Cluster = ClusterId; return H;
	};
	for (int32 B = 0; B < 7; ++B)
	{
		int32 Got = 0, Tries = 0, Cl = 0;
		while (Got < Quota[B] && Tries < 60000)
		{
			++Tries;
			const float Xa = -(BandLo[B] + 17.75f) * 4.f - 6.f, Xb = -(BandHi[B] + 17.75f) * 4.f + 6.f;
			const float X = R.FRandRange(FMath::Max(Xa, -93.f), FMath::Min(Xb, -218.f)), Y = R.FRandRange(-125.f, 165.f);
			const float Zs = Hill.Base(X, Y);
			if (Zs < BandLo[B] || Zs >= BandHi[B]) continue;
			bool bNear = false; for (const FP23House& O : Out.Houses) if (FVector2D::DistSquared(FVector2D(X, Y), FVector2D(O.X, O.Y)) < FMath::Square(5.f)) { bNear = true; break; }
			if (bNear) continue;
			++ClusterId;
			const int32 Before = Out.Houses.Num();
			const float Phi0 = Down(X, Y);
			FVector2D N0, T0; Axes(Phi0, N0, T0);
			const int32 Count = 2 + FMath::Min(3, FMath::FloorToInt(R.FRand() * 4.f));
			float Off = 0.f; int32 Fails = 0;
			for (int32 K = 0; K < Count && Fails < 6; ++K)
			{
				FP23House H = MakeHouse(0.f);
				H.X = X + T0.X * Off + N0.X * R.FRandRange(-4.f, 4.f); H.Y = Y + T0.Y * Off + N0.Y * R.FRandRange(-4.f, 4.f); H.Phi = Down(H.X, H.Y) + R.FRandRange(-14.f, 14.f);
				if (Got + (Out.Houses.Num() - Before) >= Quota[B]) break;
				if (!Accept(H)) { Off += 3.f; ++Fails; --K; continue; }
				H.Z = Hill.Base(H.X, H.Y);
				Out.Houses.Add(H);
				Off += 0.5f * (H.W + 9.f) + R.FRandRange(1.f, 2.5f);
			}
			const int32 Placed = Out.Houses.Num() - Before;
			if (Placed < 2) { Out.Houses.SetNum(Before); --ClusterId; continue; }
			Got += Placed; ++Cl;
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP24 band %d (Z %.1f-%.1f) houses %d/%d clusters %d tries %d"), B, BandLo[B], BandHi[B], Got, Quota[B], Cl, Tries);
	}
	// upper outliers near Z 32-37 m flanking the chapel
	for (int32 Try = 0, Got = 0; Try < 6000 && Got < 2; ++Try)
	{
		const float X = R.FRandRange(-190.f, -225.f), Y = R.FRandRange(-90.f, 140.f);
		const float Zs = Hill.Base(X, Y); if (Zs < 32.f || Zs > 37.f) continue;
		++ClusterId; FP23House H = MakeHouse(Down(X, Y) + R.FRandRange(-14.f, 14.f)); H.Type = 2; H.S = 1.0f; H.W = IwP23::TypeW[2]; H.D = IwP23::TypeD[2]; H.H = IwP23::TypeH[2] * H.Sz; H.Ridge = IwP23::TypeRidge[2] * H.Sz;
		H.X = X; H.Y = Y;
		if (!Accept(H)) { --ClusterId; continue; }
		H.Z = Hill.Base(X, Y); Out.Houses.Add(H); ++Got;
	}
	Out.Clusters = ClusterId;
	{
		// ASCII map of the plan (X rows from seaward -90 to -210 every 6 m, Y columns -150..190 every 5 m): H house, = road
		for (float X = -90.f; X > -210.f; X -= 6.f)
		{
			FString Ln;
			for (float Y = -150.f; Y < 190.f; Y += 5.f)
			{
				TCHAR C = TEXT('.');
				if (Hill.RoadDist(X, Y) < 0.f) C = TEXT('=');
				for (const FP23House& H : Out.Houses) { FVector2D N, T; Axes(H.Phi, N, T); const FVector2D D(X - H.X, Y - H.Y); if (FMath::Abs(FVector2D::DotProduct(D, N)) < H.D * 0.5f + 1.f && FMath::Abs(FVector2D::DotProduct(D, T)) < H.W * 0.5f + 1.f) C = TEXT('H'); }
				Ln.AppendChar(C);
			}
			UE_LOG(LogIronwake, Display, TEXT("IWP23MAP %4.0f %s"), X, *Ln);
		}
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP23 rejects excl=%d env=%d notch=%d slope=%d exclc=%d road=%d dz=%d overlap=%d placed=%d"), Rj[0], Rj[1], Rj[2], Rj[3], Rj[4], Rj[5], Rj[6], Rj[7], Out.Houses.Num());

	// vegetation + boulders around the houses (edge blending, gaps between clusters)
	const auto InsideHouse = [&](float X, float Y, float M)
	{
		for (const FP23House& H : Out.Houses)
		{
			if (FVector2D::DistSquared(FVector2D(H.X, H.Y), FVector2D(X, Y)) > FMath::Square(0.8f * FMath::Max(H.W, H.D) + M)) continue;
			FVector2D N, T; Axes(H.Phi, N, T);
			const FVector2D D(X - H.X, Y - H.Y);
			if (FMath::Abs(FVector2D::DotProduct(D, N)) < H.D * 0.5f + M && FMath::Abs(FVector2D::DotProduct(D, T)) < H.W * 0.5f + M) return true;
		}
		return false;
	};
	int32 Idx = 0;
	for (const FP23House& H : Out.Houses)
	{
		++Idx;
		const int32 Nv = (R.FRand() < 0.7f ? 1 : 0) + (R.FRand() < 0.4f ? 1 : 0);
		for (int32 K = 0; K < Nv + 4; ++K)
		{
			const float A = R.FRandRange(0.f, 2.f * PI), Rr = 0.5f * FMath::Max(H.W, H.D) + R.FRandRange(2.f, 8.f);
			const float X = H.X + FMath::Cos(A) * Rr, Y = H.Y + FMath::Sin(A) * Rr;
			if (Excl(X, Y) || Hill.Base(X, Y) < 3.f || Hill.RoadDist(X, Y) < 2.4f || InsideHouse(X, Y, 3.0f) || Slope(X, Y) > 32.f) continue;
			const float Q = R.FRand();
			FP23Veg V; V.X = X; V.Y = Y;
			if (K >= Nv + 1 && Q < 0.7f) { V.Kind = R.FRand() < 0.5f ? 1 : 2; V.S = R.FRandRange(0.65f, 1.15f); }
			else { V.Kind = 0; V.S = R.FRandRange(1.3f, 2.8f); }
			Out.Veg.Add(V);
		}
		if ((Idx % 4) == 0)
		{
			const float A = R.FRandRange(0.f, 2.f * PI), Rr = 0.5f * FMath::Max(H.W, H.D) + R.FRandRange(5.f, 12.f);
			const float X = H.X + FMath::Cos(A) * Rr, Y = H.Y + FMath::Sin(A) * Rr;
			if (!Excl(X, Y) && Hill.Base(X, Y) > 3.f && Hill.RoadDist(X, Y) > 3.f && !InsideHouse(X, Y, 1.5f)) { FP23Veg V; V.X = X; V.Y = Y; V.Kind = 3; V.S = R.FRandRange(1.4f, 3.0f); Out.Veg.Add(V); }
		}
	}
	for (int32 K = 0; K < 110 && Out.Houses.Num() > 0; ++K)
	{
		const FP23House& H = Out.Houses[R.RandRange(0, Out.Houses.Num() - 1)];
		const float A = R.FRandRange(0.f, 2.f * PI), Rr = 0.5f * FMath::Max(H.W, H.D) + R.FRandRange(5.f, 14.f);
		const float X = H.X + FMath::Cos(A) * Rr, Y = H.Y + FMath::Sin(A) * Rr;
		if (Excl(X, Y) || Hill.Base(X, Y) < 3.f || Hill.RoadDist(X, Y) < 2.6f || InsideHouse(X, Y, 3.0f) || Slope(X, Y) > 32.f) continue;
		FP23Veg V; V.X = X; V.Y = Y;
		const float Q = R.FRand();
		if (Q < 0.18f) { V.Kind = 3; V.S = R.FRandRange(2.0f, 4.0f); } else if (Q < 0.5f) { V.Kind = 4; V.S = R.FRandRange(1.8f, 3.2f); } else { V.Kind = R.FRand() < 0.6f ? 1 : 2; V.S = R.FRandRange(0.8f, 1.3f); }
		Out.Veg.Add(V);
	}
}

int32 IwP23::AddRoadsScaled(FIwHill& Hill, float SpanScale)
{
	const float Sc = FMath::Clamp(SpanScale, 0.45f, 1.f);
	auto MapPt = [&](float X, float Y) { return FVector2D(-50.f + (X + 50.f) * Sc, Y * Sc); };
	auto AddR = [&](std::initializer_list<FVector2D> Pts, float HalfW, int32 Iter)
	{
		TArray<FVector2D> A;
		for (const FVector2D& P : Pts) A.Add(MapPt(P.X, P.Y));
		Hill.Roads.Add(MakeRoad(A, HalfW, 0.07f, Iter));
	};
	AddR({ { -60.f, 14.f }, { -88.f, 14.f }, { -98.f, -58.f }, { -104.f, -68.f }, { -114.f, -68.f }, { -120.f, -56.f },
		   { -134.f, 78.f }, { -140.f, 88.f }, { -150.f, 88.f }, { -156.f, 74.f }, { -164.f, -44.f }, { -174.f, 12.f } }, 3.3f, 3);
	AddR({ { -92.f, 14.f }, { -96.f, 48.f }, { -102.f, 84.f }, { -108.f, 112.f } }, 2.2f, 2);
	AddR({ { -110.f, -68.f }, { -114.f, -86.f }, { -120.f, -102.f } }, 2.2f, 2);
	AddR({ { -120.f, -54.f }, { -124.f, -20.f }, { -128.f, 18.f }, { -132.f, 52.f } }, 2.2f, 2);
	AddR({ { -152.f, 76.f }, { -156.f, 38.f }, { -160.f, 0.f }, { -162.f, -34.f } }, 2.2f, 2);
	return Hill.Roads.Num();
}

void IwP23::PlanScaled(const FIwHill& Hill, FP23Plan& Out, int32 Seed, int32 NBase, float SpanScale, float HeightScale)
{
	(void)NBase;
	const float Sc = FMath::Clamp(SpanScale, 0.45f, 1.f);
	const float Hs = FMath::Clamp(HeightScale, 0.5f, 1.2f);
	FRandomStream R(Seed);
	const auto Slope = [&](float X, float Y)
	{
		const float E = 3.5f;
		const float Dx = (Hill.Base(X + E, Y) - Hill.Base(X - E, Y)) / (2 * E), Dy = (Hill.Base(X, Y + E) - Hill.Base(X, Y - E)) / (2 * E);
		return FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(Dx * Dx + Dy * Dy)));
	};
	const auto Down = [&](float X, float Y)
	{
		const float E = 5.f;
		const float Gx = (Hill.Base(X + E, Y) - Hill.Base(X - E, Y)), Gy = (Hill.Base(X, Y + E) - Hill.Base(X, Y - E));
		return FMath::RadiansToDegrees(FMath::Atan2(-Gy, -Gx));
	};
	const float XQuayClear = -50.f - 36.f * Sc;
	const float YMaxEnv = 148.f * Sc;
	int32 ClusterId = 0;
	const auto Accept = [&](const FP23House& H) -> bool
	{
		if (H.X > XQuayClear || FMath::Abs(H.Y) > YMaxEnv) return false;
		if (Slope(H.X, H.Y) > 37.f) return false;
		FVector2D N, T; Axes(H.Phi, N, T);
		float Zmin = 1e9f, Zmax = -1e9f;
		for (int32 A = -1; A <= 1; ++A) for (int32 B = -1; B <= 1; ++B)
		{
			const float Wd = (H.W * 0.5f + 0.2f) * A, Dd = (H.D * 0.5f + 0.2f) * B;
			const float X = H.X + N.X * Dd + T.X * Wd, Y = H.Y + N.Y * Dd + T.Y * Wd;
			if (X > XQuayClear + 2.f) return false;
			if (Hill.RoadDist(X, Y) < -0.25f) return false;
			if (Hill.PadDist(X, Y) < 1.5f) return false;
			const float Z = Hill.Base(X, Y); Zmin = FMath::Min(Zmin, Z); Zmax = FMath::Max(Zmax, Z);
		}
		if (Zmin < 2.8f || Zmax - Zmin > 9.0f) return false;
		for (const FP23House& O : Out.Houses) if (Over(H, O, O.Cluster == ClusterId ? 0.15f : 0.85f)) return false;
		return true;
	};
	static const float BaseLo[6] = { 3.8f, 7.5f, 11.5f, 15.5f, 19.5f, 23.5f }, BaseHi[6] = { 7.5f, 11.5f, 15.5f, 19.5f, 23.5f, 29.5f };
	static const int32 BaseQuota[6] = { 13, 12, 10, 8, 6, 4 };
	for (int32 B = 0; B < 6; ++B)
	{
		const float Lo = 2.f + (BaseLo[B] - 2.f) * Hs, Hi = 2.f + (BaseHi[B] - 2.f) * Hs;
		const int32 Target = FMath::Max(2, FMath::RoundToInt(BaseQuota[B] * FMath::Clamp(Sc * 1.22f, 0.65f, 1.f)));
		int32 Got = 0, Tries = 0;
		while (Got < Target && Tries < 28000)
		{
			++Tries;
			const float X = R.FRandRange(XQuayClear - 4.f, -50.f - 195.f * Sc), Y = R.FRandRange(-YMaxEnv * 0.92f, YMaxEnv * 0.92f);
			const float Zs = Hill.Base(X, Y);
			if (Zs < Lo || Zs >= Hi) continue;
			++ClusterId;
			const int32 Before = Out.Houses.Num();
			const float Phi0 = Down(X, Y);
			FVector2D N0, T0; Axes(Phi0, N0, T0);
			const int32 Count = 2 + R.RandRange(0, 2);
			float Off = 0.f; int32 Fails = 0;
			for (int32 K = 0; K < Count && Fails < 5; ++K)
			{
				FP23House H;
				const float RTy = R.FRand();
				H.Type = RTy < 0.24f ? 0 : RTy < 0.58f ? 1 : 2;
				H.S = R.FRandRange(0.78f, 1.12f); H.Sz = R.FRandRange(0.84f, 1.32f);
				H.W = IwP23::TypeW[H.Type] * H.S; H.D = IwP23::TypeD[H.Type] * H.S;
				H.H = IwP23::TypeH[H.Type] * H.S * H.Sz; H.Ridge = IwP23::TypeRidge[H.Type] * H.S * H.Sz;
				H.X = X + T0.X * Off + N0.X * R.FRandRange(-3.f, 3.f);
				H.Y = Y + T0.Y * Off + N0.Y * R.FRandRange(-3.f, 3.f);
				H.Phi = Down(H.X, H.Y) + R.FRandRange(-12.f, 12.f);
				H.Tint = R.RandRange(0, 6); H.Cluster = ClusterId;
				if (Got + (Out.Houses.Num() - Before) >= Target) break;
				if (!Accept(H)) { Off += 2.8f; ++Fails; --K; continue; }
				H.Z = Hill.Base(H.X, H.Y);
				Out.Houses.Add(H);
				Off += 0.5f * (H.W + 7.5f) + R.FRandRange(0.8f, 2.0f);
			}
			const int32 Placed = Out.Houses.Num() - Before;
			if (Placed < 1) { Out.Houses.SetNum(Before); --ClusterId; continue; }
			Got += Placed;
		}
	}
	Out.Clusters = ClusterId;
	for (const FP23House& H : Out.Houses)
	{
		for (int32 K = 0; K < 4; ++K)
		{
			const float A = R.FRandRange(0.f, 2.f * PI), Rr = 0.5f * FMath::Max(H.W, H.D) + R.FRandRange(2.5f, 8.f);
			const float X = H.X + FMath::Cos(A) * Rr, Y = H.Y + FMath::Sin(A) * Rr;
			if (X > XQuayClear + 3.f || Hill.Base(X, Y) < 2.6f || Hill.RoadDist(X, Y) < 2.0f || Slope(X, Y) > 33.f) continue;
			FP23Veg V; V.X = X; V.Y = Y;
			V.Kind = (K >= 2 && R.FRand() < 0.65f) ? (R.FRand() < 0.5f ? 1 : 2) : 0;
			V.S = (V.Kind == 0) ? R.FRandRange(1.2f, 2.4f) : R.FRandRange(0.75f, 1.25f);
			Out.Veg.Add(V);
		}
	}
}

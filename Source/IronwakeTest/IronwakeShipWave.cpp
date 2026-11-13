// P19 + P27: per-ship Havelock LUT wave field. Synchronized with Content/Python/iw_p27_water_lut.py (SHIPWAVE_BODY).
#include "IronwakeShipWave.h"
#include "IronwakeHydro.h"
#include "IronwakeTest.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
	TArray<FIwShipWaveSrc> GSrc;
	int32 GActiveSlots = 0;
	constexpr double Ttl = 0.6;
	constexpr float G = 9.81f;
	constexpr int32 LutRes = 256;

	TArray<uint8> GLutBytes;
	bool bGLutTried = false;

	void EnsureKelvinLutLoaded()
	{
		if (bGLutTried) return;
		bGLutTried = true;
		const FString Path = FPaths::ProjectContentDir() / TEXT("Data/kelvin_lut_256.bin");
		if (FFileHelper::LoadFileToArray(GLutBytes, *Path) && GLutBytes.Num() == LutRes * LutRes * 4)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP27 loaded CPU Havelock LUT %s (%d bytes)"), *Path, GLutBytes.Num());
		}
		else
		{
			GLutBytes.Reset();
			UE_LOG(LogIronwake, Warning, TEXT("IWP27 CPU Havelock LUT missing (%s); using analytic fallback"), *Path);
		}
	}

	FVector4f SampleLut256(float U, float V)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f) * (LutRes - 1);
		const float Y = FMath::Clamp(V, 0.f, 1.f) * (LutRes - 1);
		const int32 X0 = FMath::Clamp((int32)FMath::FloorToInt(X), 0, LutRes - 1);
		const int32 Y0 = FMath::Clamp((int32)FMath::FloorToInt(Y), 0, LutRes - 1);
		const int32 X1 = FMath::Min(X0 + 1, LutRes - 1);
		const int32 Y1 = FMath::Min(Y0 + 1, LutRes - 1);
		const float Fx = X - X0, Fy = Y - Y0;
		auto Px = [](int32 Ix, int32 Iy) -> FVector4f
		{
			const int32 Off = (Iy * LutRes + Ix) * 4;
			const float Inv = 1.f / 255.f;
			return FVector4f(GLutBytes[Off] * Inv, GLutBytes[Off + 1] * Inv, GLutBytes[Off + 2] * Inv, GLutBytes[Off + 3] * Inv);
		};
		const FVector4f C00 = Px(X0, Y0), C10 = Px(X1, Y0), C01 = Px(X0, Y1), C11 = Px(X1, Y1);
		const FVector4f Top = C00 + (C10 - C00) * Fx;
		const FVector4f Bot = C01 + (C11 - C01) * Fx;
		return Top + (Bot - Top) * Fy;
	}

	float HullHB(float u, float L, float HB)
	{
		const float Bow = FMath::Pow(FMath::Clamp(u / (0.2f * L), 0.f, 1.f), 0.65f);
		const float Stern = 1.f - 0.5f * FMath::SmoothStep(0.85f * L, L, u);
		return (u < 0.f || u > L) ? 0.f : HB * Bow * Stern;
	}
	inline float Sq(float x) { return x * x; }
	inline float Smooth01(float Lo, float Hi, float X) { return FMath::SmoothStep(Lo, Hi, X); }
}

namespace IronwakeSea
{
	bool ShipWaveEnabled() { static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("iwnoshipwave")); return !bOff; }
	int32 ShipWaveCount() { return GSrc.Num(); }
	int32 ActiveShipWaveSlots() { return GActiveSlots; }

	float WedgeHalfAngleDeg(float Fn) { return FMath::Lerp(19.47f, 12.f, FMath::Clamp((Fn - 0.25f) / 0.25f, 0.f, 1.f)); }

	void RegisterShipWave(const void* Key, const FIwShipWaveSrc& S)
	{
		FIwShipWaveSrc* D = nullptr;
		for (FIwShipWaveSrc& E : GSrc) if (E.Key == Key) { D = &E; break; }
		if (!D) D = &GSrc.AddDefaulted_GetRef();
		*D = S; D->Key = Key; D->Stamp = FPlatformTime::Seconds();
		GSrc.RemoveAll([](const FIwShipWaveSrc& E) { return FPlatformTime::Seconds() - E.Stamp > 5.0; });
	}
	void UnregisterShipWave(const void* Key) { GSrc.RemoveAll([Key](const FIwShipWaveSrc& E) { return E.Key == Key; }); }

	float ShipWaveEval(const FIwShipWaveSrc& S, const FVector2D& PCm, float FpCm, float* OutFoam, float* OutParts)
	{
		if (OutFoam) *OutFoam = 0.f;
		if (OutParts) OutParts[0] = OutParts[1] = OutParts[2] = OutParts[3] = 0.f;
		const float L = FMath::Max(S.L, 1.f);
		if (S.V < 0.15f) return 0.f;
		const FVector2D Dm = (PCm - S.PosCm) * 0.01f;
		if (Dm.SizeSquared() > 52.f * L * L) return 0.f;

		EnsureKelvinLutLoaded();
		const float Xl = Dm.X * S.CosH + Dm.Y * S.SinH;
		const float Yl = -Dm.X * S.SinH + Dm.Y * S.CosH;
		const float U = S.X1 - Xl;
		const float Ay = FMath::Abs(Yl);

		if (GLutBytes.Num() == LutRes * LutRes * 4)
		{
			const float XLut = 0.48f - 0.96f * (U / L);
			if (XLut <= -6.45f || XLut >= 1.45f || Ay >= 2.95f * L) return 0.f;

			const float Fn = S.V / FMath::Sqrt(G * L);
			const float KnApprox = S.V / (0.514444f * 1.5f);
			const float SpdGate = Smooth01(2.2f, 7.0f, KnApprox);
			const float SpdFoam = FMath::Pow(Smooth01(3.2f, 13.5f, KnApprox), 1.55f);
			const float Gk = FMath::Pow(FMath::Clamp(Fn / 0.32f, 0.f, 1.f), 1.30f) * SpdGate;
			if (Gk < 0.0008f && FMath::Abs(S.ThrustSteam) < 0.02f) return 0.f;

			const float SAft = FMath::Max(-0.48f - XLut, 0.f);
			const float MaxWakeL = FMath::Lerp(0.65f, 6.0f, Smooth01(2.0f, 13.0f, KnApprox));
			const float AftFade = (XLut < -0.48f) ? (1.f - Smooth01(MaxWakeL * 0.58f, MaxWakeL, SAft)) : 1.f;
			if (AftFade < 0.002f) return 0.f;

			const float HB = FMath::Max(S.HalfBeam, 0.5f);
			const float UH = FMath::Abs(XLut) * (1.f / 0.48f);
			const float BLut = (UH < 1.f) ? (0.085f * FMath::Pow(FMath::Max(1.f - FMath::Pow(UH, 2.4f), 0.f), 0.65f)) : 0.f;
			float HbReal = 0.f;
			if (U >= -0.03f * L && U <= 1.03f * L)
			{
				const float BowEnv = FMath::Pow(FMath::Clamp((U + 0.03f * L) / (0.14f * L), 0.f, 1.f), 0.42f);
				const float StnEnv = 1.f - 0.25f * Smooth01(0.88f * L, 1.03f * L, U);
				HbReal = HB * 1.04f * BowEnv * StnEnv + S.RollOutM + 0.25f;
			}

			const float DOut = Ay - HbReal;
			const float DOutL = FMath::Max(DOut, 0.f) / L;
			const float WNear = 1.f / (1.f + 16.f * DOutL * DOutL);
			const float YLutAbs = (DOut >= 0.f) ? FMath::Lerp(Ay / L, BLut + DOutL, WNear) : (BLut * FMath::Clamp(Ay / FMath::Max(HbReal, 0.1f), 0.f, 1.f));
			const float YLut = (Yl >= 0.f ? 1.f : -1.f) * YLutAbs;
			if (FMath::Abs(YLut) >= 2.95f) return 0.f;

			const float SBow = FMath::Max(0.50f - XLut, 0.08f);
			const float Ray = FMath::Abs(YLut) / SBow;
			const float KelvinCone = (XLut < 0.15f) ? (1.f - Smooth01(0.365f, 0.435f, Ray)) : 1.f;
			if (KelvinCone < 0.002f) return 0.f;

			const float UvX = (YLut + 3.0f) * (1.f / 6.0f);
			const float UvY = (XLut + 6.5f) * (1.f / 8.0f);
			const float EnvX = Smooth01(-6.45f, -5.55f, XLut) * (1.f - Smooth01(1.05f, 1.40f, XLut));
			const float EnvY = 1.f - Smooth01(2.40f, 2.95f, FMath::Abs(YLut));
			const float Env = EnvX * EnvY * AftFade * KelvinCone;

			const FVector4f W4 = SampleLut256(UvX, UvY);
			const float HzR = (W4.X - 0.50196078f) * 2.007874f;
			const float HzG = (W4.Y - 0.50196078f) * 2.007874f;
			const float HzB = (W4.Z - 0.50196078f) * 2.007874f;
			const float W12 = FMath::Clamp((Fn - 0.15f) * (1.f / 0.09f), 0.f, 1.f);
			const float W23 = FMath::Clamp((Fn - 0.22f) * (1.f / 0.10f), 0.f, 1.f);
			const float HNorm = FMath::Lerp(FMath::Lerp(HzR, HzG, W12), HzB, W23);

			const float CuspBoost = (XLut < -0.52f) ? FMath::Lerp(0.76f, 1.26f, Smooth01(0.18f, 0.30f, Ray)) : 1.f;
			const float HbAmp = S.CrestK * S.Freeboard * Gk * 1.40f;
			const float TrAmp = S.TroughK * S.Draft * Gk * 1.25f;
			const float MidWin = FMath::Exp(-Sq((XLut - 0.02f) / 0.36f)) * FMath::Exp(-Sq(DOutL / 0.22f));
			const float AftWin = Smooth01(0.20f, -0.55f, XLut);
			const float PosScale = FMath::Lerp(HbAmp, HbAmp * FMath::Max(S.WedgeK, 0.70f) * 1.28f * CuspBoost, AftWin);
			const float NegScale = FMath::Lerp(HbAmp * FMath::Max(S.WedgeK, 0.70f) * 1.10f * CuspBoost, TrAmp, MidWin);
			const float Fp = FpCm > 0.f ? FpCm : 60.f;
			const float BandLim = 1.f / (1.f + Sq(Fp / FMath::Max(L * 18.0f, 45.0f)));
			const float HiM = (HNorm >= 0.f ? HNorm * PosScale : HNorm * NegScale) * Env * BandLim;
			const float HullSuppress = (UH < 1.00f && DOut < 0.15f) ? Smooth01(-0.85f, 0.15f, DOut) : 1.f;
			const float FinalM = HiM * HullSuppress;

			if (OutFoam)
			{
				const float OutGate = (UH < 1.04f) ? Smooth01(0.05f, 0.55f, DOut) : 1.f;
				*OutFoam = FMath::Clamp(W4.W * SpdFoam * OutGate * Env, 0.f, 1.f);
			}
			if (OutParts)
			{
				if (U <= 0.22f * L) OutParts[0] = FMath::Max(HiM, 0.f);
				if (U >= 0.18f * L && U <= 0.56f * L) OutParts[1] = FMath::Min(HiM, 0.f);
				if (U >= 0.50f * L && U <= 0.95f * L) OutParts[2] = FMath::Max(HiM, 0.f);
				if (U > 0.95f * L) OutParts[3] = HiM;
			}
			return FinalM * 100.f;
		}

		// Analytic fallback if Content/Data/kelvin_lut_256.bin is unavailable
		const float Fn = S.V / FMath::Sqrt(G * L);
		const float Gk = FMath::Pow(FMath::Clamp(Fn / 0.32f, 0.f, 1.f), 1.5f);
		if (Gk < 0.002f) return 0.f;
		const float HbL = HullHB(U, L, S.HalfBeam);
		const float Lat = FMath::Max(Ay - HbL, 0.f);
		const float Hb = S.CrestK * S.Freeboard * Gk * (0.8f + 0.4f * S.Bluff);
		const float SU = U < 0.f ? U / (0.05f * L) : U / (0.09f * L);
		const float Lt = Lat / (0.07f * L + 1.f);
		const float B = Hb * FMath::Exp(-(Sq(SU) + Sq(Lt)));
		const float LatW = Sq(Lat / (0.5f * S.HalfBeam + 1.f));
		const float TrW = FMath::Exp(-Sq((U - 0.36f * L) / (0.13f * L))) * FMath::Exp(-LatW);
		const float T = -S.TroughK * S.Draft * Gk * TrW;
		const float Qh = S.QuarterK * Hb;
		const float QrW = FMath::Exp(-Sq((U - 0.64f * L) / (0.07f * L))) * FMath::Exp(-LatW);
		const float Q = Qh * QrW;
		if (OutParts) { OutParts[0] = B; OutParts[1] = T; OutParts[2] = Q; OutParts[3] = 0.f; }
		return (B + T + Q) * 100.f;
	}

	float ShipWaveZ(const FVector& P, const void* Skip)
	{
		if (!ShipWaveEnabled() || GSrc.Num() == 0) return 0.f;
		const double Now = FPlatformTime::Seconds();
		float Z = 0.f;
		const FVector2D P2(P.X, P.Y);
		for (const FIwShipWaveSrc& S : GSrc)
		{
			if (S.Key == Skip || Now - S.Stamp > Ttl) continue;
			Z += ShipWaveEval(S, P2, 0.f);
		}
		return Z;
	}

	void PushShipWaveParams(UMaterialInstanceDynamic* MID, const FVector& Cam)
	{
		if (!MID) return;
		const double Now = FPlatformTime::Seconds();
		const FVector2D C2(Cam.X, Cam.Y);
		const FIwShipWaveSrc* PlayerSrc = nullptr;
		TArray<const FIwShipWaveSrc*> Others;
		for (const FIwShipWaveSrc& S : GSrc)
		{
			if (Now - S.Stamp > Ttl) continue;
			if (S.V < 0.15f && FMath::Abs(S.ThrustSteam) < 0.02f) continue;
			if (S.bPlayer && !PlayerSrc) PlayerSrc = &S;
			else Others.Add(&S);
		}
		Others.Sort([&](const FIwShipWaveSrc& A, const FIwShipWaveSrc& B) {
			return FVector2D::DistSquared(A.PosCm, C2) - Sq(A.L * 250.f) < FVector2D::DistSquared(B.PosCm, C2) - Sq(B.L * 250.f);
		});

		TArray<const FIwShipWaveSrc*> Chosen;
		Chosen.Reserve(7);
		// Guarantee player ship gets slot 0 whenever camera is within 2.5 km (or <= 6 moving NPCs exist);
		// in far-off Photo-Mode harbour shots (>2.5 km away), give all 7 slots to the moving NPCs in view!
		const bool bPlayerNearCam = PlayerSrc && (Others.Num() < 7 || FVector2D::Distance(PlayerSrc->PosCm, C2) < 250000.f);
		if (bPlayerNearCam) Chosen.Add(PlayerSrc);
		for (const FIwShipWaveSrc* O : Others)
		{
			if (Chosen.Num() >= 7) break;
			Chosen.Add(O);
		}
		if (!bPlayerNearCam && PlayerSrc && Chosen.Num() < 7) Chosen.Add(PlayerSrc);

		static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("iwnoshipwave"));
		GActiveSlots = bOff ? 0 : Chosen.Num();
		for (int32 i = 0; i < 7; ++i)
		{
			FLinearColor A(0, 0, 1, 0), B(0, 0, 0, 0), Cc(0, 0, 0, 0), D(0, 0, 0, 0);
			if (!bOff && Chosen.IsValidIndex(i))
			{
				const FIwShipWaveSrc& S = *Chosen[i];
				const float SignedThrust = (S.ThrustSteam > 0.005f && S.bTwinScrew) ? -S.ThrustSteam : S.ThrustSteam;
				A = FLinearColor(S.PosCm.X, S.PosCm.Y, S.CosH, S.SinH);
				B = FLinearColor(S.V, S.L, S.X1, S.HalfBeam);
				Cc = FLinearColor(S.Draft, S.Freeboard, S.CrestK, S.TroughK);
				D = FLinearColor(S.QuarterK, S.WedgeK, SignedThrust, S.RollOutM);
			}
			MID->SetVectorParameterValue(*FString::Printf(TEXT("IW_SA%d"), i), A);
			MID->SetVectorParameterValue(*FString::Printf(TEXT("IW_SB%d"), i), B);
			MID->SetVectorParameterValue(*FString::Printf(TEXT("IW_SC%d"), i), Cc);
			MID->SetVectorParameterValue(*FString::Printf(TEXT("IW_SD%d"), i), D);
		}
	}
}

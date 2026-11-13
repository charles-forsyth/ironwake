#pragma once
// P8: NASA night sky dome (Deep Star Maps 2020 + LROC moon phase sprites), rotating with a game clock at ~50 N.
// Star map: NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC.
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronwakeCollision.h"
#include "IronwakeP8.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture;

UCLASS()
class IRONWAKETEST_API AIronwakeNightSky : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeNightSky();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	/** Night factor (weather preset, 1 = full night) and clear-sky factor 0..1 (0 = overcast/fog: no stars). */
	void SetState(float InNight, float InClear) { Night = InNight; Clear = InClear; }
	/** Freeze the game clock at an hour (0..24); tests only. */
	void SetClock(float H) { Hour = H; bHold = true; }
	float GetFade() const { return Fade; }
	FString Describe() const;
	FLinearColor LR0, LR1, LR2;
	/** World direction (+X north, +Y east, +Z up) of a J2000 RA/Dec, using the same rotation as the dome. */
	FVector DirOf(float RaDeg, float DecDeg) const;

	float Hour = 23.f;          // game clock, hours
	float ClockRate = 60.f;     // game seconds per real second
	bool bHold = false;
	int32 DayIdx = 0;           // day of year 0..364 -> moon phase table
	float LatDeg = 50.f;
	float PtThresh = 0.05f, PtGain = 5.f, GlowGain = 0.6f, GlowMip = 4.5f;
	float SkyGamma = 0.08f;
	float StarGain = 1.f, MoonGain = 1.0f, MoonRadiusDeg = 1.0f;
	float Night = 0.f, Clear = 0.f, Fade = 0.f;
	FVector MoonDirNEU = FVector::UpVector;
	float MoonPhaseDeg = 0.f;
	int32 MoonSprite = 4;

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Dome;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MID;
	UPROPERTY(Transient) TArray<TObjectPtr<UTexture>> MoonTex;
};

/** P8: an NPC craft on a slow elliptical loop (boats ride the rendered sea, airships fly at a fixed altitude). No AI. */
UCLASS()
class IRONWAKETEST_API AIronwakeNpcCraft : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeNpcCraft();
	virtual void Tick(float Dt) override;
	void Setup(const TCHAR* MeshName, const FVector& CentreCm, float RadA, float RadB, float PeriodS, float AltCm, float KeelCm, float Phase, bool bCW, bool bAir);
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Body;
	FVector Centre = FVector::ZeroVector;
	float A = 50000.f, B = 50000.f, Period = 600.f, Alt = 0.f, Keel = 0.f, Ph = 0.f, T = 0.f, Sgn = 1.f;
	bool bAirship = false;
	FRotator Cur = FRotator::ZeroRotator;
	// P9: boats hold their loop position while blocked; ramming slows them.
	FIwShape CollShape;
	float SlowUntil = -1.f;
	bool bCollReg = false, bAudited = false;
};

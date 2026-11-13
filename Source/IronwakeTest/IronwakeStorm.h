#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IronwakeStorm.generated.h"

class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UDirectionalLightComponent;
class USoundBase;

/**
 * W2: weather -> sea state director. One weather, one sea:
 *   CALM (0.5-1 m long swell) . MODERATE (1.5-2.5 m + chop) . ROUGH (3-5 m, whitecaps) . GALE (6-8 m) . STORM (10-14 m peaks).
 * The open-sea state is faded down near land (Greyhaven harbour, St Brannoc island) so the harbour stays calm and you watch
 * the sea build as you sail out. Drives the water material's crest foam, wind, spume ripped off crests (Niagara, budget
 * capped, LOD by distance), rain, darker low cloud/fog, lightning flashes with distance-delayed thunder, and the CC0
 * wind/rain/creak audio layers. Optional slow weather cycle CALM -> ROUGH -> STORM -> ... (K key cycle mode).
 */
UCLASS()
class IRONWAKETEST_API AIronwakeStorm : public AActor
{
	GENERATED_BODY()
public:
	AIronwakeStorm();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;
	static AIronwakeStorm* Get(const UWorld* W);

	/** Open-sea target state 0..4 (blends over Seconds). */
	void SetTarget(float State, float Seconds);
	float GetTarget() const { return Target; }
	/** Effective state at the player (after the land fade), 0..4. */
	float GetLocalState() const { return Local; }
	float GetOpenState() const { return Open; }
	/** 0..1 storminess for visuals (0 below ROUGH, 1 at STORM). */
	float Storminess() const { return FMath::Clamp((Local - 2.f) / 2.f, 0.f, 1.f); }
	bool bCycle = false;
	/** P11: absolute cloud coverage 0..1 from the weather system (-1 = use the map's baseline), rain amount 0..1, and 'a test set the sea itself' flag. */
	float CloudCover = -1.f;
	float RainAmount = 0.f;
	/** P11: follow the weather system without a blend (it already smooths). */
	void SetOpenImmediate(float State) { Target = From = Open = FMath::Clamp(State, 0.f, 4.f); BlendT = 1.f; }
	/** P8: 0..1 clear-sky request (clear night): thins the volumetric clouds so the star dome shows. */
	float ClearSky = 0.f;
	float CycleT = 0.f;
	/** Lightning flash brightness 0..1 this frame (HUD/post can use it). */
	float Flash = 0.f;
	float FlashPeak = 0.f, FlashRe = -1.f, CamYaw = 0.f;
	int32 Strikes = 0;
	/** Land fade: 1 at sea, ~0.2 in the harbour. */
	float LandFade(const FVector& P) const;

	UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> Rain;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> AudLayers; // wind_sea, gale, howl, rain, strain
	UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> SpumeFX;
	UPROPERTY(Transient) TObjectPtr<USoundBase> ThunderNear;
	UPROPERTY(Transient) TObjectPtr<USoundBase> ThunderFar;
	UPROPERTY(Transient) TObjectPtr<USoundBase> CreakSnd;
	UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> Bolt; // lightning flash light
	int32 SpumeLive = 0;
	int32 SpumeTotal = 0;
private:
	float Target = 1.f, From = 1.f, Open = 1.f, Local = 1.f, BlendT = 1.f, BlendDur = 1.f;
	float LastApplied = -10.f;
	float SpumeAcc = 0.f, NextStrike = 8.f, ThunderAt = -1.f, ThunderVol = 1.f, CreakIn = 6.f;
	bool bThunderNear = true;
	TArray<float> SpumeDeaths;
	float CloudBase = -1.f, FogBase = -1.f, SkyBase = -1.f, SunBase = -1.f;
	TWeakObjectPtr<class AVolumetricCloud> Cloud;
	TWeakObjectPtr<class AExponentialHeightFog> Fog;
	TWeakObjectPtr<class ADirectionalLight> Sun;
	TWeakObjectPtr<class ASkyLight> Sky;
	class UMaterialInstanceDynamic* CloudMID = nullptr;
	void TickSpume(float Dt, const FVector& Cam, const FVector& ShipLoc);
	void TickLightning(float Dt, const FVector& Cam);
	void TickAudio(float Dt);
	void TickAtmosphere(float Dt);
	TArray<FVector> LandPts; TArray<float> LandR; FVector HarbourC = FVector(-345000.f, -335000.f, 0.f);
	float NextLog = 0.f;
};

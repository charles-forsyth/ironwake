// W2: weather -> sea state, open-ocean storm (spume, rain, lightning + thunder, audio layers), land fade.
#include "IronwakeStorm.h"
#include "IronwakeArchipelago.h"
#include "IronwakeHydro.h"
#include "IronwakeShell.h"
#include "IronwakeShipPawn.h"
#include "IronwakeVoyage.h"
#include "IronwakeLivingSea.h"
#include "IronwakeFleet.h"
#include "IronwakeTest.h"
#include "EngineUtils.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/ExponentialHeightFogComponent.h"

#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

namespace { TWeakObjectPtr<AIronwakeStorm> GStorm; }

AIronwakeStorm* AIronwakeStorm::Get(const UWorld* W)
{
	AIronwakeStorm* S = GStorm.Get();
	return (S && S->GetWorld() == W) ? S : nullptr;
}

AIronwakeStorm::AIronwakeStorm()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AIronwakeStorm::BeginPlay()
{
	Super::BeginPlay();
	GStorm = this;
	UWorld* W = GetWorld();
	// P11: the harbour basin (inside the breakwaters) shelters at most 0.6 and fades out over ~400 m; every other shore only gives a
	// 0.3 lee that fades over ~300 m. Open water outside always shows the current sea state.
	if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(W)) { HarbourC = Vy->HarbourC; LandPts.Add(Vy->StacksC); LandR.Add(50000.f); }
	{
		FVector Island(-50000.f, 190000.f, 0.f);
		if (AIronwakeLivingSea* LS = AIronwakeLivingSea::Get(W)) Island = LS->IslandCentre;
		LandPts.Add(Island); LandR.Add(70000.f);
	}
	if (AIronwakeArchipelago* Ar = AIronwakeArchipelago::Get(W)) for (const FIsle& I : Ar->Isles) if (I.bTerrain) { LandPts.Add(I.C); LandR.Add(I.R * 1.2f); }
	auto Snd = [](const TCHAR* N) { return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Audio/%s.%s"), N, N)); };
	for (const TCHAR* N : { TEXT("wind_sea_proc"), TEXT("gale_proc"), TEXT("storm_howling_wind"), TEXT("rain_proc"), TEXT("creak_storm_strain") })
	{
		UAudioComponent* A = NewObject<UAudioComponent>(this);
		A->SetupAttachment(RootComponent);
		A->bAllowSpatialization = false; A->bIsUISound = false; A->bAutoActivate = false;
		A->SetSound(Snd(N));
		A->RegisterComponent();
		A->SetVolumeMultiplier(0.001f);
		if (A->Sound) A->Play(FMath::FRand() * 5.f);
		AudLayers.Add(A);
	}
	ThunderNear = Snd(TEXT("thunder_near")); ThunderFar = Snd(TEXT("thunder_far")); CreakSnd = Snd(TEXT("creak_wooden_hull"));
	SpumeFX = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_Spume.NS_IW_Spume"));
	if (UNiagaraSystem* R = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Ironwake/FX/NS_IW_Rain.NS_IW_Rain")))
	{
		Rain = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, R, GetActorLocation(), FRotator::ZeroRotator, FVector(1.f), false, false);
		if (Rain) Rain->Deactivate();
	}
	Bolt = NewObject<UDirectionalLightComponent>(this, TEXT("LightningBolt"));
	Bolt->SetupAttachment(RootComponent);
	Bolt->SetIntensity(0.f); Bolt->SetLightColor(FLinearColor(0.75f, 0.82f, 1.f)); Bolt->SetCastShadows(false);
	// W3: the flash also lights the volumetric clouds (second atmosphere light, no sun disk).
	Bolt->SetAtmosphereSunLight(true); Bolt->SetAtmosphereSunLightIndex(1); Bolt->SetAtmosphereSunDiskColorScale(FLinearColor::Black);
	Bolt->RegisterComponent();
	{ TActorIterator<AVolumetricCloud> It(W); if (It) Cloud = *It; }
	if (AVolumetricCloud* C = Cloud.Get())
		if (UVolumetricCloudComponent* CC = C->FindComponentByClass<UVolumetricCloudComponent>())
			if (UMaterialInterface* M = CC->GetMaterial())
			{
				float V = 0.f;
				if (M->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Cloud_GlobalCoverage")), V)) CloudBase = V;
				CloudMID = UMaterialInstanceDynamic::Create(M, this);
				CC->SetMaterial(CloudMID);
			}
	UE_LOG(LogIronwake, Display, TEXT("IWW2 storm director up: land=%d layers=%d rain=%d spume=%d cloudCov=%.2f"), LandPts.Num(), AudLayers.Num(), Rain != nullptr, SpumeFX != nullptr, CloudBase);
}

void AIronwakeStorm::SetTarget(float State, float Seconds)
{
	From = Open; Target = FMath::Clamp(State, 0.f, 4.f); BlendT = 0.f; BlendDur = FMath::Max(Seconds, 0.05f);
	UE_LOG(LogIronwake, Display, TEXT("IWW2 open-sea target -> %.1f (%s) over %.0f s"), Target, IronwakeSea::SeaStateName(Target), BlendDur);
}

float AIronwakeStorm::LandFade(const FVector& P) const
{
	float F = 1.f;
	// P11 + P29: every harbour basin (inside the breakwaters of Greyhaven, Ironcrag, Stigsby) shelters at most 0.40 (leaving >= 0.60 swell)
	// and fades out over ~250 m; every other shore only gives a 0.30 lee that fades over ~300 m.
	auto BasinFade = [&](const FVector& CentreCm, float YawDeg, float XMin, float XMax, float YMin, float YMax)
	{
		const FVector Hf = FRotator(0.f, YawDeg, 0.f).UnrotateVector(P - CentreCm) / 100.f;
		const float Dx = FMath::Max(FMath::Max(XMin - (float)Hf.X, 0.f), (float)Hf.X - XMax);
		const float Dy = FMath::Max(FMath::Max(YMin - (float)Hf.Y, 0.f), (float)Hf.Y - YMax);
		const float D = FMath::Sqrt(Dx * Dx + Dy * Dy);
		F = FMath::Min(F, FMath::Lerp(1.f - 0.4f, 1.f, FMath::SmoothStep(0.f, 250.f, D)));
	};
	BasinFade(HarbourC, 45.f, -50.f, 150.f, -160.f, 170.f);
	if (const AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()))
	{
		for (const FIwPortSpec& Ps : Vy->WorldPorts)
			if (!Ps.bMainHarbour) BasinFade(Ps.WorldCentreCm, Ps.SeawardYaw, -55.f, 115.f, -110.f, 110.f);
	}
	for (int32 i = 0; i < LandPts.Num(); ++i)
	{
		const float D = FVector::Dist2D(P, LandPts[i]);
		const float R = LandR[i];
		const float U = FMath::SmoothStep(R, R + 30000.f, D);   // shoreline lee: 0.3 max, gone 300 m past the shore
		F = FMath::Min(F, FMath::Lerp(1.f - 0.3f, 1.f, U));
	}
	return F;
}

void AIronwakeStorm::Tick(float DtIn)
{
	Super::Tick(DtIn);
	UWorld* W = GetWorld();
	if (!W || W->IsPaused()) return;
	const float Dt = FMath::Min(DtIn, 0.1f);
	if (BlendT < 1.f) { BlendT = FMath::Min(1.f, BlendT + Dt / BlendDur); Open = FMath::Lerp(From, Target, FMath::SmoothStep(0.f, 1.f, BlendT)); }
	else Open = Target;
	APlayerController* PCtl = W->GetFirstPlayerController();
	APawn* Pn = PCtl ? PCtl->GetPawn() : nullptr;
	const FVector ShipLoc = Pn ? Pn->GetActorLocation() : FVector::ZeroVector;
	const FVector Cam = (PCtl && PCtl->PlayerCameraManager) ? PCtl->PlayerCameraManager->GetCameraLocation() : ShipLoc;
	// The sea at the player: calm in the harbour lee, building to the open-sea state offshore. The Water plugin ocean
	// has one global spectrum, so the whole visible sea follows the player's local state (you watch it build).
	const float Fade = LandFade(ShipLoc);
	const float Want = Open * Fade;
	Local = FMath::FInterpTo(Local, Want, Dt, 0.5f);
	if (FMath::Abs(Local - LastApplied) > 0.02f || (LastApplied < 0.f))
	{
		IronwakeSea::SetSeaState(W, Local);
		LastApplied = Local;
	}
	IronwakeSea::SetFoamCamera(W, Cam);
	TickAtmosphere(Dt);
	CamYaw = (PCtl && PCtl->PlayerCameraManager) ? PCtl->PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
	TickSpume(Dt, Cam, ShipLoc);
	TickLightning(Dt, Cam);
	TickAudio(Dt);
	if (Rain)
	{
		const bool bRain = RainAmount > 0.05f || Local >= 2.6f;
		if (bRain != Rain->IsActive()) { if (bRain) Rain->Activate(true); else Rain->Deactivate(); }
		const FRotator CR = (PCtl && PCtl->PlayerCameraManager) ? PCtl->PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator;
		Rain->SetWorldLocation(Cam + FRotator(0.f, CR.Yaw, 0.f).Vector() * 1800.f + FVector(0, 0, 1800.f));
		Rain->SetWorldRotation(FRotator(0.f, IronwakeSea::WaveTravelYaw(), 0.f)); // slant downwind
	}
	if (W->GetTimeSeconds() >= NextLog)
	{
		NextLog = W->GetTimeSeconds() + 10.f;
		UE_LOG(LogIronwake, Display, TEXT("IWW2 tick open=%.2f fade=%.2f local=%.2f (%s) Hs=%.1f m wind=%.0f kn spume live=%d total=%d strikes=%d rain=%d"),
			Open, Fade, Local, IronwakeSea::SeaStateName(Local), IronwakeSea::SigWaveHeightM(), IronwakeFleet::WindKn(W), SpumeLive, SpumeTotal, Strikes, Rain && Rain->IsActive());
	}
}

void AIronwakeStorm::TickAtmosphere(float Dt)
{
	const float St = Storminess();
	if (CloudMID && CloudBase >= 0.f)
	{
		const float Base = CloudCover >= 0.f ? CloudCover : CloudBase;
		CloudMID->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), FMath::Lerp(Base, FMath::Max(Base, 0.95f), St) * (CloudCover >= 0.f ? 1.f : 1.f - 0.92f * ClearSky));
	}
	// P13: overcast reads as a structured, darker deck: denser and greyer as the cover rises (coverage alone is a flat white-grey sheet).
	if (CloudMID && CloudCover >= 0.f)
	{
		const float Ov = FMath::SmoothStep(0.5f, 0.95f, CloudCover) * (1.f - ClearSky);
		CloudMID->SetScalarParameterValue(TEXT("Cloud_GlobalDensity"), FMath::Lerp(0.008f, 0.022f, Ov));
		CloudMID->SetVectorParameterValue(TEXT("Cloud_AlbedoColor"), FMath::Lerp(FLinearColor(0.98f, 0.98f, 0.98f, 0.5f), FLinearColor(0.52f, 0.56f, 0.62f, 0.5f), Ov));
	}
	// Lightning flash decays fast (two pulses).
	// Flicker: a main stroke plus 1-2 return strokes ~80-150 ms apart.
	Flash = FMath::Max(0.f, Flash - Dt * 4.f);
	if (FlashRe > 0.f) { FlashRe -= Dt; if (FlashRe <= 0.f) Flash = FMath::Max(Flash, FlashPeak * 0.8f); }
	if (Bolt) Bolt->SetIntensity(Flash * 30.f);
}

void AIronwakeStorm::TickSpume(float Dt, const FVector& Cam, const FVector& ShipLoc)
{
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	for (int32 i = SpumeDeaths.Num() - 1; i >= 0; --i) if (SpumeDeaths[i] < Now) SpumeDeaths.RemoveAtSwap(i);
	SpumeLive = SpumeDeaths.Num();
	// Spume only in GALE+ (wind > ~30 kn): ripped off sharp crests downwind. Budget-capped live bursts, LOD by distance
	// (more candidates close to the camera, none beyond 450 m).
	const float G = FMath::Clamp((Local - 1.8f) / 2.2f, 0.f, 1.f); // whitecap spray from ROUGH, spume in GALE/STORM
	if (G <= 0.f || !SpumeFX) return;
	static const bool bNoSpume = FParse::Param(FCommandLine::Get(), TEXT("iwnospume"));
	if (bNoSpume) return;
	const int32 Budget = FMath::RoundToInt(FMath::Lerp(40.f, 160.f, G)); // W3
	SpumeAcc += Dt * FMath::Lerp(60.f, 260.f, G); // candidate tests per second
	const float Down = FMath::DegreesToRadians(IronwakeSea::WaveTravelYaw());
	const FVector DownV(FMath::Cos(Down), FMath::Sin(Down), 0.f);
	while (SpumeAcc >= 1.f)
	{
		SpumeAcc -= 1.f;
		if (SpumeDeaths.Num() >= Budget) { SpumeAcc = 0.f; break; }
		const float R = FMath::Lerp(30.f, 500.f, FMath::Square(FMath::FRand())) * 100.f;
		// W3: candidates inside the camera's view cone (+/-50 deg), so the budget is spent where it can be seen.
		const float A = FMath::DegreesToRadians(CamYaw + FMath::FRandRange(-50.f, 50.f));
		FVector P = Cam + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
		if (FVector::Dist2D(P, ShipLoc) < 2500.f) continue;
		float J = 1.f;
		const float Z = IronwakeSea::FastZ(W, P, &J);
		// Crest test: the Gerstner Jacobian pinches on sharp crests; above-mean height too.
		if (J > FMath::Lerp(0.55f, 0.72f, G) || Z < IronwakeSea::FastZ(W, ShipLoc) - 150.f) continue;
		P.Z = Z + 30.f;
		const FVector Axis = (DownV * 0.97f + FVector::UpVector * 0.1f).GetSafeNormal(); // W4: low, long, blown downwind
		const FRotator Rot = FRotationMatrix::MakeFromZ(Axis).Rotator();
		if (UNiagaraComponent* N = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpumeFX, P, Rot, FVector(FMath::Lerp(0.7f, 1.4f, G) * FMath::Clamp(R / 15000.f, 0.8f, 2.2f)), true, true, ENCPoolMethod::AutoRelease))
		{
			SpumeDeaths.Add(Now + 3.5f);
			++SpumeTotal;
		}
	}
}

void AIronwakeStorm::TickLightning(float Dt, const FVector& Cam)
{
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	const float St = Storminess();
	if (ThunderAt > 0.f && Now >= ThunderAt)
	{
		ThunderAt = -1.f;
		if (USoundBase* S = bThunderNear ? ThunderNear.Get() : ThunderFar.Get()) UGameplayStatics::PlaySound2D(this, S, ThunderVol, FMath::FRandRange(0.85f, 1.05f));
	}
	if (St < 0.6f) { NextStrike = Now + 6.f; return; }
	if (Now < NextStrike) return;
	NextStrike = Now + FMath::FRandRange(7.f, 18.f) / FMath::Max(St, 0.6f);
	const float DistM = FMath::FRandRange(900.f, 7000.f);
	Flash = FMath::Lerp(0.5f, 1.4f, FMath::Clamp(1.f - DistM / 7000.f, 0.f, 1.f)); FlashPeak = Flash; FlashRe = FMath::FRandRange(0.08f, 0.16f);
	if (Bolt) Bolt->SetWorldRotation(FRotator(FMath::FRandRange(-70.f, -35.f), FMath::FRand() * 360.f, 0.f));
	// Thunder arrives distance / 343 m/s later (3-20 s), near strikes crack, far ones roll.
	ThunderAt = Now + DistM / 343.f;
	bThunderNear = DistM < 2800.f;
	ThunderVol = FMath::Lerp(1.6f, 0.6f, DistM / 7000.f);
	++Strikes;
	UE_LOG(LogIronwake, Display, TEXT("IWW2 lightning #%d at %.0f m, thunder in %.1f s"), Strikes, DistM, DistM / 343.f);
}

void AIronwakeStorm::TickAudio(float Dt)
{
	UWorld* W = GetWorld();
	const float Wind = IronwakeFleet::WindKn(W);
	const bool bRain = Rain && Rain->IsActive();
	AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr);
	const bool bSail = P && P->HasSails();
	const float Vol[5] = {
		FMath::Clamp(Wind / 20.f, 0.15f, 1.f) * 0.55f * (1.f - 0.5f * FMath::SmoothStep(30.f, 45.f, Wind)),
		FMath::SmoothStep(24.f, 36.f, Wind) * 0.9f,
		FMath::SmoothStep(38.f, 48.f, Wind) * 0.8f,
		bRain ? FMath::Lerp(0.25f, 0.9f, FMath::Max(RainAmount, Storminess())) : 0.f,
		(bSail && Local >= 2.8f) ? FMath::SmoothStep(2.8f, 4.f, Local) * 0.75f : 0.f };
	for (int32 i = 0; i < AudLayers.Num() && i < 5; ++i)
		if (UAudioComponent* A = AudLayers[i]) if (A->Sound) A->SetVolumeMultiplier(FMath::Max(Vol[i], 0.001f));
	// Random hull/rigging creaks on sailing ships (more with roll and wind).
	if (bSail && CreakSnd)
	{
		CreakIn -= Dt * (1.f + FMath::Abs(P->GetActorRotation().Roll) / 6.f);
		if (CreakIn <= 0.f)
		{
			CreakIn = FMath::FRandRange(6.f, 20.f);
			UGameplayStatics::PlaySound2D(this, CreakSnd, FMath::Clamp(0.35f + Wind / 60.f, 0.3f, 1.1f), FMath::FRandRange(0.85f, 1.1f));
		}
	}
}

// Card C: gunnery, damage/fires/sinking, NPC utility AI, combat audio, combat demo automation.
// Methods of AIronwakeShipPawn kept in their own file so IronwakeShipPawn.cpp stays about sailing.
#include "IronwakeShipPawn.h"
#include "IronwakeShell.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"

#include "BuoyancyComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UnrealClient.h"
#include "IronwakeSettings.h"

namespace
{
	constexpr float G = 980.f;
	constexpr float DetectRange = 900000.f;   // 9 km: spotting range in this storm
	constexpr float CombatRange = 700000.f;   // 7 km: "beat to quarters"
	constexpr float BroadsideRange = 600000.f;
	constexpr float TurretArc = 150.f;        // +/- deg from the turret's rest bearing
	constexpr float ShellDamage = 6.f;
	constexpr float FireBurnPerSec = 0.35f;

	float Now(const UWorld* W) { return W ? W->GetTimeSeconds() : 0.f; }

	UNiagaraSystem* FX(const TCHAR* Name)
	{
		return LoadObject<UNiagaraSystem>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/FX/%s.%s"), Name, Name));
	}
}

USoundBase* AIronwakeShipPawn::Snd(const TCHAR* Name)
{
	const FName Key(Name);
	if (TObjectPtr<USoundBase>* Found = SoundCache.Find(Key)) return *Found;
	USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Audio/%s.%s"), Name, Name));
	if (!S) UE_LOG(LogIronwake, Warning, TEXT("IWAUDIO missing sound %s"), Name);
	SoundCache.Add(Key, S);
	return S;
}

AIronwakeShipPawn* AIronwakeShipPawn::FindEnemy() const
{
	AIronwakeShipPawn* Best = nullptr;
	float BestD = TNumericLimits<float>::Max();
	for (TActorIterator<AIronwakeShipPawn> It(GetWorld()); It; ++It)
	{
		AIronwakeShipPawn* S = *It;
		if (S == this || S->bIsNPC == bIsNPC || !IsValid(S)) continue;
		const float D = FVector::DistSquared2D(S->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = S; }
	}
	return Best;
}

FVector AIronwakeShipPawn::GetHullVelocity() const
{
	if (bSinking || !HullBody->IsSimulatingPhysics()) return FVector::ZeroVector;
	FVector V = HullBody->GetPhysicsLinearVelocity();
	V.Z = 0.f;
	return V;
}

bool AIronwakeShipPawn::SolveLaunch(const FVector& From, const FVector& To, float V, const FVector& Carry, FVector& OutVel, float& OutT)
{
	// Low-arc solution; iterate so the muzzle carry (own-ship velocity) is compensated.
	float T = FVector::Dist2D(From, To) / FMath::Max(V, 1.f);
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector Tgt = To - Carry * T;
		FVector D = Tgt - From;
		const float H = D.Z;
		D.Z = 0.f;
		const float X = D.Size();
		if (X < 100.f) return false;
		const float V2 = V * V;
		const float Disc = V2 * V2 - G * (G * X * X + 2.f * H * V2);
		if (Disc < 0.f) return false;
		const float Theta = FMath::Atan((V2 - FMath::Sqrt(Disc)) / (G * X));
		T = X / (V * FMath::Cos(Theta));
		OutVel = D.GetSafeNormal() * V * FMath::Cos(Theta) + FVector(0.f, 0.f, V * FMath::Sin(Theta)) + Carry;
	}
	OutT = T;
	return true;
}

void AIronwakeShipPawn::CombatBeginPlay()
{
	if (!bIsNPC) MaxHP = 160.f; // the player's ship is the hero hull (DESIGN.md damage multiplier option)
	HP = MaxHP;
	AIStateName = bIsNPC ? TEXT("PATROL") : TEXT("");
	PatrolCentre = GetActorLocation();
	AIDesiredYaw = GetActorRotation().Yaw;

	USceneComponent* Pivots[2] = { TurretFore, TurretAft };
	for (int32 i = 0; i < 2; ++i)
	{
		TurretBaseYaw[i] = Pivots[i]->GetRelativeRotation().Yaw;
		TArray<USceneComponent*> Kids;
		Pivots[i]->GetChildrenComponents(false, Kids);
		for (USceneComponent* K : Kids)
		{
			if (K->GetName().Contains(TEXT("Barrel"))) Barrels[i].Add(K);
		}
	}

	FarAtten = NewObject<USoundAttenuation>(this);
	FarAtten->Attenuation.bAttenuate = true;
	FarAtten->Attenuation.bSpatialize = true;
	FarAtten->Attenuation.AttenuationShapeExtents = FVector(30000.f, 0.f, 0.f); // full volume within 300 m
	FarAtten->Attenuation.FalloffDistance = 900000.f;                         // fades out by ~9 km

	if (bIsNPC)
	{
		// Free Captains oxblood turrets (ART_BIBLE palette) so the enemy reads as hostile.
		if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* Ox = UMaterialInstanceDynamic::Create(BaseMat, this);
			Ox->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.19f, 0.022f, 0.03f));
			for (USceneComponent* P : Pivots)
			{
				TArray<USceneComponent*> Kids;
				P->GetChildrenComponents(true, Kids);
				for (USceneComponent* K : Kids) if (UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(K)) SM->SetMaterial(0, Ox);
			}
		}
		Camera->SetActive(false);
		ReloadSeconds = 7.f;
	}
	else
	{
		if (USoundBase* Calm = Snd(TEXT("main_theme")))
		{
			MusicCalm = UGameplayStatics::CreateSound2D(this, Calm, 0.55f, 1.f, 0.f, nullptr, false, false);
			if (MusicCalm && IronwakeSettings::GetMusic()) MusicCalm->FadeIn(4.f, 0.55f); // H1: music OFF by default
		}
		const FString Trigger = FPaths::ProjectSavedDir() / TEXT("iw_combatdemo.txt");
		if (IFileManager::Get().FileExists(*Trigger))
		{
			FString Prefix;
			FFileHelper::LoadFileToString(Prefix, *Trigger);
			Prefix.TrimStartAndEndInline();
			CDPrefix = Prefix.IsEmpty() ? TEXT("C") : Prefix;
			IFileManager::Get().Delete(*Trigger);
			bCombatDemo = true;
			if (GEngine) GEngine->Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
			UE_LOG(LogIronwake, Display, TEXT("IWCDEMO start prefix=%s"), *CDPrefix);
		}
	}
}

void AIronwakeShipPawn::TickCombat(float Dt)
{
	if (bSinking) { TickSinking(Dt); TickDamage(Dt); return; }
	for (float& R : TurretReload) R = FMath::Max(0.f, R - Dt);
	RepairCooldown = FMath::Max(0.f, RepairCooldown - Dt);
	if (bIsNPC) TickAI(Dt); else TickAim(Dt);
	TickTurrets(Dt);
	TickDamage(Dt);
	if (!bIsNPC)
	{
		TickAudio(Dt);
		if (bCombatDemo) TickCombatDemo(Dt);
		TickCapture(Dt);
	}
	CombatCheckAccum += Dt;
	if (CombatCheckAccum > 3.f)
	{
		CombatCheckAccum = 0.f;
		AIronwakeShipPawn* E = FindEnemy();
		UE_LOG(LogIronwake, Display, TEXT("IWCOMBAT %s hp=%.0f/%.0f fires=%d state=%s range=%.0fm shots=%d hits=%d enemyHP=%.0f tur=(%.0f,%.0f) onT=(%d,%d)"),
			bIsNPC ? TEXT("NPC") : TEXT("PLY"), HP, MaxHP, Fires.Num(), *AIStateName,
			E ? FVector::Dist2D(E->GetActorLocation(), GetActorLocation()) / 100.f : -1.f, ShellsFired, HitsScored, E ? E->HP : -1.f,
			TurretOffset[0], TurretOffset[1], TurretOnTarget[0], TurretOnTarget[1]);
	}
}

void AIronwakeShipPawn::TickAim(float Dt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	AIronwakeShipPawn* E = FindEnemy();
	// Lead: where the enemy will be when a shell fired now arrives.
	bHasLead = false;
	if (E && !E->bSinking)
	{
		const FVector Muzzle = GetActorLocation() + FVector(0, 0, 1300.f);
		FVector P = E->GetActorLocation();
		float T = 0.f;
		FVector Vel;
		bool bOk = false;
		for (int32 i = 0; i < 3; ++i)
		{
			P = E->GetActorLocation() + E->GetHullVelocity() * T;
			P.Z = AIronwakeShell::WaterZAt(GetWorld(), P) + 600.f; // aim mid-belt
			bOk = SolveLaunch(Muzzle, P, MuzzleVelocity, GetHullVelocity(), Vel, T);
			if (!bOk) break;
		}
		if (bOk && FVector::Dist2D(P, Muzzle) < MaxGunRange)
		{
			bHasLead = true;
			LeadPoint = P;
			LastFlightTime = T;
		}
	}
	if (bFreezeAim || !PC || !PC->PlayerCameraManager) return;
	if (bCombatDemo && bHasLead && CDTime > 2.f)
	{
		// Scripted gunner: lay the camera crosshair on the lead point (same as a player using the indicator).
		const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		const FRotator R = (LeadPoint - CamLoc).Rotation();
		PC->SetControlRotation(FRotator(FMath::Clamp(R.Pitch, -60.f, 5.f), R.Yaw, 0.f));
	}
	// Tour/cinematic view target: the camera isn't ours, so the scripted gunner aims straight at the lead point.
	if (bCombatDemo && PC->GetViewTarget() != this)
	{
		if (bHasLead) { AimPoint = LeadPoint; AimRange = FVector::Dist2D(LeadPoint, GetActorLocation()); }
		return;
	}
	// Camera ray -> sea plane (or max range) = aim point.
	const FVector O = PC->PlayerCameraManager->GetCameraLocation();
	const FVector D = PC->PlayerCameraManager->GetCameraRotation().Vector();
	const float SeaZ = -150.f;
	FVector Hit;
	if (D.Z < -1e-4f)
	{
		const float T = (SeaZ - O.Z) / D.Z;
		Hit = O + D * T;
	}
	else
	{
		Hit = O + D.GetSafeNormal2D() * MaxGunRange;
		Hit.Z = SeaZ;
	}
	const FVector Me = GetActorLocation();
	if (FVector::Dist2D(Hit, Me) > MaxGunRange)
	{
		Hit = Me + (Hit - Me).GetSafeNormal2D() * MaxGunRange;
		Hit.Z = SeaZ;
	}
	// P1: never lay the guns closer than MinAimRangeCm (a steep early click used to splash beside our own hull).
	if (FVector::Dist2D(Hit, Me) < MinAimRangeCm)
	{
		FVector Dir = (Hit - Me).GetSafeNormal2D();
		if (Dir.IsNearlyZero()) Dir = D.GetSafeNormal2D();
		Hit = Me + Dir * MinAimRangeCm;
		Hit.Z = SeaZ;
	}
	// Rangefinder: when the crosshair is near the target (or its lead point), the range locks to it
	// instead of the sea surface far behind it (a grazing ray at 4 km would overshoot by ~1 km).
	if (E && !E->bSinking)
	{
		const FVector Refs[2] = { E->GetActorLocation(), bHasLead ? LeadPoint : E->GetActorLocation() };
		for (const FVector& Rf : Refs)
		{
			const float Along = FVector::DotProduct(Rf - O, D);
			const FVector Closest = O + D * Along;
			if (Along > 0.f && FVector::Dist2D(Closest, Rf) < 12000.f && FMath::Abs(Closest.Z - Rf.Z) < 6000.f)
			{
				Hit = Closest;
				Hit.Z = AIronwakeShell::WaterZAt(GetWorld(), Hit) + 600.f;
				break;
			}
		}
	}
	AimPoint = Hit;
	if (bAimAssist && bHasLead && FVector::Dist2D(AimPoint, LeadPoint) < 40000.f) AimPoint = LeadPoint;
	AimRange = FVector::Dist2D(AimPoint, Me);
}

void AIronwakeShipPawn::TickTurrets(float Dt)
{
	USceneComponent* Pivots[2] = { TurretFore, TurretAft };
	const FTransform Body = GetActorTransform();
	float ElevDeg = TurretElev;
	for (int32 i = 0; i < 2; ++i)
	{
		const FVector L = Body.InverseTransformPositionNoScale(AimPoint) - Pivots[i]->GetRelativeLocation();
		const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(L.Y, L.X));
		const float Want = FRotator::NormalizeAxis(Bearing - TurretBaseYaw[i]);
		const bool bInArc = FMath::Abs(Want) <= TurretArc;
		const float Target = FMath::Clamp(Want, -TurretArc, TurretArc);
		TurretOffset[i] = FMath::FInterpConstantTo(TurretOffset[i], Target, Dt, TurretTraverseDegPerSec);
		TurretOnTarget[i] = bInArc && FMath::Abs(TurretOffset[i] - Want) < 1.0f;
		Pivots[i]->SetRelativeRotation(FRotator(0.f, TurretBaseYaw[i] + TurretOffset[i], 0.f));
	}
	// Barrel elevation from the ballistic solution (shared by both turrets).
	FVector V; float T;
	if (SolveLaunch(GetActorLocation() + FVector(0, 0, 1300.f), AimPoint, MuzzleVelocity, GetHullVelocity(), V, T))
	{
		ElevDeg = FMath::RadiansToDegrees(FMath::Atan2(V.Z, V.Size2D()));
	}
	TurretElev = FMath::FInterpConstantTo(TurretElev, FMath::Clamp(ElevDeg, -2.f, 25.f), Dt, 4.f);
	const float Cs = FMath::Cos(FMath::DegreesToRadians(TurretElev)), Sn = FMath::Sin(FMath::DegreesToRadians(TurretElev));
	for (int32 i = 0; i < 2; ++i)
	{
		for (USceneComponent* B : Barrels[i])
		{
			// R1: library barrel mesh pivots at the trunnion; positive pitch = elevate.
			B->SetRelativeRotation(FRotator(TurretElev, 0.f, 0.f));
		}
	}
	(void)Cs; (void)Sn;
}

void AIronwakeShipPawn::TryFire()
{
	if (bSinking) return;
	for (int32 i = 0; i < 2; ++i)
	{
		if (TurretReload[i] <= 0.f && TurretOnTarget[i]) FireTurret(i, AimPoint);
	}
}

void AIronwakeShipPawn::FireTurret(int32 Index, const FVector& Target)
{
	UWorld* W = GetWorld();
	TurretReload[Index] = ReloadSeconds;
	const float SpeedKn = FMath::Abs(GetSpeedKnots());
	// Dispersion grows with own speed and turning (DESIGN.md); NPC gunners are a little rougher.
	const float Sigma = (bIsNPC ? 0.0085f : 0.0065f) * (1.f + SpeedKn / 20.f + FMath::Abs(RudderAngleDeg) / 35.f);
	for (USceneComponent* B : Barrels[Index])
	for (int32 Gun = 0; Gun < 2; ++Gun)
	{
		const FVector Muzzle = B->GetComponentTransform().TransformPosition(MuzzleLocal[Gun]);
		FVector Vel; float T;
		if (!SolveLaunch(Muzzle, Target, MuzzleVelocity, GetHullVelocity(), Vel, T)) continue;
		const FVector Carry = GetHullVelocity();
		FVector Rel = Vel - Carry;
		const float Spd = Rel.Size() * (1.f + FMath::Clamp(FMath::FRandRange(-1.f, 1.f) + FMath::FRandRange(-1.f, 1.f), -2.f, 2.f) * Sigma * 1.1f);
		const FRotator R = Rel.Rotation() + FRotator(FMath::FRandRange(-1.f, 1.f) * FMath::RadiansToDegrees(Sigma) * 0.4f, FMath::FRandRange(-1.f, 1.f) * FMath::RadiansToDegrees(Sigma), 0.f);
		Vel = R.Vector() * Spd + Carry;
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AIronwakeShell* S = W->SpawnActor<AIronwakeShell>(AIronwakeShell::StaticClass(), Muzzle, Vel.Rotation(), SP))
		{
			S->Launch(Vel, this, ShellDamage);
			LastShell = S;
			LastFlightTime = T;
			++ShellsFired;
		}
		if (UNiagaraSystem* Flash = FX(TEXT("NS_IW_Explosion2")))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, Flash, Muzzle, B->GetForwardVector().Rotation(), FVector(0.35f), true);
		}
	}
	PlayAt(Snd(TEXT("broadside_sfx")), GetActorLocation(), bIsNPC ? 1.4f : 0.9f, FMath::FRandRange(0.94f, 1.06f));
	UE_LOG(LogIronwake, Display, TEXT("IWFIRE %s turret=%d target=(%.0f,%.0f) range=%.0fm flight=%.1fs"), bIsNPC ? TEXT("NPC") : TEXT("PLY"), Index,
		Target.X, Target.Y, FVector::Dist2D(Target, GetActorLocation()) / 100.f, LastFlightTime);
	if (bIsNPC)
	{
		// The player hears contact if the enemy opens fire first.
		if (AIronwakeShipPawn* E = FindEnemy()) E->SpotTimer = 99.f;
	}
}

void AIronwakeShipPawn::ApplyShellHit(const FVector& WorldPos, float Damage, AIronwakeShipPawn* From)
{
	if (bSinking) return;
	UWorld* W = GetWorld();
	HP = FMath::Max(0.f, HP - Damage);
	++HitsTaken;
	if (UNiagaraSystem* Boom = FX(TEXT("NS_IW_Explosion2"))) UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, Boom, WorldPos, FRotator::ZeroRotator, FVector(1.f), true);
	PlayAt(Snd(TEXT("broadside_sfx")), WorldPos, 1.2f, 0.62f);
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(WorldPos);
	if (Fires.Num() < 5 && FMath::FRand() < 0.45f) StartFire(Local);
	UE_LOG(LogIronwake, Display, TEXT("IWHIT %s hp=%.0f local=(%.0f,%.0f,%.0f) fires=%d"), bIsNPC ? TEXT("NPC") : TEXT("PLY"), HP, Local.X, Local.Y, Local.Z, Fires.Num());
	if (bIsNPC && AIState == 0) { AIState = 1; AIStateName = TEXT("ENGAGE"); }
	if (!bIsNPC && Now(W) - LastDamageLine > 25.f)
	{
		LastDamageLine = Now(W);
		PlayVoice(Snd(TEXT("officer_damage")));
	}
	if (HP <= 0.f)
	{
		bSinking = true;
		SinkTime = 0.f;
		SinkStart = GetActorTransform();
		HullBody->SetSimulatePhysics(false);
		TelegraphIndex = 2;
		AIStateName = TEXT("SINKING");
		if (UNiagaraSystem* Smoke = FX(TEXT("NS_IW_Smoke")))
		{
			SinkSmoke = UNiagaraFunctionLibrary::SpawnSystemAttached(Smoke, HullBody, NAME_None, FVector(-500.f, 0.f, 1500.f), FRotator::ZeroRotator, FVector(2.f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None);
		}
		StartFire(FVector(2500.f, 0.f, 900.f));
		StartFire(FVector(-3000.f, 300.f, 900.f));
		UE_LOG(LogIronwake, Display, TEXT("IWSINK %s begins sinking at (%.0f,%.0f)"), bIsNPC ? TEXT("NPC") : TEXT("PLY"), GetActorLocation().X, GetActorLocation().Y);
		if (From && !From->bIsNPC) From->PlayAt(From->Snd(TEXT("foghorn_sfx")), From->GetActorLocation(), 0.8f);
	}
}

void AIronwakeShipPawn::StartFire(const FVector& LocalPos)
{
	const FVector P(FMath::Clamp(LocalPos.X, -6000.f, 6000.f), FMath::Clamp(LocalPos.Y, -900.f, 900.f), 900.f);
	if (UNiagaraSystem* F = FX(TEXT("NS_IW_Fire")))
	{
		if (UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAttached(F, HullBody, NAME_None, P, FRotator::ZeroRotator, FVector(1.f), EAttachLocation::KeepRelativeOffset, false, ENCPoolMethod::None))
		{
			Fires.Add(C);
		}
	}
	if (UNiagaraSystem* S = FX(TEXT("NS_IW_Smoke")))
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(S, HullBody, NAME_None, P + FVector(0, 0, 400.f), FRotator::ZeroRotator, FVector(0.6f), EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None);
	}
}

void AIronwakeShipPawn::TickDamage(float Dt)
{
	if (Fires.Num() == 0) return;
	if (!bSinking)
	{
		HP = FMath::Max(1.f, HP - FireBurnPerSec * Fires.Num() * Dt); // fires wreck, shells sink
		// Crew fight fires: each has a small chance per second to go out.
		for (int32 i = Fires.Num() - 1; i >= 0; --i)
		{
			if (FMath::FRand() < 0.01f * Dt) // ~100 s mean burn per fire unless damage control (R)
			{
				if (Fires[i]) Fires[i]->DestroyComponent();
				Fires.RemoveAt(i);
			}
		}
	}
}

void AIronwakeShipPawn::TickSinking(float Dt)
{
	SinkTime += Dt;
	const float A = FMath::Clamp(SinkTime / 40.f, 0.f, 1.f);
	const float Ease = A * A * (3.f - 2.f * A);
	FRotator R = SinkStart.Rotator();
	R.Roll += 24.f * FMath::Clamp(SinkTime / 18.f, 0.f, 1.f) + 10.f * Ease; // lists hard to port, then rolls
	R.Pitch += 9.f * Ease;                                                   // stern goes first
	FVector L = SinkStart.GetLocation();
	L.Z -= 300.f * FMath::Clamp(SinkTime / 10.f, 0.f, 1.f) + 3600.f * A * A;
	SetActorLocationAndRotation(L, R, false, nullptr, ETeleportType::TeleportPhysics);
	if (SinkTime > 70.f) Destroy();
}

void AIronwakeShipPawn::SteerToHeading(float DesiredYaw)
{
	const float Err = FRotator::NormalizeAxis(DesiredYaw - GetActorRotation().Yaw);
	const float A = FMath::Abs(Err);
	const int32 Mag = A > 25.f ? 3 : A > 10.f ? 2 : A > 3.f ? 1 : 0;
	RudderStep = Err >= 0.f ? Mag : -Mag;
}

void AIronwakeShipPawn::TickAI(float Dt)
{
	AIronwakeShipPawn* E = FindEnemy();
	AIThink -= Dt;
	if (AIThink <= 0.f)
	{
		AIThink = 0.5f;
		const float Range = E ? FVector::Dist2D(E->GetActorLocation(), GetActorLocation()) : 1e9f;
		const float HPFrac = HP / FMath::Max(MaxHP, 1.f);
		// Utility scores (deterministic, no LLM): the highest wins; fleeing is sticky.
		const float UPatrol = 0.2f;
		const float UEngage = (E && !E->bSinking && Range < DetectRange) ? 0.6f + 0.3f * (1.f - Range / DetectRange) : 0.f;
		const float UFlee = (HPFrac < 0.35f || AIState == 2) ? 0.95f + (0.35f - HPFrac) : 0.f;
		const int32 NewState = (UFlee >= UEngage && UFlee >= UPatrol) ? 2 : (UEngage >= UPatrol ? 1 : 0);
		if (NewState != AIState)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWAI state %d -> %d (patrol=%.2f engage=%.2f flee=%.2f hp=%.0f range=%.0fm)"), AIState, NewState, UPatrol, UEngage, UFlee, HP, Range / 100.f);
		}
		AIState = NewState;
		const float MyYaw = GetActorRotation().Yaw;
		if (AIState == 0)
		{
			AIStateName = TEXT("PATROL");
			static const FVector2D Pts[] = { FVector2D(1, 0), FVector2D(0, 1), FVector2D(-1, 0), FVector2D(0, -1) };
			const FVector WP = PatrolCentre + FVector(Pts[PatrolIdx].X, Pts[PatrolIdx].Y, 0.f) * 90000.f;
			if (FVector::Dist2D(WP, GetActorLocation()) < 30000.f) PatrolIdx = (PatrolIdx + 1) % 4;
			AIDesiredYaw = (WP - GetActorLocation()).Rotation().Yaw;
			TelegraphIndex = 4;
		}
		else if (AIState == 1)
		{
			const float Bearing = (E->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
			if (Range > BroadsideRange)
			{
				AIStateName = TEXT("CLOSING");
				AIDesiredYaw = Bearing;
				TelegraphIndex = 5;
			}
			else
			{
				// Turn broadside: pick the beam heading nearer the current course so both turrets bear.
				AIStateName = TEXT("BROADSIDE");
				const float A1 = Bearing + 90.f, A2 = Bearing - 90.f;
				AIDesiredYaw = FMath::Abs(FRotator::NormalizeAxis(A1 - MyYaw)) < FMath::Abs(FRotator::NormalizeAxis(A2 - MyYaw)) ? A1 : A2;
				if (Range < 200000.f) AIDesiredYaw += (FRotator::NormalizeAxis(AIDesiredYaw - Bearing) > 0.f ? 30.f : -30.f); // open the range
				TelegraphIndex = 4;
			}
		}
		else
		{
			AIStateName = TEXT("FLEEING");
			FVector Away = E ? (GetActorLocation() - E->GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
			// Stay on the 8 km ocean tile: past 3 km from the centre, bend the escape back toward open water.
			const FVector Pos2D(GetActorLocation().X, GetActorLocation().Y, 0.f);
			const float R = FMath::Max(FMath::Abs(Pos2D.X), FMath::Abs(Pos2D.Y));
			if (R > 300000.f) Away = (Away + (-Pos2D.GetSafeNormal()) * FMath::Clamp((R - 300000.f) / 50000.f, 0.f, 2.f)).GetSafeNormal2D();
			AIDesiredYaw = Away.Rotation().Yaw;
			TelegraphIndex = 6;
		}
		// Gun laying: same ballistics and lead as the player; aim at the predicted position.
		if (E && !E->bSinking && AIState != 0 && Range < 800000.f)
		{
			FVector P = E->GetActorLocation();
			float T = Range / MuzzleVelocity;
			FVector Vel;
			for (int32 i = 0; i < 3; ++i)
			{
				P = E->GetActorLocation() + E->GetHullVelocity() * T;
				P.Z = AIronwakeShell::WaterZAt(GetWorld(), P) + 600.f;
				if (!SolveLaunch(GetActorLocation() + FVector(0, 0, 1300.f), P, MuzzleVelocity, GetHullVelocity(), Vel, T)) break;
			}
			AimPoint = P;
		}
		else
		{
			AimPoint = GetActorLocation() + GetActorForwardVector() * 100000.f;
		}
	}
	AIDesiredYaw = AvoidLand(AIDesiredYaw, 120000.f);   // P9: NPC ironclad steers off land/solids too
	SteerToHeading(AIDesiredYaw);
	AliveTime += Dt;
	if (!bGunsFree && E)
	{
		// P1: give a new captain a minute: hold fire for HoldFireSeconds unless the range closes or we are hit.
		const float R = FVector::Dist2D(E->GetActorLocation(), GetActorLocation());
		const TCHAR* Why = AliveTime >= HoldFireSeconds ? TEXT("time") : R < HoldFireRangeCm ? TEXT("range") : HitsTaken > 0 ? TEXT("hit") : nullptr;
		if (Why)
		{
			bGunsFree = true;
			UE_LOG(LogIronwake, Display, TEXT("IWAI guns free (%s) t=%.1f range=%.0fm"), Why, AliveTime, R / 100.f);
		}
	}
	if (bGunsFree && E && !E->bSinking && AIState != 0)
	{
		for (int32 i = 0; i < 2; ++i)
		{
			if (TurretReload[i] <= 0.f && TurretOnTarget[i] && FVector::Dist2D(AimPoint, GetActorLocation()) < 800000.f)
			{
				FireTurret(i, AimPoint);
			}
		}
	}
}

void AIronwakeShipPawn::OnOwnShellImpact(bool bHit, const FVector& Where, AIronwakeShipPawn* Victim)
{
	UWorld* W = GetWorld();
	if (bHit) ++HitsScored;
	UE_LOG(LogIronwake, Display, TEXT("IWIMPACT %s %s at (%.0f,%.0f,%.0f)"), bIsNPC ? TEXT("NPC") : TEXT("PLY"), bHit ? TEXT("HIT") : TEXT("splash"), Where.X, Where.Y, Where.Z);
	if (bIsNPC) return;
	if (!bHit && Now(W) - LastMissLine > 20.f)
	{
		LastMissLine = Now(W);
		PlayVoice(Snd(MissLines++ % 2 == 0 ? TEXT("officer_splash") : TEXT("officer_correct")));
	}
	if (bCombatDemo)
	{
		const FVector Me = GetActorLocation();
		const FVector Back = (Me - Where).GetSafeNormal2D();
		const FVector Side(-Back.Y, Back.X, 0.f);
		if (!bHit && !CDShotDone[2] && !bPend)
		{
			CDShotDone[2] = true;
			BeginCapture(TEXT("3_splash"), Where + Back * 26000.f + Side * 9000.f + FVector(0, 0, 2200.f), Where + FVector(0, 0, 1500.f), 0.9f, false);
		}
		else if (bHit && !CDShotDone[3] && !bPend)
		{
			CDShotDone[3] = true;
			BeginCapture(TEXT("4_hit"), Where + Back * 30000.f + Side * 14000.f + FVector(0, 0, 3000.f), Where + FVector(0, 0, 800.f), 0.35f, false);
		}
	}
}

void AIronwakeShipPawn::PlayVoice(USoundBase* S, float Volume)
{
	UWorld* W = GetWorld();
	if (!S || bIsNPC || Now(W) < VoiceBusyUntil) return;
	UGameplayStatics::PlaySound2D(this, S, Volume);
	VoiceBusyUntil = Now(W) + S->GetDuration() + 0.4f;
	UE_LOG(LogIronwake, Display, TEXT("IWAUDIO voice %s"), *S->GetName());
}

void AIronwakeShipPawn::PlayAt(USoundBase* S, const FVector& Where, float Volume, float Pitch)
{
	if (!S) return;
	UGameplayStatics::PlaySoundAtLocation(this, S, Where, Volume, Pitch, 0.f, FarAtten);
}

void AIronwakeShipPawn::TickAudio(float Dt)
{
	UWorld* W = GetWorld();
	AIronwakeShipPawn* E = FindEnemy();
	const bool bNear = E && !E->bSinking && FVector::Dist2D(E->GetActorLocation(), GetActorLocation()) < DetectRange; // P1: contact on sighting
	if (bNear || SpotTimer > 50.f) CalmTimer = 0.f; else CalmTimer += Dt;
	SpotTimer = 0.f;
	if (!bInCombat && (bNear || (E && E->AIState == 1 && E->ShellsFired > 0)))
	{
		bInCombat = true;
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO contact: battle music, captain"));
		if (MusicCalm) MusicCalm->FadeOut(3.f, 0.f);
		if (USoundBase* B = Snd(TEXT("battle_night_storm")))
		{
			if (!MusicBattle) MusicBattle = UGameplayStatics::CreateSound2D(this, B, 0.6f, 1.f, 0.f, nullptr, false, false);
			if (MusicBattle && IronwakeSettings::GetMusic()) MusicBattle->FadeIn(2.5f, 0.6f);
		}
		PlayAt(Snd(TEXT("foghorn_sfx")), GetActorLocation(), 0.6f);
		VoiceBusyUntil = 0.f;
		PlayVoice(Snd(TEXT("captain_contact")));
		bCaptainLoadPending = true;
		CaptainLoadAt = VoiceBusyUntil;
	}
	if (bCaptainLoadPending && Now(W) >= CaptainLoadAt)
	{
		bCaptainLoadPending = false;
		VoiceBusyUntil = 0.f;
		PlayVoice(Snd(TEXT("captain_load")));
	}
	if (bInCombat && CalmTimer > 20.f)
	{
		bInCombat = false;
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO calm: main theme"));
		if (MusicBattle) MusicBattle->FadeOut(5.f, 0.f);
		if (MusicCalm && IronwakeSettings::GetMusic()) MusicCalm->FadeIn(5.f, 0.55f);
	}
	// Keep the active track looping (the WAVs are imported one-shot). H1: only when MUSIC is on.
	if (!IronwakeSettings::GetMusic())
	{
		if (MusicBattle && MusicBattle->IsPlaying()) MusicBattle->Stop();
		if (MusicCalm && MusicCalm->IsPlaying()) MusicCalm->Stop();
		return;
	}
	if (bInCombat && MusicBattle && !MusicBattle->IsPlaying()) MusicBattle->Play();
	if (!bInCombat && MusicCalm && !MusicCalm->IsPlaying()) MusicCalm->Play();
}

void AIronwakeShipPawn::OnFireAction(const FInputActionValue& V)
{
	// Clicking a menu button must not fire the guns.
	if (const AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetController()); PC && (PC->IsMenuOpen() || PC->IsEndScreenOpen() || !PC->bStarted)) return;
	TryFire();
}

void AIronwakeShipPawn::StartCombatDemo(const FString& Prefix)
{
	CDPrefix = Prefix;
	bCombatDemo = true;
	CDTime = 0.f;
	CDStage = 2; // the tour already rang the telegraph
	for (bool& B : CDShotDone) B = false;
	CDShellLaunch = -1.f;
	CDSinkSeen = -1.f;
	UE_LOG(LogIronwake, Display, TEXT("IWCDEMO start prefix=%s (tour)"), *CDPrefix);
}

void AIronwakeShipPawn::OnRepairAction(const FInputActionValue& V)
{
	if (RepairCooldown > 0.f || bSinking) return;
	RepairCooldown = 45.f;
	for (UNiagaraComponent* C : Fires) if (C) C->DestroyComponent();
	UE_LOG(LogIronwake, Display, TEXT("IWREPAIR damage control: %d fires out"), Fires.Num());
	Fires.Reset();
}

void AIronwakeShipPawn::IWFire() { TryFire(); }
void AIronwakeShipPawn::IWAimAssist(int32 On) { bAimAssist = On != 0; }
void AIronwakeShipPawn::IWLead(int32 On) { bLeadIndicator = On != 0; }
void AIronwakeShipPawn::IWDamage(float Amount, int32 bSelf)
{
	AIronwakeShipPawn* T = bSelf ? this : FindEnemy();
	UE_LOG(LogIronwake, Display, TEXT("IWDAMAGE %.0f -> %s"), Amount, T ? (T->bIsNPC ? TEXT("NPC") : TEXT("PLY")) : TEXT("none"));
	if (T) T->ApplyShellHit(T->GetActorLocation() + FVector(0, 0, 800.f), Amount, bSelf ? nullptr : this);
}

void AIronwakeShipPawn::ResetCameraToHorizon(float AimDistanceCm)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	// H1: WoWS default view: tight above/behind the stern, horizon at the upper third, ship in the lower half.
	ZoomIndex = FMath::Clamp(DefaultZoomIndex, 0, ZoomStepsL.Num() - 1);
	SetScope(false);
	PC->SetControlRotation(FRotator(DefaultCamPitch, GetActorRotation().Yaw, 0.f));
	UE_LOG(LogIronwake, Display, TEXT("IWCAM default view pitch=%.1f zoom=%d (%.0f m)"), DefaultCamPitch, ZoomIndex, ZoomStepsL.IsValidIndex(ZoomIndex) ? ZoomStepsL[ZoomIndex] * HullLengthM : 0.f);
}

// ---------------------------------------------------------------- demo captures
void AIronwakeShipPawn::BeginCapture(const FString& Tag, const FVector& CamLoc, const FVector& Look, float Delay, bool bFollowShell)
{
	if (GIsDumpingMovie) return; // clip recording: the tour's fight director owns the camera
	bPend = true;
	PendTag = Tag;
	PendAt = CDTime + Delay;
	PendCamLoc = CamLoc;
	PendLook = Look;
	bPendFollowShell = bFollowShell;
	PendRestoreAt = -1.f;
}

void AIronwakeShipPawn::TickCapture(float Dt)
{
	if (!bCombatDemo) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	if (PendRestoreAt > 0.f && CDTime >= PendRestoreAt)
	{
		PendRestoreAt = -1.f;
		PC->SetViewTarget(this);
		bFreezeAim = false;
	}
	if (!bPend) return;
	const bool bPlayerView = PendCamLoc.IsZero();
	if (!bPlayerView)
	{
		if (!DemoCam)
		{
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			DemoCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), PendCamLoc, FRotator::ZeroRotator, SP);
			if (DemoCam) { DemoCam->GetCameraComponent()->SetFieldOfView(55.f); DemoCam->GetCameraComponent()->bConstrainAspectRatio = false; }
		}
		if (DemoCam && CDTime >= PendAt - 0.7f)
		{
			FVector CL = PendCamLoc, LK = PendLook;
			if (bPendFollowShell && LastShell.IsValid())
			{
				const FVector S = LastShell->GetActorLocation();
				const FVector Vd = LastShell->Velocity.GetSafeNormal();
				const FVector Side = FVector::CrossProduct(Vd, FVector::UpVector).GetSafeNormal();
				CL = S - Vd * 9000.f + Side * 7000.f + FVector(0, 0, 1500.f);
				LK = S + Vd * 6000.f;
			}
			DemoCam->SetActorLocationAndRotation(CL, (LK - CL).Rotation());
			if (PC->GetViewTarget() != DemoCam) PC->SetViewTarget(DemoCam);
		}
	}
	if (CDTime >= PendAt)
	{
		bPend = false;
		const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / FString::Printf(TEXT("%s_%s.png"), *CDPrefix, *PendTag));
		if (!GIsDumpingMovie) FScreenshotRequest::RequestScreenshot(File, true, false); // movie dumps take every frame already
		AIronwakeShipPawn* E = FindEnemy();
		UE_LOG(LogIronwake, Display, TEXT("IWCDEMO screenshot %s t=%.1f enemyHP=%.0f enemyFires=%d enemyState=%s shots=%d hits=%d myHP=%.0f"), *File, CDTime,
			E ? E->HP : -1.f, E ? E->Fires.Num() : -1, E ? *E->AIStateName : TEXT("-"), ShellsFired, HitsScored, HP);
		PendRestoreAt = CDTime + 0.4f;
	}
}

void AIronwakeShipPawn::TickCombatDemo(float Dt)
{
	CDTime += Dt;
	APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Sub = (PC && PC->GetLocalPlayer()) ? PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	AIronwakeShipPawn* E = FindEnemy();
	if (CDStage == 0 && CDTime > 1.f)
	{
		// HALF AHEAD via real input (two telegraph steps).
		if (Sub) { Sub->InjectInputForAction(IA_Throttle, FInputActionValue(1.f), {}, {}); }
		CDStage = 1;
		CDNext = CDTime + 0.5f;
	}
	if (CDStage == 1 && CDTime > CDNext)
	{
		if (Sub) Sub->InjectInputForAction(IA_Throttle, FInputActionValue(1.f), {}, {});
		CDStage = 2;
	}
	// 1. Aiming: player view once the fore turret has traversed onto the lead point.
	if (!CDShotDone[0] && !bPend && CDTime > 10.f && ((TurretOnTarget[0] || TurretOnTarget[1]) || CDTime > 30.f))
	{
		CDShotDone[0] = true;
		BeginCapture(TEXT("1_aiming"), FVector::ZeroVector, FVector::ZeroVector, 0.2f, false);
	}
	// Fire whenever a turret is ready and on target (real LMB input action), after the aiming shot.
	if (CDShotDone[0] && !bPend && PendRestoreAt < 0.f && E && !E->bSinking && Sub)
	{
		if ((TurretReload[0] <= 0.f && TurretOnTarget[0]) || (TurretReload[1] <= 0.f && TurretOnTarget[1]))
		{
			Sub->InjectInputForAction(IA_Fire, FInputActionValue(true), {}, {});
			if (CDShellLaunch < 0.f && LastShell.IsValid()) { CDShellLaunch = CDTime; CDShellFlight = LastFlightTime; }
		}
	}
	if (CDShellLaunch < 0.f && LastShell.IsValid()) { CDShellLaunch = CDTime; CDShellFlight = LastFlightTime; }
	// 2. Shell in flight: chase camera beside the first shell, ~40% of the way.
	if (!CDShotDone[1] && !bPend && CDShellLaunch > 0.f && LastShell.IsValid() && CDTime > CDShellLaunch + FMath::Max(1.2f, CDShellFlight * 0.4f) - 0.8f)
	{
		CDShotDone[1] = true;
		bFreezeAim = true;
		BeginCapture(TEXT("2_shell_in_flight"), FVector(1, 1, 1), FVector::ZeroVector, 0.8f, true);
	}
	if (CDShotDone[1] && !LastShell.IsValid() && !bPend && PendRestoreAt < 0.f) bFreezeAim = false;
	// 5. Enemy burning.
	if (E && !CDShotDone[4] && !bPend && E->Fires.Num() >= 2 && !E->bSinking)
	{
		CDShotDone[4] = true;
		const FVector EL = E->GetActorLocation();
		const FVector Fw = E->GetActorForwardVector().GetSafeNormal2D();
		const FVector Side(-Fw.Y, Fw.X, 0.f);
		const FVector CamSide = FVector::DotProduct(GetActorLocation() - EL, Side) > 0.f ? Side : -Side;
		BeginCapture(TEXT("5_burning"), EL + CamSide * 26000.f - Fw * 9000.f + FVector(0, 0, 3000.f), EL + FVector(0, 0, 1000.f), 0.2f, false);
	}
	// 6. Enemy sinking.
	if (E && E->bSinking && !CDShotDone[5] && !bPend && E->SinkTime > 16.f)
	{
		CDShotDone[5] = true;
		CDSinkSeen = CDTime;
		const FVector EL(E->SinkStart.GetLocation().X, E->SinkStart.GetLocation().Y, 0.f);
		const FVector Fw = E->SinkStart.GetRotation().GetForwardVector().GetSafeNormal2D();
		const FVector Side(-Fw.Y, Fw.X, 0.f);
		BeginCapture(TEXT("6_sinking"), EL - Side * 30000.f + Fw * 14000.f + FVector(0, 0, 3500.f), EL + FVector(0, 0, -200.f), 0.2f, false);
	}
	if ((CDSinkSeen > 0.f && CDTime > CDSinkSeen + 4.f) || CDTime > 480.f)
	{
		bCombatDemo = false;
		UE_LOG(LogIronwake, Display, TEXT("IWCDEMO done t=%.1f shots=%d hits=%d myHP=%.0f enemy=%s"), CDTime, ShellsFired, HitsScored, HP,
			E ? *FString::Printf(TEXT("hp=%.0f state=%s sinking=%d"), E->HP, *E->AIStateName, E->bSinking) : TEXT("gone"));
		if (PC) PC->SetViewTarget(this);
	}
}

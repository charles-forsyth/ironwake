#include "IronwakeWorld.h"
#include "IronwakeTest.h"
#include "IronwakeCollision.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

UStaticMesh* IronwakeWorld::Mesh(const TCHAR* Name)
{
	static TMap<FString, TWeakObjectPtr<UStaticMesh>> Cache;
	if (TWeakObjectPtr<UStaticMesh>* F = Cache.Find(Name)) if (F->IsValid()) return F->Get();
	UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/World/%s/SM_%s.SM_%s"), Name, Name, Name));
	if (!M) UE_LOG(LogIronwake, Warning, TEXT("IWE1 missing world mesh %s"), Name);
	Cache.Add(Name, M);
	return M;
}

UStaticMeshComponent* IronwakeWorld::Place(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const FVector& LocCm, const FRotator& Rot, const FVector& Scale, bool bBlock, bool bSolid)
{
	UStaticMesh* M = Mesh(Name);
	if (!M || !Owner) return nullptr;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
	C->SetStaticMesh(M);
	C->SetupAttachment(Parent ? Parent : Owner->GetRootComponent());
	C->SetRelativeLocation(LocCm);
	C->SetRelativeRotation(Rot);
	C->SetRelativeScale3D(Scale);
	if (bBlock) { C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Block); }
	else C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->RegisterComponent();
	Owner->AddInstanceComponent(C);
	if (bSolid)
		if (UIronwakeColl* Co = UIronwakeColl::Get(Owner->GetWorld()))
		{
			const FString N(Name);
			const bool bRound = N.StartsWith(TEXT("Boulder")) || N.StartsWith(TEXT("CoastLandRocks")) || N.StartsWith(TEXT("CoastRocks")) || N.StartsWith(TEXT("Nama")) || N.StartsWith(TEXT("RockMoss")) || N.StartsWith(TEXT("RockFace"));
			Co->AddComponent(C, N, bRound || N.StartsWith(TEXT("SteamBuoy")), N.StartsWith(TEXT("SteamBuoy")) ? 50.f : 250.f);
		}
	return C;
}

void IronwakeWorld::TintAll(UStaticMeshComponent* C, const FLinearColor& Tint)
{
	if (!C) return;
	for (int32 i = 0; i < C->GetNumMaterials(); ++i)
		if (UMaterialInstanceDynamic* D = C->CreateAndSetMaterialInstanceDynamic(i)) D->SetVectorParameterValue(TEXT("Color"), Tint);
}

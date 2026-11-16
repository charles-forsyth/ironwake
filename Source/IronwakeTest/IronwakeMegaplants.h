#pragma once
// P31: Runtime Nanite Megaplant Foliage spawner via UE 5.8 UInstancedSkinnedMeshComponent,
// with graceful static-mesh fallback when /Game/Megaplant_Library is absent (public clone builds).
#include "CoreMinimal.h"
#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "IronwakeWorld.h"

namespace IronwakeMegaplants
{
	struct FSpeciesSpec
	{
		const TCHAR* Key;
		const TCHAR* AssetPath;
		const TCHAR* FallbackWorldMesh;
	};

	inline const FSpeciesSpec* Specs(int32& OutCount)
	{
		static const FSpeciesSpec S[] = {
			{ TEXT("spruce_B"),  TEXT("/Game/Megaplant_Library/Tree_Norway_Spruce/Tree_Norway_Spruce_01/Tree_Norway_Spruce_01_B.Tree_Norway_Spruce_01_B"),       TEXT("FirTree01") },
			{ TEXT("spruce_C"),  TEXT("/Game/Megaplant_Library/Tree_Norway_Spruce/Tree_Norway_Spruce_01/Tree_Norway_Spruce_01_C.Tree_Norway_Spruce_01_C"),       TEXT("FirTree01") },
			{ TEXT("beech_A"),   TEXT("/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_A.Tree_European_Beech_01_A"),   TEXT("IslandTree01") },
			{ TEXT("beech_B"),   TEXT("/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_B.Tree_European_Beech_01_B"),   TEXT("IslandTree02") },
			{ TEXT("birch_A"),   TEXT("/Game/Megaplant_Library/Tree_Silver_Birch/Tree_Silver_Birch_01/Tree_Silver_Birch_01_A.Tree_Silver_Birch_01_A"),           TEXT("IslandTree03") },
			{ TEXT("birch_C"),   TEXT("/Game/Megaplant_Library/Tree_Silver_Birch/Tree_Silver_Birch_01/Tree_Silver_Birch_01_C.Tree_Silver_Birch_01_C"),           TEXT("IslandTree02") },
			{ TEXT("hazel_A"),   TEXT("/Game/Megaplant_Library/Tree_Common_Hazel/Tree_Common_Hazel_01/Tree_Common_Hazel_01_A.Tree_Common_Hazel_01_A"),           TEXT("Shrub02") },
			{ TEXT("hazel_B"),   TEXT("/Game/Megaplant_Library/Tree_Common_Hazel/Tree_Common_Hazel_01/Tree_Common_Hazel_01_B.Tree_Common_Hazel_01_B"),           TEXT("Shrub03") },
			{ TEXT("deadpine_A"),TEXT("/Game/Megaplant_Library/Tree_Dead_Pine/Tree_Dead_Pine_01/Tree_Dead_Pine_A.Tree_Dead_Pine_A"),                             TEXT("FirTree01") },
			{ TEXT("bracken_A"), TEXT("/Game/Megaplant_Library/Plant_Bracken_Fern/Plant_Bracken_Fern_01/Plant_Bracken_Fern_01_A.Plant_Bracken_Fern_01_A"),       TEXT("Shrub02") },
			{ TEXT("bilberry_A"),TEXT("/Game/Megaplant_Library/Plant_Bilberry/Plant_Bilberry_01/Plant_Bilberry_01_A.Plant_Bilberry_01_A"),                       TEXT("Shrub03") },
		};
		OutCount = UE_ARRAY_COUNT(S);
		return S;
	}

	struct FLocalFoliageBatch
	{
		AActor* Owner = nullptr;
		USceneComponent* Parent = nullptr;
		TMap<FString, UInstancedSkinnedMeshComponent*> SkinnedMap;
		TMap<FString, TArray<FTransform>> PendingXforms;
		TMap<FString, float> ZMinCache;
		TMap<FString, float> HeightCache;

		void Init(AActor* InOwner, USceneComponent* InParent)
		{
			Owner = InOwner;
			Parent = InParent;
		}

		bool AddMegaplant(const TCHAR* Key, const FVector& LocalLocCm, const FQuat& LocalRot, float TargetHeightM, float WidthMul = 1.15f, float CullEndCm = 280000.f, bool bShadow = false)
		{
			if (!Owner || !Parent) return false;
			int32 N = 0;
			const FSpeciesSpec* All = Specs(N);
			const FSpeciesSpec* Found = nullptr;
			for (int32 i = 0; i < N; ++i) if (FCString::Strcmp(All[i].Key, Key) == 0) { Found = &All[i]; break; }
			if (!Found) return false;

			UInstancedSkinnedMeshComponent*& Comp = SkinnedMap.FindOrAdd(Key);
			if (!Comp)
			{
				USkinnedAsset* Sk = LoadObject<USkinnedAsset>(nullptr, Found->AssetPath);
				if (!Sk) return false; // Public clone fallback handled by caller!
				const FBoxSphereBounds B = Sk->GetBounds();
				const float NativeHM = FMath::Max(0.4f, B.BoxExtent.Z * 2.f / 100.f);
				ZMinCache.Add(Key, B.Origin.Z - B.BoxExtent.Z);
				HeightCache.Add(Key, NativeHM);

				Comp = NewObject<UInstancedSkinnedMeshComponent>(Owner);
				Comp->SetupAttachment(Parent);
				Comp->SetMobility(EComponentMobility::Static);
				Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Comp->SetCastShadow(bShadow);
				Comp->SetCullDistances(FMath::RoundToInt(CullEndCm * 0.80f), FMath::RoundToInt(CullEndCm));
				Comp->SetSkinnedAssetAndUpdate(Sk);
				Comp->RegisterComponent();
				Owner->AddInstanceComponent(Comp);
			}

			const float NativeHM = HeightCache.FindRef(Key);
			const float ZMin = ZMinCache.FindRef(Key);
			const float ScZ = FMath::Clamp(TargetHeightM / FMath::Max(NativeHM, 0.4f), 0.42f, 2.25f);
			const float ScXY = ScZ * WidthMul;
			const FVector AdjLoc = LocalLocCm + FVector(0.f, 0.f, -ZMin * ScZ - 22.f);
			PendingXforms.FindOrAdd(Key).Add(FTransform(LocalRot, AdjLoc, FVector(ScXY, ScXY, ScZ)));
			return true;
		}

		int32 Flush()
		{
			int32 Total = 0;
			for (TPair<FString, TArray<FTransform>>& Kv : PendingXforms)
			{
				UInstancedSkinnedMeshComponent* Comp = SkinnedMap.FindRef(Kv.Key);
				if (!Comp || Kv.Value.Num() == 0) continue;
				TArray<int32> AnimIdx;
				AnimIdx.Init(0, Kv.Value.Num());
				Comp->AddInstances(Kv.Value, AnimIdx, false, false);
				Total += Kv.Value.Num();
			}
			PendingXforms.Empty();
			return Total;
		}
	};
}

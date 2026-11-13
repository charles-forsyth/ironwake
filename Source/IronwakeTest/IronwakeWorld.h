#pragma once
// E1: real world assets (Poly Haven / Sketchfab scans + Hunyuan3D steampunk pieces) imported by Content/Python/iw_e1_import.py
// into /Game/Ironwake/World/<Name>/SM_<Name> (cooked via DirectoriesToAlwaysCook=/Game/Ironwake). Frame: metres->cm,
// origin = bbox centre XY, z = 0 at the model's base (WharfBuilding: its ground level; lamps: their mount point).
#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class AActor;

namespace IronwakeWorld
{
	UStaticMesh* Mesh(const TCHAR* Name);
	/** Spawn + register a static mesh component with the asset's own materials; no collision unless bBlock.
	 *  P9: bSolid registers the mesh bounds (>= 2.5 m half-extent) as a ship-collision solid in UIronwakeColl. */
	UStaticMeshComponent* Place(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const FVector& LocCm, const FRotator& Rot,
		const FVector& Scale = FVector(1.f), bool bBlock = false, bool bSolid = true);
	/** Multiply the mesh's (M_IW_ShipTex2-based) material colour by Tint via a dynamic instance. */
	void TintAll(UStaticMeshComponent* C, const FLinearColor& Tint);
}

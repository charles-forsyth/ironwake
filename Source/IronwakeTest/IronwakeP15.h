#pragma once
// P15: natural hillside under the Greyhaven town. One analytic heightfield (harbour frame, metres: +X seaward, +Y along the quay,
// quay top at Z 2.0) = smooth hill profile + low-frequency noise + ridges + two gullies + rock outcrops + rounded crest, with flat pads cut
// under the houses and road corridors carved along the street / switchback / footpath polylines. Deterministic (hash noise).
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

class USceneComponent;
class UMaterialInterface;
class AActor;

/** Flat building bench (m): Xu = uphill edge (more negative X), Xs = seaward edge. */
struct FIwPad { float Xu, Xs, Y0, Y1, Z, Bl = 1.f; };

struct FIwRoad
{
	TArray<FVector> Nodes;     // (X, Y, Z) m
	float HalfW = 1.9f;
	float Lift = 0.03f;
	bool bFollow = false;     // true: Z follows the natural ground (smoothed); false: nodes carry the design Z
	TArray<FVector> P;         // resampled every ~1.5 m
	FVector2D Mn = FVector2D::ZeroVector, Mx = FVector2D::ZeroVector;
};

class FIwHill
{
public:
	TArray<FIwPad> Pads;
	TArray<FIwRoad> Roads;
	float SpanScale = 1.f;
	float HeightScale = 1.f;
	int32 SeedOff = 0;
	bool bSolidFront = false;

	static float Prof(float U);                 // hill height along the town axis (U = -X)
	float Base(float X, float Y) const;         // natural ground
	float Final(float X, float Y) const;        // + pads + road corridors
	float RoadDist(float X, float Y, float* OutZ = nullptr) const;   // distance to the nearest road edge (m, <0 inside)
	float PadDist(float X, float Y) const;      // m to the nearest pad rectangle, 0 inside
	void BakeRoads();
	static float Noise(float X, float Y, int32 Seed, int32 Oct);     // 0..1 fbm
	static float OutcropMask(float X, float Y);
	float SlopeDeg(float X, float Y) const;
	FLinearColor Weights(float X, float Y, float Z, float SlopeD) const;   // R grass, G earth, B scree, A rock

	UProceduralMeshComponent* BuildTerrain(AActor* Owner, USceneComponent* Parent, UMaterialInterface* Mat) const;
	UProceduralMeshComponent* BuildRibbon(AActor* Owner, USceneComponent* Parent, UMaterialInterface* Mat, const FIwRoad& R) const;
	/** Land rectangles (harbour frame, m: X0 X1 Y0 Y1) where the ground is above -2.6 m, merged per 8 m column. */
	void LandRects(TArray<FVector4>& Out) const;
};

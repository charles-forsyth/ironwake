#pragma once
// P23: town placement rewrite. Switchback main road + contour lanes + houses in clusters along road edges (yaw follows the contour),
// irregular climbing envelope, benches (pads) cut under every house. Pure planning: data out, the caller spawns meshes.
#include "CoreMinimal.h"
#include "IronwakeP15.h"

struct FP23House
{
	float X = 0, Y = 0, Phi = 0;      // centre (harbour frame m), facade normal angle (deg, 0 = +X seaward)
	float S = 1, Sz = 1;              // footprint scale, height factor
	float W = 8, D = 8, H = 10, Ridge = 8;   // scaled dims (m)
	float Z = 0;                      // bench height
	int32 Type = 0, Tint = 0, Cluster = 0;
};
struct FP23Veg { float X, Y, S; int32 Kind; };   // Kind 0 shrub, 1 fir, 2 island tree, 3 boulder

struct FP23Plan
{
	TArray<FP23House> Houses;
	TArray<FP23Veg> Veg;
	int32 Clusters = 0;
};

namespace IwP23
{
	extern const float TypeW[3], TypeD[3], TypeH[3], TypeRidge[3];
	/** main switchback road (index 0) + 4 contour lanes (1..4), all terrain-following. */
	int32 AddRoads(FIwHill& Hill);
	/** wider darker verge copies of roads 0..4 (call after the other roads, before BakeRoads). */
	void AddVerges(FIwHill& Hill, int32 NumBase);
	/** after BakeRoads. */
	void Plan(const FIwHill& Hill, FP23Plan& Out, int32 Seed, int32 NBase);
	int32 AddRoadsScaled(FIwHill& Hill, float SpanScale);
	void PlanScaled(const FIwHill& Hill, FP23Plan& Out, int32 Seed, int32 NBase, float SpanScale, float HeightScale);
}

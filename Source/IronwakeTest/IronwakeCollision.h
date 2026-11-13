#pragma once
// P9: analytic ship collision + grounding. Imported scans carry no simple collision and the hull is a kinematic-velocity
// physics box, so instead of engine contact we keep a per-world registry of 2D obstacles (oriented boxes / ellipses taken from
// mesh bounds, z-range checked against the hull's draught), moving hulls (player, enemy, merchants, NPC craft: outline
// polygons) and the island seabed (FIsle::Height). Ships test their outline at the predicted pose every tick (sub-stepped,
// so nothing tunnels) and respond with speed loss / sliding / grounding. Any new placed solid object should be registered:
// IronwakeWorld::Place does it automatically for anything >= 0.8 m half-extent.
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IronwakeCollision.generated.h"


class UStaticMeshComponent;
class AActor;
class AIronwakeArchipelago;
struct FIronwakeHullProfile;

struct FIwHit
{
	bool bHit = false;
	bool bLand = false;
	FVector2D N = FVector2D::ZeroVector;   // unit, pointing from the obstacle toward the ship
	float Pen = 0.f;                        // cm
	FString Tag;
	TWeakObjectPtr<AActor> Other;           // set for ship-vs-ship
};

/** Waterline outline of a hull (local cm, bow +X) with a dense edge sampling, bounding radius and draught. */
struct FIwShape
{
	TArray<FVector2D> Poly, Dense;
	float R = 0.f;
	float DraughtCm = 500.f;
	bool IsValid() const { return Poly.Num() >= 3; }
};

UCLASS()
class IRONWAKETEST_API UIronwakeColl : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	static UIronwakeColl* Get(const UWorld* W);

	/** Hull-frame outline (cm, bow +X, relative to the actor origin) from a waterline profile. */
	static void OutlineFromProfile(const FIronwakeHullProfile& P, TArray<FVector2D>& OutPoly, float PadCm = 60.f);
	/** Rectangle with a pointed bow (cm). */
	static void OutlineRect(float X0, float X1, float HalfB, TArray<FVector2D>& OutPoly);
	/** Points along the polygon edges at <= Spacing cm. */
	static void Densify(const TArray<FVector2D>& Poly, float Spacing, TArray<FVector2D>& Out);

	// ---- static solids
	void AddObb(const FVector2D& Centre, float YawDeg, const FVector2D& Half, float ZMin, float ZMax, bool bEllipse, const FString& Tag);
	/** OBB (in the yaw frame of T) of the mesh bounds under T. */
	void AddMeshBounds(const UStaticMesh* Mesh, const FTransform& T, const FString& Tag, bool bEllipse = false, float MinHalfCm = 80.f);
	void AddComponent(UStaticMeshComponent* C, const FString& Tag, bool bEllipse = false, float MinHalfCm = 80.f);
	// ---- moving hulls
	static FIwShape MakeShape(const TArray<FVector2D>& LocalPoly, float DraughtCm);
	void RegisterHull(AActor* A, const FIwShape& Shape, bool bPawn);
	void UnregisterHull(AActor* A);
	/** Test a hull shape at a pose (position cm, yaw deg). */
	FIwHit TestPose(const AActor* Self, const FIwShape& S, const FVector2D& Pos, float YawDeg, uint8 Mask = MaskAll) const;
	/** Shallowest water (cm) under the outline at a pose (1e7 = open sea). */
	float MinDepthAt(const FIwShape& S, const FVector2D& Pos, float YawDeg) const;
	/** Clear distance (cm) along a heading before the first solid or shoal (< 1.5 draught); LenCm if clear. */
	float PathClear(const FVector2D& Start, float YawDeg, float LenCm, float HalfBeamCm, float DraughtCm) const;
	/** Log the first conflict (solid / shoal) along a polyline (cm); returns true if clear. */
	bool AuditPath(const FString& Name, const TArray<FVector>& Pts, bool bClosed, float HalfBeamCm, float DraughtCm) const;
	enum : uint8 { MaskSolids = 1, MaskPawns = 2, MaskOthers = 4, MaskAll = 7 };

	/** Sea reference z (cm). */
	float SeaZ() const;
	/** Seabed/land height (cm over the sea reference) from the terrain islands; false = open sea (deep). */
	bool SeabedAt(const FVector2D& P, float& OutH) const;
	/** Water depth (cm) at P; very large in open sea, negative on land. */
	float DepthAt(const FVector2D& P) const;

	/** Test an outline at a pose against solids (and other hulls). DenseW/PolyW are world cm. */
	FIwHit TestHull(const TArray<FVector2D>& DenseW, const TArray<FVector2D>& PolyW, const FVector2D& Centre, float RadiusCm,
		float DraughtCm, const AActor* Self, uint8 Mask = MaskAll) const;
	void Bake() const;
	struct FObb { FVector2D C, U, Half; float ZMin, ZMax, R; bool bEllipse; FString Tag; };
	const TArray<FObb>& GetObbs() const { Bake(); return Obbs; }

	int32 NumObbs() const { return Obbs.Num(); }
	int32 NumHulls() const { return Hulls.Num(); }

	/** World-space outline of a local outline at (Pos, Yaw). */
	static void ToWorld(const TArray<FVector2D>& Local, const FVector2D& Pos, float YawDeg, TArray<FVector2D>& Out);

private:
	struct FPending { TWeakObjectPtr<UStaticMeshComponent> C; FString Tag; bool bEllipse = false; float MinHalf = 100.f; };
	TArray<FPending> Pending;
	// Uniform grid over the solids (cell 250 m): built lazily, rebuilt when solids were added.
	mutable TMap<int64, TArray<int32>> Grid;
	mutable int32 GridCount = -1;
	mutable TArray<int32> Stamp; mutable int32 StampN = 0;
	static constexpr float CellCm = 25000.f;
	static int64 CellKey(int32 Cx, int32 Cy) { return ((int64)Cx << 32) ^ (int64)(uint32)Cy; }
	void BuildGrid() const;
	template <typename F> void ForEachNear(const FVector2D& P, float Rad, F&& Fn) const;
	struct FHullEnt { TWeakObjectPtr<AActor> A; FIwShape Sh; bool bPawn = false; };
	TArray<FObb> Obbs;
	mutable TArray<FHullEnt> Hulls;
	mutable TWeakObjectPtr<AIronwakeArchipelago> Arch;
	mutable float CachedSea = 0.f; mutable bool bSea = false;
	AIronwakeArchipelago* Archipelago() const;
	static bool InPoly(const FVector2D& P, const TArray<FVector2D>& Poly);
};

template <typename F>
void UIronwakeColl::ForEachNear(const FVector2D& P, float Rad, F&& Fn) const
{
	if (GridCount != Obbs.Num()) BuildGrid();
	++StampN;
	const int32 X0 = FMath::FloorToInt((P.X - Rad) / CellCm), X1 = FMath::FloorToInt((P.X + Rad) / CellCm);
	const int32 Y0 = FMath::FloorToInt((P.Y - Rad) / CellCm), Y1 = FMath::FloorToInt((P.Y + Rad) / CellCm);
	for (int32 Cx = X0; Cx <= X1; ++Cx)
		for (int32 Cy = Y0; Cy <= Y1; ++Cy)
			if (const TArray<int32>* L = Grid.Find(CellKey(Cx, Cy)))
				for (int32 Id : *L) if (Stamp[Id] != StampN) { Stamp[Id] = StampN; Fn(Obbs[Id]); }
}

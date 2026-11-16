#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "IronwakeHydro.h"
#include "ProceduralMeshComponent.h"
#include "IronwakeFleet.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class AIronwakeShipPawn;

/**
 * S2: the player's fleet.
 *  - Roster table (FIronwakeShipDef): three playable hulls with their own size, mass, handling and sea-keeping.
 *  - Progression (saved in GameUserSettings.ini [Ironwake]): the cutter is the start ship; visiting all four points of
 *    interest unlocks the frigate; 10 arrivals (more voyages) unlock the ironclad. Selected ship persists.
 *  - Wind: one prevailing wind for the sea (slow veer + gusts), used by the sailing ship's points-of-sail model.
 */
struct FIronwakeShipDef
{
	FName Id;
	FString Name;          // in-game ship name
	FString Class;         // short class line for the shipyard
	FString Blurb;         // one-line description
	FString UnlockText;    // how to unlock
	const TCHAR* HullMesh = nullptr;
	float LengthM = 30.f, BeamM = 4.f;     // LengthM drives camera, wake and bow-wave scale
	float MassKg = 6.0e4f;
	bool bTurrets = false, bProps = false, bSail = false, bFunnelSmoke = false;
	TArray<float> TelegraphKnots;          // empty = keep the pawn's own (ironclad: H1-tuned BP values)
	float MaxAccelMS2 = 0.f, SpeedTimeConstant = 0.f, MinTurnRadiusM = 0.f, YawResponse = 0.f, LateralDrag = 0.f, RudderSlew = 0.f;
	float HeavePeriod = 0.f, HeaveZeta = 0.f, PitchPeriod = 0.f, PitchZeta = 0.f, RollPeriod = 0.f, RollZeta = 0.f;
	float SeaFollowHeave = 0.7f, SeaFollowPitch = 0.7f, SeaFollowRoll = 0.6f, FitSpan = 0.6f; // P16 (ironclad defaults)
	// P19 ship-wave class data (RULES.md): bow crest as a fraction of freeboard, midbody trough as a fraction of draft, quarter crest vs bow crest, wedge vs bow crest
	float WaveCrestK = 0.40f, WaveTroughK = 0.10f, WaveQuarterK = 0.60f, WaveWedgeK = 0.45f, WaveBluff = 0.5f, FreeboardM = 3.2f, DraftM = 7.f;
	float DraftBelowBootTopCm = -60.f;     // cm the sea sits above the mesh boot-top (negative = boot-top shows)
	FVector FunnelTop = FVector::ZeroVector; // hull-mesh cm (smoke)
	float SmokeScale = 0.3f;
	FIronwakeHullProfile (*Profile)() = nullptr;
};

namespace IronwakeFleet
{
	IRONWAKETEST_API const TArray<FIronwakeShipDef>& Roster();
	IRONWAKETEST_API const FIronwakeShipDef* Find(FName Id);
	/** Selected ship (saved). -iwship=<id> on the command line overrides for tests (not saved). */
	IRONWAKETEST_API FName GetSelected();
	IRONWAKETEST_API void SetSelected(FName Id);
	IRONWAKETEST_API bool IsUnlocked(FName Id);
	IRONWAKETEST_API int32 GetArrivals();
	IRONWAKETEST_API int32 GetVisitedMask();
	/** Record a point-of-interest arrival; returns the names of ships unlocked by it (empty if none). */
	IRONWAKETEST_API TArray<FString> RecordArrival(int32 PoiIndex, int32 NumPois);
	IRONWAKETEST_API void ResetProgress();      // test helper
	IRONWAKETEST_API FString ProgressLine();    // "Points visited 2/4, voyages 5/10"
	constexpr int32 ArrivalsForIronclad = 10;

	// ---- wind (UE yaw the wind blows FROM; 0 = +X = north, clockwise)
	IRONWAKETEST_API float WindFromYaw(UWorld* W);
	IRONWAKETEST_API float WindKn(UWorld* W);
	IRONWAKETEST_API void SetFixedWind(float FromYaw, float Kn); // tests: freeze veer/gusts
	/** Sail drive fraction 0..1 vs true wind angle off the bow (deg 0..180). 0 inside 35 deg (in irons). */
	IRONWAKETEST_API float Polar(float TWA);
	IRONWAKETEST_API const TCHAR* PointOfSail(float TWA);
	IRONWAKETEST_API const TCHAR* CompassPoint(float Yaw);
}

/** S2: the frigate's moving rig: 9 yards that brace to the wind, 8 square sails and 8 fore-and-aft sails that fill,
 *  billow and flap (CPU vertex animation of procedural meshes), an ensign and 3 masthead pennants streaming downwind. */
UCLASS()
class IRONWAKETEST_API UIronwakeSailRig : public USceneComponent
{
	GENERATED_BODY()
public:
	UIronwakeSailRig();
	void Build();
	/** RelWind: wind-from angle relative to the bow (deg, + = starboard). SetLevel: telegraph index (2 furled .. 6 all sail).
	 *  Fill 0..1 (polar x wind), Kn = true wind speed. */
	void TickRig(float Dt, float RelWind, int32 SetLevel, float Fill, float WindKnots, float ApparentRelWind = 999.f);
	float GetYardYaw() const { return YardYaw; }
	float GetSpankerYaw() const { return SpankerYaw; }
	float GetHeadsailYaw() const { return HeadsailYaw; }
	float GetBellyM() const { return LastBellyM; }
	float GetLastLuffCm() const { return LastLuffCm; }

	bool bTallBrig = false;
	bool bHullSailsWPO = false;
	UPROPERTY(Transient) TArray<TObjectPtr<class UMaterialInstanceDynamic>> HullSailMIDs;
	struct FBrigPartRuntime
	{
		TObjectPtr<USceneComponent> Pivot;
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UStaticMeshComponent> FurlRoll;
		TObjectPtr<class UMaterialInstanceDynamic> SailMID;
		FVector StayAxis = FVector(0.f, 0.f, 1.f);
		uint8 Kind = 0;   // 0 yard, 1 sq_sail, 2 spanker_boom, 3 spanker_gaff, 4 spanker_sail, 5 staysail, 6 jib
		int32 Mast = -1, Tier = -1, MinStep = 3;
		float SetFrac = 0.f;
	};
	TArray<FBrigPartRuntime> BrigParts;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> BrigComps;

	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> YardPivots;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Square;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> ForeAft;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Flags;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> FlagPivots;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ClothMat;

private:
	float T = 0.f;
	float YardYaw = 0.f;
	float SpankerYaw = 0.f;
	float HeadsailYaw = 0.f;
	TArray<float> SetFrac;    // per square sail: 0 furled .. 1 set (eases)
	TArray<float> FASetFrac;
	float LastBellyM = 0.f;
	float LastLuffCm = 0.f;
	bool bBuilt = false;
	void BuildTallBrig();
	void TickTallBrig(float Dt, float RelWind, float AppWind, int32 SetLevel, float Fill, float WindKnots);
	void BuildGrid(UProceduralMeshComponent* M, int32 NU, int32 NV);
	TArray<FVector> V; TArray<FVector> N; TArray<FVector2D> UV; TArray<FLinearColor> C; TArray<FProcMeshTangent> Tan;
	void Normals(int32 NU, int32 NV);
	void Prep(int32 NU, int32 NV);
};

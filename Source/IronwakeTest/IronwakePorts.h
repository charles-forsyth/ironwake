#pragma once
// P29: Data-driven multi-port world (Content/IronwakeData/world_ports.json -> FIwPortSpec runtime loader + reusable port-kit builder),
// arrival mooring / discovery verb, live harbourmaster diorama camera, 8-good cargo & mail market, persistent Silver Crowns & Captain's Logbook.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IronwakePorts.generated.h"

class AIronwakeVoyage;
class AIronwakeArchipelago;
class USceneComponent;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UBorder;
class UScaleBox;
class USizeBox;
class UScrollBox;
class UPanelWidget;
class UIronwakeDockWidget;

inline constexpr int32 GIwNumGoods = 8;

struct FIwGoodDef
{
	FName Id;
	FString Name;
	FString Unit;
	int32 BasePrice = 20;
};

struct FIwPortBerthSpec
{
	FString Name;
	float Y0 = 0.f, XBow = -32.f, MaxLenM = 48.f;
};

struct FIwPortSpec
{
	FName Id;
	FString Name, Tag, Style, Subtitle, HarbourmasterNote;
	bool bMainHarbour = false;
	int32 HostIsleIdx = -1;
	int32 Seed = 2900;
	float WorldX = 0.f, WorldY = 0.f;      // metres (+X north, +Y east)
	float SeawardYaw = 45.f;               // deg: port local +X points seaward
	float HillScale = 0.68f;               // scales FIwHill XY footprint
	float HillHeightMul = 0.85f;
	FVector2D MoorLocal = FVector2D(42.f, 0.f);
	float MoorRadiusM = 155.f;
	FVector2D OffingLocal = FVector2D(310.f, 0.f);
	FVector DioramaEyeLocal = FVector(-28.f, 40.f, 8.2f);
	FVector DioramaLookLocal = FVector(-76.f, 0.f, 6.8f);
	float PriceMult[GIwNumGoods] = { 1,1,1,1,1,1,1,1 };
	int32 BaseStock[GIwNumGoods] = { 30,30,30,30,30,30,30,30 };
	int32 LiveStock[GIwNumGoods] = { 30,30,30,30,30,30,30,30 };

	FVector WorldCentreCm = FVector::ZeroVector;
	FVector MoorWorldCm = FVector::ZeroVector;
	FVector OffingWorldCm = FVector::ZeroVector;
	TArray<FIwPortBerthSpec> Berths;

	FTransform FrameTransform(float SeaZCm = 0.f) const
	{
		return FTransform(FRotator(0.f, SeawardYaw, 0.f), FVector(WorldX * 100.f, WorldY * 100.f, SeaZCm));
	}
	FVector H2W(float XM, float YM, float ZM = 0.f, float SeaZCm = 0.f) const
	{
		return FrameTransform(SeaZCm).TransformPosition(FVector(XM * 100.f, YM * 100.f, ZM * 100.f));
	}
	FVector2D W2H(const FVector& W) const
	{
		const FVector L = FrameTransform(0.f).InverseTransformPosition(FVector(W.X, W.Y, 0.f)) * 0.01f;
		return FVector2D(L.X, L.Y);
	}
};

struct FIwLogEntry
{
	FString Stamp;
	FString Place;
	FString Detail;
};

namespace IwPorts
{
	IRONWAKETEST_API bool LoadWorldPorts(TArray<FIwGoodDef>& OutGoods, TArray<FIwPortSpec>& OutPorts);
	IRONWAKETEST_API bool IsHostedPortIsle(int32 IsleIdx, const TArray<FIwPortSpec>& Ports, int32* OutPortIdx = nullptr);
	IRONWAKETEST_API void BuildSecondaryPort(AIronwakeVoyage* Voy, AIronwakeArchipelago* Arch, FIwPortSpec& Spec);
	IRONWAKETEST_API void PlayHarbourBell(UWorld* W);

	// Persistent Career Economy & Captain's Logbook (GGameUserSettingsIni [Ironwake])
	IRONWAKETEST_API int32 GetCrowns();
	IRONWAKETEST_API void SetCrowns(int32 Crowns);
	IRONWAKETEST_API int32 HullCargoCapTons(FName ShipId);
	IRONWAKETEST_API void GetCargoHold(int32 OutTons[GIwNumGoods]);
	IRONWAKETEST_API void GetCargoAvgCost(int32 OutAvgCr[GIwNumGoods]);
	IRONWAKETEST_API int32 TotalCargoTons();
	IRONWAKETEST_API int32 UnitPriceAt(const FIwPortSpec& Port, const FIwGoodDef& Good, int32 GoodIdx, bool bBuying);
	IRONWAKETEST_API bool BuyCargo(FIwPortSpec& Port, const TArray<FIwGoodDef>& Goods, int32 GoodIdx, int32 Tons, FName ShipId, FString& OutMsg);
	IRONWAKETEST_API int32 SellCargo(FIwPortSpec& Port, const TArray<FIwGoodDef>& Goods, int32 GoodIdx, int32 Tons, FString& OutMsg);

	IRONWAKETEST_API void AddDistanceMeters(float Meters);
	IRONWAKETEST_API float GetNauticalMiles();
	IRONWAKETEST_API int32 GetCargoDeliveredTons();
	IRONWAKETEST_API int32 GetNetTradeProfit();
	IRONWAKETEST_API int32 GetDockedPortMask();
	IRONWAKETEST_API void RecordPortDocked(int32 PortIdx);
	IRONWAKETEST_API int32 GetSalvagedMask();
	IRONWAKETEST_API bool RecordPoiDiscoveryOrSalvage(int32 PoiIdx, const FString& PoiName, const FString& Tag, const FString& ClockStr, FString& OutBanner);
	IRONWAKETEST_API void AppendLog(const FString& ClockStr, const FString& Place, const FString& Detail);
	IRONWAKETEST_API void GetLogEntries(TArray<FIwLogEntry>& OutEntries);
	IRONWAKETEST_API FString RankTitle();

	IRONWAKETEST_API FName ConsumePendingPortSpawn();
	IRONWAKETEST_API void SetPendingPortSpawn(FName PortId);
	IRONWAKETEST_API void ResetEconomyForTest(TArray<FIwPortSpec>& Ports);
}

/** Button click proxy for UIronwakeDockWidget. */
UCLASS()
class IRONWAKETEST_API UIronwakeDockProxy : public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UIronwakeDockWidget> Owner;
	int32 Id = 0;
	UFUNCTION() void Clicked();
};

/**
 * P29 Port Harbourmaster, Cargo & Mail Contracts, Captain's Logbook & Shipyard modal.
 * Pure C++ UMG scaled to 1920x1080 design space via UScaleBox + UScrollBox safety net, leaving the right ~44% of the screen
 * open over the live 3D port tavern / wharf / landmark diorama vignette.
 */
UCLASS()
class IRONWAKETEST_API UIronwakeDockWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	enum ETab : int32 { TabMarket = 0, TabLogbook = 1, TabShipyard = 2 };
	enum EAct : int32
	{
		Act_Close = 1,
		Act_Tab0 = 10,          // 10..12
		Act_Repair = 20,
		Act_ShipBase = 30,      // 30..35 (switch ship at this port)
		Act_Buy1Base = 100,     // + goodIdx (0..7)
		Act_Buy10Base = 110,    // + goodIdx
		Act_BuyMaxBase = 120,   // + goodIdx
		Act_SellAllBase = 130,  // + goodIdx
	};

	TWeakObjectPtr<AIronwakeVoyage> Voy;
	int32 PortIdx = 0;          // >= 0 when docked at a WorldPort; -1 when viewing Logbook off a sea landmark/wreck
	FString CustomTitle;
	FString CustomSubtitle;
	FString CustomNote;
	FString LastTradeStatus;
	int32 ActiveTab = TabMarket;

	void SetupForPort(AIronwakeVoyage* InVoy, int32 InPortIdx, int32 StartTab = TabMarket, const FString& InitialStatus = FString());
	void SetupForLandmark(AIronwakeVoyage* InVoy, const FString& Title, const FString& Sub, const FString& Note, const FString& StatusLine);
	void OpenTab(int32 InTab);
	void Refresh();
	void TickDock(float Dt);
	void OnAction(int32 Id);
	/** Walks every visible leaf widget and verifies 0 outside viewport, 0 overlaps, 0 clipped by scroll box; logs IWP29 LAYOUT. */
	int32 CheckLayout(const FString& Tag);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UButton* MakeBtn(UPanelWidget* Parent, const FString& Label, int32 Id, int32 FontSize = 16, FMargin Pad = FMargin(12.f, 5.f), UTextBlock** OutText = nullptr);
	void FitToViewport();

	UPROPERTY(Transient) TArray<TObjectPtr<UIronwakeDockProxy>> Proxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Btns;
	TArray<int32> BtnIds;

	UPROPERTY(Transient) TObjectPtr<UScaleBox> RootScale;
	UPROPERTY(Transient) TObjectPtr<USizeBox> CardBox;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QuoteText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PurseHoldText;
	UPROPERTY(Transient) TArray<TObjectPtr<UVerticalBox>> TabPanels;

	// Market rows (8 goods)
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GoodNameTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GoodStockTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GoodPriceTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GoodHoldTexts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TradeStatusText;

	// Logbook tab
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CareerSummaryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LogEntriesText;

	// Shipyard tab
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HullRepairText;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ShipTitleTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ShipDescTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ShipBtnTexts;

	FVector2D LastVP = FVector2D::ZeroVector;
	float CurScale = 1.f;
	FVector2D CurArea = FVector2D(1920.f, 1080.f);
};

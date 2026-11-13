#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "IronwakePlayerController.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UInputAction;
class UInputMappingContext;
class UIronwakeHelpWidget;

class UIronwakeMenuWidget;
class UHorizontalBox;
class UBorder;
class UScaleBox;
class USizeBox;
class UScrollBox;
class USlider;
class UPanelWidget;
class UWidget;

/** P18: one proxy per menu button (UMG delegates must be UFUNCTIONs); routes the click to UIronwakeMenuWidget::OnAction(Id). */
UCLASS()
class IRONWAKETEST_API UIronwakeMenuProxy : public UObject
{
	GENERATED_BODY()
public:
	TWeakObjectPtr<UIronwakeMenuWidget> Owner;
	int32 Id = 0;
	UFUNCTION() void Clicked();
};

/**
 * Title / pause menu, pure C++ UMG. P18 rebuild: the whole menu sits in a ScaleBox driven by the viewport
 * (scale = clamp(min(w/1920, h/1080), 0.5, 1.5), design area = viewport / scale) with a ScrollBox safety net inside the card.
 * Pages: MAIN (START/RESUME, STORM AT SEA, SHIPYARD, SETTINGS, CONTROLS, CREDITS, QUIT), SETTINGS (tabs GAMEPLAY, GRAPHICS, VIEW, AUDIO),
 * CREDITS, YARD. Esc / gamepad B goes back one page. Arrows / D-pad / Enter / A drive every control.
 * Mode 0 = title, 1 = pause (RESUME instead of START).
 */
UCLASS()
class IRONWAKETEST_API UIronwakeMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	enum EPage : int32 { PageMain = 0, PageSettings, PageCredits, PageYard };
	enum EAction : int32
	{
		A_Start = 0, A_Storm, A_Yard, A_Settings, A_Controls, A_Credits, A_Quit, A_Back,
		A_TabGameplay = 8,   // 8..11: GAMEPLAY GRAPHICS VIEW AUDIO
		A_ModeBase = 12,     // 12 PEACEFUL, 13 CAPTAIN
		A_RudBase = 14,      // 14 HOLDS, 15 AUTO-CENTRE
		A_GfxBase = 16,      // 16..18 LOW MEDIUM HIGH
		A_CamBase = 19,      // 19..21 OFF LOW MEDIUM
		A_ResetAudio = 22,
		A_YardShip0 = 24,    // 24..29 (up to 6 roster hulls)
		A_YardLaunch = 30,
		A_Test0 = 32,        // + IronwakeAudio class index
	};

	int32 Mode = 0; // 0 title, 1 pause
	int32 Page = PageMain;
	int32 Tab = 0;  // settings tab: 0 GAMEPLAY, 1 GRAPHICS, 2 VIEW, 3 AUDIO
	int32 YardSel = 0;

	void Refresh();
	/** Called by the controller every tick (also while paused): fits the menu to the viewport, applies slider changes, focus tint. */
	void TickMenu(float Dt);
	/** Put keyboard/gamepad focus on the first control of the current page. */
	void RestoreFocus();
	void OpenPage(int32 InPage, int32 InTab = 0);
	/** One page back (Settings/Credits/Yard -> Main). Returns false when already on the main page. */
	bool GoBack();
	void OnYardPublic(int32 Highlight) { OpenPage(PageYard); if (Highlight >= 0) PickYardShip(Highlight); }
	void OnAction(int32 Id);
	/** Test hook: move the slider as the player would (the tick applies it like a drag). */
	void SetSliderPct(int32 ClassIdx, int32 Pct);
	/** Walks every visible widget: counts rects outside the viewport and overlapping interactive/text widgets; logs `IWP18 LAYOUT`. Returns outside+overlaps. */
	int32 CheckLayout(const FString& Tag);
	/** Logs which control has focus. */
	FString FocusedName() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void PickYardShip(int32 i);
	UButton* MakeBtn(UPanelWidget* Parent, const FString& Label, int32 Id, int32 InPage, int32 InTab, UTextBlock** OutLabel = nullptr, int32 FontSize = 22);
	UHorizontalBox* MakeRow(UVerticalBox* Into, const FString& Label);
	bool BtnVisible(int32 Idx) const;

	UPROPERTY(Transient) TArray<TObjectPtr<UIronwakeMenuProxy>> Proxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> Btns;
	TArray<int32> BtnIds, BtnPage, BtnTab;
	TArray<FLinearColor> BtnBase;
	UPROPERTY(Transient) TObjectPtr<UScaleBox> RootScale;
	UPROPERTY(Transient) TObjectPtr<USizeBox> AreaBox;
	UPROPERTY(Transient) TObjectPtr<USizeBox> CardBox;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
	UPROPERTY(Transient) TArray<TObjectPtr<UVerticalBox>> Pages;
	UPROPERTY(Transient) TArray<TObjectPtr<UVerticalBox>> TabBoxes;
	UPROPERTY(Transient) TArray<TObjectPtr<USlider>> Sliders;       // per IronwakeAudio class
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> SliderPct;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> YardNames;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> YardInfo;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> YardButtons;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> YardProgress;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> YardLaunchLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StartLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Subtitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PageTitle;
	UPROPERTY(Transient) TObjectPtr<UButton> ModeButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GraphicsLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RudderLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CamMotionLabel;
	bool bSlidersInit = false;
	void FitToViewport();
	FVector2D LastVP = FVector2D::ZeroVector;
	float LastDpi = 0.f;
	float CurTarget = 1.f;
	FVector2D CurArea = FVector2D::ZeroVector;
};

/** P1: end-of-fight screens. Mode 0 = SHIP LOST (Restart / Main Menu), 1 = VICTORY banner (Continue). */
UCLASS()
class IRONWAKETEST_API UIronwakeEndWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	int32 Mode = 0;
	UFUNCTION() void OnRestart();
	UFUNCTION() void OnMainMenu();
	UFUNCTION() void OnContinue();
	FString StatsLine;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	UButton* MakeButton(UVerticalBox* Box, const FString& Label);
};

UCLASS()
class IRONWAKETEST_API AIronwakePlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AIronwakePlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Start the voyage from the title menu. bPeaceful = Voyager mode (no enemy). bDeferEnemy: caller spawns later. */
	void StartGame(bool bPeaceful, bool bDeferEnemy = false);
	void ShowMenu(int32 Mode);
	void HideMenu();
	bool IsMenuOpen() const;
	void TogglePause();
	/** P11: controls help panel (H / F1, pause menu CONTROLS). */
	void ToggleHelp();
	bool IsHelpOpen() const;

	bool bPeaceful = true; // PASS 2 priority change: the game opens in PEACEFUL (Voyager); Captain is a menu option
	bool bStarted = false;

	virtual void PlayerTick(float DeltaTime) override;

	/** P1 end screens. */
	void ShowEndScreen(int32 Mode);
	void HideEndScreen();
	bool IsEndScreenOpen() const;
	void RestartVoyage();
	void ReturnToMainMenu();

	UFUNCTION(Exec) void IWMenu(int32 Mode);
	/** R1: open the menu with the CREDITS panel shown. */
	UFUNCTION(Exec) void IWCredits();
	/** S2: open the SHIPYARD card (optionally highlight roster index). */
	UFUNCTION(Exec) void IWYard(int32 Highlight = -1);
	/** S2: switch to roster ship (saves the choice, restarts the voyage at the harbour). */
	void ChooseShip(FName Id);
	/** IWShot name: screenshot (with UI) to Saved/shots/name.png. */
	UFUNCTION(Exec) void IWShot(const FString& Name);
	/** IWEnd 0|1|-1: show SHIP LOST / VICTORY / hide (test helper). */
	UFUNCTION(Exec) void IWEnd(int32 Mode);
	UFUNCTION(Exec) void IWStart(int32 Peaceful);
	/** H1: IWGfx 0|1|2 (LOW/MEDIUM/HIGH, saved), IWMusic 0|1 (saved). */
	UFUNCTION(Exec) void IWGfx(int32 Level);
	UFUNCTION(Exec) void IWMusic(int32 On);
	/** P18: IWVol <Master|Music|Sea|Wind|Ship|Guns|Voices|UI> <0-100>; IWAudioTest writes build/logs/P18_audio_routes.txt. */
	UFUNCTION(Exec) void IWVol(const FString& Key, int32 Pct);
	UFUNCTION(Exec) void IWAudioTest();
	/** P18 test director (-iwp18=sizes|nav|persistset|persistread|audio). */
	void TickP18(float RealDt);
	FString P18Mode; float P18T = 0.f, P18Mark = 0.f; int32 P18Step = 0;
	/** Esc / gamepad B: close help, else one menu page back, else pause toggle. Returns true when consumed. */
	bool HandleBack();
	void EscapeOrBack();
	void ApplyMusicToShips();

	// S1 Voyager keys (F photo, H helmsman, N next point, X anchor, G fast sail, L weather, K day/night cycle)
	UFUNCTION(Exec) void IWPhoto();
	UFUNCTION(Exec) void IWWeather(int32 Preset);
	UFUNCTION(Exec) void IWHelm();
	UFUNCTION(Exec) void IWAnchor();
	UFUNCTION(Exec) void IWFastSail();
	void SetPhotoInput(bool bOn);
	/** A1: chart (M) input: mouse cursor, clicks go to the chart, guns/camera swallowed. */
	void SetChartInput(bool bOn);
	UFUNCTION(Exec) void IWAutopilot(int32 Mode);
	UFUNCTION(Exec) void IWChart();
	FVector2D ConsumePhotoLook();
	FVector PhotoMove = FVector::ZeroVector;
	bool bPhotoFast = false;
	float PhotoFovDelta = 0.f;
	bool bPhotoTimeCycle = false;

protected:
	void OnPauseAction(const FInputActionValue& V);
	void OnVoyageKey(const FInputActionValue& V, int32 Which);
	void OnPhotoWheel(const FInputActionValue& V);
	void OnPhotoTime(const FInputActionValue& V);
	bool CanUseVoyageKeys() const;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> VoyageActions;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_PhotoSink;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_PhotoWheel;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_PhotoTime;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> PhotoContext;
	bool bPhotoInput = false;
	void OnApKey(const FInputActionValue& V, int32 Which);
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> ApActions;   // P, M, Enter, Backspace
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_ChartL;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_ChartR;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_ChartSink;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> ChartContext;
	void OnChartClick(const FInputActionValue& V, bool bRight);
	UPROPERTY(Transient) TObjectPtr<UIronwakeMenuWidget> Menu;
	UPROPERTY(Transient) TObjectPtr<UIronwakeHelpWidget> HelpW;
	UPROPERTY(Transient) TObjectPtr<UIronwakeEndWidget> EndWidget;
	float PlayerSinkSeen = -1.f;
	bool bLostShown = false;
	bool bVictoryShown = false;
	float VictoryAt = -1.f;
	// P1 test driver (Saved/iw_p1.txt: idle | sink | sink2 | fight)
	FString P1Mode;
	float P1Time = 0.f;
	int32 P1Step = 0;
	float P1Mark = 0.f;
	void TickP1Test(float Dt);
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Pause;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MenuContext;
public:
	bool IsDockOpen() const;
	void SetDockInput(bool bOn);
	UFUNCTION(Exec) void IWDock(int32 PortIdx = -1);
protected:
	void OnArriveAction(const FInputActionValue& V);
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_Arrive;
	UPROPERTY(Transient) TObjectPtr<UInputAction> IA_DockSink;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> DockContext;
};

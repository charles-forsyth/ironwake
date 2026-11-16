// Card D: title / pause menu (pure C++ UMG) and the player controller that owns it.
#include "IronwakePlayerController.h"
#include "IronwakeGameMode.h"
#include "IronwakeShipPawn.h"
#include "IronwakeTest.h"
#include "IronwakeSettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/PanelWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Engine/GameViewportClient.h"
#include "IronwakeAudio.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "IronwakeLivingSea.h"
#include "IronwakeVoyage.h"
#include "Misc/App.h"
#include "IronwakeFleet.h"
#include "IronwakeP11.h"
#include "InputModifiers.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/SpringArmComponent.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
	const FLinearColor Brass(0.86f, 0.74f, 0.47f, 1.f);
	const FLinearColor Steel(0.70f, 0.74f, 0.78f, 1.f);

	// R1: CC-BY attribution for the library ship models (also ~/Games/Ironwake/CREDITS.txt).
	const TCHAR* CreditsList =
		TEXT("SHIP MODELS  (Sketchfab, CC Attribution 4.0)\n")
		TEXT("  HMS Devastation 1871 ............ martyn169\n")
		TEXT("  RSS Kracken ..................... martyn169\n")
		TEXT("  Goliath Steam Tug 1903 .......... martyn169\n")
		TEXT("  Remorqueur \"Aglae\" 1874 ......... Gabrielcambreling\n")
		TEXT("  Steampunk Airship, Brass-Plated . Pigcraft (s8819296)\n")
		TEXT("  Russian archipelago frigate Svjatoi Nikolai\n")
		TEXT("      Museovirasto Museiverket Finnish Heritage Agency\n")
		TEXT("  Models scaled, split and re-coloured for the game.\n")
		TEXT(" \n")
		TEXT("WORLD MODELS  (Sketchfab, CC Attribution 4.0)\n")
		TEXT("  Wharf building in Oslo, 1798 ..... Tidvis\n")
		TEXT("  Victorian Factory ................ Paolo Mercogliano\n")
		TEXT("  Victorian Row Boat ............... Ashley Aslett\n")
		TEXT("  Old Sea Mine ..................... Jorma Rysky\n")
		TEXT("  Wooden Barrels ................... MaX3Dd\n")
		TEXT("  creativecommons.org/licenses/by/4.0\n")
		TEXT("PUBLIC DOMAIN (CC0)\n")
		TEXT("  Stern of SS Rifle; \"Venus\", a Shetland fourareen\n")
		TEXT("      Scottish Maritime Museum (Sketchfab)\n")
		TEXT("  Coastal cliffs, coast rocks, boulder, modular pier,\n")
		TEXT("  barrel, military crate, wall lamp ... Poly Haven\n")
		TEXT("NIGHT SKY  Star map: NASA/Goddard Space Flight Center\n")
		TEXT("  Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC.\n")
		TEXT("  Moon: NASA LROC / SVS Moon Kit\n")
		TEXT("TOWN + NPC CRAFT  generated for Ironwake (Hunyuan3D-2)\n")
		TEXT("STEAMPUNK HARBOUR PIECES  generated for Ironwake\n")
		TEXT("  with Hunyuan3D-2 (Tencent) from our concept art\n")
		TEXT(" \n")
		TEXT("ENGINE   Unreal Engine 5.8, Epic Games\n")
		TEXT("MADE BY  Chuck Forsyth with Hermes");
}

// ============================================================================ Widget (P18 rebuild)
namespace
{
	const FLinearColor BtnNormal(0.08f, 0.09f, 0.10f, 0.92f);
	const FLinearColor BtnSelected(0.26f, 0.21f, 0.09f, 0.98f);
	const FLinearColor BtnFocus(0.46f, 0.37f, 0.15f, 1.f);
	constexpr float CardW = 900.f;
	constexpr float CardMarginL = 60.f, CardMarginT = 50.f;

	FSlateFontInfo BoldFont(int32 Size, int32 Spacing = 0)
	{
		FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
		F.LetterSpacing = Spacing;
		return F;
	}
}

void UIronwakeMenuProxy::Clicked() { if (UIronwakeMenuWidget* O = Owner.Get()) O->OnAction(Id); }

UButton* UIronwakeMenuWidget::MakeBtn(UPanelWidget* Parent, const FString& Label, int32 Id, int32 InPage, int32 InTab, UTextBlock** OutLabel, int32 FontSize)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetBackgroundColor(BtnNormal);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(BoldFont(FontSize));
	T->SetColorAndOpacity(FSlateColor(Brass));
	T->SetJustification(ETextJustify::Center);
	B->AddChild(T);
	if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot)) { BS->SetPadding(FMargin(20.f, 7.f)); BS->SetHorizontalAlignment(HAlign_Center); }
	UPanelSlot* S = Parent->AddChild(B);
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(S)) { VS->SetPadding(FMargin(0.f, 5.f)); VS->SetHorizontalAlignment(HAlign_Fill); }
	else if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(S)) { HS->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f)); HS->SetVerticalAlignment(VAlign_Center); }
	UIronwakeMenuProxy* P = NewObject<UIronwakeMenuProxy>(this);
	P->Owner = this; P->Id = Id;
	B->OnClicked.AddDynamic(P, &UIronwakeMenuProxy::Clicked);
	Proxies.Add(P);
	Btns.Add(B); BtnIds.Add(Id); BtnPage.Add(InPage); BtnTab.Add(InTab);
	if (OutLabel) *OutLabel = T;
	return B;
}

UHorizontalBox* UIronwakeMenuWidget::MakeRow(UVerticalBox* Into, const FString& Label)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Into->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 5.f));
	USizeBox* LB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LB->SetWidthOverride(236.f);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(BoldFont(20));
	T->SetColorAndOpacity(FSlateColor(Steel));
	LB->SetContent(T);
	UHorizontalBoxSlot* S = Cast<UHorizontalBoxSlot>(Row->AddChild(LB));
	if (S) S->SetVerticalAlignment(VAlign_Center);
	return Row;
}

bool UIronwakeMenuWidget::BtnVisible(int32 i) const
{
	if (!Btns.IsValidIndex(i)) return false;
	if (BtnPage[i] != Page) return false;
	return Page != PageSettings || BtnTab[i] < 0 || BtnTab[i] == Tab;
}

TSharedRef<SWidget> UIronwakeMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Root: a ScaleBox that ignores the engine DPI curve and applies clamp(min(w/1920, h/1080), 0.5, 1.5) (set per frame in FitToViewport).
		// Its child fills the viewport / scale, so the design area is always [0..vp/scale] and the card is placed inside it.
		RootScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		WidgetTree->RootWidget = RootScale;
		RootScale->SetStretch(EStretch::UserSpecified);
		RootScale->SetUserSpecifiedScale(1.f);
		RootScale->SetIgnoreInheritedScale(true);
		RootScale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlay* Ov = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Ov->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		RootScale->AddChild(Ov);
		if (UScaleBoxSlot* SS = Cast<UScaleBoxSlot>(Ov->Slot)) { SS->SetHorizontalAlignment(HAlign_Fill); SS->SetVerticalAlignment(VAlign_Fill); }

		CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CardBox->SetWidthOverride(CardW);
		CardBox->SetMaxDesiredHeight(980.f);
		if (UOverlaySlot* OS = Ov->AddChildToOverlay(CardBox)) { OS->SetHorizontalAlignment(HAlign_Left); OS->SetVerticalAlignment(VAlign_Top); OS->SetPadding(FMargin(CardMarginL, CardMarginT, 0.f, CardMarginT)); }

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.012f, 0.015f, 0.02f, 0.84f), 6.f, FLinearColor(0.55f, 0.45f, 0.25f, 0.55f), 2.f));
		Panel->SetPadding(FMargin(40.f, 26.f, 40.f, 26.f));
		CardBox->SetContent(Panel);
		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Col);

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("IRONWAKE")));
		Title->SetFont(BoldFont(72, 220));
		Title->SetColorAndOpacity(FSlateColor(Brass));
		Title->SetShadowOffset(FVector2D(3.f, 3.f));
		Title->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Col->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Subtitle->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 18));
		Subtitle->SetColorAndOpacity(FSlateColor(Steel));
		Col->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(4.f, 0.f, 0.f, 16.f));
		PageTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PageTitle->SetFont(BoldFont(26, 120));
		PageTitle->SetColorAndOpacity(FSlateColor(Steel));
		Col->AddChildToVerticalBox(PageTitle)->SetPadding(FMargin(4.f, 0.f, 0.f, 10.f));

		CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		CardScroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
		CardScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
		Col->AddChildToVerticalBox(CardScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CardScroll->AddChild(Host);
		for (int32 i = 0; i < 4; ++i)
		{
			UVerticalBox* Pg = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Host->AddChildToVerticalBox(Pg);
			Pages.Add(Pg);
		}
		UTextBlock* Tmp = nullptr;

		// ---- MAIN page
		UVerticalBox* M = Pages[PageMain];
		MakeBtn(M, TEXT("START VOYAGE"), A_Start, PageMain, -1, &Tmp); StartLabel = Tmp;
		MakeBtn(M, TEXT("STORM AT SEA"), A_Storm, PageMain, -1);
		MakeBtn(M, TEXT("SHIPYARD"), A_Yard, PageMain, -1);
		MakeBtn(M, TEXT("SETTINGS"), A_Settings, PageMain, -1);
		MakeBtn(M, TEXT("CONTROLS"), A_Controls, PageMain, -1);
		MakeBtn(M, TEXT("CREDITS"), A_Credits, PageMain, -1);
		MakeBtn(M, TEXT("QUIT"), A_Quit, PageMain, -1);

		// ---- SETTINGS page: tab row, tab bodies, BACK
		UVerticalBox* S = Pages[PageSettings];
		UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		S->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		static const TCHAR* TabNames[4] = { TEXT("GAMEPLAY"), TEXT("GRAPHICS"), TEXT("VIEW"), TEXT("AUDIO") };
		for (int32 t = 0; t < 4; ++t)
		{
			UButton* TB = MakeBtn(Tabs, TabNames[t], A_TabGameplay + t, PageSettings, -1, nullptr, 20);
			if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(TB->Slot)) HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		for (int32 t = 0; t < 4; ++t)
		{
			UVerticalBox* TBx = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			S->AddChildToVerticalBox(TBx);
			TabBoxes.Add(TBx);
		}
		{ // GAMEPLAY
			UHorizontalBox* R = MakeRow(TabBoxes[0], TEXT("MODE"));
			MakeBtn(R, TEXT("PEACEFUL"), A_ModeBase, PageSettings, 0, nullptr, 20); ModeButton = Btns.Last();
			MakeBtn(R, TEXT("CAPTAIN"), A_ModeBase + 1, PageSettings, 0, nullptr, 20);
			UHorizontalBox* R2 = MakeRow(TabBoxes[0], TEXT("RUDDER"));
			MakeBtn(R2, TEXT("HOLDS"), A_RudBase, PageSettings, 0, nullptr, 20);
			MakeBtn(R2, TEXT("AUTO-CENTRE"), A_RudBase + 1, PageSettings, 0, nullptr, 20);
			UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Hint->SetText(FText::FromString(TEXT("Mode is chosen per voyage: change it from the title menu.\nRUDDER HOLDS keeps the wheel where you leave it; AUTO-CENTRE springs it back.")));
			Hint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 15)); Hint->SetColorAndOpacity(FSlateColor(Steel));
			TabBoxes[0]->AddChildToVerticalBox(Hint)->SetPadding(FMargin(4.f, 14.f, 0.f, 0.f));
		}
		{ // GRAPHICS
			UHorizontalBox* R = MakeRow(TabBoxes[1], TEXT("PRESET"));
			MakeBtn(R, TEXT("LOW"), A_GfxBase, PageSettings, 1, nullptr, 20);
			MakeBtn(R, TEXT("MEDIUM"), A_GfxBase + 1, PageSettings, 1, nullptr, 20);
			MakeBtn(R, TEXT("HIGH"), A_GfxBase + 2, PageSettings, 1, nullptr, 20);
			UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Hint->SetText(FText::FromString(TEXT("LOW: no Lumen, lighter sea.  MEDIUM: 60 fps class (default).  HIGH: cinematic clouds, ~30 fps class.")));
			Hint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 15)); Hint->SetColorAndOpacity(FSlateColor(Steel));
			Hint->SetAutoWrapText(true);
			TabBoxes[1]->AddChildToVerticalBox(Hint)->SetPadding(FMargin(4.f, 14.f, 0.f, 0.f));
		}
		{ // VIEW
			UHorizontalBox* R = MakeRow(TabBoxes[2], TEXT("CAMERA MOTION"));
			MakeBtn(R, TEXT("OFF"), A_CamBase, PageSettings, 2, nullptr, 20);
			MakeBtn(R, TEXT("LOW"), A_CamBase + 1, PageSettings, 2, nullptr, 20);
			MakeBtn(R, TEXT("MEDIUM"), A_CamBase + 2, PageSettings, 2, nullptr, 20);
			UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Hint->SetText(FText::FromString(TEXT("How much the camera follows the ship's pitch, roll and heave. Binoculars and photo mode are always steady.")));
			Hint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 15)); Hint->SetColorAndOpacity(FSlateColor(Steel));
			Hint->SetAutoWrapText(true);
			TabBoxes[2]->AddChildToVerticalBox(Hint)->SetPadding(FMargin(4.f, 14.f, 0.f, 0.f));
		}
		{ // AUDIO: one slider per sound type + TEST, RESET AUDIO
			for (int32 c = 0; c < IronwakeAudio::Count; ++c)
			{
				UHorizontalBox* R = MakeRow(TabBoxes[3], IronwakeAudio::Label(c));
				USizeBox* SzB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				SzB->SetWidthOverride(250.f); SzB->SetHeightOverride(36.f);
				USlider* Sl = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
				Sl->RequiresControllerLock = false; // Left/Right/D-pad change the value directly (no extra Accept press)
				Sl->SetMinValue(0.f); Sl->SetMaxValue(1.f); Sl->SetStepSize(0.05f); Sl->SetValue(IronwakeAudio::DefaultPct(c) / 100.f);
				Sl->SetSliderBarColor(FLinearColor(0.30f, 0.30f, 0.32f, 1.f));
				Sl->SetSliderHandleColor(Brass);
				SzB->SetContent(Sl);
				if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(R->AddChild(SzB))) { HS->SetVerticalAlignment(VAlign_Center); HS->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f)); }
				USizeBox* PB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				PB->SetWidthOverride(104.f);
				UTextBlock* Pc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Pc->SetFont(BoldFont(20)); Pc->SetColorAndOpacity(FSlateColor(Brass)); Pc->SetJustification(ETextJustify::Right);
				PB->SetContent(Pc);
				if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(R->AddChild(PB))) { HS->SetVerticalAlignment(VAlign_Center); HS->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f)); }
				MakeBtn(R, TEXT("TEST"), A_Test0 + c, PageSettings, 3, nullptr, 18);
				Sliders.Add(Sl); SliderPct.Add(Pc);
			}
			MakeBtn(TabBoxes[3], TEXT("RESET AUDIO"), A_ResetAudio, PageSettings, 3, nullptr, 20);
			UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Hint->SetText(FText::FromString(TEXT("Left / Right or drag: steps of 5 %.  TEST (or Enter / A on the slider) plays a short sample at that level.  MUSIC 0 = off.")));
			Hint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 15)); Hint->SetColorAndOpacity(FSlateColor(Steel));
			Hint->SetAutoWrapText(true);
			TabBoxes[3]->AddChildToVerticalBox(Hint)->SetPadding(FMargin(4.f, 6.f, 0.f, 0.f));
		}
		{ UVerticalBox* BackBox = S; MakeBtn(BackBox, TEXT("BACK"), A_Back, PageSettings, -1); Btns.Last()->SetBackgroundColor(BtnNormal);
		  if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(Btns.Last()->Slot)) VS->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f)); }

		// ---- CREDITS page
		{
			UTextBlock* CT = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			CT->SetText(FText::FromString(CreditsList));
			CT->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 13));
			CT->SetColorAndOpacity(FSlateColor(Steel));
			Pages[PageCredits]->AddChildToVerticalBox(CT)->SetPadding(FMargin(4.f, 0.f, 0.f, 10.f));
			MakeBtn(Pages[PageCredits], TEXT("BACK"), A_Back, PageCredits, -1);
		}

		// ---- YARD page (S2 SHIPYARD): one button per roster ship, progress, LAUNCH, BACK
		{
			UVerticalBox* YB = Pages[PageYard];
			for (int32 i = 0; i < IronwakeFleet::Roster().Num() && i < 6; ++i)
			{
				UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
				B->SetBackgroundColor(BtnNormal);
				UVerticalBox* In = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
				UTextBlock* Nm = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Nm->SetFont(BoldFont(20));
				In->AddChildToVerticalBox(Nm);
				UTextBlock* If = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				If->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14));
				If->SetColorAndOpacity(FSlateColor(Steel));
				If->SetAutoWrapText(true);
				If->SetWrapTextAt(720.f);
				In->AddChildToVerticalBox(If)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
				B->AddChild(In);
				if (UButtonSlot* BS = Cast<UButtonSlot>(In->Slot)) { BS->SetPadding(FMargin(18.f, 8.f)); BS->SetHorizontalAlignment(HAlign_Left); }
				UIronwakeMenuProxy* P = NewObject<UIronwakeMenuProxy>(this); P->Owner = this; P->Id = A_YardShip0 + i;
				B->OnClicked.AddDynamic(P, &UIronwakeMenuProxy::Clicked); Proxies.Add(P);
				UVerticalBoxSlot* VS = YB->AddChildToVerticalBox(B); VS->SetPadding(FMargin(0.f, 4.f)); VS->SetHorizontalAlignment(HAlign_Fill);
				Btns.Add(B); BtnIds.Add(A_YardShip0 + i); BtnPage.Add(PageYard); BtnTab.Add(-1);
				YardNames.Add(Nm); YardInfo.Add(If); YardButtons.Add(B);
			}
			YardProgress = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			YardProgress->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 14));
			YardProgress->SetColorAndOpacity(FSlateColor(Steel));
			YardProgress->SetAutoWrapText(true);
			YB->AddChildToVerticalBox(YardProgress)->SetPadding(FMargin(2.f, 10.f, 0.f, 6.f));
			MakeBtn(YB, TEXT("LAUNCH"), A_YardLaunch, PageYard, -1, &Tmp); YardLaunchLabel = Tmp;
			MakeBtn(YB, TEXT("BACK"), A_Back, PageYard, -1);
		}
		// Down from the AUDIO tab lands on the MASTER slider (geometry alone picks the TEST button).
		for (int32 i = 0; i < Btns.Num(); ++i) if (BtnIds[i] == A_TabGameplay + 3 && Sliders.Num()) Btns[i]->SetNavigationRuleExplicit(EUINavigation::Down, Sliders[0]);
		bSlidersInit = false;
		Refresh();
	}
	return Super::RebuildWidget();
}

void UIronwakeMenuWidget::Refresh()
{
	AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer());
	const bool bPeace = PC && PC->bPeaceful;
	if (StartLabel) StartLabel->SetText(FText::FromString(Mode == 1 ? TEXT("RESUME") : TEXT("START VOYAGE")));
	if (ModeButton) ModeButton->SetIsEnabled(Mode == 0);
	for (int32 i = 0; i < Btns.Num(); ++i) if (BtnIds[i] == A_ModeBase + 1) Btns[i]->SetIsEnabled(Mode == 0);
	if (Subtitle)
	{
		Subtitle->SetText(FText::FromString(Mode == 1 ? TEXT("Paused.  The sea waits for no one.") : bPeace ? TEXT("Peaceful voyage: open sea, no enemy ships.") : TEXT("Captain: an enemy ironclad hunts these waters.")));
		Subtitle->SetVisibility(Page == PageMain ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (PageTitle)
	{
		static const TCHAR* Titles[4] = { TEXT(""), TEXT("SETTINGS"), TEXT("CREDITS"), TEXT("SHIPYARD  -  GREYHAVEN") };
		PageTitle->SetText(FText::FromString(Titles[FMath::Clamp(Page, 0, 3)]));
		PageTitle->SetVisibility(Page == PageMain ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	for (int32 i = 0; i < Pages.Num(); ++i) Pages[i]->SetVisibility(i == Page ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	for (int32 i = 0; i < TabBoxes.Num(); ++i) TabBoxes[i]->SetVisibility(i == Tab ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	for (int32 c = 0; c < Sliders.Num(); ++c)
	{
		const int32 V = IronwakeAudio::GetVol(c);
		Sliders[c]->SetValue(V / 100.f);
		SliderPct[c]->SetText(FText::FromString(c == IronwakeAudio::Music && V == 0 ? TEXT("0% OFF") : FString::Printf(TEXT("%d%%"), V)));
	}
	bSlidersInit = true;
	const FName Cur = IronwakeFleet::GetSelected();
	const TArray<FIronwakeShipDef>& R = IronwakeFleet::Roster();
	for (int32 i = 0; i < YardNames.Num(); ++i)
	{
		const bool bUn = IronwakeFleet::IsUnlocked(R[i].Id);
		const bool bCur = R[i].Id == Cur;
		const bool bHi = i == YardSel;
		YardNames[i]->SetText(FText::FromString(FString::Printf(TEXT("%s%s   %s"), bHi ? TEXT("> ") : TEXT("   "), *R[i].Name.ToUpper(),
			bCur ? TEXT("[ IN COMMAND ]") : bUn ? TEXT("") : TEXT("[ LOCKED ]"))));
		YardNames[i]->SetColorAndOpacity(FSlateColor(bUn ? Brass : FLinearColor(0.45f, 0.45f, 0.45f, 1.f)));
		YardInfo[i]->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s%s"), *R[i].Class, *R[i].Blurb,
			bUn ? TEXT("") : *FString::Printf(TEXT("\nUnlock: %s"), *R[i].UnlockText))));
	}
	if (YardProgress) YardProgress->SetText(FText::FromString(IronwakeFleet::ProgressLine()));
	if (YardLaunchLabel && R.IsValidIndex(YardSel))
	{
		const bool bUn = IronwakeFleet::IsUnlocked(R[YardSel].Id);
		YardLaunchLabel->SetText(FText::FromString(!bUn ? TEXT("LOCKED") : R[YardSel].Id == Cur ? TEXT("CLOSE") : FString::Printf(TEXT("LAUNCH  %s"), *R[YardSel].Name.ToUpper())));
	}
}

void UIronwakeMenuWidget::OpenPage(int32 InPage, int32 InTab)
{
	Page = InPage; Tab = FMath::Clamp(InTab, 0, 3);
	if (Page == PageYard)
	{
		const FName Cur = IronwakeFleet::GetSelected();
		for (int32 i = 0; i < IronwakeFleet::Roster().Num(); ++i) if (IronwakeFleet::Roster()[i].Id == Cur) YardSel = i;
	}
	Refresh();
	RestoreFocus();
}

bool UIronwakeMenuWidget::GoBack()
{
	if (Page == PageMain) return false;
	OpenPage(PageMain);
	return true;
}

void UIronwakeMenuWidget::RestoreFocus()
{
	if (!IsInViewport()) return;
	int32 Pick = INDEX_NONE;
	for (int32 i = 0; i < Btns.Num() && Pick == INDEX_NONE; ++i)
	{
		if (!BtnVisible(i) || !Btns[i]->GetIsEnabled()) continue;
		if (Page == PageSettings) { if (BtnIds[i] == A_TabGameplay + Tab) Pick = i; }
		else Pick = i;
	}
	if (Pick != INDEX_NONE) Btns[Pick]->SetKeyboardFocus();
}

FString UIronwakeMenuWidget::FocusedName() const
{
	for (int32 i = 0; i < Btns.Num(); ++i)
		if (BtnVisible(i) && (Btns[i]->HasKeyboardFocus() || Btns[i]->HasAnyUserFocus()))
		{
			if (const UTextBlock* T = Cast<UTextBlock>(Btns[i]->GetChildAt(0))) return FString::Printf(TEXT("button '%s'"), *T->GetText().ToString());
			return FString::Printf(TEXT("button id=%d"), BtnIds[i]);
		}
	for (int32 c = 0; c < Sliders.Num(); ++c)
		if (Sliders[c] && Sliders[c]->HasKeyboardFocus()) return FString::Printf(TEXT("slider %s (%d%%)"), IronwakeAudio::Key(c), FMath::RoundToInt(Sliders[c]->GetValue() * 100.f));
	return TEXT("(none)");
}

void UIronwakeMenuWidget::FitToViewport()
{
	UWorld* W = GetWorld();
	UGameViewportClient* VC = W ? W->GetGameViewport() : nullptr;
	if (!VC || !RootScale) return;
	FVector2D VP; VC->GetViewportSize(VP);
	if (VP.X < 1.f || VP.Y < 1.f) return;
	if (VP.Equals(LastVP, 0.5f)) return;
	LastVP = VP;
	CurTarget = FMath::Clamp(FMath::Min(VP.X / 1920.f, VP.Y / 1080.f), 0.5f, 1.5f);
	CurArea = VP / CurTarget;
	RootScale->SetUserSpecifiedScale(CurTarget);
	CardBox->SetWidthOverride(FMath::Min(CardW, CurArea.X - CardMarginL - 20.f));
	CardBox->SetMaxDesiredHeight(FMath::Max(200.f, CurArea.Y - 2.f * CardMarginT));
	UE_LOG(LogIronwake, Display, TEXT("IWP18 fit vp=%.0fx%.0f scale=%.3f area=%.0fx%.0f cardMaxH=%.0f"), VP.X, VP.Y, CurTarget, CurArea.X, CurArea.Y, CurArea.Y - 2.f * CardMarginT);
}

void UIronwakeMenuWidget::TickMenu(float Dt)
{
	if (!IsInViewport()) return;
	FitToViewport();
	AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer());
	UWorld* W = GetWorld();
	// Audio sliders: mouse drag / keys change the slider; snap to 5 % and apply live.
	if (bSlidersInit && Page == PageSettings && Tab == 3)
	{
		for (int32 c = 0; c < Sliders.Num(); ++c)
		{
			const int32 V = FMath::Clamp(FMath::RoundToInt(Sliders[c]->GetValue() * 20.f) * 5, 0, 100);
			if (FMath::Abs(Sliders[c]->GetValue() * 100.f - V) > 0.01f) Sliders[c]->SetValue(V / 100.f);
			if (V != IronwakeAudio::GetVol(c))
			{
				const bool bWasMusic = IronwakeAudio::MusicOn();
				IronwakeAudio::SetVol(c, V, W);
				if (c == IronwakeAudio::Music && bWasMusic != IronwakeAudio::MusicOn() && PC) PC->ApplyMusicToShips();
			}
			SliderPct[c]->SetText(FText::FromString(c == IronwakeAudio::Music && V == 0 ? TEXT("0% OFF") : FString::Printf(TEXT("%d%%"), V)));
			Sliders[c]->SetSliderHandleColor(Sliders[c]->HasKeyboardFocus() ? FLinearColor(1.f, 0.92f, 0.6f, 1.f) : Brass);
		}
	}
	// Button tint: focus beats selected beats normal.
	for (int32 i = 0; i < Btns.Num(); ++i)
	{
		if (!BtnVisible(i)) continue;
		const int32 Id = BtnIds[i];
		bool bSel = false;
		if (Id >= A_TabGameplay && Id < A_TabGameplay + 4) bSel = (Id - A_TabGameplay) == Tab;
		else if (Id == A_ModeBase || Id == A_ModeBase + 1) bSel = (Id - A_ModeBase == 0) == (PC && PC->bPeaceful);
		else if (Id == A_RudBase || Id == A_RudBase + 1) bSel = (Id - A_RudBase) == (IronwakeSettings::GetRudderAutoCentre() ? 1 : 0);
		else if (Id >= A_GfxBase && Id < A_GfxBase + 3) bSel = (Id - A_GfxBase) == IronwakeSettings::GetGraphics();
		else if (Id >= A_CamBase && Id < A_CamBase + 3) bSel = (Id - A_CamBase) == IronwakeSettings::GetCameraMotion();
		else if (Id >= A_YardShip0 && Id < A_YardShip0 + 6) bSel = (Id - A_YardShip0) == YardSel;
		const bool bFoc = Btns[i]->HasKeyboardFocus() || Btns[i]->HasAnyUserFocus();
		Btns[i]->SetBackgroundColor(bFoc ? BtnFocus : bSel ? BtnSelected : BtnNormal);
	}
}

FReply UIronwakeMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer());
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (PC) PC->EscapeOrBack();
		return FReply::Handled();
	}
	if (K == EKeys::Enter || K == EKeys::Gamepad_FaceButton_Bottom)
	{
		// Enter / A on a focused audio slider = that row's TEST button (keyboard and gamepad reach TEST this way; the mouse clicks the button).
		for (int32 c = 0; c < Sliders.Num(); ++c)
			if (Page == PageSettings && Tab == 3 && Sliders[c] && Sliders[c]->HasKeyboardFocus()) { OnAction(A_Test0 + c); return FReply::Handled(); }
	}
	if (Page == PageSettings && (K == EKeys::Gamepad_LeftShoulder || K == EKeys::Gamepad_RightShoulder || K == EKeys::PageUp || K == EKeys::PageDown))
	{
		const int32 D = (K == EKeys::Gamepad_LeftShoulder || K == EKeys::PageUp) ? -1 : 1;
		OpenPage(PageSettings, (Tab + D + 4) % 4);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UIronwakeMenuWidget::SetSliderPct(int32 ClassIdx, int32 Pct)
{
	if (Sliders.IsValidIndex(ClassIdx)) Sliders[ClassIdx]->SetValue(FMath::Clamp(Pct, 0, 100) / 100.f);
}

int32 UIronwakeMenuWidget::CheckLayout(const FString& Tag)
{
	UWorld* W = GetWorld();
	FVector2D VP(1.f, 1.f);
	if (W && W->GetGameViewport()) W->GetGameViewport()->GetViewportSize(VP);
	struct FR { FString N; FVector2D A, B; UWidget* Wd; bool bInScroll; };
	TArray<FR> Rs;
	auto Rect = [&](UWidget* Wd, FVector2D& A, FVector2D& B)
	{
		const FGeometry G = Wd->GetCachedGeometry();
		FVector2D V1, V2;
		USlateBlueprintLibrary::LocalToViewport(this, G, FVector2D::ZeroVector, A, V1);
		USlateBlueprintLibrary::LocalToViewport(this, G, G.GetLocalSize(), B, V2);
	};
	auto Shown = [&](UWidget* Wd)
	{
		for (UWidget* X = Wd; X; X = X->GetParent())
		{
			const ESlateVisibility V = X->GetVisibility();
			if (V == ESlateVisibility::Collapsed || V == ESlateVisibility::Hidden) return false;
		}
		return Wd->GetCachedGeometry().GetLocalSize().X > 0.5f && Wd->GetCachedGeometry().GetLocalSize().Y > 0.5f;
	};
	auto HasAncestor = [&](UWidget* Wd, UWidget* Anc) { for (UWidget* X = Wd->GetParent(); X; X = X->GetParent()) if (X == Anc) return true; return false; };
	WidgetTree->ForEachWidget([&](UWidget* Wd)
	{
		const bool bLeaf = Cast<UButton>(Wd) || Cast<USlider>(Wd) || Cast<UTextBlock>(Wd);
		if (!bLeaf || !Shown(Wd)) return;
		if (Cast<UTextBlock>(Wd)) { for (UWidget* X = Wd->GetParent(); X; X = X->GetParent()) if (Cast<UButton>(X)) return; } // button label: the button rect covers it
		FR R; R.Wd = Wd; R.bInScroll = CardScroll && HasAncestor(Wd, CardScroll);
		if (const UTextBlock* T = Cast<UTextBlock>(Wd)) R.N = FString::Printf(TEXT("text '%s'"), *T->GetText().ToString().Left(24).Replace(TEXT("\n"), TEXT(" ")));
		else if (Cast<USlider>(Wd)) R.N = Wd->GetName();
		else if (const UTextBlock* BT = Cast<UTextBlock>(Cast<UButton>(Wd)->GetChildAt(0))) R.N = FString::Printf(TEXT("button '%s'"), *BT->GetText().ToString());
		else R.N = Wd->GetName();
		Rect(Wd, R.A, R.B);
		Rs.Add(R);
	});
	// the card itself (the border inside the size box) must be inside the viewport too
	FVector2D CA = FVector2D::ZeroVector, CB = FVector2D::ZeroVector;
	if (CardBox && CardBox->GetCachedGeometry().GetLocalSize().X > 0.5f) Rect(CardBox, CA, CB);
	FVector2D SA = FVector2D::ZeroVector, SB = FVector2D::ZeroVector;
	if (CardScroll) Rect(CardScroll, SA, SB);
	int32 Outside = 0, Clipped = 0, Overlaps = 0;
	const float Tol = 1.5f;
	FString Detail;
	for (const FR& R : Rs)
	{
		if (R.A.X < -Tol || R.A.Y < -Tol || R.B.X > VP.X + Tol || R.B.Y > VP.Y + Tol) { ++Outside; Detail += FString::Printf(TEXT(" OUT[%s %.0f,%.0f-%.0f,%.0f]"), *R.N, R.A.X, R.A.Y, R.B.X, R.B.Y); }
		if (R.bInScroll && (R.A.X < SA.X - Tol || R.A.Y < SA.Y - Tol || R.B.X > SB.X + Tol || R.B.Y > SB.Y + Tol)) { ++Clipped; Detail += FString::Printf(TEXT(" CLIP[%s %.0f,%.0f-%.0f,%.0f vs scroll %.0f,%.0f-%.0f,%.0f]"), *R.N, R.A.X, R.A.Y, R.B.X, R.B.Y, SA.X, SA.Y, SB.X, SB.Y); }
	}
	if (CB.X > VP.X + Tol || CB.Y > VP.Y + Tol || CA.X < -Tol || CA.Y < -Tol) { ++Outside; Detail += TEXT(" OUT[card]"); }
	for (int32 i = 0; i < Rs.Num(); ++i)
		for (int32 j = i + 1; j < Rs.Num(); ++j)
		{
			if (HasAncestor(Rs[i].Wd, Rs[j].Wd) || HasAncestor(Rs[j].Wd, Rs[i].Wd)) continue;
			const float OX = FMath::Min(Rs[i].B.X, Rs[j].B.X) - FMath::Max(Rs[i].A.X, Rs[j].A.X);
			const float OY = FMath::Min(Rs[i].B.Y, Rs[j].B.Y) - FMath::Max(Rs[i].A.Y, Rs[j].A.Y);
			if (OX > 2.f && OY > 2.f) { ++Overlaps; Detail += FString::Printf(TEXT(" OVERLAP[%s | %s]"), *Rs[i].N, *Rs[j].N); }
		}
	const float ScrollNeeded = CardScroll ? CardScroll->GetScrollOffsetOfEnd() : 0.f;
	UE_LOG(LogIronwake, Display, TEXT("IWP18 LAYOUT %s vp=%.0fx%.0f scale=%.2f card=%.0f,%.0f-%.0f,%.0f widgets=%d outside=%d overlaps=%d clippedByCard=%d scrollRange=%.0f %s%s%s"),
		*Tag, VP.X, VP.Y, CurTarget, CA.X, CA.Y, CB.X, CB.Y, Rs.Num(), Outside, Overlaps, Clipped, ScrollNeeded, (Outside + Overlaps + Clipped) == 0 ? TEXT("PASS") : TEXT("FAIL"), *Detail, TEXT(""));
	return Outside + Overlaps + Clipped;
}

void UIronwakeMenuWidget::PickYardShip(int32 i) { YardSel = i; Refresh(); }

void UIronwakeMenuWidget::OnAction(int32 Id)
{
	AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer());
	UWorld* W = GetWorld();
	IronwakeAudio::Click(W);
	if (Id >= A_Test0 && Id < A_Test0 + IronwakeAudio::Count) { IronwakeAudio::TestPlay(W, Id - A_Test0); return; }
	if (Id >= A_TabGameplay && Id < A_TabGameplay + 4) { OpenPage(PageSettings, Id - A_TabGameplay); return; }
	if (Id >= A_ModeBase && Id <= A_ModeBase + 1) { if (PC && Mode == 0) PC->bPeaceful = (Id == A_ModeBase); Refresh(); return; }
	if (Id >= A_RudBase && Id <= A_RudBase + 1) { IronwakeSettings::SetRudderAutoCentre(Id == A_RudBase + 1); Refresh(); return; }
	if (Id >= A_GfxBase && Id < A_GfxBase + 3) { IronwakeSettings::SetGraphics(Id - A_GfxBase); Refresh(); return; }
	if (Id >= A_CamBase && Id < A_CamBase + 3) { IronwakeSettings::SetCameraMotion(Id - A_CamBase); Refresh(); return; }
	if (Id >= A_YardShip0 && Id < A_YardShip0 + 6) { PickYardShip(Id - A_YardShip0); return; }
	switch (Id)
	{
	case A_Start: if (PC) { if (Mode == 1) PC->TogglePause(); else PC->StartGame(PC->bPeaceful); } break;
	case A_Storm:
		if (PC)
		{
			if (Mode == 1) PC->TogglePause(); else PC->StartGame(true);
			if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(PC->GetWorld())) Vy->StartStormAtSea();
		}
		break;
	case A_Yard: OpenPage(PageYard); break;
	case A_Settings: OpenPage(PageSettings, Tab); break;
	case A_Controls: if (PC) PC->ToggleHelp(); break;
	case A_Credits: OpenPage(PageCredits); break;
	case A_Quit: UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); break;
	case A_Back: GoBack(); break;
	case A_ResetAudio:
	{
		const bool bWasMusic = IronwakeAudio::MusicOn();
		IronwakeAudio::ResetAll(W);
		if (bWasMusic && PC) PC->ApplyMusicToShips();
		Refresh();
		break;
	}
	case A_YardLaunch:
	{
		const TArray<FIronwakeShipDef>& R = IronwakeFleet::Roster();
		if (!R.IsValidIndex(YardSel) || !IronwakeFleet::IsUnlocked(R[YardSel].Id)) break;
		if (R[YardSel].Id == IronwakeFleet::GetSelected()) { GoBack(); break; }
		if (PC) PC->ChooseShip(R[YardSel].Id);
		break;
	}
	default: break;
	}
}

// ============================================================================ Controller
AIronwakePlayerController::AIronwakePlayerController()
{
	bShouldPerformFullTickWhenPaused = true;
}

void AIronwakePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	AIronwakeGameMode* GM = GetWorld()->GetAuthGameMode<AIronwakeGameMode>();
	UE_LOG(LogIronwake, Display, TEXT("IWMENU controller up, title=%d"), GM ? GM->bShowTitleMenu : -1);
	if (GM && GM->bShowTitleMenu) ShowMenu(0);
	else
	{
		bStarted = true;
		if (GM && GM->bRestarted)
		{
			bPeaceful = !GM->bSpawnEnemy;
			StartGame(bPeaceful);
		}
	}
	IronwakeAudio::Init(GetWorld());
	if (FParse::Value(FCommandLine::Get(), TEXT("iwp18="), P18Mode)) { P18Mode = P18Mode.TrimStartAndEnd(); UE_LOG(LogIronwake, Display, TEXT("IWP18 mode %s"), *P18Mode); }
	if (FParse::Param(FCommandLine::Get(), TEXT("iwaudiotest")))
	{
		FTimerHandle HA;
		GetWorldTimerManager().SetTimer(HA, [this]() { const int32 Bad = IronwakeAudio::RunRouteTest(GetWorld()); UE_LOG(LogIronwake, Display, TEXT("IWAUDIO routetest bad=%d"), Bad); ConsoleCommand(TEXT("quit")); }, 4.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("iwcredits")))
	{
		// R1 verification: title menu with the CREDITS panel, screenshot, quit.
		FTimerHandle H1, H2, H3;
		GetWorldTimerManager().SetTimer(H1, [this]() { IWCredits(); }, 3.f, false);
		GetWorldTimerManager().SetTimer(H2, [this]() { IWShot(TEXT("R_credits")); }, 8.f, false);
		GetWorldTimerManager().SetTimer(H3, [this]() { ConsoleCommand(TEXT("quit")); }, 11.f, false);
	}
	const FString Trig = FPaths::ProjectSavedDir() / TEXT("iw_p1.txt");
	if (IFileManager::Get().FileExists(*Trig))
	{
		FFileHelper::LoadFileToString(P1Mode, *Trig);
		P1Mode.TrimStartAndEndInline();
		IFileManager::Get().Delete(*Trig);
		UE_LOG(LogIronwake, Display, TEXT("IWP1 test mode '%s' restarted=%d"), *P1Mode, GM ? GM->bRestarted : -1);
	}
}

void AIronwakePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	IA_Pause = NewObject<UInputAction>(this, TEXT("IA_Pause"));
	IA_Pause->ValueType = EInputActionValueType::Boolean;
	IA_Pause->bTriggerWhenPaused = true;
	MenuContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Menu"));
	for (const FKey& K : { EKeys::Tab, EKeys::Escape, EKeys::Gamepad_Special_Right }) // A1: P is the autopilot now
	{
		MenuContext->MapKey(IA_Pause, K).Triggers.Add(NewObject<UInputTriggerPressed>(MenuContext));
	}
	// S1 Voyager keys: F photo, H help (P11; was helmsman), N next point, X anchor, G fast sail, L weather, K weather drift,
	// J helmsman, T clock speed (Shift+T +3 h), F1 help, ] +3 h, O Captain's Logbook (P29).
	const FKey VKeys[] = { EKeys::F, EKeys::H, EKeys::N, EKeys::X, EKeys::G, EKeys::L, EKeys::K, EKeys::J, EKeys::T, EKeys::F1, EKeys::RightBracket, EKeys::O, EKeys::V };
	for (int32 i = 0; i < UE_ARRAY_COUNT(VKeys); ++i)
	{
		UInputAction* A = NewObject<UInputAction>(this, *FString::Printf(TEXT("IA_Voyage%d"), i));
		A->ValueType = EInputActionValueType::Boolean;
		MenuContext->MapKey(A, VKeys[i]).Triggers.Add(NewObject<UInputTriggerPressed>(MenuContext));
		VoyageActions.Add(A);
	}
	IA_Arrive = NewObject<UInputAction>(this, TEXT("IA_Arrive"));
	IA_Arrive->ValueType = EInputActionValueType::Boolean;
	MenuContext->MapKey(IA_Arrive, EKeys::E).Triggers.Add(NewObject<UInputTriggerPressed>(MenuContext));
	MenuContext->MapKey(IA_Arrive, EKeys::Gamepad_FaceButton_Bottom).Triggers.Add(NewObject<UInputTriggerPressed>(MenuContext));

	DockContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Dock"));
	IA_DockSink = NewObject<UInputAction>(this, TEXT("IA_DockSink"));
	IA_DockSink->ValueType = EInputActionValueType::Axis2D;
	for (const FKey& K : { EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Q, EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right,
		EKeys::Mouse2D, EKeys::MouseWheelAxis, EKeys::LeftShift, EKeys::RightShift, EKeys::SpaceBar, EKeys::B, EKeys::R })
		DockContext->MapKey(IA_DockSink, K);
	// A1 autopilot keys: P cycle, M chart, Enter start route, Backspace clear route.
	const FKey AKeys[] = { EKeys::P, EKeys::M, EKeys::Enter, EKeys::BackSpace };
	for (int32 i = 0; i < 4; ++i)
	{
		UInputAction* A = NewObject<UInputAction>(this, *FString::Printf(TEXT("IA_Ap%d"), i));
		A->ValueType = EInputActionValueType::Boolean;
		MenuContext->MapKey(A, AKeys[i]).Triggers.Add(NewObject<UInputTriggerPressed>(MenuContext));
		ApActions.Add(A);
	}
	// Chart context (priority 30): clicks place/remove waypoints; mouse look, fire, zoom and A/D are swallowed
	// (W/S still set the speed with the chart open).
	ChartContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Chart"));
	IA_ChartL = NewObject<UInputAction>(this, TEXT("IA_ChartL"));
	IA_ChartL->ValueType = EInputActionValueType::Boolean;
	ChartContext->MapKey(IA_ChartL, EKeys::LeftMouseButton).Triggers.Add(NewObject<UInputTriggerPressed>(ChartContext));
	IA_ChartR = NewObject<UInputAction>(this, TEXT("IA_ChartR"));
	IA_ChartR->ValueType = EInputActionValueType::Boolean;
	ChartContext->MapKey(IA_ChartR, EKeys::RightMouseButton).Triggers.Add(NewObject<UInputTriggerPressed>(ChartContext));
	IA_ChartSink = NewObject<UInputAction>(this, TEXT("IA_ChartSink"));
	IA_ChartSink->ValueType = EInputActionValueType::Axis2D;
	for (const FKey& K : { EKeys::Mouse2D, EKeys::MouseWheelAxis, EKeys::LeftShift, EKeys::A, EKeys::D, EKeys::Left, EKeys::Right })
		ChartContext->MapKey(IA_ChartSink, K);
	// Photo mode context (priority 20): swallows the ship's keys and mouse so only the free camera moves.
	PhotoContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Photo"));
	IA_PhotoSink = NewObject<UInputAction>(this, TEXT("IA_PhotoSink"));
	IA_PhotoSink->ValueType = EInputActionValueType::Axis2D;
	for (const FKey& K : { EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Q, EKeys::E, EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right,
		EKeys::Mouse2D, EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::R, EKeys::LeftShift, EKeys::SpaceBar, EKeys::LeftControl })
		PhotoContext->MapKey(IA_PhotoSink, K);
	IA_PhotoWheel = NewObject<UInputAction>(this, TEXT("IA_PhotoWheel"));
	IA_PhotoWheel->ValueType = EInputActionValueType::Axis1D;
	PhotoContext->MapKey(IA_PhotoWheel, EKeys::MouseWheelAxis);
	IA_PhotoTime = NewObject<UInputAction>(this, TEXT("IA_PhotoTime"));
	IA_PhotoTime->ValueType = EInputActionValueType::Boolean;
	PhotoContext->MapKey(IA_PhotoTime, EKeys::T).Triggers.Add(NewObject<UInputTriggerPressed>(PhotoContext));
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(IA_Pause, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnPauseAction);
		if (IA_Arrive) EIC->BindAction(IA_Arrive, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnArriveAction);
		for (int32 i = 0; i < VoyageActions.Num(); ++i) EIC->BindAction(VoyageActions[i], ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnVoyageKey, i);
		for (int32 i = 0; i < ApActions.Num(); ++i) EIC->BindAction(ApActions[i], ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnApKey, i);
		EIC->BindAction(IA_ChartL, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnChartClick, false);
		EIC->BindAction(IA_ChartR, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnChartClick, true);
		EIC->BindAction(IA_PhotoWheel, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnPhotoWheel);
		EIC->BindAction(IA_PhotoTime, ETriggerEvent::Triggered, this, &AIronwakePlayerController::OnPhotoTime);
	}
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Sub->AddMappingContext(MenuContext, 10);
	}
}

// ============================================================================ S1 Voyager keys / photo input
bool AIronwakePlayerController::CanUseVoyageKeys() const
{
	return bStarted && !IsMenuOpen() && !IsEndScreenOpen() && GetWorld() && !GetWorld()->IsPaused();
}

void AIronwakePlayerController::OnVoyageKey(const FInputActionValue& V, int32 Which)
{
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	if (!Vy || !CanUseVoyageKeys()) return;
	if (Vy->bPhoto && Which != 0 && Which != 5) return; // photo mode: only F (exit) and L (weather); T is the photo slow-mo there
	if (Vy->bChart && Which == 5) { Vy->ToggleLoop(); return; } // A1: L on the chart = route loop
	if (Vy->bChart && Which == 0) return;
	switch (Which)
	{
	case 0: Vy->TogglePhoto(); break;
	case 1: ToggleHelp(); break;
	case 2: Vy->NextPoint(); break;
	case 3: Vy->ToggleAnchor(); break;
	case 4: Vy->ToggleFastSail(); break;
	case 5: Vy->CycleWeather(); break;
	case 6: Vy->ToggleAutoCycle(); break;
	case 7: Vy->ToggleHelm(); break;
	case 8:
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			if (AIronwakeShipPawn* Sp = Cast<AIronwakeShipPawn>(GetPawn())) if (Sp->IsScoped()) Sp->SetScope(false);   // Shift alone opens the binoculars
			if (Vy->bLiving) Vy->JumpHours(3.f);
		}
		else if (Vy->bLiving) Vy->CycleClockSpeed();
		break;
	case 9: ToggleHelp(); break;
	case 10: if (Vy->bLiving) Vy->JumpHours(3.f); break;
	case 11:
		if (Vy->bDockOpen) Vy->CloseDockModal();
		else if (Vy->CanDockPortIdx >= 0) Vy->OpenDockModal(Vy->CanDockPortIdx, UIronwakeDockWidget::TabLogbook);
		else if (Vy->CanLogPoiIdx >= 0) Vy->OpenLandmarkModal(Vy->CanLogPoiIdx);
		else
		{
			if (!Vy->DockWidget) Vy->DockWidget = CreateWidget<UIronwakeDockWidget>(this, UIronwakeDockWidget::StaticClass());
			if (Vy->DockWidget)
			{
				Vy->DockWidget->SetupForLandmark(Vy, TEXT("CAPTAIN'S LOGBOOK"), TEXT("Underway Career & Trade Ledger"),
					TEXT("\"Soundings, port arrivals, and cargo manifests of this commission.\""), TEXT("Make fast in Greyhaven, Ironcrag, or Stigsby (< 2.5 kn) to trade cargo or refit."));
				if (!Vy->DockWidget->IsInViewport()) Vy->DockWidget->AddToViewport(48);
				Vy->bDockOpen = true;
				SetDockInput(true);
			}
		}
		break;
	case 12: Vy->ToggleClearHaze(); break;
	default: break;
	}
}

void AIronwakePlayerController::OnPhotoWheel(const FInputActionValue& V) { PhotoFovDelta += V.Get<float>(); }

// ============================================================================ A1 autopilot / chart input
void AIronwakePlayerController::OnApKey(const FInputActionValue& V, int32 Which)
{
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	if (!Vy || !CanUseVoyageKeys()) return;
	switch (Which)
	{
	case 0: Vy->CycleAutopilot(); break;                    // P (works in photo mode too: sit back and watch)
	case 1: if (!Vy->bPhoto && !Vy->bDockOpen) Vy->ToggleChart(); break;      // M
	case 2:
		if (Vy->bChart) Vy->ChartStart();                                     // Enter on chart = start route
		else if (!Vy->bPhoto) Vy->TryArrivalAction();                         // Enter in port / landmark = Make Fast / Log Discovery
		break;
	case 3: if (Vy->bChart) Vy->ClearWaypoints(); break;     // Backspace
	default: break;
	}
}

void AIronwakePlayerController::OnChartClick(const FInputActionValue& V, bool bRight)
{
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	if (!Vy || !Vy->bChart) return;
	float X = 0.f, Y = 0.f;
	if (GetMousePosition(X, Y)) Vy->ChartClick(FVector2D(X, Y), bRight);
}

void AIronwakePlayerController::SetChartInput(bool bOn)
{
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (bOn) Sub->AddMappingContext(ChartContext, 30);
			else Sub->RemoveMappingContext(ChartContext);
		}
	}
	if (bOn)
	{
		FInputModeGameAndUI M;
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		M.SetHideCursorDuringCapture(false);
		SetInputMode(M);
		SetShowMouseCursor(true);
	}
	else if (!IsMenuOpen() && !IsEndScreenOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void AIronwakePlayerController::IWAutopilot(int32 Mode)
{
	if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()))
	{
		if (Mode <= 0) Vy->DisengageAutopilot(TEXT("Autopilot OFF"));
		else Vy->EngageAutopilot(FMath::Clamp(Mode, 1, 3));
	}
}
void AIronwakePlayerController::IWChart() { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->ToggleChart(); }
void AIronwakePlayerController::OnPhotoTime(const FInputActionValue& V) { bPhotoTimeCycle = true; }

void AIronwakePlayerController::SetPhotoInput(bool bOn)
{
	bPhotoInput = bOn;
	PhotoMove = FVector::ZeroVector;
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (bOn) Sub->AddMappingContext(PhotoContext, 20);
			else Sub->RemoveMappingContext(PhotoContext);
		}
	}
}

FVector2D AIronwakePlayerController::ConsumePhotoLook()
{
	float DX = 0.f, DY = 0.f;
	GetInputMouseDelta(DX, DY);
	return FVector2D(DX * 0.6f, DY * 0.6f);
}

void AIronwakePlayerController::IWPhoto() { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->TogglePhoto(); }
void AIronwakePlayerController::IWWeather(int32 Preset) { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->SetWeather(Preset, 6.f); }
void AIronwakePlayerController::IWHelm() { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->ToggleHelm(); }
void AIronwakePlayerController::IWAnchor() { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->ToggleAnchor(); }
void AIronwakePlayerController::IWFastSail() { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->ToggleFastSail(); }

void AIronwakePlayerController::OnPauseAction(const FInputActionValue& V)
{
	if (HandleBack()) return;
	if (!bStarted) return; // title menu: Start or Quit
	if (EndWidget && EndWidget->IsInViewport())
	{
		if (EndWidget->Mode == 1) HideEndScreen(); // Esc on VICTORY = Continue
		return;                                   // SHIP LOST: Restart or Main Menu only
	}
	TogglePause();
}

bool AIronwakePlayerController::HandleBack()
{
	if (IsDockOpen()) { if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->CloseDockModal(); return true; }
	if (IsHelpOpen()) { ToggleHelp(); return true; }  // P11: Esc closes the help panel first
	if (IsMenuOpen() && Menu && Menu->GoBack()) return true; // P18: Esc / B = one page back
	return false;
}

bool AIronwakePlayerController::IsDockOpen() const
{
	const AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	return Vy && Vy->bDockOpen;
}

void AIronwakePlayerController::SetDockInput(bool bOn)
{
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (bOn) Sub->AddMappingContext(DockContext, 40);
			else Sub->RemoveMappingContext(DockContext);
		}
	}
	if (bOn)
	{
		FInputModeGameAndUI M;
		M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		M.SetHideCursorDuringCapture(false);
		if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld()); Vy && Vy->DockWidget)
			M.SetWidgetToFocus(Vy->DockWidget->TakeWidget());
		SetInputMode(M);
		SetShowMouseCursor(true);
	}
	else if (!IsMenuOpen() && !IsEndScreenOpen())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void AIronwakePlayerController::OnArriveAction(const FInputActionValue& V)
{
	(void)V;
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	if (!Vy || !CanUseVoyageKeys() || Vy->bPhoto || Vy->bChart) return;
	if (!Vy->TryArrivalAction())
	{
		// Outside port / landmark mooring zones, E still centres the helm just like Q
		if (AIronwakeShipPawn* Sp = Cast<AIronwakeShipPawn>(GetPawn())) Sp->IWRudder(0);
	}
}

void AIronwakePlayerController::IWDock(int32 PortIdx)
{
	AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld());
	if (!Vy) return;
	if (Vy->bDockOpen && PortIdx < 0) { Vy->CloseDockModal(); return; }
	const int32 Target = PortIdx >= 0 ? PortIdx : (Vy->CanDockPortIdx >= 0 ? Vy->CanDockPortIdx : 0);
	Vy->OpenDockModal(Target, UIronwakeDockWidget::TabMarket);
}

void AIronwakePlayerController::EscapeOrBack()
{
	if (HandleBack()) return;
	if (!bStarted) return; // title menu main page: Start or Quit
	OnPauseAction(FInputActionValue(true));
}

void AIronwakePlayerController::ApplyMusicToShips()
{
	for (TActorIterator<AIronwakeShipPawn> It(GetWorld()); It; ++It) if (!It->bIsNPC) It->ApplyMusicSetting();
}

void AIronwakePlayerController::ShowMenu(int32 Mode)
{
	if (!Menu)
	{
		Menu = CreateWidget<UIronwakeMenuWidget>(this, UIronwakeMenuWidget::StaticClass());
	}
	if (!Menu) return;
	Menu->Mode = Mode;
	Menu->Page = UIronwakeMenuWidget::PageMain;
	Menu->Refresh();
	if (!Menu->IsInViewport()) Menu->AddToViewport(50);
	FInputModeGameAndUI M;
	M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	M.SetHideCursorDuringCapture(false);
	SetInputMode(M);
	SetShowMouseCursor(true);
	if (Mode == 1) SetPause(true);
	Menu->RestoreFocus();
	UE_LOG(LogIronwake, Display, TEXT("IWMENU show mode=%d"), Mode);
}

void AIronwakePlayerController::HideMenu()
{
	if (Menu && Menu->IsInViewport()) Menu->RemoveFromParent();
	if (IsHelpOpen()) ToggleHelp();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
	if (IsPaused()) SetPause(false);
}

bool AIronwakePlayerController::IsMenuOpen() const
{
	return Menu && Menu->IsInViewport();
}

void AIronwakePlayerController::ToggleHelp()
{
	if (IsHelpOpen()) { HelpW->RemoveFromParent(); return; }
	if (!HelpW) HelpW = CreateWidget<UIronwakeHelpWidget>(this, UIronwakeHelpWidget::StaticClass());
	if (!HelpW) return;
	HelpW->AddToViewport(60);
	HelpW->SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(LogIronwake, Display, TEXT("IWP11 help open"));
}

bool AIronwakePlayerController::IsHelpOpen() const { return HelpW && HelpW->IsInViewport(); }

void AIronwakePlayerController::TogglePause()
{
	if (IsMenuOpen()) HideMenu();
	else ShowMenu(1);
}

void AIronwakePlayerController::StartGame(bool bInPeaceful, bool bDeferEnemy)
{
	bPeaceful = bInPeaceful;
	bStarted = true;
	HideMenu();
	if (AIronwakeGameMode* GM = GetWorld()->GetAuthGameMode<AIronwakeGameMode>())
	{
		GM->bSpawnEnemy = !bPeaceful;
		if (!bPeaceful && !bDeferEnemy) GM->SpawnEnemy();
	}
	if (AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(GetPawn())) P->ResetCameraToHorizon(); // P1: over the bow at the horizon
	UE_LOG(LogIronwake, Display, TEXT("IWMENU start voyage peaceful=%d"), bPeaceful);
}

void AIronwakePlayerController::IWYard(int32 Highlight)
{
	ShowMenu(bStarted ? 1 : 0);
	if (Menu) Menu->OnYardPublic(Highlight);
}

void AIronwakePlayerController::ChooseShip(FName Id)
{
	IronwakeFleet::SetSelected(Id);
	// A new command starts at the harbour: reopen the map (keeps the mode, skips the title).
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	const FString Opt = bPeaceful ? TEXT("iwrestart?iwpeaceful=1") : TEXT("iwrestart");
	UE_LOG(LogIronwake, Display, TEXT("IWS2 shipyard: launching %s -> %s ?%s"), *Id.ToString(), *Map, *Opt);
	SetPause(false);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, Opt);
}

void AIronwakePlayerController::IWMenu(int32 Mode) { if (Mode < 0) HideMenu(); else ShowMenu(Mode); }
void AIronwakePlayerController::IWCredits()
{
	ShowMenu(0);
	if (Menu) Menu->OpenPage(UIronwakeMenuWidget::PageCredits);
}
void AIronwakePlayerController::IWStart(int32 Peaceful) { StartGame(Peaceful != 0); }
void AIronwakePlayerController::IWGfx(int32 Level) { IronwakeSettings::SetGraphics(Level); if (Menu) Menu->Refresh(); }
void AIronwakePlayerController::IWMusic(int32 On)
{
	IronwakeAudio::SetVol(IronwakeAudio::Music, On != 0 ? 50 : 0, GetWorld());
	ApplyMusicToShips();
	if (Menu) Menu->Refresh();
}

void AIronwakePlayerController::IWVol(const FString& Key, int32 Pct)
{
	for (int32 c = 0; c < IronwakeAudio::Count; ++c)
		if (Key.Equals(IronwakeAudio::Key(c), ESearchCase::IgnoreCase))
		{
			const bool bWas = IronwakeAudio::MusicOn();
			IronwakeAudio::SetVol(c, Pct, GetWorld());
			if (c == IronwakeAudio::Music && bWas != IronwakeAudio::MusicOn()) ApplyMusicToShips();
			if (Menu) Menu->Refresh();
			return;
		}
	UE_LOG(LogIronwake, Warning, TEXT("IWVol: unknown class '%s' (Master Music Sea Wind Ship Guns Voices UI)"), *Key);
}

void AIronwakePlayerController::IWAudioTest() { IronwakeAudio::RunRouteTest(GetWorld()); }

// ============================================================================ P1: end screens
UButton* UIronwakeEndWidget::MakeButton(UVerticalBox* Box, const FString& Label)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetBackgroundColor(FLinearColor(0.08f, 0.09f, 0.10f, 0.92f));
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 24));
	T->SetColorAndOpacity(FSlateColor(Brass));
	T->SetJustification(ETextJustify::Center);
	B->AddChild(T);
	if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot)) BS->SetPadding(FMargin(40.f, 10.f));
	UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B);
	S->SetPadding(FMargin(0.f, 7.f));
	S->SetHorizontalAlignment(HAlign_Fill);
	return B;
}

TSharedRef<SWidget> UIronwakeEndWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetTree->RootWidget = Canvas;
		const bool bLost = Mode == 0;

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.012f, 0.015f, 0.02f, bLost ? 0.86f : 0.72f), 6.f,
			bLost ? FLinearColor(0.55f, 0.12f, 0.08f, 0.7f) : FLinearColor(0.55f, 0.45f, 0.25f, 0.6f), 2.f));
		UCanvasPanelSlot* PS = Canvas->AddChildToCanvas(Panel);
		// SHIP LOST: centred card. VICTORY: banner in the upper third so the sinking enemy stays visible.
		PS->SetAnchors(FAnchors(0.5f, bLost ? 0.5f : 0.22f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetAutoSize(true);

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Box);
		if (UBorderSlot* BS = Cast<UBorderSlot>(Box->Slot)) BS->SetPadding(FMargin(70.f, 40.f, 70.f, 40.f));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(bLost ? TEXT("SHIP LOST") : TEXT("VICTORY")));
		FSlateFontInfo TF = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 76);
		TF.LetterSpacing = 220;
		Title->SetFont(TF);
		Title->SetColorAndOpacity(FSlateColor(bLost ? FLinearColor(0.85f, 0.25f, 0.18f, 1.f) : Brass));
		Title->SetShadowOffset(FVector2D(3.f, 3.f));
		Title->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Title->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* TS = Box->AddChildToVerticalBox(Title);
		TS->SetHorizontalAlignment(HAlign_Center);

		UTextBlock* Sub = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Sub->SetText(FText::FromString((bLost ? TEXT("She went down with her colours flying.") : TEXT("The enemy ironclad is going down.")) + (StatsLine.IsEmpty() ? FString() : TEXT("\n") + StatsLine)));
		Sub->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 18));
		Sub->SetColorAndOpacity(FSlateColor(Steel));
		Sub->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* SS = Box->AddChildToVerticalBox(Sub);
		SS->SetHorizontalAlignment(HAlign_Center);
		SS->SetPadding(FMargin(0.f, 6.f, 0.f, 28.f));

		if (bLost)
		{
			MakeButton(Box, TEXT("RESTART"))->OnClicked.AddDynamic(this, &UIronwakeEndWidget::OnRestart);
			MakeButton(Box, TEXT("MAIN MENU"))->OnClicked.AddDynamic(this, &UIronwakeEndWidget::OnMainMenu);
		}
		else
		{
			MakeButton(Box, TEXT("CONTINUE"))->OnClicked.AddDynamic(this, &UIronwakeEndWidget::OnContinue);
		}
	}
	return Super::RebuildWidget();
}

void UIronwakeEndWidget::OnRestart() { if (AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer())) PC->RestartVoyage(); }
void UIronwakeEndWidget::OnMainMenu() { if (AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer())) PC->ReturnToMainMenu(); }
void UIronwakeEndWidget::OnContinue() { if (AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer())) PC->HideEndScreen(); }

void AIronwakePlayerController::ShowEndScreen(int32 Mode)
{
	HideEndScreen();
	if (IsMenuOpen()) HideMenu();
	EndWidget = CreateWidget<UIronwakeEndWidget>(this, UIronwakeEndWidget::StaticClass());
	if (!EndWidget) return;
	EndWidget->Mode = Mode;
	if (const AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(GetPawn()))
	{
		EndWidget->StatsLine = FString::Printf(TEXT("Shots %d   Hits %d   Hull %.0f%%"), P->ShellsFiredCount(), P->HitsScoredCount(), 100.f * P->HP / FMath::Max(P->MaxHP, 1.f));
	}
	EndWidget->AddToViewport(60);
	FInputModeGameAndUI M;
	M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	M.SetHideCursorDuringCapture(false);
	SetInputMode(M);
	SetShowMouseCursor(true);
	UE_LOG(LogIronwake, Display, TEXT("IWEND show %s"), Mode == 0 ? TEXT("SHIP LOST") : TEXT("VICTORY"));
}

void AIronwakePlayerController::HideEndScreen()
{
	if (EndWidget && EndWidget->IsInViewport())
	{
		const int32 M = EndWidget->Mode;
		EndWidget->RemoveFromParent();
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		UE_LOG(LogIronwake, Display, TEXT("IWEND hide %s%s"), M == 0 ? TEXT("SHIP LOST") : TEXT("VICTORY"), M == 1 ? TEXT(" (CONTINUE: keep sailing)") : TEXT(""));
	}
	EndWidget = nullptr;
}

bool AIronwakePlayerController::IsEndScreenOpen() const { return EndWidget && EndWidget->IsInViewport(); }

void AIronwakePlayerController::RestartVoyage()
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	const FString Opt = bPeaceful ? TEXT("iwrestart?iwpeaceful=1") : TEXT("iwrestart");
	UE_LOG(LogIronwake, Display, TEXT("IWEND RESTART -> %s ?%s"), *Map, *Opt);
	SetPause(false);
	UGameplayStatics::OpenLevel(this, FName(*Map), true, Opt);
}

void AIronwakePlayerController::ReturnToMainMenu()
{
	const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
	UE_LOG(LogIronwake, Display, TEXT("IWEND MAIN MENU -> %s"), *Map);
	SetPause(false);
	UGameplayStatics::OpenLevel(this, FName(*Map), true);
}

void AIronwakePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (IsLocalController())
	{
		if (Menu && Menu->IsInViewport()) Menu->TickMenu(DeltaTime);
		IronwakeAudio::Tick();
		if (!P18Mode.IsEmpty()) TickP18(FApp::GetDeltaTime());
	}
	if (bPhotoInput)
	{
		auto K = [&](const FKey& Key) { return IsInputKeyDown(Key) ? 1.f : 0.f; };
		PhotoMove = FVector(K(EKeys::W) - K(EKeys::S), K(EKeys::D) - K(EKeys::A), K(EKeys::E) - K(EKeys::Q));
		bPhotoFast = IsInputKeyDown(EKeys::LeftShift);
	}
	UWorld* W = GetWorld();
	if (!W || !IsLocalController() || W->IsPaused()) return;
	if (!P1Mode.IsEmpty()) TickP1Test(DeltaTime);
	// The cinematic tour/clip owns the screen; no end screens there.
	for (TActorIterator<AIronwakeLivingSea> It(W); It; ++It) if (It->bTour) return;
	if (GIsDumpingMovie) return;
	AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(GetPawn());
	if (!P || !bStarted) return;
	const float Now = W->GetTimeSeconds();
	if (P->bSinking && !bLostShown)
	{
		if (PlayerSinkSeen < 0.f) PlayerSinkSeen = Now;
		if (Now - PlayerSinkSeen >= 8.f) { bLostShown = true; ShowEndScreen(0); }
		return;
	}
	if (!bVictoryShown && !P->bSinking)
	{
		AIronwakeShipPawn* E = P->FindEnemy();
		if (E && E->bSinking)
		{
			if (VictoryAt < 0.f) VictoryAt = Now + 3.f; // let the killing blow land first
			if (Now >= VictoryAt) { bVictoryShown = true; ShowEndScreen(1); }
		}
	}
}

void AIronwakePlayerController::IWShot(const FString& Name)
{
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(File, true, false);
	UE_LOG(LogIronwake, Display, TEXT("IWP1 screenshot %s"), *File);
}

void AIronwakePlayerController::IWEnd(int32 Mode) { if (Mode < 0) HideEndScreen(); else ShowEndScreen(Mode); }

// P1 test driver. Saved/iw_p1.txt = idle | sink | fight. Everything goes through the same code paths as the
// player: StartGame = the START VOYAGE button, IWDamage = console, End-screen buttons call the widget handlers.
void AIronwakePlayerController::TickP1Test(float Dt)
{
	P1Time += Dt;
	AIronwakeShipPawn* P = Cast<AIronwakeShipPawn>(GetPawn());
	AIronwakeShipPawn* E = P ? P->FindEnemy() : nullptr;
	auto Log = [&](const TCHAR* What)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWP1 %s t=%.1f myHP=%.0f sinking=%d enemy=%s range=%.0fm enemyShots=%d gunsFree=%d end=%s"), What, P1Time,
			P ? P->HP : -1.f, P ? P->bSinking : -1, E ? *E->AIStateName : TEXT("none"), (P && E) ? FVector::Dist2D(P->GetActorLocation(), E->GetActorLocation()) / 100.f : -1.f,
			E ? E->ShellsFiredCount() : -1, E ? E->bGunsFree : -1, IsEndScreenOpen() ? (EndWidget->Mode == 0 ? TEXT("LOST") : TEXT("VICTORY")) : TEXT("-"));
	};
	auto At = [&](float T) { return P1Step == 0 && P1Time >= T; };
	if (P1Mode == TEXT("idle"))
	{
		// Press START VOYAGE (Captain) and then touch nothing.
		if (P1Step == 0 && P1Time >= 2.f && !bStarted) { if (Menu) Menu->Mode = 0; StartGame(false); }
		if (P1Step == 0 && P1Time >= 2.f) { P1Step = 1; P1Mark = P1Time; Log(TEXT("idle start")); }
		if (P1Step >= 1)
		{
			const float T = P1Time - P1Mark;
			if (P1Step == 1 && T >= 3.f)
			{
				IWShot(TEXT("P1_1_opening_view")); Log(TEXT("opening")); P1Step = 2;
				UE_LOG(LogIronwake, Display, TEXT("IWP1 cam control=%s camRot=%s camLoc=%s pawnYaw=%.1f viewTarget=%s aimRange=%.0fm"), *GetControlRotation().ToString(),
					PlayerCameraManager ? *PlayerCameraManager->GetCameraRotation().ToString() : TEXT("-"), PlayerCameraManager ? *PlayerCameraManager->GetCameraLocation().ToString() : TEXT("-"),
					P ? P->GetActorRotation().Yaw : 0.f, *GetNameSafe(GetViewTarget()), P ? P->AimRange / 100.f : -1.f);
				if (P) UE_LOG(LogIronwake, Display, TEXT("IWP1 arm usePCR=%d absRot=%d len=%.0f armLenProp=%.0f armRot=%s offset=%s socket=%s"), P->CameraArm->bUsePawnControlRotation, P->CameraArm->IsUsingAbsoluteRotation(),
					P->CameraArm->TargetArmLength, P->ArmLength, *P->CameraArm->GetComponentRotation().ToString(), *P->CameraArm->TargetOffset.ToString(), *P->CameraArm->SocketOffset.ToString());
			}
			if (P1Step == 2 && FMath::Fmod(T, 10.f) < Dt) Log(TEXT("idle"));
			if (P1Step == 2 && T >= 90.f) { IWShot(TEXT("P1_2_idle_90s")); Log(TEXT("idle 90s")); P1Step = 3; }
			if (P1Step == 3 && T >= 92.f) { Log(TEXT("idle done")); P1Mode.Empty(); }
		}
	}
	else if (P1Mode == TEXT("title"))
	{
		// Default title (no test start): screenshot the menu so the default MODE can be checked.
		if (P1Step == 0 && P1Time >= 2.f) { IWShot(TEXT("P1_0_title_default")); UE_LOG(LogIronwake, Display, TEXT("IWP1 title default peaceful=%d started=%d"), bPeaceful, bStarted); P1Step = 1; }
		if (P1Step == 1 && P1Time >= 3.f) { StartGame(bPeaceful); Log(TEXT("title START pressed (default mode)")); P1Step = 2; }
		if (P1Step == 2 && P1Time >= 6.f) { Log(TEXT("title done")); P1Mode.Empty(); }
	}
	else if (P1Mode == TEXT("sink") || P1Mode == TEXT("sinkmenu"))
	{
		if (P1Step == 0 && P1Time >= 2.f) { if (!bStarted) StartGame(false); P1Step = 1; P1Mark = P1Time; Log(TEXT("sink start")); }
		const float T = P1Time - P1Mark;
		if (P1Step == 1 && T >= 6.f && P) { P->IWDamage(1000.f, 1); Log(TEXT("forced sink")); P1Step = 2; P1Mark = P1Time; }
		if (P1Step == 2 && T >= 5.f) { IWShot(TEXT("P1_3_player_sinking")); Log(TEXT("sinking 5s")); P1Step = 3; }
		if (P1Step == 3 && IsEndScreenOpen()) { P1Step = 4; P1Mark = P1Time; Log(TEXT("lost screen up")); }
		if (P1Step == 4 && P1Time - P1Mark >= 1.5f) { IWShot(TEXT("P1_4_ship_lost")); P1Step = 5; }
		if (P1Step == 5 && P1Time - P1Mark >= 4.f)
		{
			// Hand the test to the reloaded level, then press RESTART (or MAIN MENU).
			const bool bMenu = P1Mode == TEXT("sinkmenu");
			FFileHelper::SaveStringToFile(bMenu ? TEXT("title") : TEXT("afterrestart"), *(FPaths::ProjectSavedDir() / TEXT("iw_p1.txt")));
			Log(bMenu ? TEXT("pressing MAIN MENU") : TEXT("pressing RESTART"));
			P1Mode.Empty();
			if (EndWidget) { if (bMenu) EndWidget->OnMainMenu(); else EndWidget->OnRestart(); }
		}
	}
	else if (P1Mode == TEXT("afterrestart"))
	{
		if (P1Step == 0 && P1Time >= 4.f) { Log(TEXT("after restart")); IWShot(TEXT("P1_5_after_restart")); P1Step = 1; }
		if (P1Step == 1 && P1Time >= 6.f) { Log(TEXT("restart done")); P1Mode.Empty(); }
	}
	else if (P1Mode == TEXT("fight"))
	{
		if (P1Step == 0 && P1Time >= 2.f) { if (!bStarted) StartGame(false); P1Step = 1; P1Mark = P1Time; Log(TEXT("fight start")); }
		if (P1Step == 1 && P1Time - P1Mark >= 3.f && P)
		{
			// Combat tuning is parked (PASS 2 priority change): finish the fight with the IWDamage console command
			// on the enemy, which runs the same ApplyShellHit -> sinking path as a real killing hit.
			P->IWTelegraph(4);
			P->IWDamage(1000.f, 0);
			P1Step = 2; P1Mark = P1Time; Log(TEXT("enemy killed (IWDamage)"));
		}
		if (P1Step == 2 && FMath::Fmod(P1Time, 15.f) < Dt) Log(TEXT("fight"));
		if (P1Step == 2 && IsEndScreenOpen() && EndWidget->Mode == 1) { P1Step = 3; P1Mark = P1Time; Log(TEXT("victory up")); }
		if (P1Step == 2 && IsEndScreenOpen() && EndWidget->Mode == 0) { Log(TEXT("fight LOST")); P1Mode.Empty(); }
		if (P1Step == 2 && P1Time > 600.f) { Log(TEXT("fight timeout")); P1Mode.Empty(); }
		if (P1Step == 3 && P1Time - P1Mark >= 1.5f) { IWShot(TEXT("P1_6_victory")); P1Step = 4; }
		if (P1Step == 4 && P1Time - P1Mark >= 4.f) { if (EndWidget) EndWidget->OnContinue(); Log(TEXT("pressed CONTINUE")); P1Step = 5; P1Mark = P1Time; }
		if (P1Step == 5 && P1Time - P1Mark >= 4.f) { IWShot(TEXT("P1_7_after_continue")); Log(TEXT("fight done")); P1Mode.Empty(); }
	}
	(void)At;
}

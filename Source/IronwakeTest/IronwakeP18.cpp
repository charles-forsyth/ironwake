// P18 test director (a member of AIronwakePlayerController): -iwp18=sizes|nav|persistset|persistread [-iwp18tag=<label>] [-iwp18quit].
// Silent offscreen -game runs (-nosound). Log lines "IWP18 ...", shots in Saved/shots/P18_<tag>_<name>.png.
#include "IronwakePlayerController.h"
#include "IronwakeAudio.h"
#include "IronwakeSettings.h"
#include "IronwakeTest.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AIronwakePlayerController::TickP18(float RealDt)
{
	P18T += RealDt;
	const float Since = P18T - P18Mark;
	const bool bQuit = FParse::Param(FCommandLine::Get(), TEXT("iwp18quit"));
	FString Tag = TEXT("run");
	FParse::Value(FCommandLine::Get(), TEXT("iwp18tag="), Tag);
	auto Next = [&]() { ++P18Step; P18Mark = P18T; };
	auto Done = [&](const TCHAR* What)
	{
		UE_LOG(LogIronwake, Display, TEXT("IWP18 DONE %s %s"), *P18Mode, What);
		P18Mode.Empty();
		if (bQuit) UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
	};
	auto Shot = [&](const FString& Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / FString::Printf(TEXT("P18_%s_%s.png"), *Tag, *Name)), true, false);
	};
	auto Key = [&](const FKey& K)
	{
		FSlateApplication& SA = FSlateApplication::Get();
		const uint32 Code = K == EKeys::Enter ? 13 : 0;
		FKeyEvent Dn(K, FModifierKeysState(), 0, false, Code, 0);
		SA.ProcessKeyDownEvent(Dn);
		FKeyEvent Up(K, FModifierKeysState(), 0, false, Code, 0);
		SA.ProcessKeyUpEvent(Up);
	};
	if (P18Step == 0)
	{
		if (P18T < 4.f) return;
		UE_LOG(LogIronwake, Display, TEXT("IWP18 start mode=%s tag=%s menu=%d"), *P18Mode, *Tag, IsMenuOpen());
		FString Res;
		if (FParse::Value(FCommandLine::Get(), TEXT("iwp18res="), Res)) { ConsoleCommand(FString::Printf(TEXT("r.SetRes %sw"), *Res)); UE_LOG(LogIronwake, Display, TEXT("IWP18 r.SetRes %sw"), *Res); }
		Next(); return;
	}

	// ------------------------------------------------------------------ sizes: every page, layout check + shot (one launch per resolution)
	if (P18Mode == TEXT("sizes"))
	{
		struct FStep { int32 Page, Tab, Mode; const TCHAR* Name; };
		static const FStep Steps[] = {
			{ UIronwakeMenuWidget::PageMain, 0, 0, TEXT("title_main") },
			{ UIronwakeMenuWidget::PageSettings, 0, 0, TEXT("settings_gameplay") },
			{ UIronwakeMenuWidget::PageSettings, 1, 0, TEXT("settings_graphics") },
			{ UIronwakeMenuWidget::PageSettings, 2, 0, TEXT("settings_view") },
			{ UIronwakeMenuWidget::PageSettings, 3, 0, TEXT("settings_audio") },
			{ UIronwakeMenuWidget::PageCredits, 0, 0, TEXT("credits") },
			{ UIronwakeMenuWidget::PageYard, 0, 0, TEXT("shipyard") },
			{ UIronwakeMenuWidget::PageMain, 0, 1, TEXT("pause_main") },
			{ UIronwakeMenuWidget::PageSettings, 3, 1, TEXT("pause_settings_audio") },
		};
		const int32 N = UE_ARRAY_COUNT(Steps);
		const int32 I = (P18Step - 1) / 3, Phase = (P18Step - 1) % 3;
		if (I >= N) { Done(TEXT("all pages")); return; }
		const FStep& St = Steps[I];
		if (Phase == 0)
		{
			if (St.Mode == 1 && !(Menu && Menu->Mode == 1)) { if (!bStarted) StartGame(true); ShowMenu(1); }
			if (!Menu) return;
			Menu->OpenPage(St.Page, St.Tab);
			Next(); return;
		}
		if (Phase == 1 && Since > 1.4f)
		{
			if (Menu) Menu->CheckLayout(FString::Printf(TEXT("%s/%s"), *Tag, St.Name));
			Shot(St.Name);
			Next(); return;
		}
		if (Phase == 2 && Since > 1.2f) { Next(); return; }
		return;
	}

	// ------------------------------------------------------------------ nav: keyboard-only walk through the menu, log focus after every key
	if (P18Mode == TEXT("nav"))
	{
		struct FNav { const TCHAR* Desc; FKey K; };
		static const FNav Seq[] = {
			{ TEXT("(initial focus)"), FKey() },
			{ TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down },
			{ TEXT("Enter -> SETTINGS"), EKeys::Enter },
			{ TEXT("Right"), EKeys::Right }, { TEXT("Right"), EKeys::Right }, { TEXT("Right"), EKeys::Right },
			{ TEXT("Enter -> AUDIO tab"), EKeys::Enter },
			{ TEXT("Down -> first slider"), EKeys::Down },
			{ TEXT("Right (+5)"), EKeys::Right }, { TEXT("Right (+5)"), EKeys::Right }, { TEXT("Left (-5)"), EKeys::Left },
			{ TEXT("Down -> MUSIC slider"), EKeys::Down },
			{ TEXT("Right x4 (music 0 -> 20)"), EKeys::Right }, { TEXT("Right"), EKeys::Right }, { TEXT("Right"), EKeys::Right }, { TEXT("Right"), EKeys::Right },
			{ TEXT("Enter (TEST on slider)"), EKeys::Enter },
			{ TEXT("Down x6 -> RESET AUDIO"), EKeys::Down }, { TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down }, { TEXT("Down"), EKeys::Down },
			{ TEXT("Down"), EKeys::Down },
			{ TEXT("Enter (RESET AUDIO)"), EKeys::Enter },
			{ TEXT("Escape -> back one page"), EKeys::Escape },
			{ TEXT("(end)"), FKey() },
		};
		const int32 N = UE_ARRAY_COUNT(Seq);
		const int32 I = P18Step - 1;
		if (I >= N) { Done(TEXT("nav")); return; }
		if (Since < 0.9f) return;
		if (Menu)
		{
			if (Seq[I].K.IsValid()) Key(Seq[I].K);
		}
		P18Mark = P18T - 0.0f;
		++P18Step;
		// log on the next tick via a short timer so the key has been processed
		FTimerHandle H; const FString D = Seq[I].Desc; const int32 Idx = I;
		GetWorldTimerManager().SetTimer(H, [this, D, Idx]()
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP18 NAV %02d %-30s page=%d tab=%d focus=%s vols=Master %d Music %d Sea %d Wind %d"), Idx, *D, Menu ? Menu->Page : -1, Menu ? Menu->Tab : -1,
				Menu ? *Menu->FocusedName() : TEXT("-"), IronwakeAudio::GetVol(0), IronwakeAudio::GetVol(1), IronwakeAudio::GetVol(2), IronwakeAudio::GetVol(3));
		}, 0.35f, false);
		return;
	}

	// ------------------------------------------------------------------ drag: move sliders as the mouse would, check live apply
	if (P18Mode == TEXT("drag"))
	{
		if (P18Step == 1) { if (Menu) Menu->OpenPage(UIronwakeMenuWidget::PageSettings, 3); Next(); return; }
		if (P18Step == 2 && Since > 1.5f)
		{
			if (Menu) { Menu->SetSliderPct(IronwakeAudio::Wind, 35); Menu->SetSliderPct(IronwakeAudio::Sea, 62); Menu->SetSliderPct(IronwakeAudio::Music, 15); }
			Next(); return;
		}
		if (P18Step == 3 && Since > 0.6f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP18 DRAG set Wind 35 -> %d (snapped to 5 %%), Sea 62 -> %d, Music 15 -> %d (music on=%d)"), IronwakeAudio::GetVol(IronwakeAudio::Wind), IronwakeAudio::GetVol(IronwakeAudio::Sea), IronwakeAudio::GetVol(IronwakeAudio::Music), IronwakeAudio::MusicOn());
			Shot(TEXT("audio_dragged"));
			Next(); return;
		}
		if (P18Step == 4 && Since > 1.2f) { IronwakeAudio::ResetAll(GetWorld()); if (Menu) Menu->Refresh(); Done(TEXT("drag")); }
		return;
	}

	// ------------------------------------------------------------------ persistence: set, (relaunch), read back
	if (P18Mode == TEXT("persistset"))
	{
		static const int32 Vals[IronwakeAudio::Count] = { 35, 20, 65, 10, 90, 45, 0, 55 };
		for (int32 c = 0; c < IronwakeAudio::Count; ++c) IronwakeAudio::SetVol(c, Vals[c], GetWorld());
		FString L; for (int32 c = 0; c < IronwakeAudio::Count; ++c) L += FString::Printf(TEXT("%s=%d "), IronwakeAudio::Key(c), IronwakeAudio::GetVol(c));
		UE_LOG(LogIronwake, Display, TEXT("IWP18 PERSIST set: %s"), *L.TrimEnd());
		Done(TEXT("persistset")); return;
	}
	if (P18Mode == TEXT("persistread"))
	{
		static const int32 Vals[IronwakeAudio::Count] = { 35, 20, 65, 10, 90, 45, 0, 55 };
		if (P18Step == 1)
		{
			int32 Bad = 0; FString L;
			for (int32 c = 0; c < IronwakeAudio::Count; ++c) { const int32 V = IronwakeAudio::GetVol(c); L += FString::Printf(TEXT("%s=%d "), IronwakeAudio::Key(c), V); if (V != Vals[c]) ++Bad; }
			UE_LOG(LogIronwake, Display, TEXT("IWP18 PERSIST read after relaunch: %s -> %s"), *L.TrimEnd(), Bad == 0 ? TEXT("PASS (all 8 restored)") : TEXT("FAIL"));
			if (Menu) Menu->OpenPage(UIronwakeMenuWidget::PageSettings, 3);
			Next(); return;
		}
		if (Since > 1.5f) { Shot(TEXT("audio_restored")); Next(); return; }
		if (P18Step == 3 && Since > 1.2f) { IronwakeAudio::ResetAll(GetWorld()); Done(TEXT("persistread")); }
		return;
	}
	P18Mode.Empty();
}

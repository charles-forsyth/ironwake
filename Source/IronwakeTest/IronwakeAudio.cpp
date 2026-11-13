#include "IronwakeAudio.h"
#include "IronwakeTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	const TCHAR* GKey[IronwakeAudio::Count] = { TEXT("Master"), TEXT("Music"), TEXT("Sea"), TEXT("Wind"), TEXT("Ship"), TEXT("Guns"), TEXT("Voices"), TEXT("UI") };
	const TCHAR* GLabel[IronwakeAudio::Count] = { TEXT("MASTER"), TEXT("MUSIC"), TEXT("SEA & WATER"), TEXT("WIND & WEATHER"), TEXT("SHIP & HULL"), TEXT("GUNS & EFFECTS"), TEXT("VOICES"), TEXT("UI") };
	const int32 GDef[IronwakeAudio::Count] = { 80, 0, 100, 100, 100, 100, 100, 100 };
	const TCHAR* Section = TEXT("Ironwake");

	int32 GOverride[IronwakeAudio::Count] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	bool bOverridesParsed = false;
	int32 GCache[IronwakeAudio::Count] = { -1, -1, -1, -1, -1, -1, -1, -1 };

	TStrongObjectPtr<USoundMix> GMix;
	TWeakObjectPtr<UWorld> GPushedWorld;
	TStrongObjectPtr<USoundClass> GClass[IronwakeAudio::Count];

	struct FTestSnd { TWeakObjectPtr<UAudioComponent> A; double StopAt; };
	TArray<FTestSnd> GTests;

	void ParseOverrides()
	{
		if (bOverridesParsed) return;
		bOverridesParsed = true;
		const FString Cmd = FCommandLine::Get();
		int32 From = 0;
		const FString Tok = TEXT("iwvol=");
		while (true)
		{
			const int32 At = Cmd.Find(Tok, ESearchCase::IgnoreCase, ESearchDir::FromStart, From);
			if (At == INDEX_NONE) break;
			From = At + Tok.Len();
			FString Rest = Cmd.Mid(From);
			int32 Sp; if (Rest.FindChar(TEXT(' '), Sp)) Rest = Rest.Left(Sp);
			FString K, V;
			if (!Rest.Split(TEXT(":"), &K, &V)) continue;
			for (int32 c = 0; c < IronwakeAudio::Count; ++c)
				if (K.Equals(GKey[c], ESearchCase::IgnoreCase)) { GOverride[c] = FMath::Clamp(FCString::Atoi(*V), 0, 100); UE_LOG(LogIronwake, Display, TEXT("IWAUDIO override %s=%d (command line)"), GKey[c], GOverride[c]); }
		}
	}

	USoundClass* LoadClass(int32 C)
	{
		if (!GClass[C].IsValid())
		{
			const FString N = FString::Printf(TEXT("SC_%s"), GKey[C]);
			USoundClass* S = LoadObject<USoundClass>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Audio/Classes/%s.%s"), *N, *N));
			if (!S) { UE_LOG(LogIronwake, Warning, TEXT("IWAUDIO missing sound class %s"), *N); return nullptr; }
			GClass[C].Reset(S);
		}
		return GClass[C].Get();
	}

	// Largest per-component volume the code gives each sound (docs/spec/10_audio.md), for the route report.
	struct FPeak { const TCHAR* N; float V; const TCHAR* Who; };
	const FPeak GPeaks[] = {
		{ TEXT("battle_night_storm"), 0.6f, TEXT("battle music (only when Music > 0)") }, { TEXT("main_theme"), 0.55f, TEXT("calm music (only when Music > 0)") },
		{ TEXT("bow_wash_loop"), 0.95f, TEXT("bow wash, speed driven") }, { TEXT("hull_wake_loop"), 0.8f, TEXT("stern wake, speed driven") },
		{ TEXT("hull_hiss_proc"), 0.7f, TEXT("hull hiss, speed driven") }, { TEXT("slow_lap_loop"), 0.7f, TEXT("slow lapping, speed driven") },
		{ TEXT("bow_rush_proc"), 0.9f, TEXT("bow rush, speed driven") }, { TEXT("harbour_lap_proc"), 1.6f, TEXT("harbour lap (dock bed)") },
		{ TEXT("surf_proc"), 1.4f, TEXT("island surf") },
		{ TEXT("wind_sea_proc"), 0.55f, TEXT("wind layer 0") }, { TEXT("gale_proc"), 0.9f, TEXT("wind layer 1") }, { TEXT("storm_howling_wind"), 0.8f, TEXT("wind layer 2") },
		{ TEXT("rain_proc"), 0.9f, TEXT("rain layer") }, { TEXT("thunder_near"), 1.6f, TEXT("thunder") }, { TEXT("thunder_far"), 1.6f, TEXT("thunder") },
		{ TEXT("creak_storm_strain"), 1.4f, TEXT("storm creak, collision impact, grounding") }, { TEXT("creak_wooden_hull"), 1.1f, TEXT("hull creak, scrape") },
		{ TEXT("foghorn_sfx"), 1.6f, TEXT("foghorn") }, { TEXT("broadside_sfx"), 1.4f, TEXT("gunfire, shell impact") },
		{ TEXT("captain_contact"), 1.f, TEXT("voice") }, { TEXT("captain_load"), 1.f, TEXT("voice") }, { TEXT("officer_correct"), 1.f, TEXT("voice") },
		{ TEXT("officer_damage"), 1.f, TEXT("voice") }, { TEXT("officer_splash"), 1.f, TEXT("voice") },
		{ TEXT("ui_click"), 1.f, TEXT("menu click, test sound") },
	};
	float PeakOf(const FString& N, const TCHAR*& Who)
	{
		for (const FPeak& P : GPeaks) if (N == P.N) { Who = P.Who; return P.V; }
		Who = TEXT("(no component table entry)");
		return 1.f;
	}
}

namespace IronwakeAudio
{
	const TCHAR* Key(int32 C) { return GKey[FMath::Clamp(C, 0, Count - 1)]; }
	const TCHAR* Label(int32 C) { return GLabel[FMath::Clamp(C, 0, Count - 1)]; }
	int32 DefaultPct(int32 C) { return GDef[FMath::Clamp(C, 0, Count - 1)]; }

	int32 GetVol(int32 C)
	{
		C = FMath::Clamp(C, 0, Count - 1);
		ParseOverrides();
		if (GOverride[C] >= 0) return GOverride[C];
		if (GCache[C] < 0)
		{
			int32 V = GDef[C];
			bool bHave = false;
			if (GConfig)
			{
				bHave = GConfig->GetInt(Section, *FString::Printf(TEXT("Vol_%s"), GKey[C]), V, GGameUserSettingsIni);
				// Migration: the old MUSIC ON/OFF button (bool "Music") becomes 50 % when it was on.
				if (!bHave && C == Music) { bool bOld = false; if (GConfig->GetBool(Section, TEXT("Music"), bOld, GGameUserSettingsIni) && bOld) V = 50; }
			}
			GCache[C] = FMath::Clamp(V, 0, 100);
		}
		return GCache[C];
	}

	bool MusicOn() { return GetVol(Music) > 0; }

	void SetVol(int32 C, int32 Pct, UWorld* W)
	{
		C = FMath::Clamp(C, 0, Count - 1);
		Pct = FMath::Clamp(Pct, 0, 100);
		ParseOverrides();
		GOverride[C] = -1; // a player change beats a command-line override
		GCache[C] = Pct;
		if (GConfig)
		{
			GConfig->SetInt(Section, *FString::Printf(TEXT("Vol_%s"), GKey[C]), Pct, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		Apply(W);
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO set %s=%d"), GKey[C], Pct);
	}

	void ResetAll(UWorld* W)
	{
		for (int32 c = 0; c < Count; ++c)
		{
			GOverride[c] = -1; GCache[c] = GDef[c];
			if (GConfig) GConfig->SetInt(Section, *FString::Printf(TEXT("Vol_%s"), GKey[c]), GDef[c], GGameUserSettingsIni);
		}
		if (GConfig) GConfig->Flush(false, GGameUserSettingsIni);
		Apply(W);
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO reset to defaults"));
	}

	void Apply(UWorld* W)
	{
		if (!W) return;
		if (!GMix.IsValid()) GMix.Reset(NewObject<USoundMix>(GetTransientPackage(), TEXT("IWVolumeMix")));
		if (GPushedWorld.Get() != W) { UGameplayStatics::PushSoundMixModifier(W, GMix.Get()); GPushedWorld = W; }
		FString Line;
		for (int32 c = 0; c < Count; ++c)
		{
			USoundClass* SC = LoadClass(c);
			const float V = GetVol(c) / 100.f;
			// Master's override applies to children (they inherit class volume x parent); the others are set on their own class.
			if (SC) UGameplayStatics::SetSoundMixClassOverride(W, GMix.Get(), SC, V, 1.f, 0.05f, c == Master);
			Line += FString::Printf(TEXT("%s=%d "), GKey[c], GetVol(c));
		}
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO applied %s"), *Line.TrimEnd());
	}

	int32 ClassOf(USoundBase* S)
	{
		if (!S) return -1;
		USoundClass* SC = S->SoundClassObject; // assigned class only (the project default class is Master, so unclassed sounds fall under Master)
		if (!SC) return -1;
		for (int32 c = 0; c < Count; ++c) if (SC->GetName() == FString::Printf(TEXT("SC_%s"), GKey[c])) return c;
		return -1;
	}

	void Init(UWorld* W)
	{
		ParseOverrides();
		Apply(W);
		int32 N = 0, Un = 0;
		FAssetRegistryModule& Mod = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (Mod.Get().IsLoadingAssets()) Mod.Get().WaitForCompletion();
		TArray<FAssetData> Assets;
		Mod.Get().GetAssetsByPath(FName(TEXT("/Game/Ironwake/Audio")), Assets, false);
		for (const FAssetData& A : Assets)
		{
			USoundBase* S = Cast<USoundBase>(A.GetAsset());
			if (!S) continue;
			++N;
			if (ClassOf(S) < 0) { ++Un; UE_LOG(LogIronwake, Warning, TEXT("IWAUDIO unclassed %s"), *S->GetName()); }
		}
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO init: %d sounds scanned, %d unclassed (those play under MASTER only)"), N, Un);
	}

	void Click(UWorld* W)
	{
		if (!W) return;
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, TEXT("/Game/Ironwake/Audio/ui_click.ui_click")))
			UGameplayStatics::PlaySound2D(W, S, 1.f);
	}

	void TestPlay(UWorld* W, int32 C)
	{
		if (!W) return;
		static const TCHAR* Rep[Count] = { TEXT("foghorn_sfx"), TEXT("main_theme"), TEXT("slow_lap_loop"), TEXT("wind_sea_proc"), TEXT("creak_wooden_hull"), TEXT("broadside_sfx"), TEXT("officer_splash"), TEXT("ui_click") };
		C = FMath::Clamp(C, 0, Count - 1);
		USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Audio/%s.%s"), Rep[C], Rep[C]));
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO test %s sound=%s slider=%d%% master=%d%% class=%d"), GKey[C], Rep[C], GetVol(C), GetVol(Master), ClassOf(S));
		if (!S) return;
		UAudioComponent* A = UGameplayStatics::CreateSound2D(W, S, 1.f, 1.f, 0.f, nullptr, false, true);
		if (!A) return; // -nosound: no audio device
		if (C == Master) A->SoundClassOverride = LoadClass(Master); // MASTER test: the horn at master level only (no other class slider)
		A->bIsUISound = true; // plays while the pause menu is up
		A->Play();
		GTests.Add({ A, FPlatformTime::Seconds() + 3.0 });
	}

	void Tick()
	{
		if (GTests.IsEmpty()) return;
		const double Now = FPlatformTime::Seconds();
		for (int32 i = GTests.Num() - 1; i >= 0; --i)
		{
			UAudioComponent* A = GTests[i].A.Get();
			if (!A) { GTests.RemoveAt(i); continue; }
			if (Now >= GTests[i].StopAt) { A->FadeOut(0.25f, 0.f); GTests.RemoveAt(i); }
		}
	}

	int32 RunRouteTest(UWorld* W)
	{
		FString Out;
		Out += TEXT("IRONWAKE P18 audio routing report (-iwaudiotest; no sound is played, -nosound runs)\n");
		Out += TEXT("effective = class volume x master volume x the component's own peak volume (from docs/spec/10_audio.md); sliders map 0..100 % linearly to 0..1\n");
		Out += FString::Printf(TEXT("defaults: Master %d, Music %d (off), the other five %d\n\n"), GDef[Master], GDef[Music], GDef[Sea]);

		Out += TEXT("Class tree (read from the cooked SoundClass assets):\n");
		USoundClass* Root = LoadClass(Master);
		int32 BadTree = 0;
		for (int32 c = 0; c < Count; ++c)
		{
			USoundClass* SC = LoadClass(c);
			const bool bOk = SC && (c == Master ? SC->ParentClass == nullptr : SC->ParentClass == Root);
			if (!bOk) ++BadTree;
			Out += FString::Printf(TEXT("  SC_%-7s parent=%-9s volume=%.2f isUI=%d %s\n"), GKey[c], SC && SC->ParentClass ? *SC->ParentClass->GetName() : TEXT("(root)"), SC ? SC->Properties.Volume : -1.f, SC ? (int32)SC->Properties.bIsUISound : -1, bOk ? TEXT("ok") : TEXT("BAD"));
		}
		Out += FString::Printf(TEXT("tree problems: %d\n\n"), BadTree);

		FAssetRegistryModule& Mod = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (Mod.Get().IsLoadingAssets()) Mod.Get().WaitForCompletion();
		TArray<FAssetData> Assets;
		Mod.Get().GetAssetsByPath(FName(TEXT("/Game/Ironwake/Audio")), Assets, false);
		Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });

		struct FRow { FString N; int32 C; float Peak; const TCHAR* Who; };
		TArray<FRow> Rows;
		int32 Unclassed = 0;
		for (const FAssetData& A : Assets)
		{
			USoundBase* S = Cast<USoundBase>(A.GetAsset());
			if (!S) continue;
			FRow R; R.N = S->GetName(); R.C = ClassOf(S); R.Peak = PeakOf(R.N, R.Who);
			if (R.C < 0) { ++Unclassed; UE_LOG(LogIronwake, Warning, TEXT("IWAUDIO unclassed %s"), *R.N); }
			Rows.Add(R);
		}
		const int32 Levels[3] = { 0, 50, 100 };
		// Effective multiplier = product over the sound's class chain (its class up to the root) of (class asset volume x slider).
		auto Eff = [&](USoundBase* S, const float Slider[Count]) -> float
		{
			float M = 1.f;
			for (USoundClass* SC = S ? S->SoundClassObject.Get() : nullptr; SC; SC = SC->ParentClass.Get())
			{
				float Sl = 1.f;
				for (int32 c = 0; c < Count; ++c) if (SC->GetName() == FString::Printf(TEXT("SC_%s"), GKey[c])) Sl = Slider[c];
				M *= SC->Properties.Volume * Sl;
			}
			return M; // unclassed sound: no chain here, and it plays under MASTER only
		};
		auto EffUnclassed = [&](const float Slider[Count]) { return Slider[Master]; };
		TArray<USoundBase*> Snds;
		for (const FAssetData& A : Assets) if (USoundBase* S = Cast<USoundBase>(A.GetAsset())) Snds.Add(S);
		Out += FString::Printf(TEXT("A. all eight sliders at the level shown (walks the real class chain: class x master x the component's peak):\n%-22s %-15s %5s | %8s %8s %8s | role\n"), TEXT("sound"), TEXT("class"), TEXT("peak"), TEXT("0%"), TEXT("50%"), TEXT("100%"));
		for (int32 r = 0; r < Rows.Num(); ++r)
		{
			const FRow& R = Rows[r];
			FString Cls = R.C >= 0 ? FString(GLabel[R.C]) : TEXT("UNCLASSED (master only)");
			float E[3];
			for (int32 i = 0; i < 3; ++i) { float Sl[Count]; for (int32 c = 0; c < Count; ++c) Sl[c] = Levels[i] / 100.f; E[i] = (R.C >= 0 ? Eff(Snds[r], Sl) : EffUnclassed(Sl)) * R.Peak; }
			Out += FString::Printf(TEXT("%-22s %-15s %5.2f | %8.3f %8.3f %8.3f | %s\n"), *R.N, *Cls, R.Peak, E[0], E[1], E[2], R.Who);
		}
		Out += FString::Printf(TEXT("\nB. master 100 %%, other classes 100 %%, only the sound's own class slider at the level shown:\n%-22s %-15s | %8s %8s %8s\n"), TEXT("sound"), TEXT("class"), TEXT("0%"), TEXT("50%"), TEXT("100%"));
		for (int32 r = 0; r < Rows.Num(); ++r)
		{
			const FRow& R = Rows[r];
			FString Cls = R.C >= 0 ? FString(GLabel[R.C]) : TEXT("UNCLASSED");
			float E[3];
			for (int32 i = 0; i < 3; ++i) { float Sl[Count]; for (int32 c = 0; c < Count; ++c) Sl[c] = 1.f; if (R.C >= 0) Sl[R.C] = Levels[i] / 100.f; E[i] = (R.C >= 0 ? Eff(Snds[r], Sl) : 1.f) * R.Peak; }
			Out += FString::Printf(TEXT("%-22s %-15s | %8.3f %8.3f %8.3f\n"), *R.N, *Cls, E[0], E[1], E[2]);
		}
		Out += TEXT("\nC. master slider 0 %, every other slider 100 %: ");
		{
			float Sl[Count]; for (int32 c = 0; c < Count; ++c) Sl[c] = 1.f; Sl[Master] = 0.f;
			float MaxE = 0.f; for (int32 r = 0; r < Rows.Num(); ++r) MaxE = FMath::Max(MaxE, (Rows[r].C >= 0 ? Eff(Snds[r], Sl) : EffUnclassed(Sl)) * Rows[r].Peak);
			Out += FString::Printf(TEXT("largest effective multiplier over all %d sounds = %.3f (must be 0)\n"), Rows.Num(), MaxE);
		}
		Out += TEXT("D. music: default slider 0 % = off (code starts music only when MUSIC > 0). At the saved/default sliders now: ");
		for (int32 c = 0; c < Count; ++c) Out += FString::Printf(TEXT("%s=%d%% "), GKey[c], GetVol(c));
		Out += TEXT("\nNote: this report is computed from the cooked assets' class assignments and parent chain; with -nosound there is no audio device to query a live mix.");
		Out += FString::Printf(TEXT("\n\nsounds scanned: %d   unclassed: %d   class tree problems: %d\n"), Rows.Num(), Unclassed, BadTree);
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("build/logs/P18_audio_routes.txt"));
		FFileHelper::SaveStringToFile(Out, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		UE_LOG(LogIronwake, Display, TEXT("IWAUDIO routes: %d sounds, %d unclassed, %d tree problems -> %s"), Rows.Num(), Unclassed, BadTree, *Path);
		return Unclassed + BadTree;
	}
}

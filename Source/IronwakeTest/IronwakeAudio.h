#pragma once
#include "CoreMinimal.h"

class UWorld;
class USoundBase;

/**
 * P18: per-sound-type volumes. Eight USoundClass assets (/Game/Ironwake/Audio/Classes/SC_*: Master is the root,
 * the other seven are its children) plus a runtime USoundMix whose class overrides carry the slider values, so every
 * sound of a class is scaled engine-wide (class x master x the component's own volume). Values are saved as
 * [Ironwake] Vol_<Key> (percent, 5 % steps in the UI). -iwvol=<key>:<0-100> overrides for a run (not saved).
 */
namespace IronwakeAudio
{
	enum EClass : int32 { Master = 0, Music, Sea, Wind, Ship, Guns, Voices, UI, Count };

	IRONWAKETEST_API const TCHAR* Key(int32 C);      // "Master", "Music", "Sea", ... (config + command line)
	IRONWAKETEST_API const TCHAR* Label(int32 C);    // "MASTER", "SEA & WATER", ...
	IRONWAKETEST_API int32 DefaultPct(int32 C);      // Master 80, Music 0 (off), the rest 100

	IRONWAKETEST_API int32 GetVol(int32 C);          // percent 0..100 (command-line override wins)
	IRONWAKETEST_API void SetVol(int32 C, int32 Pct, UWorld* W); // clamps to 0..100, saves, applies live
	IRONWAKETEST_API void ResetAll(UWorld* W);
	IRONWAKETEST_API bool MusicOn();                 // Music slider > 0

	/** Push the mix and set one override per class (call at game start and after every change). */
	IRONWAKETEST_API void Apply(UWorld* W);
	/** Game start: apply, log volumes, scan /Game/Ironwake/Audio and log `IWAUDIO unclassed <name>` for any sound with no class. */
	IRONWAKETEST_API void Init(UWorld* W);
	IRONWAKETEST_API void Click(UWorld* W);          // UI click
	/** TEST button: ~3 s of a representative sound of the class, played at the slider level (only when pressed). */
	IRONWAKETEST_API void TestPlay(UWorld* W, int32 C);
	IRONWAKETEST_API void Tick();                    // stops test sounds (real time; the menu ticks while paused)
	/** -iwaudiotest: writes build/logs/P18_audio_routes.txt; returns the number of unclassed sounds. */
	IRONWAKETEST_API int32 RunRouteTest(UWorld* W);
	IRONWAKETEST_API int32 ClassOf(USoundBase* S);   // EClass of a sound's assigned SoundClass, or -1
}

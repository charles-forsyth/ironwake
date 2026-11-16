#include "IronwakeSettings.h"
#include "IronwakeAudio.h"
#include "IronwakeTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	int32 GCamMotionOverride = -1;
	const TCHAR* Section = TEXT("Ironwake");
	int32 CachedGraphics = -1;

	void SetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			V->Set(Value, ECVF_SetByCode);
		}
		else
		{
			UE_LOG(LogIronwake, Warning, TEXT("IWGFX unknown cvar %s"), Name);
		}
	}

	bool ReadBool(const TCHAR* Key, bool bDefault)
	{
		bool b = bDefault;
		if (GConfig) GConfig->GetBool(Section, Key, b, GGameUserSettingsIni);
		return b;
	}

	void WriteBool(const TCHAR* Key, bool b)
	{
		if (!GConfig) return;
		GConfig->SetBool(Section, Key, b, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

namespace IronwakeSettings
{
	const TCHAR* GraphicsName(int32 Level)
	{
		return Level <= 0 ? TEXT("LOW") : Level == 1 ? TEXT("MEDIUM") : TEXT("HIGH");
	}

	int32 GetGraphics()
	{
		if (CachedGraphics < 0)
		{
			int32 L = 1;
			if (GConfig) GConfig->GetInt(Section, TEXT("Graphics"), L, GGameUserSettingsIni);
			int32 Cmd = -1; // -iwgfx=N overrides (benchmarks)
			if (FParse::Value(FCommandLine::Get(), TEXT("iwgfx="), Cmd)) L = Cmd;
			CachedGraphics = FMath::Clamp(L, 0, 2);
		}
		return CachedGraphics;
	}

	void SetGraphics(int32 Level)
	{
		CachedGraphics = FMath::Clamp(Level, 0, 2);
		if (GConfig)
		{
			GConfig->SetInt(Section, TEXT("Graphics"), CachedGraphics, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ApplyGraphics();
	}

	void ApplyGraphics()
	{
		const int32 L = GetGraphics();
		// Every level: 60 fps cap with vsync; dynamic resolution (TSR upscale) holds the GPU frame budget.
		// Measured (build/logs/H1.md): the volumetric cloud deck was 40 of 57 ms GPU at 1600x900; the water ~4 ms.
		SetCVar(TEXT("t.MaxFPS"), TEXT("60"));
		SetCVar(TEXT("r.VSync"), TEXT("1"));
		SetCVar(TEXT("r.DynamicRes.OperationMode"), TEXT("2"));
		SetCVar(TEXT("r.DynamicRes.FrameTimeBudget"), L == 2 ? TEXT("33.3") : TEXT("15.0"));
		SetCVar(TEXT("r.DynamicRes.MinScreenPercentage"), L == 0 ? TEXT("55") : TEXT("70"));
		SetCVar(TEXT("r.DynamicRes.MaxScreenPercentage"), L == 0 ? TEXT("75") : L == 1 ? TEXT("92") : TEXT("100"));
		SetCVar(TEXT("r.VolumetricCloud.SampleMinCount"), L == 2 ? TEXT("2") : TEXT("2"));
		SetCVar(TEXT("r.VolumetricCloud.ViewRaySampleMaxCount"), L == 0 ? TEXT("12") : L == 1 ? TEXT("16") : TEXT("64"));
		SetCVar(TEXT("r.VolumetricCloud.ReflectionRaySampleMaxCount"), L == 2 ? TEXT("16") : TEXT("8"));
		SetCVar(TEXT("r.VolumetricCloud.SkyAO"), L == 2 ? TEXT("1") : TEXT("0"));
		SetCVar(TEXT("r.VolumetricRenderTarget.Mode"), L == 2 ? TEXT("0") : TEXT("2"));
		// P34: keep full sub-40cm water mesh tessellation across the 36 km zone (MaxDimensionInTiles=2048 prevents UE's 256-tile coarse auto-downscale!)
		SetCVar(TEXT("r.Water.WaterMesh.MaxDimensionInTiles"), TEXT("2048"));
		SetCVar(TEXT("r.Water.WaterMesh.TessFactorBias"), L == 0 ? TEXT("0") : TEXT("1"));
		// LOW drops Lumen (GI -> none + sky light, reflections -> SSR) and virtual shadow maps.
		SetCVar(TEXT("r.DynamicGlobalIlluminationMethod"), L == 0 ? TEXT("0") : TEXT("1"));
		SetCVar(TEXT("r.ReflectionMethod"), L == 0 ? TEXT("2") : TEXT("1"));
		SetCVar(TEXT("r.Shadow.Virtual.Enable"), L == 0 ? TEXT("0") : TEXT("1"));
		UE_LOG(LogIronwake, Display, TEXT("IWGFX graphics=%s applied"), GraphicsName(L));
	}

	// P18: music is the MUSIC volume slider (0 = off, the default); the old on/off setting maps to 0 / 50 %.
	bool GetMusic() { return IronwakeAudio::MusicOn(); }
	void SetMusic(bool bOn) { IronwakeAudio::SetVol(IronwakeAudio::Music, bOn ? 50 : 0, nullptr); UE_LOG(LogIronwake, Display, TEXT("IWSET music=%d"), bOn); }

	bool GetRudderAutoCentre() { return ReadBool(TEXT("RudderAutoCentre"), false); }
	void SetRudderAutoCentre(bool bOn) { WriteBool(TEXT("RudderAutoCentre"), bOn); UE_LOG(LogIronwake, Display, TEXT("IWSET rudderAuto=%d"), bOn); }

	const TCHAR* CameraMotionName(int32 Level) { return Level <= 0 ? TEXT("OFF") : Level == 1 ? TEXT("LOW") : TEXT("MEDIUM"); }
	int32 GetCameraMotion()
	{
		if (GCamMotionOverride >= 0) return GCamMotionOverride;
		static int32 Cached = -1;
		if (Cached < 0)
		{
			int32 L = 1;
			if (GConfig) GConfig->GetInt(Section, TEXT("CameraMotion"), L, GGameUserSettingsIni);
			int32 Cmd = -1; if (FParse::Value(FCommandLine::Get(), TEXT("iwcammotion="), Cmd)) L = Cmd;
			Cached = FMath::Clamp(L, 0, 2);
		}
		return Cached;
	}
	void SetCameraMotion(int32 Level)
	{
		Level = FMath::Clamp(Level, 0, 2);
		if (GConfig) { GConfig->SetInt(Section, TEXT("CameraMotion"), Level, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
		GCamMotionOverride = Level;
		UE_LOG(LogIronwake, Display, TEXT("IWSET cameraMotion=%s"), CameraMotionName(Level));
	}
}

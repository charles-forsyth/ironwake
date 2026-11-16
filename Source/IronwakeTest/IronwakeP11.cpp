// P11: living world. Continuous clock (sun, moon, stars), independent weather that drifts, sea state that follows the weather,
// the controls help panel and the -iwp11 test director. Deterministic code only.
#include "IronwakeP11.h"
#include "IronwakeVoyage.h"
#include "IronwakeStorm.h"
#include "IronwakeP8.h"
#include "IronwakeHydro.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BoxComponent.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/PrimitiveComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
#include "IronwakeP8.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/DateTime.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

void IronwakeSetBasinCalm(bool bOn);
namespace
{
	const TCHAR* GStateNames[] = { TEXT("Clear"), TEXT("Fair"), TEXT("Overcast"), TEXT("Mist"), TEXT("Rain"), TEXT("Squall") };

	// Weather states: cloud cover, sun/sky/fog multipliers, rain, open-sea state (0 CALM .. 3 GALE; never the parked STORM look).
	AIronwakeVoyage::FLiveWx StateWx(int32 S)
	{
		AIronwakeVoyage::FLiveWx W;
		switch (S)
		{
		case 0: W.Cover = 0.04f; W.SunMul = 1.25f; W.SkyMul = 1.05f; W.FogMul = 0.45f; W.Rain = 0.f; W.Sea = 0.55f; break; // clear: crisp sunlit maritime horizon
		case 1: W.Cover = 0.25f; W.SunMul = 1.15f; W.SkyMul = 1.00f; W.FogMul = 0.65f; W.Rain = 0.f; W.Sea = 0.90f; break; // fair: slight to moderate
		case 2: W.Cover = 0.78f; W.SunMul = 0.60f; W.SkyMul = 0.95f; W.FogMul = 0.85f; W.Rain = 0.f; W.Sea = 1.65f; break; // overcast: moderate to rough
		case 3: W.Cover = 0.65f; W.SunMul = 0.60f; W.SkyMul = 0.90f; W.FogMul = 3.20f; W.Rain = 0.f; W.Sea = 0.30f; break; // mist: calm, misty
		case 4: W.Cover = 0.92f; W.SunMul = 0.35f; W.SkyMul = 0.82f; W.FogMul = 2.00f; W.Rain = 0.7f; W.Sea = 2.35f; break; // rain: rough
		default: W.Cover = 0.97f; W.SunMul = 0.22f; W.SkyMul = 0.72f; W.FogMul = 2.10f; W.Rain = 1.f; W.Sea = 3.0f; break;   // squall: very rough (gale)
		}
		return W;
	}
	template <typename T> T L(const T& A, const T& B, float t) { return A + (B - A) * t; }
}

FString AIronwakeVoyage::ClockString() const
{
	const int32 Hh = (int32)ClockHour, Mm = (int32)((ClockHour - Hh) * 60.f);
	const float H = ClockHour;
	const TCHAR* Part = H < 4.5f ? TEXT("small hours") : H < 7.f ? TEXT("dawn") : H < 11.f ? TEXT("morning") : H < 13.5f ? TEXT("noon") : H < 15.5f ? TEXT("afternoon")
		: H < 18.f ? TEXT("dusk") : H < 21.f ? TEXT("evening") : TEXT("night");
	static const TCHAR* Spd[] = { TEXT(""), TEXT(" x10"), TEXT(" (paused)") };
	return FString::Printf(TEXT("%02d:%02d %s%s"), Hh, Mm, Part, Spd[ClockSpeedIdx]);
}

FString AIronwakeVoyage::LiveWeatherName() const
{
	const float S = AIronwakeStorm::Get(GetWorld()) ? AIronwakeStorm::Get(GetWorld())->GetLocalState() : 1.f;
	const TCHAR* Sea = S < 0.3f ? TEXT("calm") : S < 0.8f ? TEXT("slight") : S < 1.5f ? TEXT("moderate") : S < 2.4f ? TEXT("rough") : TEXT("very rough");
	return FString::Printf(TEXT("%s   sea %s"), GStateNames[FMath::Clamp(LiveState, 0, 5)], Sea);
}

void AIronwakeVoyage::StartLiving()
{
	bLiving = true;
	WxRng.Initialize((int32)(FPlatformTime::Cycles() & 0x7fffffff));
	float V = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("iwdaymin="), V)) DayLenMin = FMath::Max(V, 0.5f);
	if (FParse::Value(FCommandLine::Get(), TEXT("iwhour="), V)) ClockHour = FMath::Fmod(V + 24.f, 24.f);
	if (FParse::Value(FCommandLine::Get(), TEXT("iwskyhour="), V)) { ClockHour = FMath::Fmod(V + 24.f, 24.f); ClockSpeedIdx = 2; }
	if (FParse::Value(FCommandLine::Get(), TEXT("iwwx="), V)) LiveState = FMath::Clamp((int32)V, 0, 5);
	else LiveState = 0;
	if (FParse::Param(FCommandLine::Get(), TEXT("iwnodrift"))) bWxDrift = false;
	LiveCur = LiveFrom = LiveTo = StateWx(LiveState);
	LiveBlendT = 1.f; WxHoldT = 0.f; WxNextIn = WxRng.FRandRange(10.f, 30.f);
	if (NightSky) { NightSky->bHold = true; NightSky->Hour = ClockHour; }
	UE_LOG(LogIronwake, Display, TEXT("IWP11 living world on: %s, day = %.0f real min, weather %s, drift=%d"), *ClockString(), DayLenMin, GStateNames[LiveState], bWxDrift);
}

void AIronwakeVoyage::CycleClockSpeed()
{
	ClockSpeedIdx = (ClockSpeedIdx + 1) % 3;
	ShowToast(FString::Printf(TEXT("Clock %s"), ClockSpeedIdx == 0 ? TEXT("x1") : ClockSpeedIdx == 1 ? TEXT("x10") : TEXT("paused")), 2.5f);
}

void AIronwakeVoyage::JumpHours(float Hrs)
{
	const float Old = ClockHour;
	ClockHour = FMath::Fmod(ClockHour + Hrs, 24.f);
	if (ClockHour < Old && NightSky) NightSky->DayIdx = (NightSky->DayIdx + 1) % 365;
	ShowToast(FString::Printf(TEXT("Time: %s"), *ClockString()), 2.5f);
}

void AIronwakeVoyage::ForceWeatherState(int32 S, float BlendSeconds)
{
	S = ((S % 6) + 6) % 6;
	LiveState = S;
	LiveFrom = LiveCur; LiveTo = StateWx(S);
	LiveBlendT = 0.f; LiveBlendDur = FMath::Max(BlendSeconds, 0.05f);
	WxHoldT = 0.f; WxNextIn = WxRng.FRandRange(10.f, 30.f);
	UE_LOG(LogIronwake, Display, TEXT("IWP11 weather -> %s over %.0f s"), GStateNames[S], LiveBlendDur);
}

void AIronwakeVoyage::CycleWeatherLive()
{
	bHazeOff = false;
	ForceWeatherState(LiveState + 1, 6.f);
	ShowToast(FString::Printf(TEXT("Weather: %s"), GStateNames[LiveState]), 3.f);
}

void AIronwakeVoyage::ToggleClearHaze()
{
	bHazeOff = !bHazeOff;
	if (bHazeOff)
	{
		bWxDrift = false;
		ForceWeatherState(0, 0.5f);
		ShowToast(TEXT("Clear Air ON (haze off, weather held)"), 3.5f);
	}
	else
	{
		ShowToast(TEXT("Natural maritime atmosphere restored"), 3.f);
	}
}

void AIronwakeVoyage::TickLiving(float Dt)
{
	static const float Spd[3] = { 1.f, 10.f, 0.f };
	const float Sp = Spd[ClockSpeedIdx];
	const float HoursPerRealSec = 24.f / (DayLenMin * 60.f);
	// ---- clock
	const float Old = ClockHour;
	ClockHour += Dt * HoursPerRealSec * Sp;
	if (ClockHour >= 24.f) { ClockHour -= 24.f; if (NightSky) NightSky->DayIdx = (NightSky->DayIdx + 1) % 365; }
	(void)Old;
	if (NightSky) { NightSky->bHold = true; NightSky->Hour = ClockHour; }
	// ---- weather drift: a decision every 10-30 game minutes, one step along the chain (or stay), blends of 2-4 real minutes
	WxHoldT += Dt * HoursPerRealSec * 60.f * Sp;
	if (bWxDrift && WxHoldT >= WxNextIn)
	{
		WxHoldT = 0.f; WxNextIn = WxRng.FRandRange(10.f, 30.f);
		static const int32 Nb[6][3] = { { 1, 1, 1 }, { 0, 2, 3 }, { 1, 4, 3 }, { 1, 2, 2 }, { 2, 5, 1 }, { 4, 2, 2 } };
		if (WxRng.FRand() > 0.5f)
		{
			int32 N = Nb[LiveState][WxRng.RandRange(0, 2)];
			if (N == 5 && WxRng.FRand() > 0.4f) N = 4;   // squalls are rarer
			if (N != LiveState)
			{
				LiveState = N; LiveFrom = LiveCur; LiveTo = StateWx(N); LiveBlendT = 0.f; LiveBlendDur = WxRng.FRandRange(120.f, 240.f);
				UE_LOG(LogIronwake, Display, TEXT("IWP11 weather drifts -> %s over %.0f s (clock %s)"), GStateNames[N], LiveBlendDur, *ClockString());
			}
		}
	}
	if (LiveBlendT < 1.f) LiveBlendT = FMath::Min(1.f, LiveBlendT + Dt * FMath::Max(Sp, 1.f) / LiveBlendDur);
	const float A = FMath::SmoothStep(0.f, 1.f, LiveBlendT);
	LiveCur.Cover = L(LiveFrom.Cover, LiveTo.Cover, A); LiveCur.SunMul = L(LiveFrom.SunMul, LiveTo.SunMul, A); LiveCur.SkyMul = L(LiveFrom.SkyMul, LiveTo.SkyMul, A);
	LiveCur.FogMul = L(LiveFrom.FogMul, LiveTo.FogMul, A); LiveCur.Rain = L(LiveFrom.Rain, LiveTo.Rain, A); LiveCur.Sea = L(LiveFrom.Sea, LiveTo.Sea, A);

	// ---- sun position (same observer as the star dome: lat 50 N, +X north, +Y east)
	const double D2R = PI / 180.0, Phi = 50.0 * D2R;
	const int32 Doy = (NightSky ? NightSky->DayIdx : 313) + 1;
	const double LamS = 360.0 * ((Doy - 80.0) / 365.25) * D2R, Eps = 23.44 * D2R;
	const double Dec = FMath::Asin(FMath::Sin(LamS) * FMath::Sin(Eps));
	const double Ha = (ClockHour - 12.0) * 15.0 * D2R;
	const FVector SunDir(FMath::Cos(Phi) * FMath::Sin(Dec) - FMath::Sin(Phi) * FMath::Cos(Dec) * FMath::Cos(Ha), -FMath::Cos(Dec) * FMath::Sin(Ha),
		FMath::Sin(Phi) * FMath::Sin(Dec) + FMath::Cos(Phi) * FMath::Cos(Dec) * FMath::Cos(Ha));
	const float Alt = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp((double)SunDir.Z, -1.0, 1.0)));
	const FRotator LightRot = (-SunDir).Rotation();
	LiveSunYaw = LightRot.Yaw;

	// ---- lighting: night / dusk / day looks weighted by sun altitude, then the weather scales them
	const float Wn = 1.f - FMath::SmoothStep(-9.f, -1.f, Alt), Wd = FMath::SmoothStep(2.f, 10.f, Alt), Wk = FMath::Max(0.f, 1.f - Wn - Wd);
	const float Clear = FMath::Clamp(1.f - (LiveCur.Cover - 0.2f) / 0.3f, 0.f, 1.f);
	auto Mix = [&](auto Fn) { const float Tot = Wn + Wd + Wk; return (Wn * Fn(0) + Wd * Fn(1) + Wk * Fn(2)) / FMath::Max(Tot, 1e-3f); };
	const FWx NightBase = [&]() { FWx N = Presets[2]; const FWx& C = Presets[5]; N.SunI = L(N.SunI, C.SunI, Clear); N.SkyI = L(N.SkyI, C.SkyI, Clear); N.Fog = L(N.Fog, C.Fog, Clear); N.Bias = L(N.Bias, C.Bias, Clear); N.FogC = L(N.FogC, C.FogC, Clear); return N; }();
	const FWx* Src[3] = { &NightBase, &Presets[0], &Presets[1] };
	Cur.SunPitch = LightRot.Pitch;
	Cur.SunI = Mix([&](int i) { return Src[i]->SunI; }) * LiveCur.SunMul;
	Cur.SkyI = Mix([&](int i) { return Src[i]->SkyI; }) * LiveCur.SkyMul;
	Cur.Fog = Mix([&](int i) { return Src[i]->Fog; }) * LiveCur.FogMul;
	Cur.Bias = Mix([&](int i) { return Src[i]->Bias; });
	Cur.Night = Mix([&](int i) { return Src[i]->Night; });
	const FLinearColor SC = (Src[0]->SunC * Wn + Src[1]->SunC * Wd + Src[2]->SunC * Wk) * (1.f / FMath::Max(Wn + Wd + Wk, 1e-3f));
	const FLinearColor FC = (Src[0]->FogC * Wn + Src[1]->FogC * Wd + Src[2]->FogC * Wk) * (1.f / FMath::Max(Wn + Wd + Wk, 1e-3f));
	Cur.SunC = SC; Cur.FogC = FC;
	Cur.Clear = Clear;
	// ---- P14: clear/fair noon lift. Low winter sun (~22 deg) + grey grade read dark and flat: stronger sky fill and sun, more exposure.
	{
		static float DS = 2.2f, DU = 1.2f, DB = 0.5f; static bool bD = false;
		if (!bD) { bD = true; FString V; if (FParse::Value(FCommandLine::Get(), TEXT("iwday="), V, false)) { TArray<FString> Pp; V.ParseIntoArray(Pp, TEXT(",")); if (Pp.Num() > 0) DS = FCString::Atof(*Pp[0]); if (Pp.Num() > 1) DU = FCString::Atof(*Pp[1]); if (Pp.Num() > 2) DB = FCString::Atof(*Pp[2]); } }
		const float Dl = Wd * FMath::Clamp(1.f - (LiveCur.Cover - 0.15f) / 0.6f, 0.f, 1.f);
		Cur.Fog *= L(1.f, 0.55f, Dl); Cur.SkyI *= L(1.f, DS, Dl); Cur.SunI *= L(1.f, DU, Dl); Cur.Bias += DB * Dl;
	}
	// ---- P13: dawn/dusk ramps, clear-day blue, moonlit night. All from sun/moon altitude, deterministic.
	{
		const float Tw = FMath::SmoothStep(-13.f, -3.f, Alt) * (1.f - FMath::SmoothStep(3.f, 11.f, Alt));   // twilight weight: 0 at night and in full day
		const bool bDawn = ClockHour < 12.f;
		// twilight is not night: release the locked night exposure and fade the star dome with sun altitude (stars gone by ~-7 deg, full below ~-16)
		Cur.Night = FMath::Min(Cur.Night, 1.f - 0.55f * FMath::SmoothStep(-16.f, -7.f, Alt));
		const float Ww = FMath::SmoothStep(-6.f, 0.f, Alt) * (1.f - FMath::SmoothStep(1.f, 9.f, Alt));     // warm horizon weight inside the twilight
		const float DaySunI = Presets[0].SunI;
		// the atmosphere does the reddening: the sun light itself stays near-white (orange sun x orange scatter was the muddy brown)
		const FLinearColor TwSun = bDawn ? FLinearColor(1.f, 0.86f, 0.80f) : FLinearColor(1.f, 0.80f, 0.60f);
		const float TwI = DaySunI * 0.85f * FMath::SmoothStep(-13.f, 5.f, Alt) * LiveCur.SunMul;
		Cur.SunI = L(Cur.SunI, FMath::Max(TwI, Cur.SunI * Wd), Tw);
		Cur.SunC = L(Cur.SunC, TwSun, Tw);
		// horizon / fog colour: blue hour -> pink (dawn) or orange (dusk) near the horizon -> day grey-blue
		const FLinearColor FogBlue(0.030f, 0.045f, 0.115f), FogWarm = bDawn ? FLinearColor(0.34f, 0.19f, 0.24f) : FLinearColor(0.40f, 0.20f, 0.075f);
		FLinearColor FogTw = L(FogBlue, FogWarm, Ww);
		Cur.FogC = L(Cur.FogC, FogTw, Tw * (0.45f + 0.55f * Clear));
		Cur.Fog *= L(1.f, bDawn ? 1.3f : 0.85f, Tw);
		// clear day: blue-grey horizon haze instead of storm grey
		Cur.FogC = L(Cur.FogC, FLinearColor(0.34f, 0.46f, 0.68f), FMath::Clamp(FMath::Clamp(1.f - (LiveCur.Cover - 0.15f) / 0.5f, 0.f, 1.f) * Wd * 0.85f * (1.f - Tw), 0.f, 1.f));
		// clear night: faint blue horizon glow (airglow + starlight) so the horizon line reads; overcast night a dim grey-blue
		const FLinearColor NightHz = L(FLinearColor(0.028f, 0.032f, 0.042f), FLinearColor(0.034f, 0.050f, 0.100f), Clear);
		Cur.FogC = L(Cur.FogC, NightHz, Wn * (1.f - Tw));
		Cur.SkyI = FMath::Max(Cur.SkyI, Presets[0].SkyI * 0.9f * Wn * LiveCur.SkyMul);   // ambient floor at night
		// sky tint on the atmosphere (sky + aerial perspective only, objects keep the sun colour)
		static TWeakObjectPtr<ASkyAtmosphere> SkyAtmo;
		if (!SkyAtmo.IsValid()) { TActorIterator<ASkyAtmosphere> It(GetWorld()); if (It) SkyAtmo = *It; }
		if (ASkyAtmosphere* SA = SkyAtmo.Get())
			if (USkyAtmosphereComponent* AC = SA->GetComponent())
			{
				static FLinearColor TD(0.62f, 0.78f, 1.55f), TK(0.80f, 0.80f, 1.25f); static float Oz = 3.f; static bool bP = false;
				if (!bP) { bP = true; FString V; if (FParse::Value(FCommandLine::Get(), TEXT("iwtintdawn="), V, false)) { TArray<FString> Pp; V.ParseIntoArray(Pp, TEXT(",")); if (Pp.Num() == 3) TD = FLinearColor(FCString::Atof(*Pp[0]), FCString::Atof(*Pp[1]), FCString::Atof(*Pp[2])); }
					if (FParse::Value(FCommandLine::Get(), TEXT("iwtintdusk="), V, false)) { TArray<FString> Pp; V.ParseIntoArray(Pp, TEXT(",")); if (Pp.Num() == 3) TK = FLinearColor(FCString::Atof(*Pp[0]), FCString::Atof(*Pp[1]), FCString::Atof(*Pp[2])); }
					FParse::Value(FCommandLine::Get(), TEXT("iwozone="), Oz); }
				const FLinearColor TwTint = bDawn ? TD : TK;
				AC->SetSkyAndAerialPerspectiveLuminanceFactor(L(FLinearColor::White, TwTint, Tw * (0.35f + 0.65f * Clear)));
				AC->SetOtherAbsorptionScale(L(0.001881f, 0.001881f * Oz, Tw));
			}
		// twilight grade: the map's grade (sat 0.6, toe 0.55, contrast 1.12) crushes the blue zenith to black; relax it while the sky is in twilight
		{
			static float K[5] = { 1.05f, 1.0f, 0.30f, 5.0f, 0.8f }; static bool bK = false;
			if (!bK) { bK = true; FString V; if (FParse::Value(FCommandLine::Get(), TEXT("iwtw="), V, false)) { TArray<FString> Pp; V.ParseIntoArray(Pp, TEXT(",")); for (int32 i = 0; i < 5 && i < Pp.Num(); ++i) K[i] = FCString::Atof(*Pp[i]); } }
			static TWeakObjectPtr<APostProcessVolume> Pv;
			if (!Pv.IsValid()) { for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It) { if (It->bUnbound) { Pv = *It; break; } } }
			if (APostProcessVolume* PV = Pv.Get())
			{
				static FVector4 S0, G0; static float C0 = 1.12f, T0 = 0.55f; static bool bI = false;
				if (!bI) { bI = true; S0 = PV->Settings.ColorSaturation; C0 = PV->Settings.ColorContrast.X; T0 = PV->Settings.FilmToe; }
				const float Tk = Tw * (0.4f + 0.6f * Clear);
				// clear/fair day: lift the storm-grey grade so the sky & water read natural colour across all daytime states
				const float Dk = Wd * FMath::Clamp(1.f - (LiveCur.Cover - 0.35f) / 0.55f, 0.25f, 1.f) * (1.f - Tw);
				PV->Settings.ColorSaturation = FVector4(L(L((float)S0.X, K[0], Tk), 1.06f, Dk), L(L((float)S0.Y, K[0], Tk), 1.06f, Dk), L(L((float)S0.Z, K[0], Tk), 1.12f, Dk), 1.f);
				PV->Settings.ColorContrast = FVector4(L(C0, K[1], Tk), L(C0, K[1], Tk), L(C0, K[1], Tk), 1.f);
				PV->Settings.FilmToe = L(T0, K[2], Tk);
				Cur.Bias += K[4] * Tk + 0.75f * Dk;
			}
		}
		const float TwLumK = [] { float V = 5.0f; FString F; if (FParse::Value(FCommandLine::Get(), TEXT("iwtw="), F, false)) { TArray<FString> Pp; F.ParseIntoArray(Pp, TEXT(",")); if (Pp.Num() > 3) V = FCString::Atof(*Pp[3]); } return V; }();
		if (ASkyAtmosphere* SA2 = SkyAtmo.Get()) if (USkyAtmosphereComponent* AC2 = SA2->GetComponent())
		{
			const FLinearColor Cf = AC2->SkyAndAerialPerspectiveLuminanceFactor; AC2->SetSkyAndAerialPerspectiveLuminanceFactor(L(Cf, Cf * TwLumK, Tw));
		}
		// moonlight: bluish directional light from the moon's real direction and phase, only when the sun is down
		if (!MoonLight)
		{
			MoonLight = NewObject<UDirectionalLightComponent>(this, TEXT("MoonLight"));
			MoonLight->SetupAttachment(RootComponent);
			MoonLight->SetMobility(EComponentMobility::Movable);
			MoonLight->SetIntensity(0.f); MoonLight->SetLightColor(FLinearColor(0.55f, 0.68f, 1.f));
			MoonLight->SetAtmosphereSunLight(false);
			MoonLight->SetCastShadows(!FParse::Param(FCommandLine::Get(), TEXT("iwmoonnoshadow")));
			MoonLight->RegisterComponent();
		}
		float MoonMax = 4.f; FParse::Value(FCommandLine::Get(), TEXT("iwmoon="), MoonMax);
		if (NightSky && MoonLight)
		{
			const FVector MD = NightSky->MoonDirNEU.GetSafeNormal();
			const float MAlt = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp((float)MD.Z, -1.f, 1.f)));
			const float Illum = 0.5f * (1.f - FMath::Cos(FMath::DegreesToRadians(NightSky->MoonPhaseDeg)));
			const float Up = FMath::SmoothStep(-2.f, 9.f, MAlt);
			LiveMoonI = MoonMax * (0.10f + 0.90f * Illum) * Up * Wn * (0.22f + 0.78f * Clear);
			MoonLight->SetWorldRotation((-MD).Rotation());
			{ float Wg = 1.f; FParse::Value(FCommandLine::Get(), TEXT("iwwater="), Wg); IronwakeSea::SetNightWater(GetWorld(), FMath::Clamp(Up * Wn * (0.15f + 0.85f * Clear) * (0.3f + 0.7f * Illum) * Wg, 0.f, 1.f)); }
			MoonLight->SetIntensity(LiveMoonI);
			static float NextMoonLog = 0.f;
			if (GetWorld()->GetTimeSeconds() >= NextMoonLog) { NextMoonLog = GetWorld()->GetTimeSeconds() + 15.f; UE_LOG(LogIronwake, Display, TEXT("IWP13 moon alt=%.1f illum=%.2f I=%.3f tw=%.2f sunI=%.2f skyI=%.2f lamp=%.2f"), MAlt, Illum, LiveMoonI, Tw, Cur.SunI, Cur.SkyI, 1.f - FMath::SmoothStep(-5.f, 3.f, Alt)); }
		}
	}
	LiveLamp = 1.f - FMath::SmoothStep(-5.f, 3.f, Alt);   // (UE SmoothStep needs A<B)   // lamps come on as the sun sets below ~3 deg, full by -5
	if (LiveCur.Cover > 0.8f) LiveLamp = FMath::Max(LiveLamp, (1.f - FMath::SmoothStep(-2.f, 8.f, Alt)) * 0.8f);
	Cur.Sea = SeaOverride >= 0.f ? SeaOverride : LiveCur.Sea;
	IronwakeSetBasinCalm(Cur.Sea > 0.75f);   // P14
	// ---- feed the sea / cloud / rain director
	if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld()))
	{
		const float NightK = FMath::Clamp((Cur.Night - 0.5f) / 0.5f, 0.f, 1.f);
		St->CloudCover = LiveCur.Cover * (1.f - 0.8f * NightK * Clear);   // clear moonlit nights keep the P10 star-dome look
		St->RainAmount = LiveCur.Rain;
		St->ClearSky = 0.f;
		St->SetOpenImmediate(Cur.Sea);
	}
	static float NextLog = 0.f;
	if (GetWorld()->GetTimeSeconds() >= NextLog)
	{
		NextLog = GetWorld()->GetTimeSeconds() + 15.f;
		UE_LOG(LogIronwake, Display, TEXT("IWP11 tick %s alt=%.1f wx=%s cover=%.2f rain=%.2f sea=%.2f night=%.2f clear=%.2f sunI=%.2f fog=%.4f"), *ClockString(), Alt, GStateNames[LiveState], LiveCur.Cover, LiveCur.Rain, Cur.Sea, Cur.Night, Clear, Cur.SunI, Cur.Fog);
	}
}

// ---------------------------------------------------------------------------- help panel
TSharedRef<SWidget> UIronwakeHelpWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const FLinearColor Brass(0.86f, 0.74f, 0.47f, 1.f), Steel(0.74f, 0.78f, 0.82f, 1.f);
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		WidgetTree->RootWidget = Canvas;
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.012f, 0.015f, 0.02f, 0.90f), 8.f, FLinearColor(0.55f, 0.45f, 0.25f, 0.7f), 2.f));
		Panel->SetPadding(FMargin(40.f, 28.f));
		UCanvasPanelSlot* PS = Canvas->AddChildToCanvas(Panel);
		PS->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PS->SetAlignment(FVector2D(0.5f, 0.5f));
		PS->SetPosition(FVector2D(0.f, 0.f));
		PS->SetAutoSize(true);
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Root);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("CONTROLS")));
		FSlateFontInfo TF = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 30); TF.LetterSpacing = 160;
		T->SetFont(TF); T->SetColorAndOpacity(FSlateColor(Brass));
		Root->AddChildToVerticalBox(T)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
		UHorizontalBox* Cols = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Root->AddChildToVerticalBox(Cols);
		auto Section = [&](UVerticalBox* Col, const TCHAR* Head, const TCHAR* Body)
		{
			UTextBlock* H = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			H->SetText(FText::FromString(Head)); H->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18)); H->SetColorAndOpacity(FSlateColor(Brass));
			Col->AddChildToVerticalBox(H)->SetPadding(FMargin(0.f, 10.f, 0.f, 4.f));
			UTextBlock* B = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			B->SetText(FText::FromString(Body)); B->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 13)); B->SetColorAndOpacity(FSlateColor(Steel));
			Col->AddChildToVerticalBox(B);
		};
		auto Column = [&](bool bLast) { UVerticalBox* C = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass()); UHorizontalBoxSlot* S = Cols->AddChildToHorizontalBox(C); S->SetPadding(FMargin(0.f, 0.f, bLast ? 0.f : 44.f, 0.f)); S->SetVerticalAlignment(VAlign_Top); return C; };
		UVerticalBox* C1 = Column(false); UVerticalBox* C2 = Column(false); UVerticalBox* C3 = Column(true);
		Section(C1, TEXT("HELM"),
			TEXT("W / S      telegraph, one step per press\n")
			TEXT("           (sailing ship: sail plan)\n")
			TEXT("A / D      hold to turn the wheel\n")
			TEXT("Q / E      rudder midships\n")
			TEXT("J          helmsman steers to next point\n")
			TEXT("X          drop / weigh anchor\n")
			TEXT("G          fast sail x4 (peaceful)"));
		Section(C1, TEXT("ENGINE & SAILS"),
			TEXT("B          auxiliary steam (sailing ship)\n")
			TEXT("           can't sail within 40 deg of wind\n")
			TEXT("R          damage control (fires)\n")
			TEXT("Left click fire when guns READY and ON"));
		Section(C2, TEXT("NAVIGATION & PORTS"),
			TEXT("M          chart: left-click add waypoint,\n")
			TEXT("           right-click remove, Enter start,\n")
			TEXT("           Backspace clear, L loop route\n")
			TEXT("P          autopilot: off / hold course /\n")
			TEXT("           waypoints / tour\n")
			TEXT("           (hold: A / D nudge course 5 deg)\n")
			TEXT("N          next point of interest\n")
			TEXT("E / Enter  make fast in port (< 2.5 kn)\n")
			TEXT("           or salvage / survey landmark\n")
			TEXT("O          captain's logbook & manifest"));
		Section(C2, TEXT("CAMERA"),
			TEXT("Mouse      orbit camera / aim the guns\n")
			TEXT("Wheel      camera zoom\n")
			TEXT("Shift      binoculars (wheel = zoom)\n")
			TEXT("           Shift or right-click exits\n")
			TEXT("C / Mid    hold: free look\n")
			TEXT("Menu       CAMERA MOTION OFF / LOW / MEDIUM\n")
			TEXT("           (binoculars, photo always steady)\n")
			TEXT("F          photo mode: WASD / QE fly,\n")
			TEXT("           wheel FOV, T slow-mo, Shift fast"));
		Section(C3, TEXT("VIEW & WORLD"),
			TEXT("T          clock speed: x1 / x10 / pause\n")
			TEXT("Shift+T    jump forward 3 hours (also ])\n")
			TEXT("L          next weather\n")
			TEXT("V          clear air (toggle haze off)\n")
			TEXT("K          weather drift on / off\n")
			TEXT("H / F1     this help\n")
			TEXT("Tab / Esc  pause menu"));
		Section(C3, TEXT("GAMEPAD"),
			TEXT("D-pad      telegraph / rudder\n")
			TEXT("R trigger  fire     R stick  look\n")
			TEXT("L trigger  exit binoculars"));
		UTextBlock* F = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		F->SetText(FText::FromString(TEXT("H / F1 / Esc: close")));
		F->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 14)); F->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f, 1.f)));
		Root->AddChildToVerticalBox(F)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------- test director
// -iwp11=day|wx|harbour|help [-iwp11quit]: shots to Saved/shots, log lines "IWP11 shot".
void AIronwakeVoyage::TickP11Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;
	P11T += RealDt;
	const float Since = P11T - P11Mark;
	const FVector Town = HarbourC + FRotator(0.f, 45.f, 0.f).RotateVector(FVector(-9000.f, 0.f, 0.f));
	auto Stand = [&](float StandM, float FromYaw)
	{
		const FVector D = FRotator(0.f, FromYaw, 0.f).Vector();
		const FVector Lc = FVector(Town.X, Town.Y, 0.f) + D * StandM * 100.f;
		P->SetActorLocationAndRotation(FVector(Lc.X, Lc.Y, P->GetActorLocation().Z), FRotator(0.f, FromYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(2);
		P->ResetCameraToHorizon();
	};
	struct FStep { const TCHAR* Shot; float Hour; int32 Wx; float Sea; float StandM; float Yaw; bool bHigh; bool bHelp; };
	static const FStep Day[] = {
		{ TEXT("P11_day_dawn"), 7.7f, 1, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_day_noon"), 12.f, 1, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_day_dusk"), 16.3f, 1, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_day_night"), 22.f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_day_small"), 3.f, 0, -1.f, 450.f, 55.f, false, false },
	};
	static const FStep P14[] = {
		{ TEXT("P14_noon_400"), 12.f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P14_noon_900"), 12.f, 0, -1.f, 900.f, 55.f, false, false },
		{ TEXT("P14_night_chase"), 22.f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P14_night_high"), 22.f, 0, -1.f, 700.f, 50.f, true, false },
		{ TEXT("P14_squall_chase"), 11.f, 5, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P14_squall_high"), 11.f, 5, -1.f, 700.f, 50.f, true, false },
		{ TEXT("P14_harbour_high"), 11.f, 1, 1.f, 1250.f, 35.f, true, false },
	};
	static const FStep Tw[] = {
		{ TEXT("P13_tw_dawn_a"), 6.6f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P13_tw_dawn_b"), 7.3f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P13_tw_dusk_a"), 16.9f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P13_tw_dusk_b"), 16.5f, 0, -1.f, 450.f, 55.f, false, false },
	};
	static const FStep Wx[] = {
		{ TEXT("P11_wx_clear"), 11.f, 0, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_wx_fair"), 11.f, 1, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_wx_overcast"), 11.f, 2, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_wx_mist"), 11.f, 3, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_wx_rain"), 11.f, 4, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_wx_squall"), 11.f, 5, -1.f, 450.f, 55.f, false, false },
	};
	static const FStep Hb[] = {
		{ TEXT("P11_harbour_calm_high"), 11.f, 1, 0.f, 700.f, 50.f, true, false },
		{ TEXT("P11_harbour_mod_high"), 11.f, 1, 1.f, 700.f, 50.f, true, false },
		{ TEXT("P11_harbour_mod_chase"), 11.f, 1, 1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_offshore_mod_chase"), 11.f, 1, 1.f, 1500.f, 55.f, false, false },
	};
	static const FStep Hp[] = {
		{ TEXT("P11_hint_only"), 11.f, 1, -1.f, 450.f, 55.f, false, false },
		{ TEXT("P11_help"), 11.f, 1, -1.f, 450.f, 55.f, false, true },
	};
	const FStep* Steps = P11Mode == TEXT("p14") ? P14 : P11Mode == TEXT("tw") ? Tw : P11Mode == TEXT("day") ? Day : P11Mode == TEXT("wx") ? Wx : P11Mode == TEXT("harbour") ? Hb : Hp;
	const int32 NS = P11Mode == TEXT("p14") ? UE_ARRAY_COUNT(P14) : P11Mode == TEXT("tw") ? UE_ARRAY_COUNT(Tw) : P11Mode == TEXT("day") ? UE_ARRAY_COUNT(Day) : P11Mode == TEXT("wx") ? UE_ARRAY_COUNT(Wx) : P11Mode == TEXT("harbour") ? UE_ARRAY_COUNT(Hb) : UE_ARRAY_COUNT(Hp);
	auto Apply = [&](const FStep& S)
	{
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(S.Hour); SeaOverride = S.Sea;
		ForceWeatherState(S.Wx, 0.05f);
		LiveCur = StateWx(S.Wx); LiveFrom = LiveTo = LiveCur; LiveBlendT = 1.f;
		Stand(S.StandM, S.Yaw);
	};
	if (P11Mode == TEXT("bench"))
	{
		if (P11Step == 0 && P11T >= 3.f) { if (!C->bStarted) C->StartGame(true); bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(12.f); Stand(330.f, 60.f); P11Step = 1; P11Mark = P11T; }
		if (P11Step == 1 && Since > 2.f && Since < 2.3f) P->ResetCameraToHorizon();
		return;
	}
	if (P11Step == 0)
	{
		if (P11T < 3.f) return;
		if (!C->bStarted) C->StartGame(true);
		for (FIronwakePOI& Pt : POIs) Pt.bVisited = true;
		P11Step = 1; P11Mark = P11T; Apply(Steps[0]); return;
	}
	if (P11Step >= 1 && P11Step <= NS)
	{
		const FStep& S = Steps[P11Step - 1];
		if (Since > 2.f && Since < 2.3f && !S.bHigh) P->ResetCameraToHorizon();
		if (Since > 3.f && Since < 3.3f && S.bHigh)
		{
			if (!bPhoto) TogglePhoto();
			const FVector D = FRotator(0.f, S.Yaw, 0.f).Vector();
			const FVector Eye = FVector(Town.X, Town.Y, 0.f) + D * S.StandM * 100.f + FVector(0, 0, 6500.f);
			SetPhotoView(Eye, (Town + FVector(0, 0, 1500.f) - Eye).Rotation());
			if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(70.f);
		}
		if (S.bHelp && Since > 6.f && !C->IsHelpOpen()) C->ToggleHelp();
		const float Wait = S.Wx >= 4 ? 18.f : 12.f;   // let the sea and clouds settle
		if (Since >= Wait && !bP11Shot)
		{
			C->IWShot(S.Shot); bP11Shot = true;
			AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld());
			UE_LOG(LogIronwake, Display, TEXT("IWP11 shot %s hour=%.1f wx=%s localSea=%.2f Hs=%.1f m help=%d"), S.Shot, ClockHour, GStateNames[LiveState], St ? St->GetLocalState() : -1.f, IronwakeSea::SigWaveHeightM(), C->IsHelpOpen());
		}
		if (Since >= Wait + 1.5f)
		{
			bP11Shot = false; ++P11Step; P11Mark = P11T;
			if (bPhoto) TogglePhoto();
			if (C->IsHelpOpen()) C->ToggleHelp();
			if (P11Step <= NS) Apply(Steps[P11Step - 1]);
		}
		return;
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP11 DONE"));
	P11Mode.Empty();
	if (FParse::Param(FCommandLine::Get(), TEXT("iwp11quit"))) UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
}

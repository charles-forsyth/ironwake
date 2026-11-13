#pragma once
#include "CoreMinimal.h"

/**
 * H1: player options saved in GameUserSettings.ini [Ironwake] (survive restarts and re-packages).
 *  Graphics 0 LOW / 1 MEDIUM (default, 60 fps on the M5 at 1080p) / 2 HIGH (cinematic clouds, ~30 fps class)
 *  Music    off by default (Chuck 10:21); sound effects always on
 *  RudderAutoCentre  off by default: the rudder holds where you leave it; on = springs back when A/D released
 */
namespace IronwakeSettings
{
	IRONWAKETEST_API int32 GetGraphics();
	IRONWAKETEST_API void SetGraphics(int32 Level); // saves + applies
	IRONWAKETEST_API void ApplyGraphics();           // re-apply saved level (call at game start)
	IRONWAKETEST_API const TCHAR* GraphicsName(int32 Level);

	IRONWAKETEST_API bool GetMusic();
	IRONWAKETEST_API void SetMusic(bool bOn);

	IRONWAKETEST_API bool GetRudderAutoCentre();
	IRONWAKETEST_API void SetRudderAutoCentre(bool bOn);

	/** P16 CAMERA MOTION: 0 OFF (fully steady), 1 LOW (default; <=15 % of ship motion, low-passed, 1.5/1.0 deg, 0.3 m), 2 MEDIUM (3/2 deg, 0.6 m). */
	IRONWAKETEST_API int32 GetCameraMotion();
	IRONWAKETEST_API void SetCameraMotion(int32 Level);
	IRONWAKETEST_API const TCHAR* CameraMotionName(int32 Level);
}

#include "IronwakeHUD.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeVoyage.h"
#include "IronwakeFleet.h"
#include "IronwakeNpc.h"
#include "Engine/Canvas.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

void AIronwakeHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	const AIronwakeShipPawn* Ship = PlayerOwner ? Cast<AIronwakeShipPawn>(PlayerOwner->GetPawn()) : nullptr;
	if (!Ship) return;
	if (PlayerOwner->GetViewTarget() != Ship) return; // cinematic demo cameras: no HUD
	if (const AIronwakePlayerController* IPC = Cast<AIronwakePlayerController>(PlayerOwner); IPC && (IPC->IsMenuOpen() || (IPC->IsEndScreenOpen() && Ship->bSinking))) return; // title/pause menu, SHIP LOST

	if (const AIronwakePlayerController* HP = Cast<AIronwakePlayerController>(PlayerOwner); HP && HP->IsHelpOpen()) return; // P11: help panel open -> no HUD behind it

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	const float S = FMath::Clamp(Canvas->ClipY / 1080.f, 0.75f, 2.5f);

	// H1: binocular / gun-sight overlay (SHIFT). Dark vignette ring, reticle with mil ticks, range + magnification.
	if (Ship->IsScoped())
	{
		const float CX0 = Canvas->ClipX * 0.5f, CY0 = Canvas->ClipY * 0.5f;
		const float R = Canvas->ClipY * 0.46f;
		const FLinearColor Black(0.f, 0.f, 0.f, 1.f);
		DrawRect(Black, 0.f, 0.f, CX0 - R, Canvas->ClipY);
		DrawRect(Black, CX0 + R, 0.f, Canvas->ClipX - (CX0 + R), Canvas->ClipY);
		// Ring mask: horizontal strips outside the circle.
		const int32 Strips = 400;
		for (int32 i = 0; i < Strips; ++i)
		{
			const float Y0 = CY0 - R + 2.f * R * i / Strips, Y1 = CY0 - R + 2.f * R * (i + 1) / Strips;
			const float Ym = 0.5f * (Y0 + Y1) - CY0;
			const float Hw = FMath::Sqrt(FMath::Max(R * R - Ym * Ym, 0.f));
			DrawRect(Black, CX0 - R, Y0, R - Hw, Y1 - Y0 + 1.f);
			DrawRect(Black, CX0 + Hw, Y0, R - Hw, Y1 - Y0 + 1.f);
		}
		DrawRect(Black, CX0 - R, 0.f, 2.f * R, CY0 - R + 1.f);
		DrawRect(Black, CX0 - R, CY0 + R - 1.f, 2.f * R, Canvas->ClipY - (CY0 + R) + 1.f);
		const FLinearColor Ret(0.05f, 0.05f, 0.05f, 0.9f), Lt(0.86f, 0.74f, 0.47f, 0.95f);
		DrawLine(CX0 - R, CY0, CX0 - 18.f * S, CY0, Ret, 2.f);
		DrawLine(CX0 + 18.f * S, CY0, CX0 + R, CY0, Ret, 2.f);
		DrawLine(CX0, CY0 + 18.f * S, CX0, CY0 + R, Ret, 2.f);
		for (int32 k = 1; k <= 8; ++k)
		{
			const float Off = k * R / 9.f, H = (k % 2 == 0 ? 10.f : 5.f) * S;
			DrawLine(CX0 - Off, CY0 - H, CX0 - Off, CY0 + H, Ret, 1.5f);
			DrawLine(CX0 + Off, CY0 - H, CX0 + Off, CY0 + H, Ret, 1.5f);
			DrawLine(CX0 - H, CY0 + Off, CX0 + H, CY0 + Off, Ret, 1.5f);
		}
		DrawLine(CX0 - 6.f * S, CY0, CX0 + 6.f * S, CY0, Lt, 2.f);
		DrawLine(CX0, CY0 - 6.f * S, CX0, CY0 + 6.f * S, Lt, 2.f);
		const float Mag = 70.f / FMath::Max(Ship->GetViewFov(), 1.f);
		const FString RangeTxt = Ship->AimRange > 0.f ? FString::Printf(TEXT("RANGE  %.2f km"), Ship->AimRange / 100000.f) : FString(TEXT("RANGE  ---"));
		DrawText(RangeTxt, Lt, CX0 + R * 0.30f, CY0 + R * 0.72f, Font, 1.2f * S);
		DrawText(FString::Printf(TEXT("x%.1f"), Mag), Lt, CX0 - R * 0.55f, CY0 + R * 0.72f, Font, 1.2f * S);
		DrawText(FString::Printf(TEXT("BRG %03.0f"), FMath::Fmod(PlayerOwner->GetControlRotation().Yaw + 720.f, 360.f)), Lt, CX0 - 40.f * S, CY0 - R * 0.85f, Font, 1.1f * S);
		// P21: name + role tooltip for the NPC craft under the reticle (identifying it marks it known on the minimap)
		if (AIronwakeNpcDirector* Nd = AIronwakeNpcDirector::Get(GetWorld()))
		{
			float Rg = 0.f; const FVector Eye = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraLocation() : Ship->GetActorLocation();
			if (const FNpcRec* T = Nd->ScopeTarget(Eye, PlayerOwner->GetControlRotation().Yaw, 600000.f, Rg))
			{
				const FString L1 = T->Name, L2 = FString::Printf(TEXT("%s   %s   %.2f km"), NpcRoleName(T->Role), NpcFactionName(T->Faction), Rg / 100000.f);
				float W1, H1, W2, H2; GetTextSize(L1, W1, H1, Font, 1.25f * S); GetTextSize(L2, W2, H2, Font, 0.95f * S);
				const float BW = FMath::Max(W1, W2) + 24.f * S, BX = CX0 - BW * 0.5f, BY = CY0 + R * 0.18f;
				DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.78f), BX, BY, BW, H1 + H2 + 18.f * S);
				DrawText(L1, Lt, CX0 - W1 * 0.5f, BY + 6.f * S, Font, 1.25f * S);
				DrawText(L2, FLinearColor(0.78f, 0.82f, 0.86f, 1.f), CX0 - W2 * 0.5f, BY + 8.f * S + H1, Font, 0.95f * S);
			}
		}
		DrawText(TEXT("SHIFT / RIGHT-CLICK: exit     WHEEL: magnification"), FLinearColor(0.62f, 0.66f, 0.70f, 0.9f), CX0 - 230.f * S, Canvas->ClipY - 34.f * S, Font, 0.9f * S);
		return;
	}
	const float X = 28.f * S;
	float Y = Canvas->ClipY - 205.f * S;
	const FLinearColor Brass(0.86f, 0.74f, 0.47f, 1.f);
	const FLinearColor Dim(0.62f, 0.66f, 0.70f, 1.f);
	const FLinearColor Red(0.85f, 0.25f, 0.18f, 1.f);

	const AIronwakePlayerController* IPC2 = Cast<AIronwakePlayerController>(PlayerOwner);
	const bool bPeace = IPC2 && IPC2->bPeaceful && !Ship->FindEnemy();
	if (bPeace) Y += 54.f * S; // no HULL/GUNS lines in a peaceful voyage
	const int32 Extra = Ship->HasSails() ? 3 : 1; // S2: SHIP line (+ WIND / SAIL lines on the sailing ship)
	Y -= 27.f * S * Extra;
	DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.55f), X - 12.f * S, Y - 10.f * S, (Ship->HasSails() ? 430.f : 340.f) * S, (bPeace ? 129.f : 183.f) * S + 27.f * S * Extra);
	auto Line = [&](const FString& Label, const FString& Value, const FLinearColor& C)
	{
		DrawText(Label, Dim, X, Y, Font, 1.1f * S);
		DrawText(Value, C, X + 120.f * S, Y, Font, 1.1f * S);
		Y += 27.f * S;
	};
	Line(TEXT("SHIP"), Ship->GetShipName(), Brass);
	if (Ship->HasSails())
	{
		const float From = IronwakeFleet::WindFromYaw(GetWorld());
		Line(TEXT("WIND"), FString::Printf(TEXT("from %s %03.0f  %.0f kn   app %.0f kn %.0f%s"), IronwakeFleet::CompassPoint(From), From, IronwakeFleet::WindKn(GetWorld()),
			Ship->ApparentKn, FMath::Abs(Ship->ApparentAngle), Ship->ApparentAngle >= 0.f ? TEXT("S") : TEXT("P")), Brass);
		const float Twa = Ship->TrueWindAngle;
		Line(TEXT("POINT"), FString::Printf(TEXT("%s  %.0f deg %s   %s"), IronwakeFleet::PointOfSail(Twa), FMath::Abs(Twa), Twa >= 0.f ? TEXT("stbd") : TEXT("port"),
			Ship->bAuxSteam ? TEXT("STEAM ON") : TEXT("steam off (B)")), FMath::Abs(Twa) < 40.f ? Red : Brass);
	}
	Line(Ship->HasSails() ? TEXT("SAIL") : TEXT("TELEGRAPH"), Ship->GetTelegraphName(), Brass);
	if (Ship->HasSails())
	{
		// Wind dial (right of the block): ship silhouette pointing up, arrow = true wind blowing onto her, red arc = in irons.
		const float DX = X + 470.f * S, DY = Canvas->ClipY - 150.f * S, R = 62.f * S;
		DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.55f), DX - R - 14.f * S, DY - R - 14.f * S, 2.f * R + 28.f * S, 2.f * R + 40.f * S);
		for (int32 k = 0; k < 48; ++k)
		{
			const float A0 = k * 7.5f, A1 = (k + 1) * 7.5f;
			const bool bIrons = FMath::Abs(FRotator::NormalizeAxis(A0 + 3.75f - Ship->TrueWindAngle)) < 35.f;
			auto Pt = [&](float A) { const float Rr = FMath::DegreesToRadians(A); return FVector2D(DX + FMath::Sin(Rr) * R, DY - FMath::Cos(Rr) * R); };
			const FVector2D P0 = Pt(A0), P1 = Pt(A1);
			DrawLine(P0.X, P0.Y, P1.X, P1.Y, bIrons ? Red : Dim, bIrons ? 3.f : 1.f);
		}
		DrawLine(DX, DY - 26.f * S, DX - 7.f * S, DY + 18.f * S, Brass, 2.f); DrawLine(DX, DY - 26.f * S, DX + 7.f * S, DY + 18.f * S, Brass, 2.f);
		DrawLine(DX - 7.f * S, DY + 18.f * S, DX + 7.f * S, DY + 18.f * S, Brass, 2.f);
		auto Arrow = [&](float AngDeg, float Len, const FLinearColor& Col, float Th)
		{
			const float Rr = FMath::DegreesToRadians(AngDeg);
			const FVector2D Dir(FMath::Sin(Rr), -FMath::Cos(Rr));
			const FVector2D Tail = FVector2D(DX, DY) + Dir * R * 1.0f, Head = FVector2D(DX, DY) + Dir * R * (1.f - Len);
			DrawLine(Tail.X, Tail.Y, Head.X, Head.Y, Col, Th);
			const FVector2D Pp(-Dir.Y, Dir.X);
			const FVector2D A1 = Head + Dir * 9.f * S + Pp * 6.f * S, A2 = Head + Dir * 9.f * S - Pp * 6.f * S;
			DrawLine(Head.X, Head.Y, A1.X, A1.Y, Col, Th); DrawLine(Head.X, Head.Y, A2.X, A2.Y, Col, Th);
		};
		Arrow(Ship->TrueWindAngle, 0.55f, FLinearColor(0.55f, 0.75f, 1.f, 1.f), 3.f);
		Arrow(Ship->ApparentAngle, 0.38f, FLinearColor(0.9f, 0.9f, 0.9f, 0.8f), 1.5f);
		DrawText(TEXT("WIND  true / app"), Dim, DX - 52.f * S, DY + R + 6.f * S, Font, 0.75f * S);
	}
	Line(TEXT("RUDDER"), FString::Printf(TEXT("%s  (%+.0f deg)"), *Ship->GetRudderName(), Ship->RudderAngleDeg), Brass);
	Line(TEXT("SPEED"), FString::Printf(TEXT("%.1f kn"), Ship->GetSpeedKnots()), Brass);
	Line(TEXT("HEADING"), FString::Printf(TEXT("%03.0f"), Ship->GetHeadingDeg()), Brass);
	if (AIronwakeVoyage* Vy = AIronwakeVoyage::Get(GetWorld())) Vy->DrawVoyageHUD(this, Canvas, const_cast<AIronwakeShipPawn*>(Ship), Font, S, bPeace);
	if (bPeace) return; // S1: no gunnery HUD (crosshair, aim ring) when sailing in peace
	const float HPFrac = Ship->HP / FMath::Max(Ship->MaxHP, 1.f);
	Line(TEXT("HULL"), FString::Printf(TEXT("%.0f%%  fires %d"), HPFrac * 100.f, Ship->NumFires()), HPFrac < 0.4f ? Red : Brass);
	const bool bReady = Ship->TurretReload[0] <= 0.f || Ship->TurretReload[1] <= 0.f;
	Line(TEXT("GUNS"), FString::Printf(TEXT("%s  %s  %.1f km"), bReady ? TEXT("READY") : TEXT("LOADING"),
		(Ship->TurretOnTarget[0] || Ship->TurretOnTarget[1]) ? TEXT("ON") : TEXT("TRAINING"), Ship->AimRange / 100000.f), bReady ? Brass : Dim);

	// H1: turret panel (bottom centre): ship plan with both turrets' arcs, current train and ready state.
	{
		const float PX = Canvas->ClipX - 170.f * S, PY = Canvas->ClipY * 0.50f, PR = 58.f * S; // right edge, clear of the stern
		DrawRect(FLinearColor(0.02f, 0.025f, 0.03f, 0.55f), PX - 150.f * S, PY - 78.f * S, 300.f * S, 150.f * S);
		const float ViewYaw = PlayerOwner->GetControlRotation().Yaw;
		const float ShipYaw = Ship->GetActorRotation().Yaw;
		// Up on the panel = ship's bow. Hull outline.
		auto P2 = [&](float Fwd, float Side) { return FVector2D(PX + Side * S, PY - Fwd * S); };
		const FVector2D HullPts[] = { P2(52, 0), P2(30, 9), P2(-40, 9), P2(-50, 6), P2(-50, -6), P2(-40, -9), P2(30, -9), P2(52, 0) };
		for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(HullPts); ++i) DrawLine(HullPts[i].X, HullPts[i].Y, HullPts[i + 1].X, HullPts[i + 1].Y, Dim, 1.5f);
		for (int32 t = 0; t < 2; ++t)
		{
			const FVector2D C = P2(t == 0 ? 33.f : -38.f, 0.f);
			const float Rest = Ship->GetTurretRestYaw(t);
			const float Arc = AIronwakeShipPawn::TurretArcDeg;
			auto Dir = [&](float RelDeg, float Len) { const float A = FMath::DegreesToRadians(RelDeg); return FVector2D(C.X + FMath::Sin(A) * Len, C.Y - FMath::Cos(A) * Len); };
			// Arc fan (thin, dim) and dead zone hint.
			for (float a = -Arc; a < Arc; a += 15.f) { const FVector2D A0 = Dir(Rest + a, 30.f * S), A1 = Dir(Rest + FMath::Min(a + 15.f, Arc), 30.f * S); DrawLine(A0.X, A0.Y, A1.X, A1.Y, FLinearColor(0.5f, 0.5f, 0.5f, 0.5f), 1.f); }
			const bool bReadyT = Ship->TurretReload[t] <= 0.f;
			const bool bOn = Ship->TurretOnTarget[t];
			const FLinearColor TC = bOn && bReadyT ? FLinearColor(0.35f, 0.95f, 0.4f, 1.f) : bReadyT ? Brass : Red;
			const FVector2D Tip = Dir(Ship->GetTurretBearingRel(t), 30.f * S);
			DrawLine(C.X, C.Y, Tip.X, Tip.Y, TC, 3.f);
			DrawRect(TC, C.X - 3.f * S, C.Y - 3.f * S, 6.f * S, 6.f * S);
			const FString St = bReadyT ? (bOn ? TEXT("ON") : TEXT("TRAIN")) : FString::Printf(TEXT("%.1fs"), Ship->TurretReload[t]);
			DrawText(St, TC, PX + (t == 0 ? 60.f : 60.f) * S, PY - (t == 0 ? 52.f : -30.f) * S, Font, 0.8f * S);
		}
		// Camera look direction relative to the bow.
		const float LookRel = FMath::FindDeltaAngleDegrees(ShipYaw, ViewYaw);
		const float LA = FMath::DegreesToRadians(LookRel);
		DrawLine(PX, PY, PX + FMath::Sin(LA) * PR, PY - FMath::Cos(LA) * PR, FLinearColor(0.9f, 0.9f, 0.9f, 0.6f), 1.f);
		DrawText(TEXT("FWD"), Dim, PX - 120.f * S, PY - 70.f * S, Font, 0.7f * S);
		DrawText(TEXT("AFT"), Dim, PX - 120.f * S, PY + 50.f * S, Font, 0.7f * S);
	}

	// Crosshair at screen centre (the camera aim).
	const float CX = Canvas->ClipX * 0.5f, CY = Canvas->ClipY * 0.5f;
	const float L = 14.f * S, Gp = 5.f * S;
	DrawLine(CX - L - Gp, CY, CX - Gp, CY, Brass, 2.f);
	DrawLine(CX + Gp, CY, CX + L + Gp, CY, Brass, 2.f);
	DrawLine(CX, CY + Gp, CX, CY + L + Gp, Brass, 2.f);

	// Turret aim marker (where the guns actually point now): small ring at the aim point.
	auto Ring = [&](const FVector& W, float R, const FLinearColor& C)
	{
		const FVector P = Project(W);
		if (P.Z <= 0.f) return FVector2D(-1, -1);
		const int32 N = 16;
		for (int32 i = 0; i < N; ++i)
		{
			const float A0 = 2.f * PI * i / N, A1 = 2.f * PI * (i + 1) / N;
			DrawLine(P.X + R * FMath::Cos(A0), P.Y + R * FMath::Sin(A0), P.X + R * FMath::Cos(A1), P.Y + R * FMath::Sin(A1), C, 1.5f);
		}
		return FVector2D(P.X, P.Y);
	};

	// Enemy marker + lead indicator (DESIGN.md: lead indicator on by default).
	if (AIronwakeShipPawn* E = Ship->FindEnemy())
	{
		const FVector EP = Project(E->GetActorLocation() + FVector(0, 0, 2500.f));
		if (EP.Z > 0.f)
		{
			const float D = FVector::Dist2D(E->GetActorLocation(), Ship->GetActorLocation()) / 100000.f;
			const FString Tag = E->bSinking ? TEXT("SINKING") : FString::Printf(TEXT("IRONCLAD  %.1f km  %.0f%%"), D, 100.f * E->HP / FMath::Max(E->MaxHP, 1.f));
			float TW, TH;
			GetTextSize(Tag, TW, TH, Font, 0.9f * S);
			DrawText(Tag, Red, EP.X - TW * 0.5f, EP.Y - 40.f * S, Font, 0.9f * S);
			DrawLine(EP.X - 10 * S, EP.Y - 14 * S, EP.X, EP.Y - 4 * S, Red, 2.f);
			DrawLine(EP.X + 10 * S, EP.Y - 14 * S, EP.X, EP.Y - 4 * S, Red, 2.f);
		}
		if (Ship->bLeadIndicator && Ship->bHasLead)
		{
			const FVector2D LP = Ring(Ship->LeadPoint, 11.f * S, FLinearColor(1.f, 0.55f, 0.15f, 1.f));
			if (LP.X >= 0.f)
			{
				DrawText(FString::Printf(TEXT("LEAD  %.1fs"), Ship->LastFlightTime), FLinearColor(1.f, 0.55f, 0.15f, 1.f), LP.X + 14.f * S, LP.Y - 8.f * S, Font, 0.8f * S);
				const FVector EG = Project(E->GetActorLocation() + FVector(0, 0, 600.f));
				if (EG.Z > 0.f) DrawLine(EG.X, EG.Y, LP.X, LP.Y, FLinearColor(1.f, 0.55f, 0.15f, 0.5f), 1.f);
			}
		}
	}
	Ring(Ship->AimPoint, 6.f * S, FLinearColor(0.9f, 0.9f, 0.9f, 0.8f));
}

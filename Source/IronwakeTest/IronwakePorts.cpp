// P29: Data-driven multi-port world (Content/IronwakeData/world_ports.json -> FIwPortSpec), reusable secondary port builder
// (Ironcrag Coaling & Foundry Haven + Stigsby Fishing & Timber Outpost), arrival mooring & salvage verb, live harbourmaster
// diorama camera, DPI-scaled UIronwakeDockWidget (8-good cargo & mail market, Captain's Logbook, Shipyard & Repairs), and -iwp29=test harness.

#include "IronwakePorts.h"
#include "IronwakeVoyage.h"
#include "IronwakeArchipelago.h"
#include "IronwakeP15.h"
#include "IronwakeP23.h"
#include "IronwakeWorld.h"
#include "IronwakeCollision.h"
#include "IronwakeShell.h"
#include "IronwakeLivingSea.h"
#include "IronwakeStorm.h"
#include "IronwakeHydro.h"
#include "IronwakeNpc.h"
#include "IronwakeFleet.h"
#include "IronwakeAudio.h"
#include "IronwakeShipPawn.h"
#include "IronwakePlayerController.h"
#include "IronwakeTest.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/BoxComponent.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/PointLightComponent.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SpotLightComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"

namespace
{
	const TCHAR* GPortCfgSec = TEXT("Ironwake");

	int32 ReadCfgInt(const TCHAR* Key, int32 Def)
	{
		int32 V = Def;
		if (GConfig) GConfig->GetInt(GPortCfgSec, Key, V, GGameUserSettingsIni);
		return V;
	}
	void WriteCfgInt(const TCHAR* Key, int32 V, bool bFlush = true)
	{
		if (!GConfig) return;
		GConfig->SetInt(GPortCfgSec, Key, V, GGameUserSettingsIni);
		if (bFlush) GConfig->Flush(false, GGameUserSettingsIni);
	}
	FString ReadCfgStr(const TCHAR* Key, const FString& Def = FString())
	{
		FString V = Def;
		if (GConfig) GConfig->GetString(GPortCfgSec, Key, V, GGameUserSettingsIni);
		return V;
	}
	void WriteCfgStr(const TCHAR* Key, const FString& V, bool bFlush = true)
	{
		if (!GConfig) return;
		GConfig->SetString(GPortCfgSec, Key, *V, GGameUserSettingsIni);
		if (bFlush) GConfig->Flush(false, GGameUserSettingsIni);
	}

	UMaterialInstanceDynamic* PortShapeTint(UObject* Outer, const FLinearColor& C)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (!Base) return nullptr;
		UMaterialInstanceDynamic* Mi = UMaterialInstanceDynamic::Create(Base, Outer);
		Mi->SetVectorParameterValue(TEXT("Color"), C);
		return Mi;
	}

	void FinalizePortDerived(FIwPortSpec& P)
	{
		P.WorldCentreCm = FVector(P.WorldX * 100.f, P.WorldY * 100.f, 0.f);
		P.MoorWorldCm = P.H2W(P.MoorLocal.X, P.MoorLocal.Y, 0.f);
		P.MoorWorldCm.Z = 0.f;
		P.OffingWorldCm = P.H2W(P.OffingLocal.X, P.OffingLocal.Y, 0.f);
		P.OffingWorldCm.Z = 0.f;
		for (int32 g = 0; g < GIwNumGoods; ++g) P.LiveStock[g] = P.BaseStock[g];
	}

	void PopulateFallbackWorldPorts(TArray<FIwGoodDef>& OutGoods, TArray<FIwPortSpec>& OutPorts)
	{
		OutGoods = {
			{ TEXT("coal"),         TEXT("Coal"),         TEXT("tons"), 14 },
			{ TEXT("pig_iron"),     TEXT("Pig Iron"),     TEXT("tons"), 32 },
			{ TEXT("salt_fish"),    TEXT("Salt Fish"),    TEXT("tons"), 18 },
			{ TEXT("naval_timber"), TEXT("Naval Timber"), TEXT("tons"), 24 },
			{ TEXT("brasswork"),    TEXT("Brasswork"),    TEXT("tons"), 85 },
			{ TEXT("sailcloth"),    TEXT("Sailcloth"),    TEXT("tons"), 38 },
			{ TEXT("whale_oil"),    TEXT("Whale Oil"),    TEXT("tons"), 64 },
			{ TEXT("mail_packet"),  TEXT("Mail Packet"),  TEXT("tons"), 115 },
		};
		auto AddPort = [&](FName Id, const TCHAR* Nm, const TCHAR* Tg, const TCHAR* St, const TCHAR* Sub, const TCHAR* Note,
			bool bMain, int32 Isle, int32 Sd, float WX, float WY, float Yaw, float Hs, float Hh,
			FVector2D Moor, float MoorR, FVector2D Off, FVector Eye, FVector Look,
			std::initializer_list<float> Mults, std::initializer_list<int32> Stocks)
		{
			FIwPortSpec P;
			P.Id = Id; P.Name = Nm; P.Tag = Tg; P.Style = St; P.Subtitle = Sub; P.HarbourmasterNote = Note;
			P.bMainHarbour = bMain; P.HostIsleIdx = Isle; P.Seed = Sd;
			P.WorldX = WX; P.WorldY = WY; P.SeawardYaw = Yaw; P.HillScale = Hs; P.HillHeightMul = Hh;
			P.MoorLocal = Moor; P.MoorRadiusM = MoorR; P.OffingLocal = Off;
			P.DioramaEyeLocal = Eye; P.DioramaLookLocal = Look;
			int32 k = 0; for (float m : Mults) if (k < GIwNumGoods) P.PriceMult[k++] = m;
			k = 0; for (int32 s : Stocks) if (k < GIwNumGoods) P.BaseStock[k++] = s;
			FinalizePortDerived(P);
			OutPorts.Add(P);
		};
		AddPort(TEXT("greyhaven"), TEXT("Greyhaven Harbour"), TEXT("H"), TEXT("capital"),
			TEXT("Principal Crown & Merchant Port — Customs Quay"),
			TEXT("Steam packets clear on the half-tide. Ironcrag cries out for brasswork and mail; Stigsby pays silver for coal."),
			true, -1, 1912, -3450.f, -3350.f, 45.f, 1.0f, 1.0f, FVector2D(32.f, -35.f), 175.f, FVector2D(280.f, 0.f),
			FVector(-64.f, 44.f, 6.2f), FVector(-86.f, 26.f, 5.4f),
			{ 1.35f, 1.40f, 1.15f, 1.30f, 0.66f, 0.65f, 1.35f, 0.68f }, { 30, 22, 40, 28, 55, 80, 20, 45 });
		AddPort(TEXT("ironcrag"), TEXT("Ironcrag"), TEXT("Ic"), TEXT("foundry"),
			TEXT("Northern Basalt Coaling & Foundry Haven"),
			TEXT("Three blast furnaces roaring day and night. Cheap bunker coal and pig iron on the quay; bring salt fish, timber, or brasswork."),
			false, 8, 2901, 2590.f, 90.f, 205.f, 0.72f, 0.94f, FVector2D(42.f, 0.f), 155.f, FVector2D(320.f, 0.f),
			FVector(-30.f, 44.f, 8.8f), FVector(-78.f, -8.f, 7.2f),
			{ 0.55f, 0.60f, 1.52f, 1.45f, 1.42f, 1.24f, 1.38f, 1.45f }, { 145, 100, 14, 14, 12, 18, 12, 10 });
		AddPort(TEXT("stigsby"), TEXT("Stigsby"), TEXT("St"), TEXT("fishing"),
			TEXT("Eastern Wooded Fishing & Timber Outpost"),
			TEXT("Smacks landed a heavy catch and the sawmill is stacked high. Salt fish, naval timber, and whale oil ready; we lack coal and iron."),
			false, 9, 2902, -570.f, 3550.f, 235.f, 0.62f, 0.74f, FVector2D(46.f, 4.f), 155.f, FVector2D(310.f, 0.f),
			FVector(-24.f, -32.f, 7.6f), FVector(-68.f, 10.f, 5.8f),
			{ 1.50f, 1.22f, 0.58f, 0.60f, 1.25f, 1.40f, 0.66f, 1.42f }, { 12, 18, 135, 115, 14, 14, 60, 10 });
	}
}

namespace IwPorts
{
	bool LoadWorldPorts(TArray<FIwGoodDef>& OutGoods, TArray<FIwPortSpec>& OutPorts)
	{
		OutGoods.Reset();
		OutPorts.Reset();
		const FString PathPrimary = FPaths::ProjectContentDir() / TEXT("IronwakeData/world_ports.json");
		const FString PathFallback = FPaths::ProjectContentDir() / TEXT("Data/ports.json");
		FString Chosen = IFileManager::Get().FileExists(*PathPrimary) ? PathPrimary : PathFallback;
		FString Raw;
		if (!FFileHelper::LoadFileToString(Raw, *Chosen))
		{
			UE_LOG(LogIronwake, Warning, TEXT("IWP29 world_ports.json not found at %s; using built-in port table"), *PathPrimary);
			PopulateFallbackWorldPorts(OutGoods, OutPorts);
			return false;
		}
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogIronwake, Warning, TEXT("IWP29 failed to parse %s; using built-in port table"), *Chosen);
			PopulateFallbackWorldPorts(OutGoods, OutPorts);
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* GoodsArr = nullptr;
		if (Root->TryGetArrayField(TEXT("goods"), GoodsArr) && GoodsArr)
		{
			for (const TSharedPtr<FJsonValue>& V : *GoodsArr)
			{
				if (!V.IsValid() || V->Type != EJson::Object) continue;
				const TSharedPtr<FJsonObject> O = V->AsObject();
				FIwGoodDef G;
				G.Id = FName(*O->GetStringField(TEXT("id")));
				G.Name = O->GetStringField(TEXT("name"));
				G.Unit = O->GetStringField(TEXT("unit"));
				G.BasePrice = (int32)O->GetIntegerField(TEXT("basePrice"));
				OutGoods.Add(G);
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* PortsArr = nullptr;
		if (Root->TryGetArrayField(TEXT("ports"), PortsArr) && PortsArr)
		{
			auto ReadVec2 = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Field, FVector2D Def) -> FVector2D
			{
				const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
				if (O->TryGetArrayField(Field, A) && A && A->Num() >= 2)
					return FVector2D((float)(*A)[0]->AsNumber(), (float)(*A)[1]->AsNumber());
				return Def;
			};
			auto ReadVec3 = [](const TSharedPtr<FJsonObject>& O, const TCHAR* Field, FVector Def) -> FVector
			{
				const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
				if (O->TryGetArrayField(Field, A) && A && A->Num() >= 3)
					return FVector((float)(*A)[0]->AsNumber(), (float)(*A)[1]->AsNumber(), (float)(*A)[2]->AsNumber());
				return Def;
			};
			for (const TSharedPtr<FJsonValue>& V : *PortsArr)
			{
				if (!V.IsValid() || V->Type != EJson::Object) continue;
				const TSharedPtr<FJsonObject> O = V->AsObject();
				FIwPortSpec P;
				P.Id = FName(*O->GetStringField(TEXT("id")));
				P.Name = O->GetStringField(TEXT("name"));
				P.Tag = O->GetStringField(TEXT("tag"));
				P.Style = O->GetStringField(TEXT("style"));
				P.Subtitle = O->GetStringField(TEXT("subtitle"));
				P.HarbourmasterNote = O->GetStringField(TEXT("harbourmasterNote"));
				O->TryGetBoolField(TEXT("bMainHarbour"), P.bMainHarbour);
				P.HostIsleIdx = (int32)O->GetIntegerField(TEXT("hostIsleIdx"));
				P.Seed = (int32)O->GetIntegerField(TEXT("seed"));
				P.WorldX = (float)O->GetNumberField(TEXT("worldX"));
				P.WorldY = (float)O->GetNumberField(TEXT("worldY"));
				P.SeawardYaw = (float)O->GetNumberField(TEXT("seawardYaw"));
				P.HillScale = (float)O->GetNumberField(TEXT("hillScale"));
				P.HillHeightMul = (float)O->GetNumberField(TEXT("hillHeightMul"));
				P.MoorLocal = ReadVec2(O, TEXT("moorLocal"), P.MoorLocal);
				P.MoorRadiusM = (float)O->GetNumberField(TEXT("moorRadiusM"));
				P.OffingLocal = ReadVec2(O, TEXT("offingLocal"), P.OffingLocal);
				P.DioramaEyeLocal = ReadVec3(O, TEXT("dioramaEyeLocal"), P.DioramaEyeLocal);
				P.DioramaLookLocal = ReadVec3(O, TEXT("dioramaLookLocal"), P.DioramaLookLocal);
				const TArray<TSharedPtr<FJsonValue>>* MultA = nullptr;
				if (O->TryGetArrayField(TEXT("priceMult"), MultA) && MultA)
					for (int32 g = 0; g < GIwNumGoods && g < MultA->Num(); ++g) P.PriceMult[g] = (float)(*MultA)[g]->AsNumber();
				const TArray<TSharedPtr<FJsonValue>>* StkA = nullptr;
				if (O->TryGetArrayField(TEXT("baseStock"), StkA) && StkA)
					for (int32 g = 0; g < GIwNumGoods && g < StkA->Num(); ++g) P.BaseStock[g] = (int32)(*StkA)[g]->AsNumber();
				const TArray<TSharedPtr<FJsonValue>>* BerthA = nullptr;
				if (O->TryGetArrayField(TEXT("berths"), BerthA) && BerthA)
				{
					for (const TSharedPtr<FJsonValue>& Bv : *BerthA)
					{
						if (!Bv.IsValid() || Bv->Type != EJson::Object) continue;
						const TSharedPtr<FJsonObject> Bo = Bv->AsObject();
						FIwPortBerthSpec B;
						B.Name = Bo->GetStringField(TEXT("name"));
						B.Y0 = (float)Bo->GetNumberField(TEXT("y0"));
						B.XBow = (float)Bo->GetNumberField(TEXT("xBow"));
						B.MaxLenM = (float)Bo->GetNumberField(TEXT("maxLenM"));
						P.Berths.Add(B);
					}
				}
				FinalizePortDerived(P);
				OutPorts.Add(P);
			}
		}
		if (OutGoods.Num() < GIwNumGoods || OutPorts.Num() == 0)
		{
			PopulateFallbackWorldPorts(OutGoods, OutPorts);
			return false;
		}
		UE_LOG(LogIronwake, Display, TEXT("IWP29 loaded %s: %d ports, %d goods"), *Chosen, OutPorts.Num(), OutGoods.Num());
		return true;
	}

	bool IsHostedPortIsle(int32 IsleIdx, const TArray<FIwPortSpec>& Ports, int32* OutPortIdx)
	{
		for (int32 i = 0; i < Ports.Num(); ++i)
		{
			if (!Ports[i].bMainHarbour && Ports[i].HostIsleIdx == IsleIdx)
			{
				if (OutPortIdx) *OutPortIdx = i;
				return true;
			}
		}
		return false;
	}

	// ========================================================================= Parametric Secondary Port Builder
	void BuildSecondaryPort(AIronwakeVoyage* Voy, AIronwakeArchipelago* Arch, FIwPortSpec& Spec)
	{
		if (!Voy || Spec.bMainHarbour) return;
		UWorld* W = Voy->GetWorld();
		if (!W) return;
		(void)Arch;

		const FVector CentreW(Spec.WorldX * 100.f, Spec.WorldY * 100.f, 0.f);
		float SeaZ = 0.f;
		for (int32 i = 0; i < 9; ++i)
			SeaZ += AIronwakeShell::WaterZAt(W, CentreW + FVector((i % 3 - 1) * 4000.f, (i / 3 - 1) * 4000.f, 0.f));
		SeaZ /= 9.f;

		USceneComponent* PortRoot = NewObject<USceneComponent>(Voy, *FString::Printf(TEXT("Port_%s"), *Spec.Id.ToString()));
		PortRoot->SetupAttachment(Voy->GetRootComponent());
		PortRoot->RegisterComponent();
		Voy->AddInstanceComponent(PortRoot);
		PortRoot->SetWorldLocationAndRotation(FVector(CentreW.X, CentreW.Y, SeaZ + AIronwakeShell::RenderedSeaOffsetCm), FRotator(0.f, Spec.SeawardYaw, 0.f));

		const bool bFoundry = Spec.Style.Equals(TEXT("foundry"), ESearchCase::IgnoreCase);
		const float Sc = FMath::Clamp(Spec.HillScale, 0.48f, 0.95f);
		const float HsMul = FMath::Clamp(Spec.HillHeightMul, 0.55f, 1.15f);
		const float Q = bFoundry ? 2.0f : 1.85f;
		const auto M = [](float X, float Y, float Z) { return FVector(X * 100.f, Y * 100.f, Z * 100.f); };
		const auto MatOf = [](const TCHAR* N) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Ironwake/Town/%s.%s"), N, N)); };

		auto SetM = [&](UStaticMeshComponent* C, const TCHAR* N)
		{
			if (!C) return;
			if (UMaterialInterface* Mi = MatOf(N)) C->SetMaterial(0, Mi);
		};
		auto Block = [&](float X0, float X1, float Y0, float Y1, float Z0, float Z1, const TCHAR* Mat, float CullCm = 340000.f)
		{
			UStaticMeshComponent* C = Voy->PortMesh(PortRoot, Voy->CubeMesh, M((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, (Z0 + Z1) * 0.5f),
				FRotator::ZeroRotator, FVector(FMath::Abs(X1 - X0), FMath::Abs(Y1 - Y0), FMath::Abs(Z1 - Z0)));
			if (C) { SetM(C, Mat); if (CullCm > 0.f) C->SetCullDistance(CullCm); }
			return C;
		};
		auto PlaceP = [&](const TCHAR* Nm, float X, float Y, float Z, float Yaw, float S = 1.f, bool bBlock = true, bool bSolid = true, float CullCm = 340000.f)
		{
			UStaticMeshComponent* C = IronwakeWorld::Place(Voy, PortRoot, Nm, M(X, Y, Z), FRotator(0.f, Yaw, 0.f), FVector(S), bBlock, bSolid);
			if (C && CullCm > 0.f) C->SetCullDistance(CullCm);
			return C;
		};
		auto LampP = [&](float X, float Y, float Z, const FLinearColor& Col, float Cd, float RadCm)
		{
			UPointLightComponent* Lp = Voy->PortLamp(PortRoot, M(X, Y, Z), Col, Cd, RadCm);
			if (Lp) { Lp->SetMaxDrawDistance(280000.f); Lp->SetMaxDistanceFadeRange(50000.f); }
			return Lp;
		};

		UIronwakeColl* Co = UIronwakeColl::Get(W);
		const FTransform PtXf = PortRoot->GetComponentTransform();
		auto Solid = [&](float Xc, float Yc, float Hx, float Hy, const TCHAR* Tag)
		{
			if (!Co) return;
			const FVector Wc = PtXf.TransformPosition(M(Xc, Yc, 0.f));
			Co->AddObb(FVector2D(Wc.X, Wc.Y), PtXf.Rotator().Yaw, FVector2D(Hx * 100.f, Hy * 100.f), Wc.Z - 3000.f, Wc.Z + 6000.f, false, Tag);
		};

		// ---- 1) Stepped-bench hillside terrain & switchback roads
		FIwHill Hill;
		Hill.SpanScale = Sc;
		Hill.HeightScale = HsMul;
		Hill.SeedOff = Spec.Seed % 997;
		Hill.bSolidFront = bFoundry;

		const float ApronY = 135.f * Sc;
		Hill.Pads.Add({ -50.f - 44.f * Sc, -50.f, -ApronY, ApronY, Q + 0.12f, 0.45f });
		// Anchor pads for upper landmark (chapel at Ironcrag, windmill at Stigsby)
		const float CrestX = -50.f - 165.f * Sc, CrestY = 18.f * Sc;
		const float CrestZ = Hill.Base(CrestX, CrestY);
		Hill.Pads.Add({ CrestX - 16.f, CrestX + 14.f, CrestY - 18.f, CrestY + 18.f, CrestZ, 1.4f });

		const int32 NBase = IwP23::AddRoadsScaled(Hill, Sc);
		IwP23::AddVerges(Hill, NBase);
		Hill.BakeRoads();

		FP23Plan Plan;
		IwP23::PlanScaled(Hill, Plan, Spec.Seed, NBase, Sc, HsMul);
		for (const FP23House& H : Plan.Houses)
		{
			const float Rad = FMath::DegreesToRadians(H.Phi);
			const float Ex = FMath::Abs(FMath::Cos(Rad)) * H.D + FMath::Abs(FMath::Sin(Rad)) * H.W;
			const float Ey = FMath::Abs(FMath::Sin(Rad)) * H.D + FMath::Abs(FMath::Cos(Rad)) * H.W;
			Hill.Pads.Add({ H.X - Ex * 0.5f - 0.6f, H.X + Ex * 0.5f + 0.6f, H.Y - Ey * 0.5f - 0.6f, H.Y + Ey * 0.5f + 0.6f, H.Z, 1.15f });
		}

		UMaterialInterface* HillM = MatOf(TEXT("MI_IW_Hill"));
		if (HillM)
		{
			if (UMaterialInstanceDynamic* Hd = UMaterialInstanceDynamic::Create(HillM, Voy))
			{
				if (bFoundry)
				{
					Hd->SetVectorParameterValue(TEXT("GrassTint"), FLinearColor(0.24f, 0.31f, 0.18f, 1.f));
					Hd->SetVectorParameterValue(TEXT("EarthTint"), FLinearColor(0.28f, 0.24f, 0.20f, 1.f));
				}
				else
				{
					Hd->SetVectorParameterValue(TEXT("GrassTint"), FLinearColor(0.28f, 0.42f, 0.21f, 1.f));
					Hd->SetVectorParameterValue(TEXT("EarthTint"), FLinearColor(0.35f, 0.29f, 0.22f, 1.f));
				}
				HillM = Hd;
			}
		}
		Hill.BuildTerrain(Voy, PortRoot, HillM);

		UMaterialInstanceDynamic* MainRd = nullptr;
		UMaterialInstanceDynamic* VergeRd = nullptr;
		if (UMaterialInterface* Rm = MatOf(TEXT("MI_IW_Road")))
		{
			MainRd = UMaterialInstanceDynamic::Create(Rm, Voy);
			MainRd->SetVectorParameterValue(TEXT("Tint"), bFoundry ? FLinearColor(0.95f, 0.90f, 0.84f, 1.f) : FLinearColor(1.25f, 1.15f, 0.98f, 1.f));
			MainRd->SetScalarParameterValue(TEXT("DirtAmt"), 0.15f);
			VergeRd = UMaterialInstanceDynamic::Create(Rm, Voy);
			VergeRd->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.20f, 0.15f, 0.11f, 1.f));
		}
		for (int32 k = 0; k < Hill.Roads.Num(); ++k)
		{
			UMaterialInterface* Mr = (k < NBase) ? (MainRd ? (UMaterialInterface*)MainRd : MatOf(TEXT("MI_IW_Road")))
				: (VergeRd ? (UMaterialInterface*)VergeRd : MatOf(TEXT("MI_IW_Path")));
			if (UProceduralMeshComponent* Rb = Hill.BuildRibbon(Voy, PortRoot, Mr, Hill.Roads[k]))
				Rb->SetCullDistance(320000.f);
		}

		// Register merged hillside land solids
		if (Co)
		{
			TArray<FVector4> Rc;
			Hill.LandRects(Rc);
			for (const FVector4& R : Rc)
			{
				const FVector Wc = PtXf.TransformPosition(M((R.X + R.Y) * 0.5f, (R.Z + R.W) * 0.5f, 0.f));
				Co->AddObb(FVector2D(Wc.X, Wc.Y), PtXf.Rotator().Yaw, FVector2D(FMath::Abs(R.Y - R.X) * 50.f + 30.f, FMath::Abs(R.W - R.Z) * 50.f + 30.f),
					Wc.Z - 3000.f, Wc.Z + 6000.f, false, TEXT("quay"));
			}
		}

		// ---- 2) Per-port night windows, lamp bulbs, chimneys, smoke & HISM vegetation/dockhands
		UInstancedStaticMeshComponent* PWinISM = nullptr;
		UInstancedStaticMeshComponent* PBulbISM = nullptr;
		if (Voy->WinMID)
		{
			auto MakeGlow = [&](UStaticMesh* Sm)
			{
				UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(Voy);
				I->SetStaticMesh(Sm); I->SetupAttachment(PortRoot);
				I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
				I->SetMaterial(0, Voy->WinMID); I->SetCullDistance(320000.f);
				I->SetVisibility(false);
				I->RegisterComponent(); Voy->AddInstanceComponent(I);
				Voy->PortGlowISMs.Add(I);
				return I;
			};
			PWinISM = MakeGlow(Voy->CubeMesh);
			PBulbISM = MakeGlow(Voy->SphereMesh);
		}

		UInstancedStaticMeshComponent* ChimIsm = NewObject<UInstancedStaticMeshComponent>(Voy);
		ChimIsm->SetStaticMesh(Voy->CubeMesh); ChimIsm->SetupAttachment(PortRoot);
		ChimIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision); ChimIsm->SetCullDistance(280000.f);
		if (UMaterialInstanceDynamic* Bm = PortShapeTint(Voy, bFoundry ? FLinearColor(0.16f, 0.065f, 0.05f) : FLinearColor(0.22f, 0.095f, 0.065f)))
			ChimIsm->SetMaterial(0, Bm);
		ChimIsm->RegisterComponent(); Voy->AddInstanceComponent(ChimIsm);

		UInstancedStaticMeshComponent* SmkIsm = NewObject<UInstancedStaticMeshComponent>(Voy);
		SmkIsm->SetStaticMesh(Voy->ConeMesh); SmkIsm->SetupAttachment(PortRoot);
		SmkIsm->SetCollisionEnabled(ECollisionEnabled::NoCollision); SmkIsm->SetCastShadow(false); SmkIsm->SetCullDistance(480000.f);
		if (UMaterialInstanceDynamic* DarkSoot = PortShapeTint(Voy, FLinearColor(0.028f, 0.026f, 0.025f)))
			SmkIsm->SetMaterial(0, DarkSoot);
		SmkIsm->RegisterComponent(); Voy->AddInstanceComponent(SmkIsm);

		TMap<FString, UInstancedStaticMeshComponent*> VegIsm;
		auto AddVeg = [&](const TCHAR* Nm, const FVector& LocM, float Yaw, float S, float Sz = 1.f, float LeanY = 0.f, float CullCm = 240000.f)
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("iwnoveg"))) return;
			UInstancedStaticMeshComponent*& I = VegIsm.FindOrAdd(Nm);
			if (!I)
			{
				UStaticMesh* Sm = IronwakeWorld::Mesh(Nm); if (!Sm) return;
				I = NewObject<UInstancedStaticMeshComponent>(Voy);
				I->SetStaticMesh(Sm); I->SetupAttachment(PortRoot);
				I->SetCollisionEnabled(ECollisionEnabled::NoCollision); I->SetCastShadow(false);
				I->SetCullDistance(CullCm);
				I->SetForcedLodModel(1); // Keep LOD0 full leaf canopy cards within port cull radius!
				I->RegisterComponent(); Voy->AddInstanceComponent(I);
				// Match Archipelago::Hism leaf albedo boost so M_IW_Foliage crowns read lush green instead of dark sticks!
				for (int32 mi = 0; mi < I->GetNumMaterials(); ++mi)
					if (UMaterialInterface* Mt = I->GetMaterial(mi))
						if (Mt->GetBaseMaterial() && Mt->GetBaseMaterial()->GetName() == TEXT("M_IW_Foliage"))
							if (UMaterialInstanceDynamic* D = I->CreateAndSetMaterialInstanceDynamic(mi))
								D->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.25f, 2.55f, 1.75f));
			}
			const FQuat Qt = (LeanY != 0.f)
				? (FQuat(FVector(1.f, 0.f, 0.f), FMath::DegreesToRadians(-LeanY)) * FQuat(FRotator(0.f, Yaw, 0.f)))
				: FQuat(FRotator(0.f, Yaw, 0.f));
			I->AddInstance(FTransform(Qt, M(LocM.X, LocM.Y, LocM.Z), FVector(S, S, S * Sz)));
		};

		UHierarchicalInstancedStaticMeshComponent* CrewHism = nullptr;
		if (UStaticMesh* CrewSm = IronwakeWorld::Mesh(TEXT("SailorCrewStand")))
		{
			CrewHism = NewObject<UHierarchicalInstancedStaticMeshComponent>(Voy);
			CrewHism->SetStaticMesh(CrewSm); CrewHism->SetupAttachment(PortRoot);
			CrewHism->SetCollisionEnabled(ECollisionEnabled::NoCollision); CrewHism->SetCastShadow(false);
			CrewHism->InstanceStartCullDistance = 28000; CrewHism->InstanceEndCullDistance = 36000;
			CrewHism->RegisterComponent(); Voy->AddInstanceComponent(CrewHism);
		}
		auto AddCrewGroup = [&](float X, float Y, float Z, int32 N, float BaseYaw)
		{
			if (!CrewHism) return;
			for (int32 i = 0; i < N; ++i)
			{
				const float Ang = FMath::DegreesToRadians(BaseYaw + i * (360.f / FMath::Max(N, 1)));
				const FVector Off(FMath::Cos(Ang) * 0.85f, FMath::Sin(Ang) * 0.85f, 0.f);
				CrewHism->AddInstance(FTransform(FRotator(0.f, BaseYaw + 180.f + i * (360.f / FMath::Max(N, 1)), 0.f), M(X + Off.X, Y + Off.Y, Z), FVector(1.f)));
			}
		};

		// ---- 3) Retaining walls along switchback roads + clustered houses with line-traced facade windows
		FRandomStream Rg(Spec.Seed + 77);
		for (int32 Ri = 0; Ri < NBase; ++Ri)
		{
			const FIwRoad& Rd = Hill.Roads[Ri];
			for (int32 I = 3; I + 4 < Rd.P.Num(); I += 5)
			{
				const FVector2D Pc(Rd.P[I]);
				const FVector2D Tg = (FVector2D(Rd.P[I + 3]) - FVector2D(Rd.P[I - 3])).GetSafeNormal(), Nl(-Tg.Y, Tg.X);
				const float Sd = Hill.Base(Pc.X + Nl.X * 5.f, Pc.Y + Nl.Y * 5.f) < Hill.Base(Pc.X - Nl.X * 5.f, Pc.Y - Nl.Y * 5.f) ? 1.f : -1.f;
				const FVector2D Ed = Pc + Nl * Sd * (Rd.HalfW + 0.45f);
				const float Zrw = Rd.P[I].Z, Zg = Hill.Base(Ed.X + Nl.X * Sd * 2.0f, Ed.Y + Nl.Y * Sd * 2.0f);
				if (Zrw < 2.8f) continue;
				const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Tg.Y, Tg.X));
				if (UStaticMeshComponent* Kb = Voy->PortMesh(PortRoot, Voy->CubeMesh, M(Ed.X, Ed.Y, Zrw + 0.20f), FRotator(0.f, Yaw, 0.f), FVector(7.2f, 0.55f, 0.48f)))
				{ SetM(Kb, TEXT("MI_IW_DarkStone")); Kb->SetCullDistance(250000.f); }
				const float Hh = Zrw - Zg;
				if (Hh >= 0.85f)
				{
					const float Ht = FMath::Min(Hh + 0.55f, 3.6f);
					if (UStaticMeshComponent* Wl = Voy->PortMesh(PortRoot, Voy->CubeMesh, M(Ed.X, Ed.Y, Zrw + 0.22f - Ht * 0.5f), FRotator(0.f, Yaw, 0.f), FVector(7.5f, 0.85f, Ht)))
					{ SetM(Wl, TEXT("MI_IW_DarkStone")); Wl->SetCullDistance(320000.f); }
				}
			}
		}

		// Distinct architectural palette per port style
		static const TCHAR* FoundryHouseTypes[3] = { TEXT("TownRowCottages"), TEXT("TownStoneTownhouse"), TEXT("TownRowCottages") };
		static const TCHAR* FishingHouseTypes[3] = { TEXT("TownFisherCottage"), TEXT("TownTimberHouse"), TEXT("TownRowCottages") };
		const FLinearColor FoundryTints[7] = {
			FLinearColor(0.92f, 0.46f, 0.35f), FLinearColor(0.62f, 0.64f, 0.68f), FLinearColor(0.74f, 0.78f, 0.84f),
			FLinearColor(0.96f, 0.68f, 0.44f), FLinearColor(0.84f, 0.52f, 0.40f), FLinearColor(1.02f, 0.96f, 0.86f), FLinearColor(0.58f, 0.62f, 0.64f)
		};
		const FLinearColor FishingTints[7] = {
			FLinearColor(1.18f, 1.15f, 1.05f), FLinearColor(1.14f, 0.92f, 0.58f), FLinearColor(0.82f, 0.64f, 0.44f),
			FLinearColor(0.82f, 0.88f, 0.94f), FLinearColor(1.06f, 0.98f, 0.78f), FLinearColor(0.92f, 0.60f, 0.46f), FLinearColor(0.78f, 0.90f, 0.82f)
		};

		int32 NHouses = 0, NWin = 0;
		for (const FP23House& H : Plan.Houses)
		{
			const TCHAR* MeshName = bFoundry ? FoundryHouseTypes[H.Type % 3] : FishingHouseTypes[H.Type % 3];
			const bool bFishMesh = FCString::Strcmp(MeshName, TEXT("TownFisherCottage")) == 0;
			const FVector Sc3(H.S, H.S, H.S * H.Sz);
			const float HouseYaw = H.Phi - 90.f + (bFishMesh ? 90.f : 0.f);
			UStaticMeshComponent* C = IronwakeWorld::Place(Voy, PortRoot, MeshName, M(H.X, H.Y, H.Z - 0.12f), FRotator(0.f, HouseYaw, 0.f), Sc3, true, true);
			if (!C) continue;
			C->SetCullDistance(350000.f);
			const FLinearColor BaseTint = (bFoundry ? FoundryTints[H.Tint % 7] : FishingTints[H.Tint % 7]) * (1.68f * Rg.FRandRange(0.92f, 1.06f));
			if (!bFishMesh) IronwakeWorld::TintAll(C, BaseTint);
			++NHouses;

			FVector2D Nn(FMath::Cos(FMath::DegreesToRadians(H.Phi)), FMath::Sin(FMath::DegreesToRadians(H.Phi))), Tt(-Nn.Y, Nn.X);
			float Zlow = H.Z;
			for (int32 a = -1; a <= 1; a += 2)
				for (int32 b = -1; b <= 1; b += 2)
					Zlow = FMath::Min(Zlow, Hill.Base(H.X + Nn.X * a * H.D * 0.5f + Tt.X * b * H.W * 0.5f, H.Y + Nn.Y * a * H.D * 0.5f + Tt.Y * b * H.W * 0.5f));
			if (H.Z - Zlow > 0.35f)
			{
				if (UStaticMeshComponent* Pl = Voy->PortMesh(PortRoot, Voy->CubeMesh, M(H.X, H.Y, (H.Z + Zlow - 2.0f) * 0.5f), FRotator(0.f, H.Phi, 0.f), FVector(H.D + 1.0f, H.W + 1.0f, H.Z - Zlow + 2.1f)))
				{ SetM(Pl, TEXT("MI_IW_DarkStone")); Pl->SetCullDistance(320000.f); }
			}

			if (PWinISM)
			{
				C->UpdateComponentToWorld();
				C->RecreatePhysicsState();
				const FVector FrontW = PtXf.TransformVectorNoScale(FVector(Nn.X, Nn.Y, 0.f)).GetSafeNormal();
				const FVector TanW = PtXf.TransformVectorNoScale(FVector(Tt.X, Tt.Y, 0.f)).GetSafeNormal();
				const int32 Cols = FMath::Max(1, FMath::FloorToInt(H.W / 3.0f));
				const int32 Rows = FMath::Clamp(FMath::FloorToInt((H.H * 0.64f - 1.5f) / 3.f) + 1, 1, 3);
				for (int32 r = 0; r < Rows; ++r)
					for (int32 c = 0; c < Cols; ++c)
					{
						if (Rg.FRand() < 0.28f) continue;
						const float V = (c + 0.5f - Cols * 0.5f) * (H.W / Cols);
						const float Zw = H.Z + 1.85f + 2.9f * r;
						if (Zw > H.Z + H.Ridge - 1.1f) continue;
						const FVector Ctr = C->Bounds.Origin;
						const FVector Lat = Ctr + TanW * V * 100.f;
						const FVector S0 = FVector(Lat.X, Lat.Y, PtXf.GetLocation().Z + Zw * 100.f) + FrontW * 2400.f;
						const FVector E0 = FVector(Lat.X, Lat.Y, PtXf.GetLocation().Z + Zw * 100.f);
						FHitResult Hit; FCollisionQueryParams Qp(NAME_None, true);
						if (!C->LineTraceComponent(Hit, S0, E0, Qp)) continue;
						if (FVector::DotProduct(Hit.ImpactNormal, FrontW) < 0.80f || FMath::Abs(Hit.ImpactNormal.Z) > 0.24f) continue;
						const FVector Wp = FVector(Hit.ImpactPoint) + Hit.ImpactNormal * 3.f;
						const float Yw = FMath::RadiansToDegrees(FMath::Atan2(Hit.ImpactNormal.Y, Hit.ImpactNormal.X));
						PWinISM->AddInstance(FTransform(FRotator(0.f, Yw, 0.f), Wp, FVector(0.04f, 0.78f, 1.16f)) * PtXf.Inverse());
						++NWin;
					}
				// Ensure upper-slope cottages always get at least 2 glowing seaward windows so the whole hillside twinkles at night!
				if (Rg.FRand() < 0.78f)
				{
					const FBoxSphereBounds Lb = C->GetStaticMesh()->GetBounds();
					const float ActualH = (Lb.Origin.Z + Lb.BoxExtent.Z) * 0.01f * Sc3.Z;
					const float WinZ = H.Z + FMath::Clamp(ActualH * 0.42f, 1.45f, 3.2f);
					for (int32 side = -1; side <= 1; side += 2)
					{
						const float Wx = H.X + Nn.X * (H.D * 0.47f) + Tt.X * (side * H.W * 0.24f);
						const float Wy = H.Y + Nn.Y * (H.D * 0.47f) + Tt.Y * (side * H.W * 0.24f);
						PWinISM->AddInstance(FTransform(FRotator(0.f, H.Phi, 0.f), M(Wx, Wy, WinZ), FVector(0.08f, 0.72f, 1.05f))); // local to PortRoot!
						++NWin;
					}
				}
			}

			// Chimney pot anchored strictly to actual mesh roof height (skip TownFisherCottage which already has a built-in chimney!)
			if (!bFishMesh && C->GetStaticMesh())
			{
				const FBoxSphereBounds LocalB = C->GetStaticMesh()->GetBounds();
				const float ActualRoofZ = H.Z - 0.12f + (LocalB.Origin.Z + LocalB.BoxExtent.Z) * 0.01f * Sc3.Z;
				const float Lx = Rg.FRandRange(-0.24f, 0.24f) * H.W;
				const float Top = ActualRoofZ + Rg.FRandRange(0.45f, 0.95f), Bot = FMath::Max(H.Z + 0.5f, ActualRoofZ - 1.6f);
				const float Cx = H.X - Nn.X * H.D * 0.12f + Tt.X * Lx, Cy = H.Y - Nn.Y * H.D * 0.12f + Tt.Y * Lx;
				ChimIsm->AddInstance(FTransform(FRotator::ZeroRotator, M(Cx, Cy, (Top + Bot) * 0.5f), FVector(0.90f, 0.90f, FMath::Max(0.8f, Top - Bot))));
			}
		}

		// Scatter IwP23 cluster-hugging hedges, shrubs, trees & terrace-bank boulders (eliminates bare cut banks!)
		for (const FP23Veg& V : Plan.Veg)
		{
			const float Z = Hill.Final(V.X, V.Y);
			if (V.Kind == 0) AddVeg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(V.X, V.Y, Z - 0.1f), Rg.FRandRange(0.f, 360.f), V.S * 1.15f, 1.f, 0.f, 180000.f);
			else if (V.Kind == 4) AddVeg(Rg.FRand() < 0.5f ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(V.X, V.Y, Z - 0.1f), 90.f + Rg.FRandRange(-25.f, 25.f), V.S * 1.15f, 0.68f, 8.f, 180000.f);
			else if (V.Kind == 1) AddVeg(TEXT("IslandTree02"), FVector(V.X, V.Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), V.S * 1.75f, 0.88f, 10.f, 260000.f);
			else if (V.Kind == 2) AddVeg(TEXT("IslandTree01"), FVector(V.X, V.Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), V.S * 1.85f, 0.90f, 8.f, 260000.f);
			else if (UStaticMeshComponent* Brk = IronwakeWorld::Place(Voy, PortRoot, Rg.FRand() < 0.5f ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03"),
				M(V.X, V.Y, Z - 0.45f - 1.5f * V.S), FRotator(Rg.FRandRange(-8.f, 8.f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(-8.f, 8.f)),
				FVector(V.S, V.S * Rg.FRandRange(0.85f, 1.25f), V.S * Rg.FRandRange(0.75f, 1.15f)), false, false))
			{
				Brk->SetCullDistance(260000.f);
			}
		}

		// Calibrated warm gas lamps (low candelas & tight radius so discrete cottage windows shine crisply at night without whiteout!)
		auto AddGasLamp = [&](float X, float Y, float Z, float Cd = 280.f, float BulbS = 0.16f)
		{
			PlaceP(TEXT("GasLamp"), X, Y, Z, 0.f, 1.f, false, false, 240000.f);
			LampP(X, Y, Z + 4.35f, FLinearColor(1.f, 0.64f, 0.28f), Cd, 1100.f);
			if (PBulbISM) PBulbISM->AddInstance(FTransform(FRotator::ZeroRotator, M(X, Y, Z + 4.30f), FVector(BulbS)));
		};
		for (int32 Ri = 0; Ri < FMath::Min(NBase, 2); ++Ri)
		{
			const FIwRoad& Rd = Hill.Roads[Ri];
			for (int32 I = 8; I + 1 < Rd.P.Num(); I += (Ri == 0 ? 15 : 22))
			{
				const FVector2D Tg = (FVector2D(Rd.P[I + 1]) - FVector2D(Rd.P[I - 1])).GetSafeNormal(), Nl(-Tg.Y, Tg.X);
				const float Lx = Rd.P[I].X + Nl.X * (Rd.HalfW + 1.1f), Ly = Rd.P[I].Y + Nl.Y * (Rd.HalfW + 1.1f);
				AddGasLamp(Lx, Ly, Hill.Final(Lx, Ly) + 0.1f, 260.f, 0.16f);
			}
		}

		// ---- 4) Flank headland cliffs, shoreline boulders & breakwaters (leaving [-68, +68] m deep mouth open!)
		PlaceP(bFoundry ? TEXT("CoastalCliff04") : TEXT("CoastalCliff02"), -135.f * Sc - 40.f, -240.f * Sc, -5.5f, -118.f, 3.1f * Sc + 0.8f, true, true, 0.f);
		PlaceP(bFoundry ? TEXT("NamaCliff02") : TEXT("CoastalCliff01"), -130.f * Sc - 40.f, 245.f * Sc, -5.5f, -62.f, 3.0f * Sc + 0.8f, true, true, 0.f);
		PlaceP(TEXT("CoastRocks02"), -115.f * Sc - 35.f, -185.f * Sc, -2.0f, 22.f, 1.85f, true, true, 0.f);
		PlaceP(TEXT("CoastRocks01"), -115.f * Sc - 35.f, 190.f * Sc, -2.0f, 74.f, 1.80f, true, true, 0.f);
		// Shoreline riprap boulders softening both ends of the seawall
		PlaceP(TEXT("CoastLandRocks02"), -52.f, -96.f, -2.2f, 18.f, 2.6f, true, true, 300000.f);
		PlaceP(TEXT("CoastLandRocks03"), -52.f,  96.f, -2.2f, -42.f, 2.5f, true, true, 300000.f);

		auto BuildBreakwater = [&](float CX, float CY, float YawDeg, float LenM)
		{
			const float Yr = FMath::DegreesToRadians(YawDeg);
			int32 k = 0;
			for (float t = -LenM * 0.5f; t <= LenM * 0.5f + 1.f; t += 22.f, ++k)
			{
				const float X = CX + FMath::Cos(Yr) * t, Y = CY + FMath::Sin(Yr) * t;
				const float J = Rg.FRandRange(-11.f, 11.f), S = Rg.FRandRange(0.90f, 1.15f);
				const TCHAR* Rn = (k % 2 == 0) ? TEXT("CoastLandRocks02") : TEXT("CoastLandRocks03");
				UStaticMeshComponent* Rc = IronwakeWorld::Place(Voy, PortRoot, Rn, M(X, Y, -3.6f),
					FRotator(0.f, YawDeg + (k % 2 == 0 ? -90.f : 0.f) + J, 0.f), FVector(3.8f, 3.2f, 4.8f) * S, true, true);
				(void)Rc;
			}
		};
		BuildBreakwater(42.f, -114.f, 14.f, 150.f);
		BuildBreakwater(38.f, 114.f, -14.f, 145.f);

		// Harbour mouth red (port-hand entering = -Y) & green (+Y) navigation beacons (crisp maritime night beacons!)
		{
			const FLinearColor RedGlow(3.8f, 0.28f, 0.14f), GrnGlow(0.22f, 3.6f, 0.48f);
			PlaceP(TEXT("CoastRocks03"), 112.f, -94.f, -1.5f, 35.f, 0.95f, true, true, 0.f);
			PlaceP(TEXT("SignalTower"), 112.f, -94.f, 1.8f, 195.f, 0.85f, true, true, 380000.f);
			Voy->PortMesh(PortRoot, Voy->SphereMesh, M(112.f, -94.f, 26.8f), FRotator::ZeroRotator, FVector(1.15f), &RedGlow);
			LampP(112.f, -94.f, 26.8f, FLinearColor(1.f, 0.18f, 0.10f), 2600.f, 6500.f);

			PlaceP(TEXT("CoastRocks03"), 106.f, 94.f, -1.5f, 125.f, 0.85f, true, true, 0.f);
			PlaceP(TEXT("GasLamp"), 106.f, 94.f, 1.3f, 0.f, 2.2f, false, false, 380000.f);
			Voy->PortMesh(PortRoot, Voy->SphereMesh, M(106.f, 94.f, 11.6f), FRotator::ZeroRotator, FVector(1.05f), &GrnGlow);
			LampP(106.f, 94.f, 11.6f, FLinearColor(0.22f, 1.f, 0.35f), 2200.f, 5800.f);
		}

		// Moored swell-bobbing vessel helper (returns root static mesh at calibrated boot-top waterline!)
		auto AddFloater = [&](const TCHAR* Nm, float X, float Y, float KeelZ, float Yaw, float S = 1.f) -> UStaticMeshComponent*
		{
			if (UStaticMeshComponent* C = PlaceP(Nm, X, Y, KeelZ, Yaw, S, false, false, 340000.f))
			{
				Voy->Floaters.Add(C);
				Voy->FloaterBase.Add(FVector(KeelZ * 100.f, Yaw, Voy->Floaters.Num() * 1.73f));
				return C;
			}
			return nullptr;
		};

		// ---- 5) Style-specific Harbourfront, Landmarks & Vegetation
		const FLinearColor WetBand(0.012f, 0.018f, 0.016f), TimberPile(0.05f, 0.035f, 0.025f);
		if (bFoundry)
		{
			// === IRONCRAG COALING & FOUNDRY HAVEN ===
			Block(-50.f, -88.f, -98.f, 98.f, -3.5f, Q + 0.15f, TEXT("MI_IW_DarkStone"), 0.f);
			Block(-53.f, -88.f, -96.f, 96.f, Q + 0.14f, Q + 0.20f, TEXT("MI_IW_Setts"), 280000.f);
			Block(-49.85f, -53.2f, -98.f, 98.f, Q + 0.15f, Q + 0.28f, TEXT("MI_IW_Cobble"), 280000.f); // bevelled granite quay coping kerb
			Voy->PortMesh(PortRoot, Voy->CubeMesh, M(-49.88f, 0.f, 0.55f), FRotator::ZeroRotator, FVector(0.18f, 196.f, 2.2f), &WetBand);
			for (float Y = -92.f; Y <= 92.f; Y += 5.5f)
				if (FMath::Abs(Y + 38.f) > 5.f && FMath::Abs(Y - 52.f) > 5.f)
					if (UStaticMeshComponent* Pf = Voy->PortMesh(PortRoot, Voy->CylMesh, M(-50.35f, Y, 0.1f), FRotator::ZeroRotator, FVector(0.44f, 0.44f, 5.8f), &TimberPile))
						Pf->SetCullDistance(220000.f);

			// 85 m heavy stone & timber-fendered Coaling Pier at Y = -38 m (X = -50 .. +35 m) + north Iron Jetty at Y = +52 m (X = -50 .. -8 m)
			Block(-50.f, 35.f, -42.2f, -33.8f, -4.5f, Q + 0.16f, TEXT("MI_IW_DarkStone"), 0.f);
			Block(-50.f, 35.f, -41.5f, -34.5f, Q + 0.16f, Q + 0.25f, TEXT("MI_IW_Setts"), 280000.f);
			Voy->PortMesh(PortRoot, Voy->CubeMesh, M(-7.5f, -38.f, 0.50f), FRotator::ZeroRotator, FVector(85.2f, 8.6f, 2.0f), &WetBand);
			for (float X = -44.f; X <= 32.f; X += 6.5f)
				for (int32 Side = -1; Side <= 1; Side += 2)
					if (UStaticMeshComponent* Pp = Voy->PortMesh(PortRoot, Voy->CylMesh, M(X, -38.f + Side * 4.35f, 0.1f), FRotator::ZeroRotator, FVector(0.40f, 0.40f, 5.6f), &TimberPile))
						Pp->SetCullDistance(200000.f);
			Solid(-7.5f, -38.f, 43.f, 4.7f, TEXT("pier"));

			Block(-50.f, -8.f, 48.8f, 55.2f, -4.5f, Q + 0.16f, TEXT("MI_IW_DarkStone"), 0.f);
			Block(-50.f, -8.f, 49.3f, 54.7f, Q + 0.16f, Q + 0.23f, TEXT("MI_IW_Deck"), 280000.f);
			Solid(-29.f, 52.f, 21.5f, 3.7f, TEXT("jetty"));

			// Foundry, Smelting Works, Coaling Station, Gasometer, Harbourmaster Tavern & 3x Steam Cranes
			PlaceP(TEXT("TownFoundryV4"), -74.f, -18.f, Q + 0.16f, 0.f, 1.08f, true, true, 0.f);
			PlaceP(TEXT("VictorianFactory"), -92.f, -52.f, Q + 0.2f, 90.f, 0.95f, true, true, 0.f);
			PlaceP(TEXT("CoalingStation"), -63.f, -58.f, Q + 0.16f, 90.f, 1.05f, true, true, 380000.f);
			PlaceP(TEXT("DockWarehouse"), -74.f, 68.f, Q + 0.16f, 0.f, 0.95f, true, true, 380000.f);
			PlaceP(TEXT("TownGasometer"), -102.f, -82.f, Hill.Final(-102.f, -82.f) - 0.15f, 15.f, 0.92f, true, true, 0.f);

			// Harbourmaster & Miners' Tavern overlooking the quay square (framed by Ironcrag diorama camera!)
			PlaceP(TEXT("TownTavernV4"), -74.f, 22.f, Q + 0.16f, 0.f, 1.f, true, true, 360000.f);
			PlaceP(TEXT("SignAnchorTavern"), -69.58f, 22.f, Q + 3.64f, 0.f, 1.f, false, false, 180000.f);
			PlaceP(TEXT("TownCustomsHouse"), -66.f, 44.f, Q + 0.20f, -90.f, 0.92f, true, true, 380000.f);
			LampP(-68.5f, 22.f, Q + 3.8f, FLinearColor(1.f, 0.72f, 0.36f), 420.f, 1600.f);

			// 3x Steam Cranes along the coaling pier & foundry wharf
			PlaceP(TEXT("SteamCrane"), -12.f, -38.f, Q + 0.22f, 90.f, 1.05f, true, false, 380000.f);
			PlaceP(TEXT("SteamCrane"), 18.f, -38.f, Q + 0.22f, -90.f, 1.0f, true, false, 380000.f);
			PlaceP(TEXT("SteamCrane"), -52.f, 6.f, Q + 0.20f, 0.f, 1.05f, true, false, 380000.f);

			// Upper miners' stone chapel bedded on dark-stone retaining terrace + churchyard pines
			Block(CrestX - 13.f, CrestX + 11.f, CrestY - 14.f, CrestY + 14.f, CrestZ - 3.6f, CrestZ + 0.12f, TEXT("MI_IW_DarkStone"), 340000.f);
			PlaceP(TEXT("TownChapel"), CrestX, CrestY, CrestZ + 0.08f, -90.f, 0.92f, true, true, 0.f);
			LampP(CrestX + 5.f, CrestY, CrestZ + 7.f, FLinearColor(1.f, 0.72f, 0.38f), 450.f, 1800.f);

			// 3 tall blast-furnace brick stacks with dark coal-soot smoke drift & HDR emissive orange stack-throat + hearth glow!
			const FLinearColor DarkBrick(0.14f, 0.055f, 0.045f), CoalBlack(0.016f, 0.016f, 0.020f);
			UMaterialInstanceDynamic* ForgeGlowMat = nullptr;
			if (UMaterialInterface* Wm = MatOf(TEXT("MI_IW_Window")))
			{
				ForgeGlowMat = UMaterialInstanceDynamic::Create(Wm, Voy);
				ForgeGlowMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(8.5f, 1.8f, 0.18f, 1.f));
			}
			const FVector StackLocs[3] = { { -80.f, -26.f, Q }, { -84.f, -10.f, Q }, { -96.f, -46.f, Q + 1.f } };
			for (int32 s = 0; s < 3; ++s)
			{
				const float Sh = 19.f + 2.5f * s;
				Voy->PortMesh(PortRoot, Voy->CylMesh, M(StackLocs[s].X, StackLocs[s].Y, StackLocs[s].Z + Sh * 0.5f), FRotator::ZeroRotator, FVector(2.1f, 2.1f, Sh), &DarkBrick);
				// Hearth arch ember + stack-lip furnace throat collar (emissive MI_IW_Window so orange furnace glow reads day & night!)
				if (UStaticMeshComponent* Ha = Voy->PortMesh(PortRoot, Voy->SphereMesh, M(StackLocs[s].X + 1.9f, StackLocs[s].Y, StackLocs[s].Z + 1.8f), FRotator::ZeroRotator, FVector(1.5f, 2.0f, 1.6f)))
					if (ForgeGlowMat) Ha->SetMaterial(0, ForgeGlowMat);
				if (UStaticMeshComponent* StC = Voy->PortMesh(PortRoot, Voy->CylMesh, M(StackLocs[s].X, StackLocs[s].Y, StackLocs[s].Z + Sh + 0.35f), FRotator::ZeroRotator, FVector(2.28f, 2.28f, 0.85f)))
					if (ForgeGlowMat) StC->SetMaterial(0, ForgeGlowMat);
				LampP(StackLocs[s].X + 3.4f, StackLocs[s].Y, StackLocs[s].Z + 2.8f, FLinearColor(1.f, 0.30f, 0.05f), 2200.f, 2800.f);
				LampP(StackLocs[s].X, StackLocs[s].Y, StackLocs[s].Z + Sh + 1.2f, FLinearColor(1.f, 0.34f, 0.06f), 1600.f, 2200.f);
				for (int32 q = 0; q < 7; ++q)
				{
					const float Hq = StackLocs[s].Z + Sh + 1.6f + 2.1f * q;
					const float Rq = 0.85f + 0.32f * q;
					SmkIsm->AddInstance(FTransform(FRotator(18.f, 112.f + q * 19.f, 12.f), M(StackLocs[s].X - 0.75f * q, StackLocs[s].Y + 1.85f * q, Hq), FVector(Rq, Rq * 1.15f, Rq * 0.75f)));
				}
			}

			// Bunker coal mounds, coaling pier cargo & quay clutter
			const FVector2D CoalPiles[6] = { { -58.f, -44.f }, { -62.f, -32.f }, { -56.f, -70.f }, { -4.f, -38.f }, { 12.f, -38.f }, { -57.f, -6.f } };
			for (int32 c = 0; c < 6; ++c)
				if (UStaticMeshComponent* Cp = Voy->PortMesh(PortRoot, Voy->SphereMesh, M(CoalPiles[c].X, CoalPiles[c].Y, Q + 0.1f), FRotator(0.f, c * 37.f, 0.f), FVector(5.2f, 3.8f, 2.0f), &CoalBlack))
					Cp->SetCullDistance(240000.f);

			for (float Y = -84.f; Y <= 84.f; Y += 22.f)
			{
				PlaceP(TEXT("BollardCapstan"), -51.3f, Y, Q + 0.18f, 90.f, 1.f, false, false, 200000.f);
				if (FMath::Fmod(Y + 90.f, 44.f) < 22.f) AddGasLamp(-53.2f, Y + 6.f, Q + 0.18f, 280.f, 0.16f);
			}
			for (float X = -36.f; X <= 28.f; X += 16.f)
			{
				PlaceP(TEXT("BollardCapstan"), X, -41.2f, Q + 0.24f, 0.f, 1.f, false, false, 200000.f);
				PlaceP((FMath::Abs(X) < 12.f) ? TEXT("TownHandcart") : TEXT("MilitaryCrate"), X + 3.f, -36.2f, Q + 0.24f, 15.f, 1.f, false, false, 200000.f);
				if (FMath::Abs(X + 4.f) < 10.f || X > 20.f) AddGasLamp(X + 4.f, -35.0f, Q + 0.24f, 260.f, 0.15f);
			}
			for (int32 k = 0; k < 16; ++k)
			{
				const float Y = -82.f + k * 10.5f;
				if (FMath::Abs(Y - 22.f) < 6.f) continue;
				PlaceP((k % 3 == 0) ? TEXT("WoodenBarrels") : ((k % 3 == 1) ? TEXT("TownCrateStack") : TEXT("TownHandcart")),
					-57.f - (k % 2) * 2.5f, Y, Q + 0.18f, Rg.FRandRange(-35.f, 35.f), (k % 3 == 0) ? 1.3f : 1.0f, false, false, 220000.f);
			}
			AddCrewGroup(-54.f, -16.f, Q + 0.18f, 3, 20.f);
			AddCrewGroup(-68.f, 22.f, Q + 0.18f, 3, -15.f);
			AddCrewGroup(4.f, -38.f, Q + 0.24f, 2, 90.f);

			// West-bluff Basalt & Oxblood Lighthouse (seaward-shielded lantern + sweeping beam so hillside stays dark at night!)
			{
				const float Lx = -96.f, Ly = -132.f, Lz = FMath::Max(Hill.Final(Lx, Ly), 8.5f);
				const FLinearColor Basalt(0.26f, 0.27f, 0.29f), Ox(0.34f, 0.05f, 0.04f), IronC(0.03f, 0.03f, 0.035f), Glass(2.6f, 1.8f, 0.75f);
				Block(Lx - 5.5f, Lx + 5.5f, Ly - 5.5f, Ly + 5.5f, Lz - 3.5f, Lz + 1.5f, TEXT("MI_IW_DarkStone"), 0.f);
				Voy->PortMesh(PortRoot, Voy->CylMesh, M(Lx, Ly, Lz + 12.5f), FRotator::ZeroRotator, FVector(5.2f, 5.2f, 22.f), &Basalt);
				Voy->PortMesh(PortRoot, Voy->CylMesh, M(Lx, Ly, Lz + 13.5f), FRotator::ZeroRotator, FVector(5.35f, 5.35f, 4.2f), &Ox);
				Voy->PortMesh(PortRoot, Voy->CylMesh, M(Lx, Ly, Lz + 23.8f), FRotator::ZeroRotator, FVector(6.4f, 6.4f, 0.8f), &IronC);
				Voy->PortMesh(PortRoot, Voy->CylMesh, M(Lx, Ly, Lz + 25.3f), FRotator::ZeroRotator, FVector(2.6f, 2.6f, 2.3f), &Glass);
				Voy->PortMesh(PortRoot, Voy->ConeMesh, M(Lx, Ly, Lz + 28.0f), FRotator::ZeroRotator, FVector(3.3f, 3.3f, 3.2f), &IronC);
				LampP(Lx + 3.0f, Ly, Lz + 25.3f, FLinearColor(1.f, 0.80f, 0.46f), 2800.f, 4500.f);
				if (PBulbISM) PBulbISM->AddInstance(FTransform(FRotator::ZeroRotator, M(Lx, Ly, Lz + 25.3f), FVector(1.6f)));
				Solid(Lx, Ly, 6.5f, 6.5f, TEXT("lighthouse"));
			}

			// Craggy basalt outcrops along the upper ridge breaking the hill silhouette!
			for (int32 r = 0; r < 5; ++r)
			{
				const float Rx = -155.f * Sc - 42.f - r * 8.f, Ry = (-110.f + r * 55.f) * Sc;
				PlaceP((r & 1) ? TEXT("CoastLandRocks02") : TEXT("Boulder01"), Rx, Ry, Hill.Final(Rx, Ry) - 1.2f, r * 47.f, 2.4f, false, false, 340000.f);
			}

			// Bustling Coaling Fleet moored at Ironcrag Pier & Basin (50m Collier Steamer, Harbour Tug, Salvage Tender, Coal Lighter & Launches!)
			AddFloater(TEXT("CoastalSteamer"), -4.f, -49.5f, -2.55f, 0.f, 0.86f);
			Solid(-4.f, -49.5f, 21.f, 4.8f, TEXT("moored_collier"));
			AddFloater(TEXT("HarbourTug"), 14.f, -28.5f, -2.15f, 180.f, 1.f);
			Solid(14.f, -28.5f, 11.f, 4.2f, TEXT("moored_tug"));
			AddFloater(TEXT("CoalLighter"), -24.f, 42.2f, -0.95f, 0.f, 1.f);
			Solid(-24.f, 42.2f, 14.f, 5.2f, TEXT("moored_lighter"));
			AddFloater(TEXT("SteamPinnace"), -38.f, -18.f, -0.85f, 172.f, 1.f);
			AddFloater(TEXT("Rowboat"), -44.f, 34.f, -0.30f, 80.f, 1.f);

			// Lush Wind-Sheared Coastal Woodland & Shrubs (~220 full-crown trees + scrub!)
			const TCHAR* BroadF[3] = { TEXT("IslandTree01"), TEXT("IslandTree02"), TEXT("IslandTree03") };
			for (int32 k = 0, Nt = 0; k < 6500 && Nt < 220; ++k)
			{
				const float X = Rg.FRandRange(-50.f - 220.f * Sc, -50.f - 42.f * Sc), Y = Rg.FRandRange(-185.f * Sc, 195.f * Sc);
				const float Z = Hill.Final(X, Y);
				if (Z < 3.2f || Hill.RoadDist(X, Y) < 2.8f || Hill.PadDist(X, Y) < 3.8f || Hill.SlopeDeg(X, Y) > 32.f) continue;
				const float Pick = Rg.FRand();
				if (Pick < 0.62f) AddVeg(BroadF[k % 3], FVector(X, Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.85f, 2.85f), Rg.FRandRange(0.82f, 0.96f), Rg.FRandRange(8.f, 16.f), 280000.f);
				else if (Pick < 0.80f) AddVeg(TEXT("FirTree01"), FVector(X, Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.15f, 1.85f), 1.f, 0.f, 280000.f);
				else AddVeg((k & 1) ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(X, Y, Z - 0.1f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.9f, 3.3f), 0.85f, 8.f, 180000.f);
				++Nt;
			}
		}
		else
		{
			// === STIGSBY FISHING & TIMBER OUTPOST ===
			// Irregular stepped cobble quay + natural cove shoreline with boulders & wooden slipway
			Block(-48.f, -84.f, -88.f, -4.f, -3.2f, Q + 0.14f, TEXT("MI_IW_DarkStone"), 0.f);
			Block(-50.f, -84.f, -4.f, 44.f, -3.2f, Q + 0.08f, TEXT("MI_IW_Cobble"), 0.f);
			Block(-48.2f, -56.f, -88.f, 44.f, Q + 0.13f, Q + 0.22f, TEXT("MI_IW_Deck"), 280000.f);
			Block(-62.f, -86.f, 44.f, 92.f, -2.5f, Q + 0.12f, TEXT("MI_IW_Cobble"), 0.f); // sheltered cove plaza
			// Timber boat slipway sloping into the cove water at Y = 46..54 m
			if (UStaticMeshComponent* Slip = Voy->PortMesh(PortRoot, Voy->CubeMesh, M(-54.f, 49.f, 0.65f), FRotator(12.f, 0.f, 0.f), FVector(16.f, 7.5f, 0.45f)))
			{ SetM(Slip, TEXT("MI_IW_Deck")); Slip->SetCullDistance(240000.f); }
			PlaceP(TEXT("CoastLandRocks02"), -49.f, 44.f, -2.0f, 35.f, 1.9f, true, true, 260000.f);
			PlaceP(TEXT("CoastLandRocks03"), -49.f, -86.f, -2.0f, -25.f, 2.1f, true, true, 260000.f);

			Voy->PortMesh(PortRoot, Voy->CubeMesh, M(-47.88f, -22.f, 0.50f), FRotator::ZeroRotator, FVector(0.18f, 132.f, 2.1f), &WetBand);
			for (float Y = -84.f; Y <= 42.f; Y += 5.0f)
				if (FMath::Abs(Y + 26.f) > 4.f && FMath::Abs(Y - 32.f) > 4.f)
					if (UStaticMeshComponent* Pf = Voy->PortMesh(PortRoot, Voy->CylMesh, M(-48.35f, Y, 0.1f), FRotator::ZeroRotator, FVector(0.40f, 0.40f, 5.8f), &TimberPile))
						Pf->SetCullDistance(220000.f);

			// Twin ModularPier timber finger wharves at true deck elevation (Q - 3.62 m -> deck stands 1.85 m proud on piles!)
			for (int32 i = 0; i < 5; ++i) PlaceP(TEXT("ModularPier"), -41.f + 15.2f * i, -26.f, Q - 3.62f, 90.f, 1.f, true, false, 360000.f);
			Solid(-10.f, -26.f, 41.f, 4.2f, TEXT("pier"));
			for (int32 i = 0; i < 4; ++i) PlaceP(TEXT("ModularPier"), -41.f + 15.2f * i, 32.f, Q - 3.62f, 90.f, 1.f, true, false, 360000.f);
			Solid(-18.f, 32.f, 33.f, 4.2f, TEXT("pier"));

			// Pier-deck crates, barrels, bollards & dockhands so both timber piers read bustling and alive!
			for (int32 i = 0; i < 4; ++i)
			{
				const float Px = -32.f + 15.2f * i;
				PlaceP(TEXT("BollardCapstan"), Px, -28.2f, Q + 0.04f, 0.f, 0.9f, false, false, 200000.f);
				PlaceP((i % 2 == 0) ? TEXT("WoodenBarrels") : TEXT("TownCrateStack"), Px + 4.5f, -24.6f, Q + 0.04f, i * 28.f, 0.95f, false, false, 200000.f);
				if (i < 3) PlaceP(TEXT("WoodenBarrels"), Px + 2.f, 33.4f, Q + 0.04f, -20.f * i, 1.0f, false, false, 200000.f);
			}

			// 3 Pile-Supported Stilt Boathouses & Fisher Cottages standing over the cove & flank shallows!
			struct FStilt { float X, Y, Yaw; const TCHAR* Nm; };
			const FStilt Stilts[3] = {
				{ -52.f, 58.f, 0.f,   TEXT("TownBoathouse") },
				{ -56.f, 76.f, -8.f,  TEXT("TownFisherCottage") },
				{ -50.f, -74.f, 12.f, TEXT("TownBoathouse") },
			};
			for (const FStilt& St : Stilts)
			{
				const float Zp = Q + 0.55f;
				Block(St.X - 5.4f, St.X + 5.4f, St.Y - 5.2f, St.Y + 5.2f, Zp - 0.28f, Zp, TEXT("MI_IW_Deck"), 280000.f);
				for (int32 a = -1; a <= 1; a += 2)
					for (int32 b = -1; b <= 1; b += 2)
						if (UStaticMeshComponent* Pl = Voy->PortMesh(PortRoot, Voy->CylMesh, M(St.X + a * 4.5f, St.Y + b * 4.4f, Zp - 3.2f), FRotator::ZeroRotator, FVector(0.42f, 0.42f, 6.4f), &TimberPile))
							Pl->SetCullDistance(240000.f);
				PlaceP(St.Nm, St.X, St.Y, Zp, St.Yaw, 0.96f, true, true, 340000.f);
			}

			// 6x Cod-Drying Fish Racks along foreshore + Harbourfront Tavern, Market Hall & Market Stalls
			const float RackY[6] = { -62.f, -50.f, -8.f, 48.f, 64.f, 84.f };
			for (int32 r = 0; r < 6; ++r)
				PlaceP(TEXT("TownFishRacks"), (r >= 3 ? -66.f : -55.f), RackY[r], Q + 0.16f, (r % 2) ? 8.f : -6.f, 1.12f, false, false, 240000.f);

			PlaceP(TEXT("TownTavernV4"), -66.f, 6.f, Q + 0.16f, 0.f, 1.02f, true, true, 360000.f);
			PlaceP(TEXT("SignAnchorTavern"), -61.50f, 6.f, Q + 3.64f, 0.f, 1.f, false, false, 180000.f);
			LampP(-60.5f, 6.f, Q + 3.8f, FLinearColor(1.f, 0.74f, 0.38f), 420.f, 1600.f);

			PlaceP(TEXT("TownMarketHall"), -68.f, -34.f, Q + 0.16f, 0.f, 1.0f, true, true, 360000.f);
			PlaceP(TEXT("WharfBuilding"), -64.f, -14.f, Q + 0.16f, 0.f, 0.85f, true, true, 360000.f);
			for (int32 m = 0; m < 4; ++m)
				PlaceP(TEXT("TownMarketStall"), -57.f, -20.f + 5.2f * m, Q + 0.18f, -90.f + (m % 2 ? 4.f : -4.f), 1.f, false, false, 200000.f);

			// Stacked fresh-sawn Naval Timber baulk piles on the timber wharf
			const FLinearColor SawnPine(0.56f, 0.40f, 0.21f);
			for (int32 p = 0; p < 4; ++p)
			{
				const float Px = -56.f - (p % 2) * 3.5f, Py = 18.f + p * 6.5f;
				for (int32 row = 0; row < 3; ++row)
					if (UStaticMeshComponent* Tb = Voy->PortMesh(PortRoot, Voy->CubeMesh, M(Px, Py, Q + 0.38f + row * 0.42f), FRotator(0.f, (row % 2) * 3.f, 0.f), FVector(5.4f, 1.6f - row * 0.18f, 0.36f), &SawnPine))
						Tb->SetCullDistance(220000.f);
			}

			for (float Y = -72.f; Y <= 40.f; Y += 22.f)
			{
				PlaceP(TEXT("BollardCapstan"), -49.2f, Y, Q + 0.20f, 90.f, 1.f, false, false, 200000.f);
				if (FMath::Fmod(Y + 80.f, 44.f) < 22.f) AddGasLamp(-50.8f, Y + 5.f, Q + 0.18f, 340.f, 0.20f);
			}
			AddGasLamp(-14.f, -24.3f, Q + 0.04f, 380.f, 0.22f);
			AddGasLamp(18.f,  -24.3f, Q + 0.04f, 380.f, 0.22f);
			AddGasLamp(-6.f,   33.7f, Q + 0.04f, 380.f, 0.22f);
			AddGasLamp(12.f,   33.7f, Q + 0.04f, 380.f, 0.22f);
			for (int32 k = 0; k < 12; ++k)
			{
				const float Y = -68.f + k * 9.5f;
				PlaceP((k % 2 == 0) ? TEXT("WoodenBarrels") : TEXT("TownCrateStack"), -53.5f, Y, Q + 0.20f, Rg.FRandRange(-30.f, 30.f), (k % 2 == 0) ? 1.32f : 0.95f, false, false, 220000.f);
			}
			AddCrewGroup(-52.f, 4.f, Q + 0.20f, 3, 15.f);
			AddCrewGroup(-54.f, -28.f, Q + 0.20f, 3, 65.f);
			AddCrewGroup(2.f, -26.f, Q + 0.08f, 2, -40.f);

			// Ridge-Crest Windmill anchored on a dry-stone plinth with Live Spinning Sail Cross!
			Block(CrestX - 9.5f, CrestX + 9.5f, CrestY - 9.5f, CrestY + 9.5f, CrestZ - 3.4f, CrestZ + 0.12f, TEXT("MI_IW_DarkStone"), 340000.f);
			if (UStaticMeshComponent* Wb = PlaceP(TEXT("WindmillBody"), CrestX, CrestY, CrestZ + 0.05f, 0.f, 1.f, true, true, 0.f))
			{
				USceneComponent* Pv = NewObject<USceneComponent>(Voy);
				Pv->SetupAttachment(Wb);
				Pv->SetRelativeLocation(FVector(805.f, -9.f, 1641.f));
				Pv->RegisterComponent(); Voy->AddInstanceComponent(Pv);
				if (UStaticMesh* SailSm = IronwakeWorld::Mesh(TEXT("WindmillBody_sails")))
				{
					UStaticMeshComponent* ScM = NewObject<UStaticMeshComponent>(Voy);
					ScM->SetStaticMesh(SailSm); ScM->SetupAttachment(Pv);
					ScM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					ScM->RegisterComponent(); Voy->AddInstanceComponent(ScM);
				}
				AIronwakeNpcDirector::Windmills.Add(Pv);
				LampP(CrestX + 6.f, CrestY, CrestZ + 4.5f, FLinearColor(1.f, 0.72f, 0.38f), 420.f, 1600.f);
			}

			// Bustling Fishing Smack, Cutter & Launch Fleet in Stigsby Cove at calibrated hull waterlines!
			AddFloater(TEXT("RevenueCutter"), 6.f, -34.2f, -1.20f, 3.f, 1.f);
			Solid(6.f, -34.2f, 12.f, 3.2f, TEXT("moored_cutter"));
			AddFloater(TEXT("FishingSmack"), -18.f, -18.5f, 0.12f, 8.f, 1.15f);
			AddFloater(TEXT("FishingSmack"), -16.f,  23.5f, 0.12f, -6.f, 1.18f);
			AddFloater(TEXT("FishingSmack"), 8.f,    40.2f, 0.12f, 184.f, 1.12f);
			AddFloater(TEXT("SteamPinnace"), -28.f, 40.5f, -0.75f, 4.f, 1.f);
			AddFloater(TEXT("Rowboat"), -42.f, 48.f, -0.20f, 72.f, 1.f);

			// Dense Lush Broadleaf & Conifer Forest Canopy (~420 full-LOD0 vibrant green crowns + understory shrubs!)
			const TCHAR* Broadleaf[3] = { TEXT("IslandTree01"), TEXT("IslandTree02"), TEXT("IslandTree03") };
			for (int32 k = 0, Nt = 0; k < 11000 && Nt < 420; ++k)
			{
				const float X = Rg.FRandRange(-50.f - 238.f * Sc, -50.f - 34.f * Sc), Y = Rg.FRandRange(-202.f * Sc, 210.f * Sc);
				const float Z = Hill.Final(X, Y);
				if (Z < 2.5f || Hill.RoadDist(X, Y) < 2.6f || Hill.PadDist(X, Y) < 3.4f || Hill.SlopeDeg(X, Y) > 34.f) continue;
				const float Pick = Rg.FRand();
				if (Pick < 0.74f) AddVeg(Broadleaf[k % 3], FVector(X, Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(2.15f, 3.35f), Rg.FRandRange(0.86f, 1.04f), Rg.FRandRange(4.f, 12.f), 280000.f);
				else if (Pick < 0.88f) AddVeg(TEXT("FirTree01"), FVector(X, Y, Z - 0.15f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(1.25f, 2.05f), 1.f, 0.f, 280000.f);
				else AddVeg((k & 1) ? TEXT("Shrub02") : TEXT("Shrub03"), FVector(X, Y, Z - 0.1f), Rg.FRandRange(0.f, 360.f), Rg.FRandRange(2.0f, 3.6f), 0.90f, 0.f, 180000.f);
				++Nt;
			}
		}

		// Spatialised harbour dock lapping ambience
		if (Voy->HarbourSnd)
		{
			USoundAttenuation* Att = NewObject<USoundAttenuation>(Voy);
			Att->Attenuation.bAttenuate = true;
			Att->Attenuation.bSpatialize = true;
			Att->Attenuation.AttenuationShapeExtents = FVector(16000.f, 0.f, 0.f);
			Att->Attenuation.FalloffDistance = 140000.f;
			UGameplayStatics::SpawnSoundAtLocation(Voy, Voy->HarbourSnd, Spec.MoorWorldCm + FVector(0.f, 0.f, 600.f), FRotator::ZeroRotator, 1.4f, 0.97f, 0.f, Att, nullptr, false);
		}

		UE_LOG(LogIronwake, Display, TEXT("IWP29 built port '%s' (%s) at (%.0f, %.0f) m yaw=%.0f houses=%d windows=%d berths=%d"),
			*Spec.Name, *Spec.Style, Spec.WorldX, Spec.WorldY, Spec.SeawardYaw, NHouses, NWin, Spec.Berths.Num());
	}

	// ========================================================================= Procedural Harbourmaster Bell (Audio-Route Safe)
	void PlayHarbourBell(UWorld* W)
	{
		if (!W || FParse::Param(FCommandLine::Get(), TEXT("nosound")) || !FApp::CanEverRenderAudio()) return;
		const float Gain = (IronwakeAudio::GetVol(IronwakeAudio::Ship) / 100.f) * (IronwakeAudio::GetVol(IronwakeAudio::Master) / 100.f);
		if (Gain <= 0.005f) return;

		constexpr int32 SR = 24000;
		constexpr float DurS = 1.35f;
		const int32 NumSamples = FMath::RoundToInt(SR * DurS);
		TArray<int16> Pcm;
		Pcm.SetNumZeroed(NumSamples);
		for (int32 i = 0; i < NumSamples; ++i)
		{
			const float t = (float)i / SR;
			auto Strike = [](float dt) -> float
			{
				if (dt < 0.f) return 0.f;
				const float env1 = FMath::Exp(-dt * 4.2f);
				const float env2 = FMath::Exp(-dt * 9.5f);
				return env1 * (0.52f * FMath::Sin(2.f * PI * 587.33f * dt) + 0.28f * FMath::Sin(2.f * PI * 1174.66f * dt))
					 + env2 * (0.18f * FMath::Sin(2.f * PI * 1410.0f * dt) + 0.12f * FMath::Sin(2.f * PI * 2093.0f * dt));
			};
			const float s = (Strike(t) + 0.92f * Strike(t - 0.28f)) * Gain * 0.42f;
			Pcm[i] = (int16)FMath::Clamp(FMath::RoundToInt(s * 32767.f), -32760, 32760);
		}
		USoundWaveProcedural* Bell = NewObject<USoundWaveProcedural>(GetTransientPackage());
		Bell->SetSampleRate(SR);
		Bell->NumChannels = 1;
		Bell->Duration = DurS;
		Bell->SoundGroup = SOUNDGROUP_Effects;
		Bell->bLooping = false;
		Bell->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
		UGameplayStatics::PlaySound2D(W, Bell, 1.f);
	}

	// ========================================================================= Economy, Cargo Hold & Persistent Logbook
	int32 GetCrowns() { return ReadCfgInt(TEXT("Crowns"), 250); }
	void SetCrowns(int32 Crowns) { WriteCfgInt(TEXT("Crowns"), FMath::Max(0, Crowns)); }

	int32 HullCargoCapTons(FName ShipId)
	{
		if (ShipId == TEXT("cutter")) return 12;
		if (ShipId == TEXT("brig")) return 140;
		if (ShipId == TEXT("ironclad")) return 60;
		return 90; // frigate (Albatross) default
	}

	void GetCargoHold(int32 OutTons[GIwNumGoods])
	{
		for (int32 g = 0; g < GIwNumGoods; ++g)
			OutTons[g] = FMath::Max(0, ReadCfgInt(*FString::Printf(TEXT("Hold_%d"), g), 0));
	}

	void GetCargoAvgCost(int32 OutAvgCr[GIwNumGoods])
	{
		for (int32 g = 0; g < GIwNumGoods; ++g)
			OutAvgCr[g] = FMath::Max(0, ReadCfgInt(*FString::Printf(TEXT("HoldAvg_%d"), g), 0));
	}

	int32 TotalCargoTons()
	{
		int32 Tons[GIwNumGoods];
		GetCargoHold(Tons);
		int32 Sum = 0;
		for (int32 g = 0; g < GIwNumGoods; ++g) Sum += Tons[g];
		return Sum;
	}

	int32 UnitPriceAt(const FIwPortSpec& Port, const FIwGoodDef& Good, int32 GoodIdx, bool bBuying)
	{
		const int32 g = FMath::Clamp(GoodIdx, 0, GIwNumGoods - 1);
		const float Scarcity = FMath::Clamp(1.f + 0.22f * float(Port.BaseStock[g] - Port.LiveStock[g]) / FMath::Max(Port.BaseStock[g], 10), 0.74f, 1.42f);
		const float Raw = Good.BasePrice * Port.PriceMult[g] * Scarcity;
		return bBuying ? FMath::Max(1, FMath::RoundToInt(Raw * 1.02f))
					   : FMath::Max(1, FMath::RoundToInt(Raw * 0.98f));
	}

	bool BuyCargo(FIwPortSpec& Port, const TArray<FIwGoodDef>& Goods, int32 GoodIdx, int32 Tons, FName ShipId, FString& OutMsg)
	{
		if (!Goods.IsValidIndex(GoodIdx) || Tons <= 0) return false;
		const int32 Cap = HullCargoCapTons(ShipId);
		const int32 FreeTons = Cap - TotalCargoTons();
		if (FreeTons <= 0) { OutMsg = FString::Printf(TEXT("Hold is full (%d / %d tons)."), Cap, Cap); return false; }
		if (Port.LiveStock[GoodIdx] <= 0) { OutMsg = FString::Printf(TEXT("%s has no %s left on the quay."), *Port.Name, *Goods[GoodIdx].Name); return false; }

		int32 CurCrowns = GetCrowns();
		int32 Hold[GIwNumGoods], Avg[GIwNumGoods];
		GetCargoHold(Hold); GetCargoAvgCost(Avg);

		int32 Bought = 0, Spent = 0;
		const int32 MaxReq = FMath::Min3(Tons, FreeTons, Port.LiveStock[GoodIdx]);
		for (int32 k = 0; k < MaxReq; ++k)
		{
			const int32 P = UnitPriceAt(Port, Goods[GoodIdx], GoodIdx, true);
			if (CurCrowns < P) break;
			CurCrowns -= P;
			Spent += P;
			Port.LiveStock[GoodIdx]--;
			Bought++;
		}
		if (Bought <= 0)
		{
			OutMsg = FString::Printf(TEXT("Insufficient Silver Crowns (need %d cr for 1 t of %s)."), UnitPriceAt(Port, Goods[GoodIdx], GoodIdx, true), *Goods[GoodIdx].Name);
			return false;
		}
		const int32 PrevTons = Hold[GoodIdx];
		const int32 NewTons = PrevTons + Bought;
		const int32 NewAvg = FMath::RoundToInt(float(PrevTons * Avg[GoodIdx] + Spent) / FMath::Max(NewTons, 1));
		WriteCfgInt(*FString::Printf(TEXT("Hold_%d"), GoodIdx), NewTons, false);
		WriteCfgInt(*FString::Printf(TEXT("HoldAvg_%d"), GoodIdx), NewAvg, false);
		SetCrowns(CurCrowns);
		OutMsg = FString::Printf(TEXT("Loaded %d %s %s at %s for %d Silver Crowns (Hold %d / %d t)."),
			Bought, *Goods[GoodIdx].Unit, *Goods[GoodIdx].Name, *Port.Name, Spent, TotalCargoTons(), Cap);
		UE_LOG(LogIronwake, Display, TEXT("IWP29 BUY port=%s good=%s tons=%d spent=%d crowns=%d hold=%d/%d"),
			*Port.Name, *Goods[GoodIdx].Name, Bought, Spent, CurCrowns, TotalCargoTons(), Cap);
		return true;
	}

	int32 SellCargo(FIwPortSpec& Port, const TArray<FIwGoodDef>& Goods, int32 GoodIdx, int32 Tons, FString& OutMsg)
	{
		if (!Goods.IsValidIndex(GoodIdx) || Tons <= 0) return 0;
		int32 Hold[GIwNumGoods], Avg[GIwNumGoods];
		GetCargoHold(Hold); GetCargoAvgCost(Avg);
		const int32 SellN = FMath::Min(Tons, Hold[GoodIdx]);
		if (SellN <= 0) { OutMsg = FString::Printf(TEXT("No %s in your cargo hold."), *Goods[GoodIdx].Name); return 0; }

		int32 Earned = 0;
		for (int32 k = 0; k < SellN; ++k)
		{
			Earned += UnitPriceAt(Port, Goods[GoodIdx], GoodIdx, false);
			Port.LiveStock[GoodIdx]++;
		}
		const int32 CostBasis = SellN * Avg[GoodIdx];
		const int32 Profit = Earned - CostBasis;
		const int32 Rem = Hold[GoodIdx] - SellN;
		WriteCfgInt(*FString::Printf(TEXT("Hold_%d"), GoodIdx), Rem, false);
		if (Rem <= 0) WriteCfgInt(*FString::Printf(TEXT("HoldAvg_%d"), GoodIdx), 0, false);
		WriteCfgInt(TEXT("CareerTonsDelivered"), GetCargoDeliveredTons() + SellN, false);
		WriteCfgInt(TEXT("CareerTradeProfit"), GetNetTradeProfit() + Profit, false);
		const int32 NewCrowns = GetCrowns() + Earned;
		SetCrowns(NewCrowns);
		OutMsg = FString::Printf(TEXT("Discharged %d %s %s at %s for %d Silver Crowns (%s%d cr net)."),
			SellN, *Goods[GoodIdx].Unit, *Goods[GoodIdx].Name, *Port.Name, Earned, Profit >= 0 ? TEXT("+") : TEXT(""), Profit);
		UE_LOG(LogIronwake, Display, TEXT("IWP29 SELL port=%s good=%s tons=%d earned=%d profit=%d crowns=%d hold=%d"),
			*Port.Name, *Goods[GoodIdx].Name, SellN, Earned, Profit, NewCrowns, TotalCargoTons());
		return Earned;
	}

	void AddDistanceMeters(float Meters)
	{
		if (Meters <= 0.f) return;
		const int32 AddTenths = FMath::FloorToInt((Meters / 1852.f) * 10.f);
		if (AddTenths > 0) WriteCfgInt(TEXT("CareerMilesTenths"), ReadCfgInt(TEXT("CareerMilesTenths"), 0) + AddTenths);
	}
	float GetNauticalMiles() { return ReadCfgInt(TEXT("CareerMilesTenths"), 0) * 0.1f; }
	int32 GetCargoDeliveredTons() { return ReadCfgInt(TEXT("CareerTonsDelivered"), 0); }
	int32 GetNetTradeProfit() { return ReadCfgInt(TEXT("CareerTradeProfit"), 0); }
	int32 GetDockedPortMask() { return ReadCfgInt(TEXT("PortDockedMask"), 0); }
	void RecordPortDocked(int32 PortIdx) { if (PortIdx >= 0 && PortIdx < 16) WriteCfgInt(TEXT("PortDockedMask"), GetDockedPortMask() | (1 << PortIdx)); }
	int32 GetSalvagedMask() { return ReadCfgInt(TEXT("SalvagedMask"), 0); }

	bool RecordPoiDiscoveryOrSalvage(int32 PoiIdx, const FString& PoiName, const FString& Tag, const FString& ClockStr, FString& OutBanner)
	{
		const int32 Mask = GetSalvagedMask();
		const int32 Bit = (PoiIdx >= 0 && PoiIdx < 30) ? (1 << PoiIdx) : 0;
		const bool bFirst = (Bit != 0) && ((Mask & Bit) == 0);
		const bool bWreck = (Tag == TEXT("W") || Tag == TEXT("D") || PoiName.Contains(TEXT("Wreck")) || PoiName.Contains(TEXT("Shoal")));
		const bool bLight = (Tag == TEXT("L") || Tag == TEXT("M") || PoiName.Contains(TEXT("Light")) || PoiName.Contains(TEXT("Morrow")));

		if (bFirst)
		{
			WriteCfgInt(TEXT("SalvagedMask"), Mask | Bit, false);
			const int32 Reward = bWreck ? 65 : (bLight ? 35 : 25);
			SetCrowns(GetCrowns() + Reward);
			const FString Detail = bWreck
				? FString::Printf(TEXT("Salvaged brass fittings & casks (+%d Silver Crowns)"), Reward)
				: (bLight ? FString::Printf(TEXT("Logged lighthouse bearings (+%d Silver Crowns Admiralty bounty)"), Reward)
						  : FString::Printf(TEXT("Charted island anchorage & soundings (+%d Silver Crowns survey fee)"), Reward));
			AppendLog(ClockStr, PoiName, Detail);
			OutBanner = Detail;
			return true;
		}
		OutBanner = FString::Printf(TEXT("Anchored off %s — entry verified in Captain's Logbook."), *PoiName);
		return false;
	}

	void AppendLog(const FString& ClockStr, const FString& Place, const FString& Detail)
	{
		TArray<FIwLogEntry> Cur;
		GetLogEntries(Cur);
		FIwLogEntry E; E.Stamp = ClockStr; E.Place = Place; E.Detail = Detail;
		Cur.Insert(E, 0);
		if (Cur.Num() > 16) Cur.SetNum(16);
		WriteCfgInt(TEXT("LogCount"), Cur.Num(), false);
		for (int32 i = 0; i < Cur.Num(); ++i)
		{
			const FString Packed = FString::Printf(TEXT("%s|%s|%s"), *Cur[i].Stamp, *Cur[i].Place, *Cur[i].Detail);
			WriteCfgStr(*FString::Printf(TEXT("Log_%d"), i), Packed, i == Cur.Num() - 1);
		}
	}

	void GetLogEntries(TArray<FIwLogEntry>& OutEntries)
	{
		OutEntries.Reset();
		const int32 N = FMath::Clamp(ReadCfgInt(TEXT("LogCount"), 0), 0, 16);
		for (int32 i = 0; i < N; ++i)
		{
			const FString Raw = ReadCfgStr(*FString::Printf(TEXT("Log_%d"), i));
			TArray<FString> Parts;
			Raw.ParseIntoArray(Parts, TEXT("|"), false);
			if (Parts.Num() >= 3) OutEntries.Add({ Parts[0], Parts[1], Parts[2] });
		}
	}

	FString RankTitle()
	{
		const int32 Score = GetCrowns() + FMath::Max(0, GetNetTradeProfit()) + GetCargoDeliveredTons() * 4;
		if (Score >= 3000) return TEXT("ELDER BRETHREN");
		if (Score >= 1400) return TEXT("MERCHANT COMMODORE");
		if (Score >= 600)  return TEXT("PACKET COMMANDER");
		return TEXT("COASTAL MASTER");
	}

	FName ConsumePendingPortSpawn()
	{
		const FString S = ReadCfgStr(TEXT("PendingPortSpawn"));
		if (!S.IsEmpty()) WriteCfgStr(TEXT("PendingPortSpawn"), TEXT(""));
		return S.IsEmpty() ? NAME_None : FName(*S);
	}
	void SetPendingPortSpawn(FName PortId) { WriteCfgStr(TEXT("PendingPortSpawn"), PortId.IsNone() ? TEXT("") : PortId.ToString()); }

	void ResetEconomyForTest(TArray<FIwPortSpec>& Ports)
	{
		for (int32 g = 0; g < GIwNumGoods; ++g)
		{
			WriteCfgInt(*FString::Printf(TEXT("Hold_%d"), g), 0, false);
			WriteCfgInt(*FString::Printf(TEXT("HoldAvg_%d"), g), 0, false);
		}
		WriteCfgInt(TEXT("CareerTonsDelivered"), 0, false);
		WriteCfgInt(TEXT("CareerTradeProfit"), 0, false);
		WriteCfgInt(TEXT("CareerMilesTenths"), 0, false);
		WriteCfgInt(TEXT("PortDockedMask"), 0, false);
		WriteCfgInt(TEXT("SalvagedMask"), 0, false);
		WriteCfgInt(TEXT("LogCount"), 0, false);
		SetCrowns(250);
		for (FIwPortSpec& P : Ports)
			for (int32 g = 0; g < GIwNumGoods; ++g) P.LiveStock[g] = P.BaseStock[g];
	}
}

// ============================================================================ UIronwakeDockWidget (DPI-Scaled Pure C++ UMG)
namespace
{
	const FLinearColor DkBrass(0.88f, 0.75f, 0.46f, 1.f);
	const FLinearColor DkGold(1.00f, 0.86f, 0.42f, 1.f);
	const FLinearColor DkGreen(0.42f, 0.90f, 0.68f, 1.f);
	const FLinearColor DkSteel(0.74f, 0.78f, 0.83f, 1.f);
	const FLinearColor DkDim(0.55f, 0.60f, 0.65f, 1.f);
	const FLinearColor DkBtn(0.08f, 0.10f, 0.12f, 0.94f);
	const FLinearColor DkBtnSel(0.28f, 0.22f, 0.09f, 0.98f);
	const FLinearColor DkBtnFoc(0.46f, 0.37f, 0.15f, 1.00f);
	constexpr float DockCardW = 1040.f;
	constexpr float DockMarginL = 44.f, DockMarginT = 34.f;

	FSlateFontInfo DockBold(int32 Sz, int32 Spacing = 0)
	{
		FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Sz);
		F.LetterSpacing = Spacing;
		return F;
	}
}

void UIronwakeDockProxy::Clicked()
{
	if (UIronwakeDockWidget* O = Owner.Get()) O->OnAction(Id);
}

UButton* UIronwakeDockWidget::MakeBtn(UPanelWidget* Parent, const FString& Label, int32 Id, int32 FontSize, FMargin Pad, UTextBlock** OutText)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetBackgroundColor(DkBtn);
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(Label));
	T->SetFont(DockBold(FontSize));
	T->SetColorAndOpacity(FSlateColor(DkBrass));
	T->SetJustification(ETextJustify::Center);
	B->AddChild(T);
	if (UButtonSlot* BS = Cast<UButtonSlot>(T->Slot)) { BS->SetPadding(Pad); BS->SetHorizontalAlignment(HAlign_Center); }
	UPanelSlot* S = Parent->AddChild(B);
	if (UVerticalBoxSlot* VS = Cast<UVerticalBoxSlot>(S)) { VS->SetPadding(FMargin(0.f, 3.f)); VS->SetHorizontalAlignment(HAlign_Fill); }
	else if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(S)) { HS->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f)); HS->SetVerticalAlignment(VAlign_Center); }
	UIronwakeDockProxy* P = NewObject<UIronwakeDockProxy>(this);
	P->Owner = this; P->Id = Id;
	B->OnClicked.AddDynamic(P, &UIronwakeDockProxy::Clicked);
	Proxies.Add(P);
	Btns.Add(B); BtnIds.Add(Id);
	if (OutText) *OutText = T;
	return B;
}

TSharedRef<SWidget> UIronwakeDockWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
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
		CardBox->SetWidthOverride(DockCardW);
		CardBox->SetMaxDesiredHeight(1010.f);
		if (UOverlaySlot* OS = Ov->AddChildToOverlay(CardBox))
		{
			OS->SetHorizontalAlignment(HAlign_Left);
			OS->SetVerticalAlignment(VAlign_Top);
			OS->SetPadding(FMargin(DockMarginL, DockMarginT, 0.f, DockMarginT));
		}

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.010f, 0.014f, 0.020f, 0.88f), 6.f, FLinearColor(0.62f, 0.50f, 0.27f, 0.70f), 2.f));
		Panel->SetPadding(FMargin(28.f, 18.f, 28.f, 18.f));
		CardBox->SetContent(Panel);

		UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Panel->SetContent(Col);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TitleText->SetFont(DockBold(32, 110));
		TitleText->SetColorAndOpacity(FSlateColor(DkBrass));
		Col->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));

		SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SubtitleText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 15));
		SubtitleText->SetColorAndOpacity(FSlateColor(DkSteel));
		Col->AddChildToVerticalBox(SubtitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

		QuoteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		QuoteText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 13));
		QuoteText->SetColorAndOpacity(FSlateColor(DkDim));
		QuoteText->SetAutoWrapText(true);
		QuoteText->SetWrapTextAt(940.f);
		Col->AddChildToVerticalBox(QuoteText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

		UBorder* StatBar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		StatBar->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.03f, 0.04f, 0.05f, 0.92f), 4.f, FLinearColor(0.45f, 0.36f, 0.20f, 0.55f), 1.f));
		StatBar->SetPadding(FMargin(14.f, 6.f));
		PurseHoldText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PurseHoldText->SetFont(DockBold(15));
		PurseHoldText->SetColorAndOpacity(FSlateColor(DkGold));
		StatBar->SetContent(PurseHoldText);
		Col->AddChildToVerticalBox(StatBar)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

		// Tab Bar
		UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Col->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		static const TCHAR* TabLabels[3] = { TEXT("1. CARGO & MAIL"), TEXT("2. CAPTAIN'S LOGBOOK"), TEXT("3. SHIPYARD & REPAIRS") };
		for (int32 t = 0; t < 3; ++t)
		{
			UButton* TB = MakeBtn(Tabs, TabLabels[t], Act_Tab0 + t, 15, FMargin(14.f, 6.f));
			if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(TB->Slot)) HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		MakeBtn(Tabs, TEXT("CAST OFF (Esc)"), Act_Close, 15, FMargin(16.f, 6.f));

		CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		CardScroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
		CardScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
		Col->AddChildToVerticalBox(CardScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CardScroll->AddChild(Host);
		for (int32 t = 0; t < 3; ++t)
		{
			UVerticalBox* Pg = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Host->AddChildToVerticalBox(Pg);
			TabPanels.Add(Pg);
		}

		auto AddColCell = [&](UHorizontalBox* Row, float Width, const FString& Txt, const FSlateFontInfo& Fnt, const FLinearColor& Colr, float RightPad = 8.f) -> UTextBlock*
		{
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			SB->SetWidthOverride(Width);
			UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			TB->SetText(FText::FromString(Txt));
			TB->SetFont(Fnt);
			TB->SetColorAndOpacity(FSlateColor(Colr));
			SB->SetContent(TB);
			if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(Row->AddChild(SB)))
			{
				HS->SetVerticalAlignment(VAlign_Center);
				HS->SetPadding(FMargin(0.f, 0.f, RightPad, 0.f));
			}
			return TB;
		};

		// ---- TAB 0: 8-Good Cargo & Mail Contracts
		{
			UVerticalBox* Mkt = TabPanels[TabMarket];
			UHorizontalBox* Hdr = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Mkt->AddChildToVerticalBox(Hdr)->SetPadding(FMargin(4.f, 0.f, 0.f, 6.f));
			AddColCell(Hdr, 250.f, TEXT("COMMODITY"), DockBold(13), DkDim);
			AddColCell(Hdr, 88.f,  TEXT("STOCK"),     DockBold(13), DkDim);
			AddColCell(Hdr, 150.f, TEXT("BUY / SELL"),DockBold(13), DkDim);
			AddColCell(Hdr, 118.f, TEXT("IN HOLD"),   DockBold(13), DkDim);
			AddColCell(Hdr, 270.f, TEXT("MANIFEST ORDERS"), DockBold(13), DkDim, 0.f);

			for (int32 g = 0; g < GIwNumGoods; ++g)
			{
				UBorder* RowB = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
				RowB->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.022f, 0.028f, 0.036f, (g & 1) ? 0.72f : 0.45f), 3.f, FLinearColor::Transparent, 0.f));
				RowB->SetPadding(FMargin(6.f, 4.f));
				Mkt->AddChildToVerticalBox(RowB)->SetPadding(FMargin(0.f, 2.f));

				UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				RowB->SetContent(Row);
				GoodNameTexts.Add(AddColCell(Row, 246.f, TEXT("-"), DockBold(15), DkBrass));
				GoodStockTexts.Add(AddColCell(Row, 88.f, TEXT("-"), FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 14), DkSteel));
				GoodPriceTexts.Add(AddColCell(Row, 150.f, TEXT("-"), DockBold(14), DkSteel));
				GoodHoldTexts.Add(AddColCell(Row, 118.f, TEXT("-"), DockBold(14), DkGold));

				MakeBtn(Row, TEXT("+1"),   Act_Buy1Base + g,    14, FMargin(11.f, 4.f));
				MakeBtn(Row, TEXT("+10"),  Act_Buy10Base + g,   14, FMargin(11.f, 4.f));
				MakeBtn(Row, TEXT("MAX"),  Act_BuyMaxBase + g,  14, FMargin(11.f, 4.f));
				MakeBtn(Row, TEXT("SELL"), Act_SellAllBase + g, 14, FMargin(12.f, 4.f));
			}

			TradeStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			TradeStatusText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Italic"), 14));
			TradeStatusText->SetColorAndOpacity(FSlateColor(DkGreen));
			TradeStatusText->SetAutoWrapText(true);
			TradeStatusText->SetWrapTextAt(940.f);
			Mkt->AddChildToVerticalBox(TradeStatusText)->SetPadding(FMargin(4.f, 8.f, 0.f, 2.f));
		}

		// ---- TAB 1: Captain's Logbook
		{
			UVerticalBox* Lg = TabPanels[TabLogbook];
			CareerSummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			CareerSummaryText->SetFont(DockBold(15));
			CareerSummaryText->SetColorAndOpacity(FSlateColor(DkSteel));
			CareerSummaryText->SetAutoWrapText(true);
			CareerSummaryText->SetWrapTextAt(940.f);
			Lg->AddChildToVerticalBox(CareerSummaryText)->SetPadding(FMargin(4.f, 2.f, 0.f, 10.f));

			UTextBlock* Lh = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Lh->SetText(FText::FromString(TEXT("CHRONOLOGICAL VOYAGE & HARBOURMASTER ENTRIES")));
			Lh->SetFont(DockBold(14, 70));
			Lh->SetColorAndOpacity(FSlateColor(DkBrass));
			Lg->AddChildToVerticalBox(Lh)->SetPadding(FMargin(4.f, 2.f, 0.f, 6.f));

			LogEntriesText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			LogEntriesText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 13));
			LogEntriesText->SetColorAndOpacity(FSlateColor(DkSteel));
			LogEntriesText->SetAutoWrapText(true);
			LogEntriesText->SetWrapTextAt(940.f);
			Lg->AddChildToVerticalBox(LogEntriesText)->SetPadding(FMargin(4.f, 0.f, 0.f, 4.f));
		}

		// ---- TAB 2: Shipyard & Hull Repairs
		{
			UVerticalBox* Sy = TabPanels[TabShipyard];
			UHorizontalBox* RepRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Sy->AddChildToVerticalBox(RepRow)->SetPadding(FMargin(4.f, 2.f, 0.f, 10.f));
			HullRepairText = AddColCell(RepRow, 640.f, TEXT("HULL INTEGRITY: 100%"), DockBold(15), DkGreen, 16.f);
			MakeBtn(RepRow, TEXT("REPAIR & CAREEN HULL (25 cr)"), Act_Repair, 14, FMargin(16.f, 6.f));

			const TArray<FIronwakeShipDef>& Roster = IronwakeFleet::Roster();
			for (int32 i = 0; i < Roster.Num() && i < 6; ++i)
			{
				UBorder* Sb = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
				Sb->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.025f, 0.032f, 0.040f, 0.78f), 4.f, FLinearColor(0.42f, 0.34f, 0.18f, 0.45f), 1.f));
				Sb->SetPadding(FMargin(12.f, 7.f));
				Sy->AddChildToVerticalBox(Sb)->SetPadding(FMargin(0.f, 3.f));

				UHorizontalBox* Sr = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				Sb->SetContent(Sr);
				USizeBox* InfoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				InfoBox->SetWidthOverride(730.f);
				UVerticalBox* Iv = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
				InfoBox->SetContent(Iv);
				UTextBlock* St = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				St->SetFont(DockBold(16));
				St->SetColorAndOpacity(FSlateColor(DkBrass));
				Iv->AddChildToVerticalBox(St);
				UTextBlock* Sd = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Sd->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 13));
				Sd->SetColorAndOpacity(FSlateColor(DkSteel));
				Sd->SetAutoWrapText(true);
				Sd->SetWrapTextAt(710.f);
				Iv->AddChildToVerticalBox(Sd)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
				if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(Sr->AddChild(InfoBox)))
				{
					HS->SetVerticalAlignment(VAlign_Center);
					HS->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
				}
				UTextBlock* BtnLbl = nullptr;
				MakeBtn(Sr, TEXT("TAKE COMMAND"), Act_ShipBase + i, 14, FMargin(14.f, 7.f), &BtnLbl);
				ShipTitleTexts.Add(St);
				ShipDescTexts.Add(Sd);
				ShipBtnTexts.Add(BtnLbl);
			}
		}
		Refresh();
	}
	return Super::RebuildWidget();
}

void UIronwakeDockWidget::FitToViewport()
{
	UWorld* W = GetWorld();
	UGameViewportClient* VC = W ? W->GetGameViewport() : nullptr;
	if (!VC || !RootScale || !CardBox) return;
	FVector2D VP;
	VC->GetViewportSize(VP);
	if (VP.X < 1.f || VP.Y < 1.f) return;
	if (VP.Equals(LastVP, 0.5f)) return;
	LastVP = VP;
	CurScale = FMath::Clamp(FMath::Min(VP.X / 1920.f, VP.Y / 1080.f), 0.5f, 1.5f);
	CurArea = VP / CurScale;
	RootScale->SetUserSpecifiedScale(CurScale);
	CardBox->SetWidthOverride(FMath::Min(DockCardW, CurArea.X - DockMarginL - 24.f));
	CardBox->SetMaxDesiredHeight(FMath::Max(240.f, CurArea.Y - 2.f * DockMarginT));
}

void UIronwakeDockWidget::SetupForPort(AIronwakeVoyage* InVoy, int32 InPortIdx, int32 StartTab, const FString& InitialStatus)
{
	Voy = InVoy;
	PortIdx = InPortIdx;
	CustomTitle.Empty(); CustomSubtitle.Empty(); CustomNote.Empty();
	ActiveTab = FMath::Clamp(StartTab, 0, 2);
	LastTradeStatus = InitialStatus.IsEmpty()
		? TEXT("Select [+1], [+10], or [MAX] to load cargo into your hold; [SELL] discharges cargo for Silver Crowns.")
		: InitialStatus;
	Refresh();
}

void UIronwakeDockWidget::SetupForLandmark(AIronwakeVoyage* InVoy, const FString& Title, const FString& Sub, const FString& Note, const FString& StatusLine)
{
	Voy = InVoy;
	PortIdx = -1;
	CustomTitle = Title; CustomSubtitle = Sub; CustomNote = Note;
	ActiveTab = TabLogbook;
	LastTradeStatus = StatusLine;
	Refresh();
}

void UIronwakeDockWidget::OpenTab(int32 InTab)
{
	if (PortIdx < 0 && InTab != TabLogbook) InTab = TabLogbook;
	ActiveTab = FMath::Clamp(InTab, 0, 2);
	Refresh();
}

void UIronwakeDockWidget::Refresh()
{
	AIronwakeVoyage* V = Voy.Get();
	const bool bInPort = V && V->WorldPorts.IsValidIndex(PortIdx);
	const FName CurShipId = IronwakeFleet::GetSelected();
	const FIronwakeShipDef* CurShipDef = IronwakeFleet::Find(CurShipId);
	const FString ShipName = CurShipDef ? CurShipDef->Name.ToUpper() : CurShipId.ToString().ToUpper();
	const int32 Cap = IwPorts::HullCargoCapTons(CurShipId);
	const int32 Used = IwPorts::TotalCargoTons();

	if (TitleText)
		TitleText->SetText(FText::FromString(bInPort ? FString::Printf(TEXT("PORT OF %s"), *V->WorldPorts[PortIdx].Name.ToUpper()) : CustomTitle));
	if (SubtitleText)
		SubtitleText->SetText(FText::FromString(bInPort ? V->WorldPorts[PortIdx].Subtitle : CustomSubtitle));
	if (QuoteText)
		QuoteText->SetText(FText::FromString(bInPort ? FString::Printf(TEXT("\"%s\""), *V->WorldPorts[PortIdx].HarbourmasterNote) : CustomNote));
	if (PurseHoldText)
		PurseHoldText->SetText(FText::FromString(FString::Printf(TEXT("PURSE: %d Silver Crowns   |   SHIP: %s   |   HOLD: %d / %d TONS   |   %s"),
			IwPorts::GetCrowns(), *ShipName, Used, Cap, *IwPorts::RankTitle())));

	for (int32 t = 0; t < TabPanels.Num(); ++t)
		if (TabPanels[t]) TabPanels[t]->SetVisibility(t == ActiveTab ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	for (int32 i = 0; i < Btns.Num(); ++i)
		if (BtnIds[i] == Act_Tab0 || BtnIds[i] == Act_Tab0 + 2)
			Btns[i]->SetIsEnabled(bInPort);

	// Market rows
	if (bInPort && V->Goods.Num() >= GIwNumGoods && GoodNameTexts.Num() >= GIwNumGoods)
	{
		const FIwPortSpec& P = V->WorldPorts[PortIdx];
		int32 Hold[GIwNumGoods], Avg[GIwNumGoods];
		IwPorts::GetCargoHold(Hold); IwPorts::GetCargoAvgCost(Avg);
		for (int32 g = 0; g < GIwNumGoods; ++g)
		{
			const FIwGoodDef& Gd = V->Goods[g];
			const bool bExport = P.PriceMult[g] <= 0.72f;
			const bool bDemand = P.PriceMult[g] >= 1.28f;
			const FString Tag = bExport ? TEXT(" [EXPORT]") : (bDemand ? TEXT(" [DEMAND]") : TEXT(""));
			GoodNameTexts[g]->SetText(FText::FromString(Gd.Name + Tag));
			GoodNameTexts[g]->SetColorAndOpacity(FSlateColor(bExport ? DkGreen : (bDemand ? DkGold : DkBrass)));
			GoodStockTexts[g]->SetText(FText::FromString(FString::Printf(TEXT("%d t"), P.LiveStock[g])));
			const int32 BuyP = IwPorts::UnitPriceAt(P, Gd, g, true);
			const int32 SellP = IwPorts::UnitPriceAt(P, Gd, g, false);
			GoodPriceTexts[g]->SetText(FText::FromString(FString::Printf(TEXT("%d / %d cr"), BuyP, SellP)));
			GoodPriceTexts[g]->SetColorAndOpacity(FSlateColor(bExport ? DkGreen : (bDemand ? DkGold : DkSteel)));
			GoodHoldTexts[g]->SetText(FText::FromString(Hold[g] > 0 ? FString::Printf(TEXT("%d t (@%d)"), Hold[g], Avg[g]) : TEXT("0 t")));
		}
	}
	if (TradeStatusText) TradeStatusText->SetText(FText::FromString(LastTradeStatus));

	// Logbook tab
	if (CareerSummaryText && V)
	{
		int32 VisitedPlaces = 0, Lights = 0, Wrecks = 0;
		const int32 SalMask = IwPorts::GetSalvagedMask();
		const int32 VisMask = IronwakeFleet::GetVisitedMask();
		for (int32 i = 0; i < V->POIs.Num(); ++i)
		{
			const bool bVis = V->POIs[i].bVisited || ((i < 31) && ((VisMask & (1 << i)) != 0)) || ((i < 30) && ((SalMask & (1 << i)) != 0));
			if (bVis) ++VisitedPlaces;
			if (((i < 30) && ((SalMask & (1 << i)) != 0)) || V->POIs[i].bVisited)
			{
				if (V->POIs[i].Tag == TEXT("L") || V->POIs[i].Tag == TEXT("M")) ++Lights;
				if (V->POIs[i].Tag == TEXT("W") || V->POIs[i].Tag == TEXT("D")) ++Wrecks;
			}
		}
		int32 PortsDocked = 0;
		const int32 PMask = IwPorts::GetDockedPortMask();
		for (int32 p = 0; p < V->WorldPorts.Num(); ++p) if (PMask & (1 << p)) ++PortsDocked;

		CareerSummaryText->SetText(FText::FromString(FString::Printf(
			TEXT("Ports Docked: %d / %d   |   Archipelago Charted: %d / %d   |   Lighthouses: %d / 2   |   Wrecks: %d / 2\n")
			TEXT("Distance Sailed: %.1f NM   |   Cargo Delivered: %d tons   |   Net Trade Profit: %s%d Silver Crowns"),
			PortsDocked, FMath::Max(V->WorldPorts.Num(), 3), VisitedPlaces, V->POIs.Num(), Lights, Wrecks,
			IwPorts::GetNauticalMiles(), IwPorts::GetCargoDeliveredTons(),
			IwPorts::GetNetTradeProfit() >= 0 ? TEXT("+") : TEXT(""), IwPorts::GetNetTradeProfit())));
	}
	if (LogEntriesText)
	{
		TArray<FIwLogEntry> Entries;
		IwPorts::GetLogEntries(Entries);
		if (Entries.Num() == 0)
		{
			LogEntriesText->SetText(FText::FromString(TEXT("No entries logged yet. Make fast at Greyhaven, Ironcrag, or Stigsby, or drop anchor off a lighthouse or wreck.")));
		}
		else
		{
			FString Lines;
			for (int32 i = 0; i < FMath::Min(Entries.Num(), 11); ++i)
				Lines += FString::Printf(TEXT("[%s]  %-22s  %s\n"), *Entries[i].Stamp, *Entries[i].Place.Left(22), *Entries[i].Detail);
			LogEntriesText->SetText(FText::FromString(Lines.TrimEnd()));
		}
	}

	// Shipyard tab
	if (HullRepairText)
	{
		AIronwakeShipPawn* Sp = V ? Cast<AIronwakeShipPawn>(GetOwningPlayerPawn()) : nullptr;
		const float Pct = (Sp && Sp->MaxHP > 0.f) ? (Sp->HP / Sp->MaxHP * 100.f) : 100.f;
		HullRepairText->SetText(FText::FromString(FString::Printf(TEXT("HULL CONDITION: %.0f%%   |   ACTIVE FIRES: %d"), Pct, Sp ? Sp->NumFires() : 0)));
	}
	const TArray<FIronwakeShipDef>& Roster = IronwakeFleet::Roster();
	for (int32 i = 0; i < ShipTitleTexts.Num() && i < Roster.Num(); ++i)
	{
		const FIronwakeShipDef& D = Roster[i];
		const bool bUn = IronwakeFleet::IsUnlocked(D.Id);
		const bool bCur = (D.Id == CurShipId);
		const int32 HoldCap = IwPorts::HullCargoCapTons(D.Id);
		ShipTitleTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%s   (%d t Cargo Hold)   %s"),
			*D.Name.ToUpper(), HoldCap, bCur ? TEXT("[ IN COMMAND ]") : (bUn ? TEXT("[ READY ]") : TEXT("[ LOCKED ]")))));
		ShipTitleTexts[i]->SetColorAndOpacity(FSlateColor(bUn ? DkBrass : DkDim));
		ShipDescTexts[i]->SetText(FText::FromString(bUn ? FString::Printf(TEXT("%s — %s"), *D.Class, *D.Blurb)
			: FString::Printf(TEXT("%s — Unlock: %s (or 1,200 Silver Crowns)"), *D.Class, *D.UnlockText)));
		if (ShipBtnTexts.IsValidIndex(i) && ShipBtnTexts[i])
			ShipBtnTexts[i]->SetText(FText::FromString(bCur ? TEXT("ABOARD") : (bUn ? TEXT("TAKE COMMAND") : TEXT("LOCKED"))));
	}
}

void UIronwakeDockWidget::TickDock(float Dt)
{
	(void)Dt;
	if (!IsInViewport()) return;
	FitToViewport();
	for (int32 i = 0; i < Btns.Num(); ++i)
	{
		if (!Btns[i]) continue;
		const bool bSel = (BtnIds[i] >= Act_Tab0 && BtnIds[i] < Act_Tab0 + 3) && ((BtnIds[i] - Act_Tab0) == ActiveTab);
		const bool bFoc = Btns[i]->HasKeyboardFocus() || Btns[i]->HasAnyUserFocus();
		Btns[i]->SetBackgroundColor(bFoc ? DkBtnFoc : (bSel ? DkBtnSel : DkBtn));
	}
}

void UIronwakeDockWidget::OnAction(int32 Id)
{
	AIronwakeVoyage* V = Voy.Get();
	UWorld* W = GetWorld();
	IronwakeAudio::Click(W);
	if (Id == Act_Close) { if (V) V->CloseDockModal(); return; }
	if (Id >= Act_Tab0 && Id < Act_Tab0 + 3) { OpenTab(Id - Act_Tab0); return; }

	if (!V || !V->WorldPorts.IsValidIndex(PortIdx)) return;
	FIwPortSpec& Port = V->WorldPorts[PortIdx];
	const FName ShipId = IronwakeFleet::GetSelected();

	if (Id == Act_Repair)
	{
		AIronwakeShipPawn* Sp = Cast<AIronwakeShipPawn>(GetOwningPlayerPawn());
		if (Sp && Sp->HP < Sp->MaxHP - 0.5f)
		{
			if (IwPorts::GetCrowns() >= 25)
			{
				IwPorts::SetCrowns(IwPorts::GetCrowns() - 25);
				Sp->HP = Sp->MaxHP;
				LastTradeStatus = TEXT("Shipwrights caulked and repaired all hull plating (-25 Silver Crowns).");
			}
			else LastTradeStatus = TEXT("Need 25 Silver Crowns for drydock hull repairs.");
		}
		else LastTradeStatus = TEXT("Hull plating and rigging are already at 100% condition.");
		Refresh();
		return;
	}
	if (Id >= Act_ShipBase && Id < Act_ShipBase + 6)
	{
		const int32 Idx = Id - Act_ShipBase;
		const TArray<FIronwakeShipDef>& R = IronwakeFleet::Roster();
		if (R.IsValidIndex(Idx) && IronwakeFleet::IsUnlocked(R[Idx].Id) && R[Idx].Id != ShipId)
		{
			IwPorts::SetPendingPortSpawn(Port.Id);
			V->CloseDockModal();
			if (AIronwakePlayerController* PC = Cast<AIronwakePlayerController>(GetOwningPlayer()))
				PC->ChooseShip(R[Idx].Id);
		}
		return;
	}
	if (Id >= Act_Buy1Base && Id < Act_Buy1Base + GIwNumGoods)
	{
		IwPorts::BuyCargo(Port, V->Goods, Id - Act_Buy1Base, 1, ShipId, LastTradeStatus);
		Refresh(); return;
	}
	if (Id >= Act_Buy10Base && Id < Act_Buy10Base + GIwNumGoods)
	{
		IwPorts::BuyCargo(Port, V->Goods, Id - Act_Buy10Base, 10, ShipId, LastTradeStatus);
		Refresh(); return;
	}
	if (Id >= Act_BuyMaxBase && Id < Act_BuyMaxBase + GIwNumGoods)
	{
		IwPorts::BuyCargo(Port, V->Goods, Id - Act_BuyMaxBase, 999, ShipId, LastTradeStatus);
		Refresh(); return;
	}
	if (Id >= Act_SellAllBase && Id < Act_SellAllBase + GIwNumGoods)
	{
		IwPorts::SellCargo(Port, V->Goods, Id - Act_SellAllBase, 999, LastTradeStatus);
		Refresh(); return;
	}
}

FReply UIronwakeDockWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::E || K == EKeys::Gamepad_FaceButton_Right)
	{
		if (AIronwakeVoyage* V = Voy.Get()) V->CloseDockModal();
		return FReply::Handled();
	}
	if (K == EKeys::One)   { OpenTab(TabMarket);   return FReply::Handled(); }
	if (K == EKeys::Two)   { OpenTab(TabLogbook);  return FReply::Handled(); }
	if (K == EKeys::Three) { OpenTab(TabShipyard); return FReply::Handled(); }
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

int32 UIronwakeDockWidget::CheckLayout(const FString& Tag)
{
	FitToViewport();
	ForceLayoutPrepass();
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
		const bool bLeaf = Cast<UButton>(Wd) || Cast<UTextBlock>(Wd);
		if (!bLeaf || !Shown(Wd)) return;
		if (Cast<UTextBlock>(Wd)) { for (UWidget* X = Wd->GetParent(); X; X = X->GetParent()) if (Cast<UButton>(X)) return; }
		FR R; R.Wd = Wd; R.bInScroll = CardScroll && HasAncestor(Wd, CardScroll);
		if (const UTextBlock* T = Cast<UTextBlock>(Wd)) R.N = FString::Printf(TEXT("text '%s'"), *T->GetText().ToString().Left(24).Replace(TEXT("\n"), TEXT(" ")));
		else if (const UTextBlock* BT = Cast<UTextBlock>(Cast<UButton>(Wd)->GetChildAt(0))) R.N = FString::Printf(TEXT("btn '%s'"), *BT->GetText().ToString());
		else R.N = Wd->GetName();
		Rect(Wd, R.A, R.B);
		Rs.Add(R);
	});
	FVector2D CA = FVector2D::ZeroVector, CB = FVector2D::ZeroVector;
	if (CardBox && CardBox->GetCachedGeometry().GetLocalSize().X > 0.5f) Rect(CardBox, CA, CB);
	FVector2D SA = FVector2D::ZeroVector, SB = FVector2D::ZeroVector;
	if (CardScroll) Rect(CardScroll, SA, SB);
	int32 Outside = 0, Clipped = 0, Overlaps = 0;
	const float Tol = 1.5f;
	FString Detail;
	for (const FR& R : Rs)
	{
		if (R.A.X < -Tol || R.A.Y < -Tol || R.B.X > VP.X + Tol || R.B.Y > VP.Y + Tol) { ++Outside; Detail += FString::Printf(TEXT(" OUT[%s]"), *R.N); }
		if (R.bInScroll && (R.A.X < SA.X - Tol || R.A.Y < SA.Y - Tol || R.B.X > SB.X + Tol || R.B.Y > SB.Y + Tol)) { ++Clipped; Detail += FString::Printf(TEXT(" CLIP[%s]"), *R.N); }
	}
	if (CB.X > VP.X + Tol || CB.Y > VP.Y + Tol || CA.X < -Tol || CA.Y < -Tol) { ++Outside; Detail += TEXT(" OUT[card]"); }
	for (int32 i = 0; i < Rs.Num(); ++i)
		for (int32 j = i + 1; j < Rs.Num(); ++j)
		{
			if (HasAncestor(Rs[i].Wd, Rs[j].Wd) || HasAncestor(Rs[j].Wd, Rs[i].Wd)) continue;
			const float OX = FMath::Min(Rs[i].B.X, Rs[j].B.X) - FMath::Max(Rs[i].A.X, Rs[j].A.X);
			const float OY = FMath::Min(Rs[i].B.Y, Rs[j].B.Y) - FMath::Max(Rs[i].A.Y, Rs[j].A.Y);
			if (OX > 2.f && OY > 2.f) { ++Overlaps; Detail += FString::Printf(TEXT(" OVERLAP[%s|%s]"), *Rs[i].N, *Rs[j].N); }
		}
	UE_LOG(LogIronwake, Display, TEXT("IWP29 LAYOUT %s vp=%.0fx%.0f scale=%.3f widgets=%d outside=%d overlaps=%d clipped=%d %s%s"),
		*Tag, VP.X, VP.Y, CurScale, Rs.Num(), Outside, Overlaps, Clipped, (Outside + Overlaps + Clipped) == 0 ? TEXT("PASS") : TEXT("FAIL"), *Detail);
	return Outside + Overlaps + Clipped;
}

// ============================================================================ AIronwakeVoyage P29 Integration & Test Harness
void AIronwakeVoyage::InitWorldPorts()
{
	IwPorts::LoadWorldPorts(Goods, WorldPorts);
	if (WorldPorts.Num() > 0 && WorldPorts[0].bMainHarbour)
		HarbourC = FVector(WorldPorts[0].WorldX * 100.f, WorldPorts[0].WorldY * 100.f, 0.f);
}

void AIronwakeVoyage::BuildSecondaryPorts()
{
	AIronwakeArchipelago* Arch = AIronwakeArchipelago::Get(GetWorld());
	for (int32 i = 0; i < WorldPorts.Num(); ++i)
	{
		if (!WorldPorts[i].bMainHarbour)
			IwPorts::BuildSecondaryPort(this, Arch, WorldPorts[i]);
	}
	// If player switched ships at Ironcrag or Stigsby, place their new command at that port's anchorage!
	const FName PendingPort = IwPorts::ConsumePendingPortSpawn();
	if (!PendingPort.IsNone())
	{
		for (const FIwPortSpec& P : WorldPorts)
		{
			if (P.Id == PendingPort && !P.bMainHarbour)
			{
				if (AIronwakeShipPawn* Sp = Player())
				{
					const FVector Dest(P.MoorWorldCm.X, P.MoorWorldCm.Y, Sp->GetActorLocation().Z);
					Sp->SetActorLocationAndRotation(Dest, FRotator(0.f, P.SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
					Sp->ResetCameraToHorizon();
				}
				break;
			}
		}
	}
}

bool AIronwakeVoyage::TryArrivalAction()
{
	if (bDockOpen) { CloseDockModal(); return true; }
	if (bPhoto || bChart) return false;
	if (CanDockPortIdx >= 0 && WorldPorts.IsValidIndex(CanDockPortIdx))
	{
		OpenDockModal(CanDockPortIdx, UIronwakeDockWidget::TabMarket);
		return true;
	}
	if (CanLogPoiIdx >= 0 && POIs.IsValidIndex(CanLogPoiIdx))
	{
		OpenLandmarkModal(CanLogPoiIdx);
		return true;
	}
	return false;
}

void AIronwakeVoyage::OpenDockModal(int32 InPortIdx, int32 StartTab)
{
	if (!WorldPorts.IsValidIndex(InPortIdx)) return;
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;

	if (ApMode != 0) DisengageAutopilot(TEXT("Made fast in port"));
	bHelm = false;
	bAnchored = true;
	P->IWTelegraph(2);
	P->IWRudder(0);
	if (P->HullBody)
	{
		P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
		P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	const FIwPortSpec& Spec = WorldPorts[InPortIdx];
	IwPorts::RecordPortDocked(InPortIdx);
	for (int32 i = 0; i < POIs.Num(); ++i)
	{
		if (POIs[i].Tag == Spec.Tag)
		{
			POIs[i].bVisited = true;
			IronwakeFleet::RecordArrival(i, POIs.Num());
			break;
		}
	}
	IwPorts::PlayHarbourBell(GetWorld());
	const FString Clk = bLiving ? ClockString().Left(5) : TEXT("12:00");
	IwPorts::AppendLog(Clk, Spec.Name, FString::Printf(TEXT("Made fast — %s"), *Spec.Subtitle));

	if (!DockCam)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DockCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
		DockCam->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	const float SeaZ = AIronwakeShell::WaterZAt(GetWorld(), Spec.WorldCentreCm) + AIronwakeShell::RenderedSeaOffsetCm;
	const FVector Eye = Spec.H2W(Spec.DioramaEyeLocal.X, Spec.DioramaEyeLocal.Y, Spec.DioramaEyeLocal.Z, SeaZ);
	const FVector Look = Spec.H2W(Spec.DioramaLookLocal.X, Spec.DioramaLookLocal.Y, Spec.DioramaLookLocal.Z, SeaZ);
	DockCam->SetActorLocationAndRotation(Eye, (Look - Eye).Rotation());
	DockCam->GetCameraComponent()->SetFieldOfView(54.f);
	C->SetViewTargetWithBlend(DockCam, 0.35f);

	if (!DockWidget) DockWidget = CreateWidget<UIronwakeDockWidget>(C, UIronwakeDockWidget::StaticClass());
	if (DockWidget)
	{
		DockWidget->SetupForPort(this, InPortIdx, StartTab);
		if (!DockWidget->IsInViewport()) DockWidget->AddToViewport(48);
	}
	ActiveDockPortIdx = InPortIdx;
	bDockOpen = true;
	C->SetDockInput(true);
	UE_LOG(LogIronwake, Display, TEXT("IWP29 DOCK OPEN port=%s (%d) crowns=%d hold=%d"), *Spec.Name, InPortIdx, IwPorts::GetCrowns(), IwPorts::TotalCargoTons());
}

void AIronwakeVoyage::OpenLandmarkModal(int32 PoiIdx)
{
	if (!POIs.IsValidIndex(PoiIdx)) return;
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P) return;

	if (ApMode != 0) DisengageAutopilot(TEXT("Anchored for survey & salvage"));
	bHelm = false;
	bAnchored = true;
	P->IWTelegraph(2);
	P->IWRudder(0);

	FIronwakePOI& Pt = POIs[PoiIdx];
	Pt.bVisited = true;
	IronwakeFleet::RecordArrival(PoiIdx, POIs.Num());
	IwPorts::PlayHarbourBell(GetWorld());

	const FString Clk = bLiving ? ClockString().Left(5) : TEXT("12:00");
	FString Banner;
	IwPorts::RecordPoiDiscoveryOrSalvage(PoiIdx, Pt.Name, Pt.Tag, Clk, Banner);

	if (!DockCam)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DockCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SP);
		DockCam->GetCameraComponent()->bConstrainAspectRatio = false;
	}
	const FVector ShipLoc = P->GetActorLocation();
	const FVector ToPoi = (FVector(Pt.Loc.X, Pt.Loc.Y, ShipLoc.Z) - ShipLoc).GetSafeNormal2D();
	const FVector Side(-ToPoi.Y, ToPoi.X, 0.f);
	const FVector Eye = ShipLoc - ToPoi * 3800.f + Side * 2400.f + FVector(0.f, 0.f, 1200.f);
	const FVector Look = FVector(Pt.Loc.X, Pt.Loc.Y, ShipLoc.Z + 1400.f);
	DockCam->SetActorLocationAndRotation(Eye, (Look - Eye).Rotation());
	DockCam->GetCameraComponent()->SetFieldOfView(56.f);
	C->SetViewTargetWithBlend(DockCam, 0.35f);

	if (!DockWidget) DockWidget = CreateWidget<UIronwakeDockWidget>(C, UIronwakeDockWidget::StaticClass());
	if (DockWidget)
	{
		DockWidget->SetupForLandmark(this, Pt.Name.ToUpper(), TEXT("Admiralty Hydrographic Survey & Salvage Station"),
			TEXT("\"Soundings recorded and salvage boat lowered alongside.\""), Banner);
		if (!DockWidget->IsInViewport()) DockWidget->AddToViewport(48);
	}
	ActiveDockPortIdx = -1;
	bDockOpen = true;
	C->SetDockInput(true);
	UE_LOG(LogIronwake, Display, TEXT("IWP29 LANDMARK LOGGED poi=%s (%d) crowns=%d"), *Pt.Name, PoiIdx, IwPorts::GetCrowns());
}

void AIronwakeVoyage::CloseDockModal()
{
	if (!bDockOpen) return;
	bDockOpen = false;
	ActiveDockPortIdx = -1;
	if (DockWidget && DockWidget->IsInViewport()) DockWidget->RemoveFromParent();
	if (AIronwakePlayerController* C = PC())
	{
		if (AIronwakeShipPawn* P = Player()) C->SetViewTargetWithBlend(P, 0.25f);
		C->SetDockInput(false);
	}
	UE_LOG(LogIronwake, Display, TEXT("IWP29 DOCK CLOSED crowns=%d hold=%d"), IwPorts::GetCrowns(), IwPorts::TotalCargoTons());
}

// ============================================================================ -iwp29=test Automated End-to-End Verification Director
void AIronwakeVoyage::TickP29Test(float RealDt)
{
	AIronwakePlayerController* C = PC();
	AIronwakeShipPawn* P = Player();
	if (!C || !P || WorldPorts.Num() < 3) return;
	P29T += RealDt;
	const float Since = P29T - P29Mark;
	auto Next = [&](int32 Step) { P29Step = Step; P29Sub = 0; P29Mark = P29T; bP29Shot = false; };

	auto ShotScene = [&](const FString& Name, bool bUI = false)
	{
		const FString Full = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("shots") / (Name + TEXT(".png")));
		FScreenshotRequest::RequestScreenshot(Full, bUI, false);
		UE_LOG(LogIronwake, Display, TEXT("IWP29 SHOT %s ui=%d lamp=%.2f"), *Name, bUI ? 1 : 0, LiveLamp);
	};

	auto PlaceSeaCam = [&](const FIwPortSpec& Port, float DistM, float YawOffDeg, float EyeZ, float AimInlandM, float AimZ, float Hour, int32 Wx, float Fov = 54.f)
	{
		if (bDockOpen) CloseDockModal();
		bWxDrift = false; ClockSpeedIdx = 2; SetClockHour(Hour); SeaOverride = 0.f;
		ForceWeatherState(Wx, 0.05f);
		if (AIronwakeStorm* St = AIronwakeStorm::Get(GetWorld()))
		{
			const float SeaTarget = (Hour > 6.f && Hour < 19.f) ? 0.35f : 0.22f;
			St->SetTarget(SeaTarget, 0.05f);
			St->SetOpenImmediate(SeaTarget);
		}
		const FVector OutDir = FRotator(0.f, Port.SeawardYaw + YawOffDeg, 0.f).Vector();
		const FVector QuayRef = Port.H2W(-50.f, 0.f, 0.f);
		const FVector ShipPos = QuayRef + OutDir * (DistM + 220.f) * 100.f;
		P->SetActorLocationAndRotation(FVector(ShipPos.X, ShipPos.Y, P->GetActorLocation().Z), FRotator(0.f, Port.SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
		P->IWTelegraph(2);
		if (!bPhoto) TogglePhoto();
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		const float SeaZ = AIronwakeShell::WaterZAt(GetWorld(), Port.WorldCentreCm) + AIronwakeShell::RenderedSeaOffsetCm;
		const FVector Eye = FVector(QuayRef.X, QuayRef.Y, 0.f) + OutDir * DistM * 100.f + FVector(0.f, 0.f, SeaZ + EyeZ * 100.f);
		const FVector Aim = Port.H2W(-AimInlandM, 0.f, AimZ, SeaZ);
		SetPhotoView(Eye, (Aim - Eye).Rotation());
		if (PhotoCam) PhotoCam->GetCameraComponent()->SetFieldOfView(Fov);
	};

	switch (P29Step)
	{
	case 0: // Init peaceful world, full native 100% TSR resolution for clean look captures, reset economy
		if (P29T >= 3.0f)
		{
			if (ULocalPlayer* LP = C->GetLocalPlayer())
				if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
					Sub->ClearAllMappings();
			if (!C->bStarted) C->StartGame(true);
			C->bPeaceful = true;
			for (TActorIterator<AIronwakeShipPawn> It(GetWorld()); It; ++It)
				if (*It != P) It->Destroy();
			P->HP = P->MaxHP;
			C->ConsoleCommand(TEXT("r.DynamicRes.OperationMode 0"));
			C->ConsoleCommand(TEXT("r.ScreenPercentage 100"));
			StartLiving();
			IwPorts::ResetEconomyForTest(WorldPorts);
			UE_LOG(LogIronwake, Display, TEXT("IWP29 INIT ports=%d goods=%d crowns=%d cap=%d"),
				WorldPorts.Num(), Goods.Num(), IwPorts::GetCrowns(), IwPorts::HullCargoCapTons(IronwakeFleet::GetSelected()));
			PlaceSeaCam(WorldPorts[1], 400.f, 0.f, 9.0f, 92.f, 12.5f, 13.6f, 0, 52.f);
			Next(1);
		}
		break;

	case 1: // Ironcrag 400 m Afternoon Sun (13:36 SSW illuminates the 205-deg harbourfront!)
		if (Since >= 7.0f && !bP29Shot) { ShotScene(TEXT("P29_ironcrag_noon_400"), false); bP29Shot = true; }
		if (Since >= 8.4f) { PlaceSeaCam(WorldPorts[1], 400.f, 0.f, 9.0f, 92.f, 12.5f, 22.5f, 0, 52.f); Next(2); }
		break;

	case 2: // Ironcrag 400 m Clear Night
		if (Since >= 7.0f && !bP29Shot) { ShotScene(TEXT("P29_ironcrag_night_400"), false); bP29Shot = true; }
		if (Since >= 8.4f) { PlaceSeaCam(WorldPorts[1], 145.f, 12.f, 10.5f, 70.f, 8.5f, 13.6f, 0, 56.f); Next(3); }
		break;

	case 3: // Ironcrag Quay & Coaling Pier 145 m
		if (Since >= 6.0f && !bP29Shot) { ShotScene(TEXT("P29_ironcrag_quay_145"), false); bP29Shot = true; }
		if (Since >= 7.4f) { PlaceSeaCam(WorldPorts[2], 400.f, 0.f, 9.0f, 86.f, 11.5f, 14.0f, 0, 52.f); Next(4); }
		break;

	case 4: // Stigsby 400 m Afternoon Sun (14:00 SW illuminates the 235-deg cove!)
		if (Since >= 7.0f && !bP29Shot) { ShotScene(TEXT("P29_stigsby_noon_400"), false); bP29Shot = true; }
		if (Since >= 8.4f) { PlaceSeaCam(WorldPorts[2], 400.f, 0.f, 9.0f, 86.f, 11.5f, 22.5f, 0, 52.f); Next(5); }
		break;

	case 5: // Stigsby 400 m Clear Night
		if (Since >= 7.0f && !bP29Shot) { ShotScene(TEXT("P29_stigsby_night_400"), false); bP29Shot = true; }
		if (Since >= 8.4f) { PlaceSeaCam(WorldPorts[2], 140.f, -10.f, 10.f, 62.f, 7.5f, 14.0f, 0, 56.f); Next(6); }
		break;

	case 6: // Stigsby Timber Piers & Cove 140 m
		if (Since >= 6.0f && !bP29Shot) { ShotScene(TEXT("P29_stigsby_cove_140"), false); bP29Shot = true; }
		if (Since >= 7.4f)
		{
			if (bPhoto) { TogglePhoto(); UGameplayStatics::SetGlobalTimeDilation(this, 1.f); }
			SetClockHour(13.6f);
			P->SetActorLocationAndRotation(FVector(WorldPorts[0].OffingWorldCm.X, WorldPorts[0].OffingWorldCm.Y, P->GetActorLocation().Z), FRotator(0.f, 45.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			P->ResetCameraToHorizon();
			ClearWaypoints();
			Waypoints.Add(WorldPorts[1].MoorWorldCm);
			Waypoints.Add(WorldPorts[2].MoorWorldCm);
			Waypoints.Add(WorldPorts[0].MoorWorldCm);
			if (!bChart) ToggleChart();
			Next(7);
		}
		break;

	case 7: // 3-Port Navigation Chart & Trade Triangle Route Verification
		if (Since >= 2.5f && !bP29Shot)
		{
			TArray<FVector> Preview; TArray<FString> Det;
			BuildPreview(P->GetActorLocation(), Preview, &Det);
			ShotScene(TEXT("P29_chart_3ports"), true);
			bP29Shot = true;
			UE_LOG(LogIronwake, Display, TEXT("IWP29 CHART waypoints=%d planned_pts=%d detours=[%s]"), Waypoints.Num(), Preview.Num(), *FString::Join(Det, TEXT(", ")));
		}
		if (Since >= 4.2f)
		{
			if (bChart) ToggleChart();
			ClearWaypoints();
			P->SetActorLocationAndRotation(FVector(WorldPorts[0].MoorWorldCm.X, WorldPorts[0].MoorWorldCm.Y, P->GetActorLocation().Z), FRotator(0.f, 225.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (P->HullBody) P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
			P->IWTelegraph(2); P->ResetCameraToHorizon();
			Next(8);
		}
		break;

	case 8: // HUD Mooring Prompt Shot + Open Greyhaven Dock + Buy Exports + 1280x720 & 1920x1080 UI Layout Verification
		if (P29Sub == 0 && Since >= 2.0f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP29 PROMPT canDock=%d port=%s"), CanDockPortIdx, CanDockPortIdx >= 0 ? *WorldPorts[CanDockPortIdx].Name : TEXT("none"));
			ShotScene(TEXT("P29_prompt_greyhaven"), true);
			P29Sub = 1;
		}
		if (P29Sub == 1 && Since >= 3.2f)
		{
			TryArrivalAction();
			FString Msg;
			IwPorts::BuyCargo(WorldPorts[0], Goods, 4, 2, IronwakeFleet::GetSelected(), Msg);
			IwPorts::BuyCargo(WorldPorts[0], Goods, 5, 3, IronwakeFleet::GetSelected(), Msg);
			if (DockWidget) DockWidget->Refresh();
			C->ConsoleCommand(TEXT("r.SetRes 1280x720w"));
			P29Sub = 2;
		}
		if (P29Sub == 2 && Since >= 5.0f)
		{
			if (DockWidget) DockWidget->CheckLayout(TEXT("Greyhaven_Market_1280x720"));
			ShotScene(TEXT("P29_dock_greyhaven_1280x720"), true);
			P29Sub = 3;
		}
		if (P29Sub == 3 && Since >= 6.2f)
		{
			C->ConsoleCommand(TEXT("r.SetRes 1920x1080w"));
			P29Sub = 4;
		}
		if (P29Sub == 4 && Since >= 8.0f)
		{
			if (DockWidget) DockWidget->CheckLayout(TEXT("Greyhaven_Market_1920x1080"));
			ShotScene(TEXT("P29_dock_greyhaven_1920x1080"), true);
			P29Sub = 5;
		}
		if (P29Sub == 5 && Since >= 9.2f)
		{
			CloseDockModal();
			IwPorts::AddDistanceMeters(6950.f);
			P->SetActorLocationAndRotation(FVector(WorldPorts[1].MoorWorldCm.X, WorldPorts[1].MoorWorldCm.Y, P->GetActorLocation().Z), FRotator(0.f, WorldPorts[1].SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Next(9);
		}
		break;

	case 9: // Arrive & Make Fast at Ironcrag -> Sell Brasswork/Sailcloth -> Buy cheap Coal & Pig Iron -> Shot Diorama
		if (P29Sub == 0 && Since >= 1.5f)
		{
			OpenDockModal(1, UIronwakeDockWidget::TabMarket);
			FString Msg;
			IwPorts::SellCargo(WorldPorts[1], Goods, 4, 99, Msg);
			IwPorts::SellCargo(WorldPorts[1], Goods, 5, 99, Msg);
			IwPorts::BuyCargo(WorldPorts[1], Goods, 0, 22, IronwakeFleet::GetSelected(), Msg);
			IwPorts::BuyCargo(WorldPorts[1], Goods, 1, 10, IronwakeFleet::GetSelected(), Msg);
			if (DockWidget) DockWidget->Refresh();
			P29Sub = 1;
		}
		if (P29Sub == 1 && Since >= 3.5f)
		{
			if (DockWidget) DockWidget->CheckLayout(TEXT("Ironcrag_Market_1920x1080"));
			ShotScene(TEXT("P29_dock_ironcrag_market"), true);
			P29Sub = 2;
		}
		if (P29Sub == 2 && Since >= 4.7f)
		{
			CloseDockModal();
			IwPorts::AddDistanceMeters(4690.f);
			P->SetActorLocationAndRotation(FVector(WorldPorts[2].MoorWorldCm.X, WorldPorts[2].MoorWorldCm.Y, P->GetActorLocation().Z), FRotator(0.f, WorldPorts[2].SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Next(10);
		}
		break;

	case 10: // Arrive & Make Fast at Stigsby -> Sell Coal -> Buy Salt Fish & Naval Timber -> Logbook & Shipyard Layout Check
		if (P29Sub == 0 && Since >= 1.5f)
		{
			FString SalvBanner;
			IwPorts::RecordPoiDiscoveryOrSalvage(0, POIs[0].Name, POIs[0].Tag, TEXT("14:20"), SalvBanner);
			OpenDockModal(2, UIronwakeDockWidget::TabMarket);
			FString Msg;
			IwPorts::SellCargo(WorldPorts[2], Goods, 0, 99, Msg);
			IwPorts::BuyCargo(WorldPorts[2], Goods, 2, 24, IronwakeFleet::GetSelected(), Msg);
			IwPorts::BuyCargo(WorldPorts[2], Goods, 3, 12, IronwakeFleet::GetSelected(), Msg);
			if (DockWidget) DockWidget->OpenTab(UIronwakeDockWidget::TabLogbook);
			P29Sub = 1;
		}
		if (P29Sub == 1 && Since >= 3.5f)
		{
			if (DockWidget) DockWidget->CheckLayout(TEXT("Stigsby_Logbook_1920x1080"));
			ShotScene(TEXT("P29_dock_stigsby_logbook"), true);
			P29Sub = 2;
		}
		if (P29Sub == 2 && Since >= 4.7f)
		{
			C->ConsoleCommand(TEXT("r.SetRes 1280x720w"));
			if (DockWidget) DockWidget->OpenTab(UIronwakeDockWidget::TabShipyard);
			P29Sub = 3;
		}
		if (P29Sub == 3 && Since >= 6.5f)
		{
			if (DockWidget) DockWidget->CheckLayout(TEXT("Stigsby_Shipyard_1280x720"));
			ShotScene(TEXT("P29_dock_stigsby_shipyard_1280x720"), true);
			P29Sub = 4;
		}
		if (P29Sub == 4 && Since >= 7.7f)
		{
			C->ConsoleCommand(TEXT("r.SetRes 1600x900w"));
			CloseDockModal();
			IwPorts::AddDistanceMeters(7480.f);
			P->SetActorLocationAndRotation(FVector(WorldPorts[0].MoorWorldCm.X, WorldPorts[0].MoorWorldCm.Y, P->GetActorLocation().Z), FRotator(0.f, 225.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			Next(11);
		}
		break;

	case 11: // Return to Greyhaven & Liquidate Cargo -> Verify Round-Trip Profit & Persistence
		if (Since >= 1.5f)
		{
			OpenDockModal(0, UIronwakeDockWidget::TabMarket);
			FString Msg;
			for (int32 g = 0; g < GIwNumGoods; ++g) IwPorts::SellCargo(WorldPorts[0], Goods, g, 999, Msg);
			CloseDockModal();
			const int32 FinalCr = IwPorts::GetCrowns();
			const int32 HoldRem = IwPorts::TotalCargoTons();
			UE_LOG(LogIronwake, Display, TEXT("IWP29 TRADE %s start_crowns=250 final_crowns=%d net_profit=%d delivered_tons=%d hold_after=%d nm=%.1f"),
				(FinalCr > 750 && HoldRem == 0) ? TEXT("PASS") : TEXT("FAIL"),
				FinalCr, IwPorts::GetNetTradeProfit(), IwPorts::GetCargoDeliveredTons(), HoldRem, IwPorts::GetNauticalMiles());
			// Start Collision Test 1: drive ship under auxiliary steam at FULL AHEAD into Ironcrag stone quay from 32 m out
			const FVector Start1 = WorldPorts[1].H2W(32.f, 6.f, 0.f);
			P->SetActorLocationAndRotation(FVector(Start1.X, Start1.Y, P->GetActorLocation().Z), FRotator(0.f, WorldPorts[1].SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
			bAnchored = false; P->bAuxSteam = true; P->IWRudder(0); P->IWTelegraph(5);
			Next(12);
		}
		break;

	case 12: // Collision Gate at Ironcrag (verify hull stops cleanly against -50 m quay wall without penetrating!)
		if (Since >= 22.0f && !bP29Shot)
		{
			const FVector2D LocH = WorldPorts[1].W2H(P->GetActorLocation());
			const float SpdKn = P->GetSpeedKnots();
			UE_LOG(LogIronwake, Display, TEXT("IWP29 COLL port=Ironcrag local_X=%.1f local_Y=%.1f speed=%.2f kn %s"),
				LocH.X, LocH.Y, SpdKn, (LocH.X < 5.f && LocH.X > -46.f) ? TEXT("PASS") : TEXT("FAIL"));
			ShotScene(TEXT("P29_coll_ironcrag"), true);
			bP29Shot = true;
		}
		if (Since >= 23.2f)
		{
			// Start Collision Test 2: drive ship under auxiliary steam at FULL AHEAD into Stigsby timber wharf from 32 m out
			const FVector Start2 = WorldPorts[2].H2W(32.f, 4.f, 0.f);
			P->SetActorLocationAndRotation(FVector(Start2.X, Start2.Y, P->GetActorLocation().Z), FRotator(0.f, WorldPorts[2].SeawardYaw + 180.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
			if (P->HullBody) { P->HullBody->SetPhysicsLinearVelocity(FVector::ZeroVector); P->HullBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector); }
			bAnchored = false; P->bAuxSteam = true; P->IWRudder(0); P->IWTelegraph(5);
			Next(13);
		}
		break;

	case 13: // Collision Gate at Stigsby + Clean Exit
		if (Since >= 22.0f && !bP29Shot)
		{
			const FVector2D LocH = WorldPorts[2].W2H(P->GetActorLocation());
			const float SpdKn = P->GetSpeedKnots();
			UE_LOG(LogIronwake, Display, TEXT("IWP29 COLL port=Stigsby local_X=%.1f local_Y=%.1f speed=%.2f kn %s"),
				LocH.X, LocH.Y, SpdKn, (LocH.X < 5.f && LocH.X > -44.f) ? TEXT("PASS") : TEXT("FAIL"));
			ShotScene(TEXT("P29_coll_stigsby"), true);
			bP29Shot = true;
		}
		if (Since >= 23.2f)
		{
			UE_LOG(LogIronwake, Display, TEXT("IWP29 ALL GATES DONE"));
			P29Mode.Empty();
			if (FParse::Param(FCommandLine::Get(), TEXT("iwp29quit")) || FParse::Param(FCommandLine::Get(), TEXT("iwp12quit")))
				UKismetSystemLibrary::QuitGame(this, C, EQuitPreference::Quit, false);
		}
		break;

	default: break;
	}
}

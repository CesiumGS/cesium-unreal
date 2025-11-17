// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEditorReparentHandler.h"
#include "CesiumEditorSubLevelMutex.h"
#include "CesiumIonServerManager.h"
#include "CesiumIonSession.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include <optional>

class FSpawnTabArgs;
class ACesium3DTileset;
class UCesiumRasterOverlay;
class UCesiumIonRasterOverlay;
struct FCesium3DTilesetLoadFailureDetails;
struct FCesiumRasterOverlayLoadFailureDetails;
class UCesiumIonServer;

DECLARE_LOG_CATEGORY_EXTERN(LogCesiumEditor, Log, All);

class FCesiumEditorModule : public IModuleInterface {
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

  static FString
  InContent(const FString& RelativePath, const ANSICHAR* Extension);

  static TSharedPtr<FSlateStyleSet> GetStyle();
  static const FName& GetStyleSetName();

  static FCesiumEditorModule* get() { return _pModule; }

  static CesiumIonServerManager& serverManager() {
    assert(_pModule);
    return get()->_serverManager;
  }

  static ACesium3DTileset* FindFirstTilesetSupportingOverlays();
  static ACesium3DTileset* FindFirstTilesetWithAssetID(int64_t assetID);
  static ACesium3DTileset*
  CreateTileset(const std::string& name, int64_t assetID);

  /**
   * Adds an overlay with the the MaterialLayerKey `OverlayN` where N is the
   * next unused index.
   */
  static UCesiumIonRasterOverlay* AddOverlay(
      ACesium3DTileset* pTilesetActor,
      const std::string& name,
      int64_t assetID);

  /**
   * Adds a base overlay, replacing the existing overlay with MaterialLayerKey
   * Overlay0, if any.
   */
  static UCesiumIonRasterOverlay* AddBaseOverlay(
      ACesium3DTileset* pTilesetActor,
      const std::string& name,
      int64_t assetID);

  /**
   * Gets the first CesiumSunSky in the current level if there is one, or
   * nullptr if there is not.
   */
  static AActor* GetCurrentLevelCesiumSunSky();

  /**
   * Gets the first DynamicPawn in the current level if there is one, or
   * nullptr if there is not.
   */
  static AActor* GetCurrentLevelDynamicPawn();

  /**
   * Spawns a new actor with the _cesiumSunSkyBlueprintClass
   * in the current level of the edited world.
   */
  static AActor* SpawnCesiumSunSky();

  /**
   * Spawns a new actor with the _dynamicPawnBlueprintClass
   * in the current level of the edited world.
   */
  static AActor* SpawnDynamicPawn();

  /**
   * Spawns a new Cesium3DTileset with default values in the current level of
   * the edited world.
   */
  static AActor* SpawnBlankTileset();

  /**
   * Spawns a new CesiumCartographicPolygon in the current level of the edited
   * world.
   */
  static AActor* SpawnCartographicPolygon();

private:
  TSharedRef<SDockTab> SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs);
  TSharedRef<SDockTab>
  SpawnCesiumIonAssetBrowserTab(const FSpawnTabArgs& TabSpawnArgs);

  void OnTilesetLoadFailure(const FCesium3DTilesetLoadFailureDetails& details);
  void OnRasterOverlayLoadFailure(
      const FCesiumRasterOverlayLoadFailureDetails& details);
  void OnTilesetIonTroubleshooting(ACesium3DTileset* pTileset);
  void OnRasterOverlayIonTroubleshooting(UCesiumRasterOverlay* pOverlay);
  void OnFeaturesMetadataAddProperties(ACesium3DTileset* pTileset);

  CesiumIonServerManager _serverManager;
  FDelegateHandle _tilesetLoadFailureSubscription;
  FDelegateHandle _rasterOverlayLoadFailureSubscription;
  FDelegateHandle _tilesetIonTroubleshootingSubscription;
  FDelegateHandle _rasterOverlayIonTroubleshootingSubscription;
  FDelegateHandle _featuresMetadataAddPropertiesSubscription;

  CesiumEditorSubLevelMutex _subLevelMutex;
  CesiumEditorReparentHandler _reparentHandler;

  static TSharedPtr<FSlateStyleSet> StyleSet;
  static FCesiumEditorModule* _pModule;

  /**
   * Gets the class of the "Cesium Sun Sky", loading it if necessary.
   * Used for spawning the CesiumSunSky.
   */
  static UClass* GetCesiumSunSkyClass();

  /**
   * Gets the class of the "Dynamic Pawn" blueprint, loading it if necessary.
   * Used for spawning the DynamicPawn.
   */
  static UClass* GetDynamicPawnBlueprintClass();
};

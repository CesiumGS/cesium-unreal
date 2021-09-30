// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonSession.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include <optional>

class FSpawnTabArgs;
class ACesium3DTileset;
class UCesiumIonRasterOverlay;

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

  static CesiumIonSession& ion() {
    assert(_pModule);
    return *_pModule->_pIonSession;
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

private:
  TSharedRef<SDockTab> SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs);
  TSharedRef<SDockTab>
  SpawnCesiumIonAssetBrowserTab(const FSpawnTabArgs& TabSpawnArgs);

  std::shared_ptr<CesiumIonSession> _pIonSession;

  static TSharedPtr<FSlateStyleSet> StyleSet;
  static FCesiumEditorModule* _pModule;

  /**
   * Gets the class of the "Cesium Sun Sky" blueprint, loading it if necessary.
   * Used for spawning the CesiumSunSky.
   */
  static UClass* GetCesiumSunSkyBlueprintClass();

  /**
   * Gets the class of the "Dynamic Pawn" blueprint, loading it if necessary.
   * Used for spawning the DynamicPawn.
   */
  static UClass* GetDynamicPawnBlueprintClass();
};

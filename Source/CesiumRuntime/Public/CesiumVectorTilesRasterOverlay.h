// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include <CesiumVectorOverlays/VectorStylingProvider.h>

#include "CesiumGeoJsonDocument.h"
#include "CesiumIonServer.h"
#include "CesiumModelMetadata.h"
#include "CesiumRasterOverlay.h"
#include "CesiumVectorStyle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "CesiumVectorTilesRasterOverlay.generated.h"

UENUM()
enum class ECesiumVectorStylingProviderType : uint8 {
  None = 0 UMETA(ToolTip = "Only the default style will be used."),
  Blueprint = 1 UMETA(
      ToolTip =
          "Uses the Blueprint class implementing the ICesumVectorTilesStylingCallbacks interface specified in BlueprintStylingProvider."),
  Lambda = 2 UMETA(
      ToolTip =
          "Calls the TFunction specified on the overlay using C++ to create a VectorStylingProvider directly.")
};

UINTERFACE(Blueprintable, MinimalAPI)
class UCesiumVectorTilesStylingCallbacks : public UInterface {
  GENERATED_BODY()
};

class ICesiumVectorTilesStylingCallbacks {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  void OnStylingBegin(const FCesiumModelMetadata& InModelMetadata);
  virtual void
  OnStylingBegin_Implementation(const FCesiumModelMetadata& InModelMetadata) {}

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  bool OnStylePoint(
      int64 InFeatureId,
      const FVector& InPointLlh,
      FCesiumVectorStyle& OutVectorStyle);
  virtual bool OnStylePoint_Implementation(
      int64 InFeatureId,
      const FVector& InPointLlh,
      FCesiumVectorStyle& OutVectorStyle) {
    return false;
  }

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  bool OnStylePolyline(
      int64 InFeatureId,
      const TArray<FVector>& InPolylineLlh,
      FCesiumVectorStyle& OutVectorStyle);
  virtual bool OnStylePolyline_Implementation(
      int64 InFeatureId,
      const TArray<FVector>& InPolylineLlh,
      FCesiumVectorStyle& OutVectorStyle) {
    return false;
  }

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  bool OnStylePolygon(
      int64 InFeatureId,
      const TArray<FVector>& InPolygonLlh,
      FCesiumVectorStyle& OutVectorStyle);
  virtual bool OnStylePolygon_Implementation(
      int64 InFeatureId,
      const TArray<FVector>& InPolygonLlh,
      FCesiumVectorStyle& OutVectorStyle) {
    return false;
  }

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
  bool ShouldRunOnWorkerThread();
  virtual bool ShouldRunOnWorkerThread_Implementation() { return false; }
};

/**
 * Configures where the CesiumVectorTilesRasterOverlay should load its vector
 * data from.
 */
UENUM(BlueprintType)
enum class ECesiumVectorTilesRasterOverlaySource : uint8 {
  /**
   * The raster overlay will load a vector tileset from Cesium ion.
   */
  FromCesiumIon = 0,
  /**
   * The raster overlay will load a vector tileset from a URL.
   */
  FromUrl = 1
};

UCLASS(
    ClassGroup = Cesium,
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVectorTilesRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumVectorTilesRasterOverlaySource Source =
      ECesiumVectorTilesRasterOverlaySource::FromCesiumIon;
  /**
   * The ID of the Cesium ion asset to use.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  int64 IonAssetID;

  /**
   * The access token to use to access the Cesium ion resource.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  FString IonAccessToken;

  /**
   * The Cesium ion Server from which this raster overlay is loaded.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      AdvancedDisplay,
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  UCesiumIonServer* CesiumIonServer;

  /**
   * A URL to load a vector tiles tileset from.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromUrl"))
  FString Url;

  /**
   * Headers to use while making a request to `Url` to load a vector tiles
   * tileset.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromUrl"))
  TMap<FString, FString> RequestHeaders;

  /**
   * The number of mip levels to generate for each tile of this raster overlay.
   *
   * Additional mip levels can improve the visual quality of tiles farther from
   * the camera at the cost of additional rasterization time to create each mip
   * level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = "0", ClampMax = "8"))
  int32 MipLevels = 0;

  /**
   * The default style to use for this raster overlay.
   *
   * If no style information is present in the vector tiles tileset, this style
   * will be used instead.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Styling")
  FCesiumVectorStyle DefaultStyle;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Styling")
  ECesiumVectorStylingProviderType StylingProviderType =
      ECesiumVectorStylingProviderType::None;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Styling",
      meta =
          (EditCondition =
               "StylingProviderType == ECesiumVectorStylingProviderType::Blueprint",
           MustImplement =
               "/Script/CesiumRuntime.CesiumVectorTilesStylingCallbacks"))
  TSubclassOf<UObject> BlueprintStylingProvider;

  TOptional<TFunction<std::shared_ptr<CesiumVectorOverlays::VectorStylingProvider>()>>
      LambdaStylingProvider;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;

private:
  UPROPERTY(Transient)
  UObject* _pStylingInterfaceObject;
};

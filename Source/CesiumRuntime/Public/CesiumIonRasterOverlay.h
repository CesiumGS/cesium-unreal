// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlay.h"
#include "CesiumIonRasterOverlay.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumIonRasterOverlay : public UCesiumRasterOverlay
{
	GENERATED_BODY()

public:
	/**
	 * The ID of the Cesium ion asset to use. 
	 * 
	 * If this property is non-zero, the Bing Maps Key and Map Style properties are ignored.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString IonAccessToken;

protected:
	virtual std::unique_ptr<Cesium3DTiles::RasterOverlay> CreateOverlay() override;
};

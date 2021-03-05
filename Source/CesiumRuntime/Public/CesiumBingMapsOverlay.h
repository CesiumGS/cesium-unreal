// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlay.h"
#include "CesiumBingMapsOverlay.generated.h"

UENUM(BlueprintType)
enum class EBingMapsStyle : uint8 {
	Aerial UMETA(DisplayName = "Aerial"),
    AerialWithLabelsOnDemand UMETA(DisplayName = "Aerial with Labels"),
	RoadOnDemand UMETA(DisplayName = "Road"),
	CanvasDark UMETA(DisplayName = "Canvas Dark"),
	CanvasLight UMETA(DisplayName = "Canvas Light"),
	CanvasGray UMETA(DisplayName = "Canvas Gray"),
	OrdnanceSurvey UMETA(DisplayName = "Ordnance Survey"),
	CollinsBart UMETA(DisplayName = "Collins Bart")
};

/**
 * 
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumBingMapsOverlay : public UCesiumRasterOverlay
{
	GENERATED_BODY()

public:
	/**
	 * The Bing Maps API key to use. 
	 * 
	 * This property is ignored if the Ion Asset ID is non-zero.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString BingMapsKey;
	
	/**
	 * The map style to use. 
	 * 
	 * This property is ignored if the Ion Asset ID is non-zero.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	EBingMapsStyle MapStyle = EBingMapsStyle::Aerial;

protected:
	virtual std::unique_ptr<Cesium3DTiles::RasterOverlay> CreateOverlay() override;
};

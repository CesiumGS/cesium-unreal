// Fill out your copyright notice in the Description page of Project Settings.

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
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CESIUM_API UCesiumBingMapsOverlay : public UCesiumRasterOverlay
{
	GENERATED_BODY()

	/**
	 * The Bing Maps API key to use. If this property is set, the Ion Asset ID and Ion Access Token are ignored.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString BingMapsKey;
	
	/**
	 * The ID of the Cesium ion asset to use. This property is ignored if the Bing Maps Key is set.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium", meta=(EditCondition="!BingMapsKey"))
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium", meta = (EditCondition = "!BingMapsKey && IonAssetID"))
	FString IonAccessToken;

	/**
	 * The map style to use.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	EBingMapsStyle MapStyle = EBingMapsStyle::Aerial;

	virtual void AddToTileset(Cesium3DTiles::Tileset& tileset) override;
};

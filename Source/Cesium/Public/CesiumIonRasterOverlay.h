// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlay.h"
#include "CesiumIonRasterOverlay.generated.h"

USTRUCT()
struct FRectangularCutout {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Cesium")
	double west;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	double south;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	double east;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	double north;
};

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CESIUM_API UCesiumIonRasterOverlay : public UCesiumRasterOverlay
{
	GENERATED_BODY()

	/**
	 * The ID of the Cesium ion asset to use. If this property is non-zero, the Bing Maps Key and Map Style properties are ignored
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	FString IonAccessToken;

	/**
	 * Rectangular cutouts where this tileset should not be drawn. Each cutout is specified as "west,south,east,north" in decimal degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium|Experimental")
	TArray<FRectangularCutout> Cutouts;

	virtual void AddToTileset(Cesium3DTiles::Tileset& tileset) override;
};

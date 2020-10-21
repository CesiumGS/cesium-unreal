// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumRasterOverlay.h"
#include "CesiumIonRasterOverlay.generated.h"

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

	virtual void AddToTileset(Cesium3DTiles::Tileset& tileset) override;
};

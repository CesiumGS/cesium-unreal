// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Cesium3DTiles/RasterOverlay.h"
#include <memory>
#include "CesiumRasterOverlay.generated.h"

namespace Cesium3DTiles {
	class CreditSystem;
	class Tileset;
}

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

UCLASS(Abstract)
class CESIUM_API UCesiumRasterOverlay : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumRasterOverlay();

	/**
	 * Rectangular cutouts where this tileset should not be drawn. Each cutout is specified as "west,south,east,north" in decimal degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium|Experimental")
	TArray<FRectangularCutout> Cutouts;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddToTileset();
	void RemoveFromTileset();
	
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

protected:
	Cesium3DTiles::Tileset* FindTileset() const;
	virtual std::unique_ptr<Cesium3DTiles::RasterOverlay> CreateOverlay() PURE_VIRTUAL(UCesiumRasterOverlay::CreateOverlay, return nullptr;);

private:
	Cesium3DTiles::RasterOverlay* _pOverlay;
};

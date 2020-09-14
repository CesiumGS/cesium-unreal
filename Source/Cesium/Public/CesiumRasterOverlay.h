// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CesiumRasterOverlay.generated.h"

namespace Cesium3DTiles {
	class Tileset;
}

UCLASS(Abstract)
class CESIUM_API UCesiumRasterOverlay : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumRasterOverlay();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void AddToTileset(Cesium3DTiles::Tileset& tileset) PURE_VIRTUAL(UCesiumRasterOverlay::AddToTilset, return;);
		
};

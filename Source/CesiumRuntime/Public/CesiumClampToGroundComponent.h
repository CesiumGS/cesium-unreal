// Copyright 2024

#pragma once

#include "CoreMinimal.h"
#include "CesiumGlobeAnchoredActorComponent.h"
#include "CesiumClampToGroundComponent.generated.h"

class UCesiumGlobeAnchorComponent;

UCLASS(ClassGroup=(Cesium), meta=(BlueprintSpawnableComponent))
class UCesiumClampToGroundComponent : public UCesiumGlobeAnchoredActorComponent
{
	GENERATED_BODY()

  // update interval counter
  uint32_t _remainingSamples = 3000;
public:

#pragma region Properties
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float HeightToMaintain = -1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  float InitialHeight = -1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  // Get the actor's current world position
  FVector InitialPosition;


  /**
   * Frames to skip between samples.
   */
  unsigned int SampleInterval = 10;

#pragma endregion

#pragma region Property Accessors
#pragma endregion

public:
	// Sets default values for this component's properties
	UCesiumClampToGroundComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Whether to enable debug visualization of the height trace
	 */
	bool drawDebugTrace;

private:
	/**
	 * Perform the height query using a line trace
	 */
	float QueryTilesetHeight();
};

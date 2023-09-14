// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumOriginShiftComponent.generated.h"

class UCesiumGlobeAnchorComponent;

/**
 * Indicates how to shift the origin as the Actor to which a
 * CesiumOriginShiftComponent is attached moves.
 */
UENUM(BlueprintType)
enum class ECesiumOriginShiftMode : uint8 {
  /**
   * This component is disabled and will have no effect.
   */
  Disabled,

  /**
   * The origin of the CesiumGeoreference will be changed when the Actor enters
   * a new sub-level, but it will otherwise not be modified as the Actor moves.
   * Any objects that are not anchored to the globe with a
   * CesiumGlobeAnchorComponent will appear to move when the Actor enters a
   * sub-level.
   */
  SwitchSubLevelsOnly,

  /**
   * The origin of the CesiumGeoreference will change as the Actor moves in
   * order to maintain small, precise coordinate values near the Actor, as well
   * as to keep the globe's local "up" direction aligned with the +Z axis. Any
   * objects that are not anchored to the globe with a
   * CesiumGlobeAnchorComponent will appear to move whenever the origin changes.
   */
  ChangeCesiumGeoreference,

  /**
   * Unreal's built-in "World Origin Location" will change as the Actor moves in
   * order to maintain small, precise coordinate values near the Actor. Because
   * this is a translation only (no rotation), +Z will not stay aligned with the
   * globe's local "up" direction. Any objects that are not anchored to the
   * globe with a CesiumGlobeAnchorComponent will appear to move when the Actor
   * enters a sub-level, but objects will generally maintain their position
   * otherwise, as long as they respond correctly to Unreal's ApplyWorldOffset
   * method.
   */
  ChangeWorldOriginLocation
};

UCLASS(ClassGroup = "Cesium", Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumOriginShiftComponent : public UActorComponent {
  GENERATED_BODY()

#pragma region Properties
private:
  /**
   * Indicates how to shift the origin as the Actor to which this component is
   * attached moves.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetMode,
      BlueprintSetter = SetMode,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ECesiumOriginShiftMode Mode = ECesiumOriginShiftMode::SwitchSubLevelsOnly;

#pragma endregion

#pragma region Property Accessors
public:
  /**
   * Gets a value indicating how to shift the origin as the Actor to which this
   * component is attached moves.
   */
  UFUNCTION(BlueprintGetter)
  ECesiumOriginShiftMode GetMode() const;

  /**
   * Sets a value indicating how to shift the origin as the Actor to which this
   * component is attached moves.
   */
  UFUNCTION(BlueprintSetter)
  void SetMode(ECesiumOriginShiftMode NewMode);
#pragma endregion

public:
  UCesiumOriginShiftComponent();

protected:
  virtual void OnRegister() override;
  virtual void BeginPlay() override;
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

private:
  void ResolveGlobeAnchor();

  // The globe anchor attached to the same Actor as this component. Don't
  // save/load or copy this. It is set in BeginPlay.
  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  UCesiumGlobeAnchorComponent* GlobeAnchor;
};

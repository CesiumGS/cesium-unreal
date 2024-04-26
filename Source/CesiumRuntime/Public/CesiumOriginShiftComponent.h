// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGlobeAnchoredActorComponent.h"
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
   *
   * When using this mode, all Cesium3DTileset instances as well as any Actors
   * with a CesiumGlobeAnchorComponent need to be marked Movable, because these
   * objects _will_ be moved when the origin is shifted.
   */
  ChangeCesiumGeoreference,
};

/**
 * Automatically shifts the origin of the Unreal world coordinate system as the
 * object to which this component is attached moves. This improves rendering
 * precision by keeping coordinate values small, and can also help world
 * building by keeping the globe's local up direction aligned with the +Z axis.
 *
 * This component is typically attached to a camera or Pawn. By default, it only
 * shifts the origin when entering a new sub-level (a Level Instance Actor with
 * a CesiumSubLevelComponent attached to it). By changing the Mode and Distance
 * properties, it can also shift the origin continually when in between
 * sub-levels (or when not using sub-levels at all).
 *
 * It is essential to add a CesiumGlobeAnchorComponent to all other non-globe
 * aware objects in the level; otherwise, they will appear to move when the
 * origin is shifted. It is not necessary to anchor objects that are in
 * sub-levels, because the origin remains constant for the entire time that a
 * sub-level is active.
 */
UCLASS(ClassGroup = "Cesium", Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumOriginShiftComponent
    : public UCesiumGlobeAnchoredActorComponent {
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

  /**
   * The maximum distance between the origin of the Unreal coordinate system and
   * the Actor to which this component is attached. When this distance is
   * exceeded, the origin is shifted to bring it close to the Actor. This
   * property is ignored if the Mode property is set to "Disabled" or "Switch
   * Sub Levels Only".
   *
   * When the value of this property is 0.0, the origin is shifted continuously.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetDistance,
      BlueprintSetter = SetDistance,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  double Distance = 0.0;
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

  /**
   * Gets the maximum distance between the origin of the Unreal coordinate
   * system and the Actor to which this component is attached. When this
   * distance is exceeded, the origin is shifted to bring it close to the Actor.
   * This property is ignored if the Mode property is set to "Disabled" or
   * "Switch Sub Levels Only".
   *
   * When the value of this property is 0.0, the origin is shifted continuously.
   */
  UFUNCTION(BlueprintGetter)
  double GetDistance() const;

  /**
   * Sets the maximum distance between the origin of the Unreal coordinate
   * system and the Actor to which this component is attached. When this
   * distance is exceeded, the origin is shifted to bring it close to the Actor.
   * This property is ignored if the Mode property is set to "Disabled" or
   * "Switch Sub Levels Only".
   *
   * When the value of this property is 0.0, the origin is shifted continuously.
   */
  UFUNCTION(BlueprintSetter)
  void SetDistance(double NewDistance);
#pragma endregion

public:
  UCesiumOriginShiftComponent();

protected:
  virtual void TickComponent(
      float DeltaTime,
      ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;
};

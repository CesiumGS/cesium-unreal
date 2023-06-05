// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "UObject/ObjectMacros.h"
#include "CesiumSubLevelInstance.generated.h"

UCLASS()
class CESIUMRUNTIME_API ACesiumSubLevelInstance : public ALevelInstance {
  GENERATED_BODY()

public:
  /**
   * Resolves the Cesium Georeference to use with this Actor. Returns
   * the value of the Georeference property if it is set. Otherwise, finds a
   * Georeference in the World and returns it, creating it if necessary. The
   * resolved Georeference is cached so subsequent calls to this function will
   * return the same instance.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* ResolveGeoreference();

  /**
   * Invalidates the cached resolved georeference, unsubscribing from it and
   * setting it to null. The next time ResolveGeoreference is called, the
   * Georeference will be re-resolved and re-subscribed.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void InvalidateResolvedGeoreference();

  /** @copydoc ACesiumSubLevelInstance::Georeference */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* GetGeoreference() const;

  /** @copydoc ACesiumSubLevelInstance::Georeference */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetGeoreference(ACesiumGeoreference* NewGeoreference);

#if WITH_EDITOR
  virtual void SetIsTemporarilyHiddenInEditor(bool bIsHidden) override;
#endif

  virtual void BeginDestroy() override;
  virtual void OnConstruction(const FTransform& Transform) override;

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

private:
  /**
   * The designated georeference actor controlling how the actor's
   * coordinate system relates to the coordinate system in this Unreal Engine
   * level.
   *
   * If this is null, the sub-level will find and use the first Georeference
   * Actor in the level, or create one if necessary. To get the active/effective
   * Georeference from Blueprints or C++, use ResolvedGeoreference instead.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetGeoreference,
      BlueprintSetter = SetGeoreference,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ACesiumGeoreference* Georeference;

  /**
   * The resolved georeference used by this sub-level. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * Actor is in a sublevel. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called.
   */
  UPROPERTY(
      Transient,
      BlueprintReadOnly,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ACesiumGeoreference* ResolvedGeoreference = nullptr;
};

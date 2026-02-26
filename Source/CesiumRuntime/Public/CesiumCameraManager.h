// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Camera/CameraComponent.h"
#include "CesiumCamera.h"
#include "Containers/Map.h"
#include "Engine/SceneCapture2D.h"
#include "GameFramework/Actor.h"

#include "CesiumCameraManager.generated.h"

/**
 * @brief Manages custom {@link FCesiumCamera}s for all
 * {@link ACesium3DTileset}s in the world.
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumCameraManager : public AActor {
  GENERATED_BODY()

public:
  /**
   * @brief Determines whether the cameras attached to PlayerControllers should
   * be used for Cesium3DTileset culling and level-of-detail.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool UsePlayerCameras = true;
  /**
   * @brief Determines whether the camera associated with the Editor's active
   * scene view should be used for Cesium3DTileset culling and level-of-detail.
   * In a game, this property has no effect.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool UseEditorCameras = true;

  /**
   * @brief Whether to find and use all scene captures within the level for
   * Cesium3DTileset culling and level-of-detail.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool UseSceneCapturesInLevel = true;

  /**
   * @brief Array of explicit scene capture actors.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      Meta = (EditCondition = "!UseSceneCapturesInLevel"))
  TArray<TObjectPtr<ASceneCapture2D>> SceneCaptures;

  /**
   * @brief Array of additional cameras.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TArray<FCesiumCamera> AdditionalCameras;

  /**
   * @brief Get the camera manager for this world.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "CesiumCameraManager",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumCameraManager*
  GetDefaultCameraManager(const UObject* WorldContextObject);

  ACesiumCameraManager();

  /**
   * @brief DEPRECATED
   * @deprecated The AdditionalCameras array should be used for directly storing
   * and accessing cameras.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use the AdditionalCameras array instead."))
  int32 AddCamera(UPARAM(ref) const FCesiumCamera& Camera);

  /**
   * @brief DEPRECATED
   * @deprecated The AdditionalCameras array should be used for directly storing
   * and accessing cameras.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use the AdditionalCameras array instead."))
  bool RemoveCamera(int32 CameraId);

  /**
   * @brief DEPRECATED
   * @deprecated The AdditionalCameras array should be used for directly storing
   * and accessing cameras.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use the AdditionalCameras array instead."))
  bool UpdateCamera(int32 CameraId, UPARAM(ref) const FCesiumCamera& Camera);

  /**
   * @brief DEPRECATED
   * @deprecated The AdditionalCameras array should be used for directly storing
   * and accessing cameras.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use the AdditionalCameras array instead."))
  const TMap<int32, FCesiumCamera>& GetCameras() const;

  virtual bool ShouldTickIfViewportsOnly() const override;

  virtual void Tick(float DeltaTime) override;

  /**
   * @brief Return a list of all cameras handled by the manager.
   */
  std::vector<FCesiumCamera> GetAllCameras() const;

private:
  /**
   * Support for deprecated camera interface.
   */
  int32 _currentCameraId = 0;
  TMap<int32, FCesiumCamera> _cameras;

  static FName DEFAULT_CAMERAMANAGER_TAG;
};

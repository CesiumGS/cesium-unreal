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
   * @brief Array of additional cameras.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TArray<FCesiumCamera> AdditionalCameras;

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
   * @brief Register a new camera with the camera manager.
   *
   * @param Camera The current state for the new camera.
   * @return The generated ID for this camera. Use this ID to refer to the
   * camera in the future when calling UpdateCamera.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  int32 AddCamera(UPARAM(ref) const FCesiumCamera& Camera);

  /**
   * @brief Unregister an existing camera with the camera manager.
   *
   * @param CameraId The ID of the camera, as returned by AddCamera during
   * registration.
   * @return Whether the updating was successful. If false, the CameraId was
   * invalid.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool RemoveCamera(int32 CameraId);

  /**
   * @brief Update the state of the specified camera.
   *
   * @param CameraId The ID of the camera, as returned by AddCamera during
   * registration.
   * @param Camera The new, updated state of the camera.
   * @return Whether the updating was successful. If false, the CameraId was
   * invalid.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool UpdateCamera(int32 CameraId, UPARAM(ref) const FCesiumCamera& Camera);

  /**
   * @brief Get a read-only map of the current camera IDs to cameras. These
   * cameras have been added to the manager with {@link AddCamera}.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  const TMap<int32, FCesiumCamera>& GetCameras() const;

  virtual bool ShouldTickIfViewportsOnly() const override;

  virtual void Tick(float DeltaTime) override;

  /**
   * @brief Return a list of all cameras handled by the manager.
   */
  std::vector<FCesiumCamera> GetAllCameras() const;

private:
  int32 _currentCameraId = 0;
  TMap<int32, FCesiumCamera> _cameras;

  static FName DEFAULT_CAMERAMANAGER_TAG;
};

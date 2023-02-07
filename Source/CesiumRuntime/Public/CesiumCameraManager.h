// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCamera.h"
#include "Containers/Map.h"
#include "GameFramework/Actor.h"

#include "CesiumCameraManager.generated.h"

/**
 * @brief Manages custom {@link FCesiumCamera}s for all
 * {@link Cesium3DTileset}s in the world.
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumCameraManager : public AActor {
  GENERATED_BODY()

public:
  /**
   * @brief Get the camera manager for this world.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "CesiumCameraManager",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumCameraManager*
  GetDefaultCameraManager(const UObject* WorldContextObject);

  ACesiumCameraManager() {}

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
   * @brief Get a read-only map of the current camera IDs to cameras.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  const TMap<int32, FCesiumCamera>& GetCameras() const;

  virtual bool ShouldTickIfViewportsOnly() const override;

  virtual void Tick(float DeltaTime) override;

private:
  int32 _currentCameraId = 0;
  TMap<int32, FCesiumCamera> _cameras;

  static FName DEFAULT_CAMERAMANAGER_TAG;
};

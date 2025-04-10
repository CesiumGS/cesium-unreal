// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Camera/CameraComponent.h"
#include "CesiumCamera.h"
#include "Containers/Map.h"
#include "GameFramework/Actor.h"
#include "CesiumCameraManager.generated.h"

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumViewGroup {
  GENERATED_USTRUCT_BODY()

  /**
   * A human-readable description of this view group.
   */ 
  UPROPERTY(Category = "Cesium", VisibleAnywhere, BlueprintReadOnly)
  FString ViewDescription = "Unknown";

  /**
   * The index of the viewing FEditorViewportClient in
   * GEditor->GetAllViewportClients(), or -1 if this view is not an editor
   * viewport client.
   */
  UPROPERTY(Category = "Cesium", VisibleAnywhere, BlueprintReadOnly)
  int32 EditorViewportIndex = -1;

  /**
   * The viewing Actor, which is expected to have either a
   * USceneCaptureComponent2D or a UCameraComponent attached to it.
   */
  UPROPERTY(Category = "Cesium", VisibleAnywhere, BlueprintReadOnly)
  TSoftObjectPtr<AActor> ViewActor = nullptr;

  /**
   * The unique ID of the FSceneViewStateInterface, as returned by its
   * GetViewKey method.
   */
  UPROPERTY(Category = "Cesium", VisibleAnywhere, BlueprintReadOnly)
  int64 ViewStateKey = -1;

  /**
   * The weight of this view group, used to prioritize tile loading.
   */
  UPROPERTY(Category = "Cesium", EditAnywhere, BlueprintReadWrite)
  double LoadWeight = 1.0;
};

/**
 * @brief Manages custom {@link FCesiumCamera}s for all
 * {@link ACesium3DTileset}s in the world.
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
   * @brief Get a read-only map of the current camera IDs to cameras.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  const TMap<int32, FCesiumCamera>& GetCameras() const;

  virtual bool ShouldTickIfViewportsOnly() const override;

  virtual void Tick(float DeltaTime) override;

  UPROPERTY(Category = "Cesium", EditAnywhere, BlueprintReadWrite)
  TArray<FCesiumViewGroup> ViewGroups;

private:
  int32 _currentCameraId = 0;
  TMap<int32, FCesiumCamera> _cameras;

  static FName DEFAULT_CAMERAMANAGER_TAG;
};

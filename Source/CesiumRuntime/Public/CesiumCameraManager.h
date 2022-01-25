// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCamera.h"
#include "Containers/Map.h"
#include "GameFramework/Actor.h"

#include "CesiumCameraManager.generated.h"

UCLASS()
class CESIUMRUNTIME_API ACesiumCameraManager : public AActor {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      Category = "CesiumCameraManager",
      meta = (WorldContext = "WorldContextObject"))
  static ACesiumCameraManager*
  GetDefaultCameraManager(const UObject* WorldContextObject);

  ACesiumCameraManager() {}

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  int32 AddCamera(UPARAM(ref) const FCesiumCamera& camera);

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool UpdateCamera(int32 cameraId, UPARAM(ref) const FCesiumCamera& camera);

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  const TMap<int32, FCesiumCamera>& GetCameras() const;

  virtual bool ShouldTickIfViewportsOnly() const override;

  virtual void Tick(float DeltaTime) override;

private:
  int32 _currentCameraId = 0;
  TMap<int32, FCesiumCamera> _cameras;

  static FName DEFAULT_CAMERAMANAGER_TAG;
};

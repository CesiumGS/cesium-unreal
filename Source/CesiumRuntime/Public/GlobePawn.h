// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"

#include "GlobePawn.generated.h"

USTRUCT()
struct FDecomposeComponents {
  GENERATED_BODY()
  FMatrix Esu_M;
  FRotator Local_R;
  FVector Local_P;
};

UENUM()
enum EGlobePawnInputType {
  Rotate,
  Pan,
  Zoom,
};

UCLASS(
    DisplayName = "Globe Pawn",
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API AGlobePawn : public APawn {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bRuntimeDebug{false};

  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bEnablePan{true};
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bEnableRotate{true};
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bEnableZoom{true};

  AGlobePawn();

  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(
      class UInputComponent* PlayerInputComponent) override;

  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (DisplayName = "GetPawnGeoHeight"))
  double GetPawnGeoHeight();

  /**
   * Ray intersection WGS84 ellipsoid with custom height
   * @param ViewportPosition pick position in screen position.
   * @param Height intersection ellipsoid height, optional, default height is 0.
   * @return return position in ecef space.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (DisplayName = "PickEllipsoidECEF"))
  FVector PickEllipsoidECEF(
      const FVector2D& ViewportPosition,
      const double Height) const;

  /**
   * Ray intersection WGS84 ellipsoid with custom height
   * @param ViewportPosition pick position in screen position.
   * @param Height intersection ellipsoid height, optional, default height is 0.
   * @return return position in unreal space.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (DisplayName = "PickEllipsoidUnreal"))
  FVector PickEllipsoidUnreal(
      const FVector2D& ViewportPosition,
      const double Height) const;

  /**
   * Ray intersection WGS84 ellipsoid with custom height
   * @param ViewportPosition pick position in screen position.
   * @param Height intersection ellipsoid height, optional, default height is 0.
   * @return return cartographic, x for longitude,y for latitude, z for height.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (DisplayName = "PickEllipsoidCartographic"))
  FVector PickEllipsoidCartographic(
      const FVector2D& ViewportPosition,
      const double Height) const;

protected:
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UInputMappingContext> InputMappingContext;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UInputAction> MousePanAction;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UInputAction> MouseRotateAction;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UInputAction> MouseZoomAction;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UCameraComponent> Camera;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UCesiumGlobeAnchorComponent> GlobeAnchor;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UCesiumOriginShiftComponent> OriginShift;

  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  TObjectPtr<class UCesiumFlyToComponent> FlyTo;

  virtual void BeginPlay() override;

private:
  TObjectPtr<class AGlobeAnchorActor> GlobeTransformer;

  bool bPanPressed{false};
  bool bSpinPressed{false};
  bool bRotatePressed{false};
  bool bZoomTriggered{false};

  FVector2D PreFrameMousePosition{0.0f, 0.0f};
  FVector2D CurFrameMousePosition{0.0f, 0.0f};
  FVector2D DeltaMousePosition{0.0f, 0.0f};

  float MaxLineTraceHeight{30000.0};

  float MaxLocalRotateHeight{60000.0};
  float MinSpinHeight{8000000.0};

  FVector RotateAnchor{};
  FVector PanAnchor{};
  FVector ZoomAnchor{};

  double PanInertia{0.0f};
  const double MaxPanInertia{40.0f};
  FVector DeltaPanCartographic;

  double SpinInertia{0.0f};
  const double MaxSpinInertia{MaxPanInertia};
  FDecomposeComponents SpinAnchor{};
  FVector2D SpinInertiaDelta{0.0f, 0.0f};
  bool bSpinHorizontal;

  double RotateInertia{0.0f};
  const double MaxRotateInertia{20.0f};
  const double ClampStart{MinSpinHeight * 10.0};
  const double ClampStop{ClampStart * 5.0};
  const double MaxZoomRadius{ClampStop * 3.5};
  FVector2D RotateInertiaDelta{0.0f, 0.0f};

  double ZoomAmount{0.0f};
  double ZoomScale{0.5f};
  double ZoomInertia{0.0f};
  const double MaxZoomInertia{30.0f};
  FVector2D ZoomMousePosition{0.0f, 0.0f};

  void SpawnGlobeTransformer();

  void UpdateGlobeTransformer() const;

  void InitInput() const;

  void MousePanPressed();

  void MousePanReleased();

  void MouseRotatePressed();

  void MouseRotateReleased();

  void MouseZoomTriggered(const FInputActionValue& Input);

  void UpdateMousePosition();

  bool
  PanActorAndSnap(const FVector& Anchor, const FVector2D& ViewportPosition);

  void PanActor(const FVector& DeltaCartographic);

  void UpdatePan();

  void StartSpin();

  void SpinAroundGlobe(const FVector2D& Delta);

  void UpdateSpin();

  void RotateActorAround(const FVector2D& Delta);

  void UpdateRotate();

  void UpdateZoom();

  static void DecreaseInertia(double& Inertia, const double MaxInertia);

  double PitchClamp(const float Radius) const;

  float GetCenterRadius(FVector& Center);

  FVector LineTraceWorld(const FVector2D& ViewportPosition) const;

  FVector PickEllipsoidOrLineTraceWorld(const FVector2D& ViewportPosition);

  bool IntersectionTest(const FVector& Start, const double Tolerance);

  FDecomposeComponents DecomposeFromLocation(const FVector& Location) const;

  void SetActorTransformDecomposeComponents(
      const FDecomposeComponents& DecomposeComponents);

  void ResetInertia(const EGlobePawnInputType& Input);

  bool OtherPressing(const EGlobePawnInputType& Input) const;

  bool DeprojectScreenPositionToWorld(
      float ScreenX,
      float ScreenY,
      FVector& WorldLocation,
      FVector& WorldDirection) const;
};

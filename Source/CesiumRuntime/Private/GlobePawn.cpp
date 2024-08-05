// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "GlobePawn.h"
#include "Camera/CameraComponent.h"
#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumOriginShiftComponent.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GlobeAnchorActor.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"
#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <optional>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

AGlobePawn::AGlobePawn() {
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContext;
    ConstructorHelpers::FObjectFinder<UInputAction> MousePanAction;
    ConstructorHelpers::FObjectFinder<UInputAction> MouseRotateAction;
    ConstructorHelpers::FObjectFinder<UInputAction> MouseZoomAction;

    FConstructorStatics()
        : InputMappingContext(
              TEXT("/CesiumForUnreal/Input/IMC_Default.IMC_Default")),
          MousePanAction(TEXT("/CesiumForUnreal/Input/IA_Pan.IA_Pan")),
          MouseRotateAction(TEXT("/CesiumForUnreal/Input/IA_Rotate.IA_Rotate")),
          MouseZoomAction(TEXT("/CesiumForUnreal/Input/IA_Zoom.IA_Zoom")) {}
  };
  static FConstructorStatics ConstructorStatics;

  InputMappingContext =
      Cast<UInputMappingContext>(ConstructorStatics.InputMappingContext.Object);
  MousePanAction = Cast<UInputAction>(ConstructorStatics.MousePanAction.Object);
  MouseRotateAction =
      Cast<UInputAction>(ConstructorStatics.MouseRotateAction.Object);
  MouseZoomAction =
      Cast<UInputAction>(ConstructorStatics.MouseZoomAction.Object);

  Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
  Camera->bUsePawnControlRotation = false;
  RootComponent = Camera;

  GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
  OriginShift =
      CreateDefaultSubobject<UCesiumOriginShiftComponent>(TEXT("OriginShift"));
  FlyTo = CreateDefaultSubobject<UCesiumFlyToComponent>(TEXT("FlyTo"));

  PrimaryActorTick.bCanEverTick = true;

  AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AGlobePawn::BeginPlay() {
  Super::BeginPlay();
  SpawnGlobeTransformer();
  InitInput();
}

void AGlobePawn::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  UpdateGlobeTransformer();
  UpdateMousePosition();
  UpdateRotate();
  UpdatePan();
  UpdateSpin();
  UpdateZoom();
}

void AGlobePawn::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  const TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent =
      CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
  if (!EnhancedInputComponent)
    return;

  // Pan
  EnhancedInputComponent->BindAction(
      MousePanAction,
      ETriggerEvent::Started,
      this,
      &AGlobePawn::MousePanPressed);
  EnhancedInputComponent->BindAction(
      MousePanAction,
      ETriggerEvent::Completed,
      this,
      &AGlobePawn::MousePanReleased);
  // Rotate
  EnhancedInputComponent->BindAction(
      MouseRotateAction,
      ETriggerEvent::Started,
      this,
      &AGlobePawn::MouseRotatePressed);
  EnhancedInputComponent->BindAction(
      MouseRotateAction,
      ETriggerEvent::Completed,
      this,
      &AGlobePawn::MouseRotateReleased);
  // Move
  EnhancedInputComponent->BindAction(
      MouseZoomAction,
      ETriggerEvent::Triggered,
      this,
      &AGlobePawn::MouseZoomTriggered);
}

double AGlobePawn::GetPawnGeoHeight() {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  return Georeference
      ->TransformUnrealPositionToLongitudeLatitudeHeight(GetActorLocation())
      .Z;
}

FVector AGlobePawn::PickEllipsoidECEF(
    const FVector2D& ViewportPosition,
    const double Height = 0) const {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  FVector Start, Direction;
  if (!DeprojectScreenPositionToWorld(
          ViewportPosition.X,
          ViewportPosition.Y,
          Start,
          Direction))
    return FVector::Zero();

  glm::dvec3 WGS84_Radii{Ellipsoid::WGS84.getRadii() + Height};
  Start = Georeference->TransformUnrealPositionToEarthCenteredEarthFixed(Start);
  Direction = Georeference->TransformUnrealDirectionToEarthCenteredEarthFixed(
      Direction);
  Direction.Normalize();

  const std::optional<glm::dvec2> Intersection =
      IntersectionTests::rayEllipsoid(
          Ray(VecMath::createVector3D(Start),
              VecMath::createVector3D(Direction)),
          WGS84_Radii);
  if (Intersection == std::nullopt)
    return FVector::Zero();
  return Start + Intersection.value().x * Direction;
}

FVector AGlobePawn::PickEllipsoidUnreal(
    const FVector2D& ViewportPosition,
    const double Height = 0) const {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();

  const FVector Result = PickEllipsoidECEF(ViewportPosition, Height);
  if (Result.Equals(FVector::Zero()))
    return FVector::Zero();
  return Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(Result);
}

FVector AGlobePawn::PickEllipsoidCartographic(
    const FVector2D& ViewportPosition,
    const double Height = 0) const {
  const FVector Result = PickEllipsoidECEF(ViewportPosition, Height);
  return UCesiumWgs84Ellipsoid::
      EarthCenteredEarthFixedToLongitudeLatitudeHeight(Result);
}

void AGlobePawn::SpawnGlobeTransformer() {
  GlobeTransformer = GetWorld()->SpawnActor<AGlobeAnchorActor>();
}

void AGlobePawn::UpdateGlobeTransformer() const {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  GlobeTransformer->GlobeAnchor->SetGeoreference(Georeference);
}

void AGlobePawn::InitInput() const {
  const TObjectPtr<APlayerController> PlayerController =
      Cast<APlayerController>(Controller);
  if (!PlayerController)
    return;
  const TObjectPtr<UEnhancedInputLocalPlayerSubsystem> Subsystem =
      ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
          PlayerController->GetLocalPlayer());
  if (!Subsystem)
    return;
  Subsystem->AddMappingContext(InputMappingContext, 0);

  PlayerController->bShowMouseCursor = true;

  const TObjectPtr<UWorld> World = GetWorld();
  TArray<AActor*> OutActors{};
  UGameplayStatics::GetAllActorsOfClass(World, StaticClass(), OutActors);
  if (const TObjectPtr<APawn> TargetPawn = Cast<APawn>(OutActors[0])) {
    PlayerController->Possess(TargetPawn);
  }
}

void AGlobePawn::MousePanPressed() {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  if (!bEnablePan || OtherPressing(Pan))
    return;
  ResetInertia(Pan);
  PanAnchor = PickEllipsoidOrLineTraceWorld(CurFrameMousePosition);
  const float Latitude =
      Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(PanAnchor)
          .Y;
  FVector Center;
  const double CenterRadius = GetCenterRadius(Center);
  if (PanAnchor != FVector::Zero() && (Latitude < 80.0 && Latitude > -80.0) &&
      CenterRadius < ClampStop) {
    bPanPressed = true;
  } else {
    StartSpin();
  }
}

void AGlobePawn::MousePanReleased() {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  if (OtherPressing(Pan))
    return;
  if (bPanPressed) {
    bPanPressed = false;
    PanInertia = MaxPanInertia;
    const FVector PrePick =
        PickEllipsoidUnreal(CurFrameMousePosition - DeltaMousePosition);
    const FVector CurPick = PickEllipsoidUnreal(CurFrameMousePosition);

    if (PrePick == FVector::Zero() || CurPick == FVector::Zero()) {
      DeltaPanCartographic.Set(0.0f, 0.0f, 0.0f);
      return;
    }

    DeltaPanCartographic =
        Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(
            PrePick) -
        Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(CurPick);
    DeltaPanCartographic /= 2.0;
    DeltaPanCartographic.Z = 0;
  }

  if (bSpinPressed) {
    bSpinPressed = false;
    SpinInertia = MaxSpinInertia;
    SpinInertiaDelta = DeltaMousePosition;
  }
}

void AGlobePawn::MouseRotatePressed() {
  if (!bEnableRotate || OtherPressing(Rotate))
    return;
  ResetInertia(Rotate);
  FVector2D TargetPosition{0.0f, 0.0f};
  if (GetPawnGeoHeight() > MaxLineTraceHeight) {
    GEngine->GameViewport->GetViewportSize(TargetPosition);
    TargetPosition /= 2.0;
  } else {
    TargetPosition.Set(CurFrameMousePosition.X, CurFrameMousePosition.Y);
  }

  RotateAnchor = PickEllipsoidOrLineTraceWorld(TargetPosition);
  if (RotateAnchor != FVector::Zero()) {
    bRotatePressed = true;
  } else {
    RotateAnchor = GetActorLocation();
    bRotatePressed = true;
  }
}

void AGlobePawn::MouseRotateReleased() {
  if (OtherPressing(Rotate))
    return;
  bRotatePressed = false;
  RotateInertia = MaxRotateInertia;
  RotateInertiaDelta.Set(DeltaMousePosition.X, DeltaMousePosition.Y);
}

void AGlobePawn::MouseZoomTriggered(const FInputActionValue& Input) {
  if (!bEnableZoom)
    return;
  ResetInertia(Zoom);
  bZoomTriggered = true;
  ZoomAmount = Input.Get<float>();
  ZoomAmount /= FMath::Abs(ZoomAmount);
  ZoomAmount *= ZoomScale;

  if (FVector2d::Distance(ZoomMousePosition, CurFrameMousePosition) > 10.0) {
    ZoomMousePosition.Set(CurFrameMousePosition.X, CurFrameMousePosition.Y);
    ZoomAnchor = PickEllipsoidOrLineTraceWorld(ZoomMousePosition);
  }
}

void AGlobePawn::UpdateMousePosition() {
  const TObjectPtr<APlayerController> PlayerController =
      Cast<APlayerController>(Controller);
  if (!PlayerController)
    return;
  PlayerController->GetLocalPlayer()->ViewportClient->GetMousePosition(
      CurFrameMousePosition);
  DeltaMousePosition = CurFrameMousePosition - PreFrameMousePosition;
  PreFrameMousePosition = CurFrameMousePosition;
}

bool AGlobePawn::PanActorAndSnap(
    const FVector& Anchor,
    const FVector2D& ViewportPosition) {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  const double Height =
      Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Anchor).Z;
  FVector Target = PickEllipsoidUnreal(ViewportPosition, Height);

  if (Target.Equals(FVector::Zero()) ||
      (GetPawnGeoHeight() > MaxLocalRotateHeight &&
       PickEllipsoidUnreal(ViewportPosition, -GetPawnGeoHeight() / 50)
           .Equals(FVector::Zero()))) {
    return false;
  }

  Target =
      Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Anchor) *
          2.0f -
      Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Target);
  Target =
      Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(Target);
  auto [Esu_M, Local_R, Local_P] = DecomposeFromLocation(Anchor);
  GlobeTransformer->SetLocationAndSnap(Target);
  Esu_M = GlobeTransformer->GetActorTransform().ToMatrixNoScale();
  SetActorTransformDecomposeComponents(
      FDecomposeComponents{Esu_M, Local_R, Local_P});
  return true;
}

void AGlobePawn::PanActor(const FVector& DeltaCartographic) {
  if (DeltaCartographic == FVector::Zero())
    return;
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  FVector Target = GetActorLocation();
  Target =
      Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Target);
  Target += DeltaCartographic;
  Target.Y = FMath::Clamp(Target.Y, -89.0, 89.0);
  Target =
      Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(Target);
  auto [Esu_M, Local_R, Local_P] = DecomposeFromLocation(GetActorLocation());
  GlobeTransformer->SetLocationAndSnap(Target);
  Esu_M = GlobeTransformer->GetActorTransform().ToMatrixNoScale();
  SetActorTransformDecomposeComponents(
      FDecomposeComponents{Esu_M, Local_R, Local_P});
}

void AGlobePawn::UpdatePan() {
  if (!bEnablePan)
    return;
  if (bPanPressed) {
    if (DeltaMousePosition == FVector2D::Zero())
      return;
    if (!PanActorAndSnap(PanAnchor, CurFrameMousePosition)) {
      StartSpin();
    }
  } else if (PanInertia > 0.0) {
    PanActor(DeltaPanCartographic * (PanInertia / MaxPanInertia));
    DecreaseInertia(PanInertia, MaxPanInertia);
  }
}

void AGlobePawn::StartSpin() {
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  const float EsuYaw = GlobeAnchor->GetEastSouthUpRotation().Rotator().Yaw;
  FVector Center;

  bSpinHorizontal = FMath::Abs(FMath::Abs(EsuYaw) - 90) < 5 &&
                    GetCenterRadius(Center) > ClampStop &&
                    Georeference->GetOriginPlacement() !=
                        EOriginPlacement::CartographicOrigin;
  if (bSpinHorizontal) {
    SpinAnchor = DecomposeFromLocation(
        Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(
            FVector::Zero()));
  } else {
    const FMatrix Esu_M =
        FTransform{
            GetActorRotation(),
            Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(
                FVector::Zero()),
            FVector::One()}
            .ToMatrixWithScale();
    const FVector Local_P = Esu_M.InverseTransformPosition(GetActorLocation());
    SpinAnchor = FDecomposeComponents{Esu_M, FRotator::ZeroRotator, Local_P};
  }
  bPanPressed = false;
  bSpinPressed = true;
}

void AGlobePawn::SpinAroundGlobe(const FVector2D& Delta) {
  FVector2D CurPosition2D;
  GEngine->GameViewport->GetViewportSize(CurPosition2D);
  CurPosition2D /= 2.0;
  FVector CurCenter = PickEllipsoidOrLineTraceWorld(CurPosition2D);
  const FVector2D PrePosition2D = CurPosition2D - Delta;
  FVector PreCenter = PickEllipsoidOrLineTraceWorld(PrePosition2D);
  CurCenter = SpinAnchor.Esu_M.InverseTransformPosition(CurCenter);
  CurCenter.Normalize();
  PreCenter = SpinAnchor.Esu_M.InverseTransformPosition(PreCenter);
  PreCenter.Normalize();
  const FRotator CurRotator = CurCenter.Rotation();
  const FRotator PreRotator = PreCenter.Rotation();
  const FRotator DeltaRotator = CurRotator - PreRotator;

  if (FMath::Abs(SpinAnchor.Local_R.Pitch) < 80)
    SpinAnchor.Local_R.Yaw -= DeltaRotator.Yaw;

  if (!(bSpinHorizontal &&
        (Delta.Y == 0.0f || FMath::Abs(Delta.X / Delta.Y) > 5.0)))
    SpinAnchor.Local_R.Pitch += DeltaRotator.Pitch;

  SetActorTransformDecomposeComponents(SpinAnchor);
}

void AGlobePawn::UpdateSpin() {
  if (!bEnablePan)
    return;
  if (bSpinPressed) {
    SpinAroundGlobe(DeltaMousePosition);
  } else if (SpinInertia > 0.0) {
    SpinAroundGlobe(SpinInertiaDelta * (SpinInertia / MaxSpinInertia));
    DecreaseInertia(SpinInertia, MaxSpinInertia);
  }
}

void AGlobePawn::RotateActorAround(const FVector2D& Delta) {
  auto [Esu_M, Local_R, Local_P] = DecomposeFromLocation(RotateAnchor);

  Local_R.Yaw += Delta.X * 0.2;
  Local_R.Pitch -= Delta.Y * 0.2;
  Local_R.Pitch = FMath::Clamp(Local_R.Pitch, -89.0, 89.0);
  if (Local_R.Roll > 1.0) {
    Local_R.Roll -= 1.0;
    Local_R.Roll = FMath::Clamp(Local_R.Roll, 0.001, 180.0);
  } else if (Local_R.Roll < -1.0) {
    Local_R.Roll += 1.0;
    Local_R.Roll = FMath::Clamp(Local_R.Roll, -180.0, -0.001);
  }
  Local_R.Pitch =
      FMath::Clamp(Local_R.Pitch, -89.0, PitchClamp(Local_P.Length()));

  SetActorTransformDecomposeComponents(
      FDecomposeComponents{Esu_M, Local_R, Local_P});
}

void AGlobePawn::UpdateRotate() {
  if (!bEnableRotate)
    return;
  if (bRotatePressed) {
    if (DeltaMousePosition == FVector2D::Zero())
      return;
    RotateActorAround(DeltaMousePosition);
  } else if (RotateInertia > 0.0) {
    RotateActorAround(RotateInertiaDelta * (RotateInertia / MaxRotateInertia));
    DecreaseInertia(RotateInertia, MaxRotateInertia);
  }
}

void AGlobePawn::UpdateZoom() {
  if (bZoomTriggered) {
    ZoomInertia = MaxZoomInertia;
    bZoomTriggered = false;
  }

  if (ZoomInertia == 0.0) {
    // ZoomAnchor.Set(0.0f, 0.0f, 0.0f);
    return;
  };

  FVector ViewportCenter3D;
  const double ZoomRadius = GetCenterRadius(ViewportCenter3D);
  if (ZoomAmount < 0.0 && ZoomRadius > MaxZoomRadius)
    return;

  FVector Offset;
  if (ZoomAnchor == FVector::Zero()) {
    Offset = GetActorForwardVector();
    Offset *= ZoomAmount * FMath::Abs(GetPawnGeoHeight()) *
              (ZoomInertia / MaxZoomInertia);
    AddActorWorldOffset(Offset);
  } else if (
      ZoomAmount < 0.0 && ZoomRadius >= ClampStart &&
      GlobeAnchor->GetEastSouthUpRotation().Rotator().Pitch > -89.0) {
    auto [Esu_M, Local_R, Local_P] = DecomposeFromLocation(ViewportCenter3D);
    Local_R.Pitch =
        FMath::Clamp(Local_R.Pitch, -89.0, PitchClamp(Local_P.Length()));
    Offset = Local_P;
    Offset.Normalize();
    Offset *= -ZoomAmount * (Local_P.Length() / 20.0) *
              (ZoomInertia / MaxZoomInertia);
    Local_P += Offset;
    SetActorTransformDecomposeComponents(
        FDecomposeComponents{Esu_M, Local_R, Local_P});
    PanActorAndSnap(ZoomAnchor, ZoomMousePosition);
  } else {
    const TObjectPtr<APlayerController> PlayerController =
        Cast<APlayerController>(Controller);
    auto [Esu_M, Local_R, Local_P] = DecomposeFromLocation(ZoomAnchor);
    FVector Start;
    PlayerController->DeprojectMousePositionToWorld(Start, Offset);
    Offset *=
        ZoomAmount * (Local_P.Length() / 20.0) * (ZoomInertia / MaxZoomInertia);

    if (const FVector Target = GetActorLocation() + Offset;
        !IntersectionTest(Target, 200.0)) {
      SetActorLocation(Target);
      PanActorAndSnap(ZoomAnchor, ZoomMousePosition);
    };
  }
  DecreaseInertia(ZoomInertia, MaxZoomInertia);
}

void AGlobePawn::DecreaseInertia(double& Inertia, const double MaxInertia) {
  Inertia--;
  Inertia = FMath::Clamp(Inertia, 0.0, MaxInertia);
}

double AGlobePawn::PitchClamp(const float Radius) const {
  if (Radius < MaxLocalRotateHeight) {
    return 89.0;
  }
  if (Radius < ClampStart) {
    return 0.0;
  }
  if (Radius < ClampStop) {
    const double Alpha = (Radius - ClampStart) / (ClampStop - ClampStart);
    return FMath::Lerp(0.0, -89.0, Alpha);
  }
  return -89.0;
}

float AGlobePawn::GetCenterRadius(FVector& Center) {
  FVector2D ViewportCenter2D;
  GEngine->GameViewport->GetViewportSize(ViewportCenter2D);
  ViewportCenter2D /= 2.0;
  Center = PickEllipsoidOrLineTraceWorld(ViewportCenter2D);
  if (Center == FVector::Zero())
    return 0.0;
  return FVector::Distance(GetActorLocation(), Center);
}

FVector AGlobePawn::LineTraceWorld(const FVector2D& ViewportPosition) const {
  const TObjectPtr<APlayerController> PlayerController =
      Cast<APlayerController>(Controller);
  if (!PlayerController)
    return FVector::Zero();
  const TSoftObjectPtr<ACesiumGeoreference> Georeference =
      GlobeAnchor->ResolveGeoreference();
  FVector Start, Direction;
  if (!PlayerController->DeprojectScreenPositionToWorld(
          ViewportPosition.X,
          ViewportPosition.Y,
          Start,
          Direction))
    return FVector::Zero();
  const FVector ECEFCenter =
      Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(
          FVector::Zero());
  float Distance = FVector::Distance(Start, ECEFCenter);
  const FVector End = Start + Distance * Direction;
  FCollisionQueryParams Params;
  Params.AddIgnoredActor(this);
  Params.AddIgnoredActor(GlobeTransformer);
  if (FHitResult Hit; GetWorld()->LineTraceSingleByChannel(
          Hit,
          Start,
          End,
          ECC_GameTraceChannel1,
          Params)) {
    return Hit.ImpactPoint;
  }
  return FVector::Zero();
}

FVector
AGlobePawn::PickEllipsoidOrLineTraceWorld(const FVector2D& ViewportPosition) {
  return GetPawnGeoHeight() > MaxLineTraceHeight
             ? PickEllipsoidUnreal(ViewportPosition)
             : LineTraceWorld(ViewportPosition);
}

bool AGlobePawn::IntersectionTest(const FVector& End, const double Tolerance) {
  if (GetPawnGeoHeight() > MaxLineTraceHeight)
    return false;
  FVector Start = GetActorLocation();
  const float Distance = FVector::Distance(Start, End);
  FVector ActualEnd = End - Start;
  ActualEnd.Normalize();
  ActualEnd = ActualEnd * (Distance + Tolerance) + Start;
  FCollisionQueryParams Params;
  Params.AddIgnoredActor(this);
  FHitResult Hit;
  return GetWorld()->LineTraceSingleByChannel(
      Hit,
      Start,
      ActualEnd,
      ECC_GameTraceChannel1,
      Params);
}

FDecomposeComponents
AGlobePawn::DecomposeFromLocation(const FVector& Location) const {
  GlobeTransformer->SetLocationAndSnap(Location);
  const FMatrix Esu_M = GlobeTransformer->GetActorTransform().ToMatrixNoScale();
  const FMatrix Inv_Esu_M = Esu_M.Inverse();
  GlobeTransformer->SetActorRotation(GetActorRotation());
  const FMatrix Actor_Eye_M =
      GlobeTransformer->GetActorTransform().ToMatrixNoScale();
  const FVector Eye_Local_P =
      Actor_Eye_M.InverseTransformPosition(GetActorLocation());
  return FDecomposeComponents{
      Esu_M,
      (Actor_Eye_M * Inv_Esu_M).Rotator(),
      Eye_Local_P};
}

void AGlobePawn::SetActorTransformDecomposeComponents(
    const FDecomposeComponents& DecomposeComponents) {
  auto [Esu_M, Local_R, Local_P] = DecomposeComponents;
  const FMatrix Composite_M = FRotationMatrix{Local_R} * Esu_M;
  const FVector Actor_World_P = Composite_M.TransformPosition(Local_P);
  if (IntersectionTest(Actor_World_P, 200.0))
    return;
  SetActorLocation(Actor_World_P);
  SetActorRotation(Composite_M.Rotator());
}

void AGlobePawn::ResetInertia(const EGlobePawnInputType& Input) {
  switch (Input) {
  case Pan:
    RotateInertia = 0.0;
    SpinInertia = 0.0;
    PanInertia = 0.0;
    ZoomInertia = 0.0;
    break;
  case Rotate:
    PanInertia = 0.0;
    SpinInertia = 0.0;
    ZoomInertia = 0.0;
    break;
  case Zoom:
    RotateInertia = 0.0;
    PanInertia = 0.0;
    SpinInertia = 0.0;
    break;
  default:
    return;
  }
}

bool AGlobePawn::OtherPressing(const EGlobePawnInputType& Input) const {
  const bool PanPressing = bEnablePan && (bSpinPressed || bPanPressed);
  const bool RotatePressing = bEnableRotate && bRotatePressed;
  const bool ZoomTriggered = bEnableZoom && bZoomTriggered;
  switch (Input) {
  case Pan:
    return RotatePressing || ZoomTriggered;
  case Rotate:
    return PanPressing || ZoomTriggered;
  case Zoom:
    return PanPressing || RotatePressing;
  }
  return false;
}

// Override To Fix Camera Not Update
bool AGlobePawn::DeprojectScreenPositionToWorld(
    float ScreenX,
    float ScreenY,
    FVector& WorldLocation,
    FVector& WorldDirection) const {
  const TObjectPtr<APlayerController> Player =
      Cast<APlayerController>(Controller);

  ULocalPlayer* const LP = Player ? Player->GetLocalPlayer() : nullptr;
  if (LP && LP->ViewportClient) {
    // get the projection data
    FSceneViewProjectionData ProjectionData;
    if (LP->GetProjectionData(
            LP->ViewportClient->Viewport,
            /*out*/ ProjectionData)) {
      ProjectionData.ViewOrigin = GetActorLocation();
      ProjectionData.ViewRotationMatrix =
          FInverseRotationMatrix(GetActorRotation()) * FMatrix(
                                                           FPlane(0, 0, 1, 0),
                                                           FPlane(1, 0, 0, 0),
                                                           FPlane(0, 1, 0, 0),
                                                           FPlane(0, 0, 0, 1));
      FMatrix const InvViewProjMatrix =
          ProjectionData.ComputeViewProjectionMatrix().InverseFast();
      FSceneView::DeprojectScreenToWorld(
          FVector2D(ScreenX, ScreenY),
          ProjectionData.GetConstrainedViewRect(),
          InvViewProjMatrix,
          /*out*/ WorldLocation,
          /*out*/ WorldDirection);
      return true;
    }
  }

  // something went wrong, zero things and return false
  WorldLocation = FVector::ZeroVector;
  WorldDirection = FVector::ZeroVector;
  return false;
}

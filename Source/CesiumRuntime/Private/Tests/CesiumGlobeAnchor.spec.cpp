// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumTestHelpers.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesiumGlobeAnchorSpec,
    "Cesium.Unit.GlobeAnchor",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

TObjectPtr<AActor> pActor;
TObjectPtr<UCesiumGlobeAnchorComponent> pGlobeAnchor;
TObjectPtr<UCesiumEllipsoid> pEllipsoid;

END_DEFINE_SPEC(FCesiumGlobeAnchorSpec)

void FCesiumGlobeAnchorSpec::Define() {
  BeforeEach([this]() {
    UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();
    this->pActor = pWorld->SpawnActor<AActor>();
    this->pActor->AddComponentByClass(
        USceneComponent::StaticClass(),
        false,
        FTransform::Identity,
        false);
    this->pActor->SetActorRelativeTransform(FTransform());

    this->pEllipsoid = NewObject<UCesiumEllipsoid>();
    this->pEllipsoid->SetRadii(UCesiumWgs84Ellipsoid::GetRadii());

    ACesiumGeoreference* pGeoreference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(pActor);
    pGeoreference->SetOriginLongitudeLatitudeHeight(FVector(1.0, 2.0, 3.0));
    pGeoreference->SetEllipsoid(this->pEllipsoid);

    this->pGlobeAnchor =
        Cast<UCesiumGlobeAnchorComponent>(pActor->AddComponentByClass(
            UCesiumGlobeAnchorComponent::StaticClass(),
            false,
            FTransform::Identity,
            false));
  });

  It("immediately syncs globe position from transform when added", [this]() {
    TestEqual("Longitude", this->pGlobeAnchor->GetLongitude(), 1.0);
    TestEqual("Latitude", this->pGlobeAnchor->GetLatitude(), 2.0);
    TestEqual("Height", this->pGlobeAnchor->GetHeight(), 3.0);
  });

  It("maintains globe position when switching to a new georeference", [this]() {
    FTransform beforeTransform = this->pActor->GetActorTransform();
    FVector beforeLLH = this->pGlobeAnchor->GetLongitudeLatitudeHeight();

    UWorld* pWorld = this->pActor->GetWorld();
    ACesiumGeoreference* pNewGeoref = pWorld->SpawnActor<ACesiumGeoreference>();
    pNewGeoref->SetOriginLongitudeLatitudeHeight(FVector(10.0, 20.0, 30.0));
    this->pGlobeAnchor->SetGeoreference(pNewGeoref);

    TestEqual(
        "ResolvedGeoreference",
        this->pGlobeAnchor->GetResolvedGeoreference(),
        pNewGeoref);
    TestFalse(
        "Transforms are equal",
        this->pActor->GetActorTransform().Equals(beforeTransform));
    TestEqual(
        "Globe Position",
        this->pGlobeAnchor->GetLongitudeLatitudeHeight(),
        beforeLLH);
  });

  It("updates actor transform when globe anchor position is changed", [this]() {
    FTransform beforeTransform = this->pActor->GetActorTransform();
    this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(FVector(4.0, 5.0, 6.0));
    TestEqual(
        "LongitudeLatitudeHeight",
        this->pGlobeAnchor->GetLongitudeLatitudeHeight(),
        FVector(4.0, 5.0, 6.0));
    TestFalse(
        "Transforms are equal",
        this->pActor->GetActorTransform().Equals(beforeTransform));
  });

  It("updates globe anchor position when actor transform is changed", [this]() {
    FVector beforeLLH = this->pGlobeAnchor->GetLongitudeLatitudeHeight();
    this->pActor->SetActorLocation(FVector(1000.0, 2000.0, 3000.0));
    TestNotEqual(
        "globe position",
        this->pGlobeAnchor->GetLongitudeLatitudeHeight(),
        beforeLLH);
  });

  It("allows the actor transform to be set when not registered", [this]() {
    FVector beforeLLH = this->pGlobeAnchor->GetLongitudeLatitudeHeight();

    this->pGlobeAnchor->UnregisterComponent();
    this->pActor->SetActorLocation(FVector(1000.0, 2000.0, 3000.0));

    // Globe position doesn't initially update while unregistered
    TestEqual(
        "globe position",
        this->pGlobeAnchor->GetLongitudeLatitudeHeight(),
        beforeLLH);

    // After we re-register, actor transform should be maintained and globe
    // transform should be updated.
    this->pGlobeAnchor->RegisterComponent();
    TestEqual(
        "actor position",
        this->pActor->GetActorLocation(),
        FVector(1000.0, 2000.0, 3000.0));
    TestNotEqual(
        "globe position",
        this->pGlobeAnchor->GetLongitudeLatitudeHeight(),
        beforeLLH);
  });

  It("adjusts orientation for globe when actor position is set immediately after adding anchor",
     [this]() {
       FRotator beforeRotation = this->pActor->GetActorRotation();

       ACesiumGeoreference* pGeoreference =
           this->pGlobeAnchor->GetResolvedGeoreference();
       this->pActor->SetActorLocation(
           pGeoreference->TransformLongitudeLatitudeHeightPositionToUnreal(
               FVector(90.0, 2.0, 3.0)));
       TestNotEqual(
           "rotation",
           this->pActor->GetActorRotation(),
           beforeRotation);
     });

  It("adjusts orientation for globe when globe position is set immediately after adding anchor",
     [this]() {
       FRotator beforeRotation = this->pActor->GetActorRotation();

       this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(
           FVector(90.0, 2.0, 3.0));
       TestNotEqual(
           "rotation",
           this->pActor->GetActorRotation(),
           beforeRotation);
     });

  It("does not adjust orientation for globe when that feature is disabled",
     [this]() {
       this->pGlobeAnchor->SetAdjustOrientationForGlobeWhenMoving(false);
       FRotator beforeRotation = this->pActor->GetActorRotation();

       ACesiumGeoreference* pGeoreference =
           this->pGlobeAnchor->GetResolvedGeoreference();
       this->pActor->SetActorLocation(
           pGeoreference->TransformLongitudeLatitudeHeightPositionToUnreal(
               FVector(90.0, 2.0, 3.0)));
       TestEqual("rotation", this->pActor->GetActorRotation(), beforeRotation);

       this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(
           FVector(45.0, 25.0, 300.0));
       TestEqual("rotation", this->pActor->GetActorRotation(), beforeRotation);
     });

  It("gains correct orientation on call to SnapToEastSouthUp", [this]() {
    ACesiumGeoreference* pGeoreference =
        this->pGlobeAnchor->GetResolvedGeoreference();

    this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(
        FVector(-20.0, -10.0, 1000.0));
    this->pGlobeAnchor->SnapToEastSouthUp();

    const FTransform& transform = this->pActor->GetActorTransform();
    FVector actualEcefEast =
        pGeoreference
            ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                transform.TransformVector(FVector::XAxisVector))
            .GetSafeNormal();
    FVector actualEcefSouth =
        pGeoreference
            ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                transform.TransformVector(FVector::YAxisVector))
            .GetSafeNormal();
    FVector actualEcefUp =
        pGeoreference
            ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                transform.TransformVector(FVector::ZAxisVector))
            .GetSafeNormal();

    FMatrix enuToEcef =
        UCesiumWgs84Ellipsoid::EastNorthUpToEarthCenteredEarthFixed(
            this->pGlobeAnchor->GetEarthCenteredEarthFixedPosition());
    FVector expectedEcefEast =
        enuToEcef.TransformVector(FVector::XAxisVector).GetSafeNormal();
    FVector expectedEcefSouth =
        -enuToEcef.TransformVector(FVector::YAxisVector).GetSafeNormal();
    FVector expectedEcefUp =
        enuToEcef.TransformVector(FVector::ZAxisVector).GetSafeNormal();

    TestEqual("east", actualEcefEast, expectedEcefEast);
    TestEqual("south", actualEcefSouth, expectedEcefSouth);
    TestEqual("up", actualEcefUp, expectedEcefUp);
  });

  It("gains correct orientation on call to SnapLocalUpToEllipsoidNormal",
     [this]() {
       ACesiumGeoreference* pGeoreference =
           this->pGlobeAnchor->GetResolvedGeoreference();

       this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(
           FVector(-20.0, -10.0, 1000.0));
       this->pActor->SetActorRotation(FQuat::Identity);
       this->pGlobeAnchor->SnapLocalUpToEllipsoidNormal();

       const FTransform& transform = this->pActor->GetActorTransform();
       FVector actualEcefUp =
           pGeoreference
               ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                   transform.TransformVector(FVector::ZAxisVector))
               .GetSafeNormal();

       FVector surfaceNormal = UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
           this->pGlobeAnchor->GetEarthCenteredEarthFixedPosition());

       TestEqual("up", actualEcefUp, surfaceNormal);
     });

  It("gives correct results for different ellipsoids", [this]() {
    const FVector Position = FVector(-20.0, -10.0, 1000.0);

    // Check with WGS84 ellipsoid (the default)
    this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(Position);

    FVector wgs84EcefPos =
        UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
            Position);

    TestEqual(
        "ecef",
        this->pGlobeAnchor->GetEarthCenteredEarthFixedPosition(),
        wgs84EcefPos);

    // Check with unit ellipsoid
    TObjectPtr<UCesiumEllipsoid> pUnitEllipsoid = NewObject<UCesiumEllipsoid>();
    pUnitEllipsoid->SetRadii(FVector::One());

    ACesiumGeoreference* pGeoreference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(this->pActor);
    pGeoreference->SetEllipsoid(pUnitEllipsoid);

    this->pGlobeAnchor->MoveToLongitudeLatitudeHeight(Position);

    FVector unitEcefPos =
        pUnitEllipsoid
            ->LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
                Position);

    TestEqual(
        "ecef",
        this->pGlobeAnchor->GetEarthCenteredEarthFixedPosition(),
        unitEcefPos);
  });
}

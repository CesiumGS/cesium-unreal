#include "CesiumClampToGroundComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

UCesiumClampToGroundComponent::UCesiumClampToGroundComponent() {
  // Enable ticking for this component
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
  PrimaryComponentTick.TickGroup = TG_PostPhysics;

  // Initialize default values
  this->drawDebugTrace = true;
}

void UCesiumClampToGroundComponent::BeginPlay() {
  this->_remainingSamples = this->SampleInterval;
  Super::BeginPlay();

  UCesiumGlobeAnchorComponent* GlobeAnchor = this->GetGlobeAnchor();
  if (!IsValid(GlobeAnchor))
    return;

  ACesiumGeoreference* Georeference = GlobeAnchor->ResolveGeoreference();

  if (!IsValid(Georeference))
    return;

  // Get the actor's current world position
  this->InitialPosition = GetOwner()->GetActorLocation();
  // Perform initial height query
  this->InitialHeight = QueryTilesetHeight();
  this->HeightToMaintain = InitialPosition.Z - this->InitialHeight;
}

void UCesiumClampToGroundComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  static bool enabled = false;
  if (!enabled)
    return;

  if (--this->_remainingSamples > 0) {
    return;
  }

  this->_remainingSamples = this->SampleInterval;

  // Query the height above the 3D tileset at the actor's location
  float terrainHeight = QueryTilesetHeight();
  FVector position = GetOwner()->GetActorLocation();

  float newHeight = this->HeightToMaintain + terrainHeight;
  position.Z = newHeight;

  GetOwner()->SetActorLocation(position);
}

float UCesiumClampToGroundComponent::QueryTilesetHeight() {
  if (!GetOwner() || !GetWorld()) {
    return -1.0f;
  }

  // Get the actor's current world position
  FVector actorPosition = GetOwner()->GetActorLocation();

  const float traceDistance = 1000000.0f;
  // Set up the line trace start and end points
  FVector rayStart = actorPosition + FVector(0.0, 0.0, traceDistance);
  FVector rayEnd = actorPosition - FVector(0.0, 0.0, traceDistance);

  FCollisionQueryParams queryParams{};

  // Ignore the owner actor to avoid self collision
  queryParams.AddIgnoredActor(GetOwner());
  queryParams.bTraceComplex = true;
  queryParams.bReturnPhysicalMaterial = false;

  // Perform the line trace
  FHitResult rayIntersection;
  bool hit = GetWorld()->LineTraceSingleByChannel(
      rayIntersection,
      rayStart,
      rayEnd,
      ECC_WorldStatic,
      // Cesium 3D Tiles are typically static
      queryParams
      );

  if (!hit) {
    return -1.0f;
  }

  // Valid tileset surface detected
  float surfaceHeight = rayIntersection.Location.Z;
  float heightAboveTileset = actorPosition.Z - surfaceHeight;

  UCesiumGlobeAnchorComponent* pGlobeAnchor = this->GetGlobeAnchor();
  if (!IsValid(pGlobeAnchor))
    return -1.0f;

  ACesiumGeoreference* pGeoreference = pGlobeAnchor->ResolveGeoreference();

  if (!IsValid(pGeoreference))
    return -1.0f;

  // Log the measurement if georeference is available
  FVector geoPosition = pGeoreference->
      TransformUnrealPositionToLongitudeLatitudeHeight(actorPosition);

  // Draw debug visualization if enabled
  if (drawDebugTrace) {
    DrawDebugLine(
        GetWorld(),
        actorPosition,
        rayIntersection.Location,
        FColor::Green,
        false,
        0.0f,
        0,
        2.0f);
    DrawDebugPoint(
        GetWorld(),
        rayIntersection.Location,
        10.0f,
        FColor::Red,
        false,
        0.0f);
    DrawDebugString(
        GetWorld(),
        rayIntersection.Location,
        FString::Printf(TEXT("Height: %.2f"), heightAboveTileset),
        nullptr,
        FColor::White,
        0.0f);
  }

  return heightAboveTileset;
}

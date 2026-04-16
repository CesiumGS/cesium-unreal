#include "CgalGltfModifierActor.h"
#include "CgalGltfModifier.h"
#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "VecMath.h"
#include "Engine/StaticMeshActor.h"

ACgalGltfModifierActor::ACgalGltfModifierActor()
    : Tileset(nullptr), StaticMeshActor(nullptr) {
  PrimaryActorTick.bCanEverTick = false;
}

void ACgalGltfModifierActor::BeginPlay() {
  Super::BeginPlay();
  AttachModifierToTileset();
}

void ACgalGltfModifierActor::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  if (Tileset) {
    Tileset->SetGltfModifier(nullptr);
  }
  Modifier.reset();
  Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void ACgalGltfModifierActor::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  FName PropertyName = PropertyChangedEvent.GetPropertyName();
  if (PropertyName == GET_MEMBER_NAME_CHECKED(ACgalGltfModifierActor, Tileset)) {
    AttachModifierToTileset();
  }
  if (PropertyName == GET_MEMBER_NAME_CHECKED(ACgalGltfModifierActor, StaticMeshActor)) {
    if (Modifier) {
      Modifier->SetStaticMeshActor(StaticMeshActor);
      if (Tileset) {
        ACesiumGeoreference* Georeference = Tileset->ResolveGeoreference();
        if (Georeference) {
          glm::dmat4 unrealToEcef = VecMath::createMatrix4D(
              Georeference
                  ->ComputeUnrealToEarthCenteredEarthFixedTransformation());
          Modifier->CacheClipMeshData(unrealToEcef);
        }
      }
      Modifier->trigger();
    }
  }
}
#endif

void ACgalGltfModifierActor::ActivateModifier() {
  if (!Modifier) {
    AttachModifierToTileset();
  }
  if (Modifier) {
    Modifier->trigger();
  }
}

void ACgalGltfModifierActor::AttachModifierToTileset() {
  if (!Tileset) {
    return;
  }

  if (!Modifier) {
    Modifier = std::make_shared<CgalGltfModifier>();
  }

  Modifier->SetStaticMeshActor(StaticMeshActor);

  // Cache the clip mesh geometry transformed to ECEF.
  ACesiumGeoreference* Georeference = Tileset->ResolveGeoreference();
  if (Georeference) {
    glm::dmat4 unrealToEcef = VecMath::createMatrix4D(
        Georeference->ComputeUnrealToEarthCenteredEarthFixedTransformation());
    Modifier->CacheClipMeshData(unrealToEcef);
  }

  Tileset->SetGltfModifier(Modifier);
  Modifier->trigger();
}

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CgalGltfModifierActor.generated.h"

class ACesium3DTileset;
class AStaticMeshActor;
class CgalGltfModifier;

/**
 * An actor that owns a CgalGltfModifier and connects it to a
 * Cesium 3D Tileset. Also holds a reference to a StaticMeshActor
 * for use as auxiliary geometry (e.g. a clipping volume or
 * boolean operand).
 */
UCLASS(BlueprintType, Blueprintable, Placeable, meta = (DisplayName = "CGAL Gltf Modifier"))
class CESIUMRUNTIME_API ACgalGltfModifierActor : public AActor {
  GENERATED_BODY()

public:
  ACgalGltfModifierActor();

  /** The Cesium 3D Tileset whose tiles this modifier will process. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|CGAL")
  ACesium3DTileset* Tileset;

  /** A static mesh actor used as auxiliary geometry for the modifier. */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|CGAL")
  AStaticMeshActor* StaticMeshActor;

  /** Activate the modifier (or re-trigger it to reprocess all tiles). */
  UFUNCTION(BlueprintCallable, Category = "Cesium|CGAL")
  void ActivateModifier();

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
  virtual void PostEditChangeProperty(
      FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  void AttachModifierToTileset();

  std::shared_ptr<CgalGltfModifier> Modifier;
};

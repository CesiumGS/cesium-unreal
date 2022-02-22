#pragma once

#include "CesiumGltf/ExtensionModelMaxarMeshVariants.h"
#include "CesiumGltf/ExtensionNodeMaxarMeshVariants.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Components/SceneComponent.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <vector>
#include "CesiumGltfMeshVariantsComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct Node;
} // namespace CesiumGltf

namespace {
} // namespace

UCLASS(BlueprintType)
class CESIUMRUNTIME_API UCesiumGltfMeshVariantsComponent : public USceneComponent {
  GENERATED_BODY()

public:
  UCesiumGltfMeshVariantsComponent() = default;
  virtual ~UCesiumGltfMeshVariantsComponent() = default;

  void InitializeVariants(
      const CesiumGltf::ExtensionModelMaxarMeshVariants& modelExtension,
      const CesiumGltf::ExtensionNodeMaxarMeshVariants& nodeExtension);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|MeshVariants")
  int32 GetCurrentVariantIndex() const;

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|MeshVariants")
  const FString& GetCurrentVariantName() const;                                                     
  
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|MeshVariants")
  bool SetVariantByIndex(int32 VariantIndex);
  
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|MeshVariants")
  bool SetVariantByName(const FString& Name);

  virtual void BeginDestroy() override;

private:
  struct MeshVariant {
    FString name;
    int32 index;
    std::vector<UCesiumGltfPrimitiveComponent*> meshPrimitives;
  };

  TArray<MeshVariant> _variants;
  MeshVariant* _pCurrentVariant;

  friend UCesiumGltfMeshVariantsBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumGltfMeshVariantsBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|MeshVariants")
  static UCesiumGltfMeshVariantsComponent*
  GetMeshVariantsComponent(UPARAM(ref) UPrimitiveComponent* Primitive);
};

#pragma once

#include "GenericPlatform/GenericPlatform.h"
#include "Components/SceneComponent.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "CesiumGltfMeshVariantsComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct Node;
struct ExtensionModelMaxarMeshVariants;
struct ExtensionNodeMaxarMeshVariants;
struct ExtensionModelMaxarMeshVariantsValue;
} // namespace CesiumGltf

class UCesiumGltfPrimitiveComponent;

UCLASS(BlueprintType)
class CESIUMRUNTIME_API UCesiumGltfMeshVariantsComponent : public USceneComponent {
  GENERATED_BODY()

public:
  UCesiumGltfMeshVariantsComponent() = default;
  virtual ~UCesiumGltfMeshVariantsComponent() = default;

  bool InitializeVariants(
      const CesiumGltf::ExtensionModelMaxarMeshVariants* pModelExtension,
      const CesiumGltf::ExtensionNodeMaxarMeshVariants* pNodeExtension);

  void AddMesh(uint32_t meshIndex, std::vector<UCesiumGltfPrimitiveComponent*>&& mesh);

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

  void ShowCurrentVariant();

  virtual void BeginDestroy() override;

private:

  const CesiumGltf::ExtensionModelMaxarMeshVariants* _pModelMeshVariants;
  const CesiumGltf::ExtensionNodeMaxarMeshVariants* _pNodeMeshVariants;

  int32_t _currentVariantIndex  = -1;

  std::unordered_map<uint32_t, std::vector<UCesiumGltfPrimitiveComponent*>> _meshes;

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

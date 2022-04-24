#pragma once

#include "Components/SceneComponent.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "GenericPlatform/GenericPlatform.h"
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
class CESIUMRUNTIME_API UCesiumGltfMeshVariantsComponent
    : public USceneComponent {
  GENERATED_BODY()

public:
  UCesiumGltfMeshVariantsComponent() = default;
  virtual ~UCesiumGltfMeshVariantsComponent() = default;

  void AddMesh(
      uint32_t meshIndex,
      std::vector<UCesiumGltfPrimitiveComponent*>&& mesh);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|MeshVariants")
  int32 GetCurrentVariantIndex() const { return this->_currentVariantIndex; }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|MeshVariants")
  FString GetCurrentVariantName() const;

  UFUNCTION(BlueprintCallable, Category = "Cesium|MeshVariants")
  bool SetVariantByIndex(int32 VariantIndex);

  UFUNCTION(BlueprintCallable, Category = "Cesium|MeshVariants")
  bool SetVariantByName(const FString& Name);

  void ShowCurrentVariant();

  static UCesiumGltfMeshVariantsComponent* CreateMeshVariantsComponent(
      USceneComponent* pOutter,
      const FName& name,
      const CesiumGltf::ExtensionModelMaxarMeshVariants* pModelExtension,
      const CesiumGltf::ExtensionNodeMaxarMeshVariants* pNodeExtension);

private:
  const CesiumGltf::ExtensionModelMaxarMeshVariants* _pModelMeshVariants;
  const CesiumGltf::ExtensionNodeMaxarMeshVariants* _pNodeMeshVariants;

  int32_t _currentVariantIndex = -1;

  std::unordered_map<uint32_t, std::vector<UCesiumGltfPrimitiveComponent*>>
      _meshes;

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

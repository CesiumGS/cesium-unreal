
#include "CesiumGltfMeshVariantsComponent.h"

using namespace CesiumGltf;

void UCesiumGltfMeshVariantsComponent::InitializeVariants(
    const ExtensionModelMaxarMeshVariants& modelExtension,
    const ExtensionNodeMaxarMeshVariants& nodeExtension) {
  
}

int32 UCesiumGltfMeshVariantsComponent::GetCurrentVariantIndex() const {
  return this->_pCurrentVariant ? this->_pCurrentVariant->index : -1;
}

const FString& UCesiumGltfMeshVariantsComponent::GetCurrentVariantName() const {
  const static FString EMPTY_STRING = "";
  return this->_pCurrentVariant ? this->_pCurrentVariant->name : EMPTY_STRING;
}                                                     

bool UCesiumGltfMeshVariantsComponent::SetVariantByIndex(int32 VariantIndex) {
  if (VariantIndex < 0 || VariantIndex >= this->_variants.Num()) {
    return false;
  }

  this->_pCurrentVariant = &this->_variants[VariantIndex];
  return true;
}

bool UCesiumGltfMeshVariantsComponent::SetVariantByName(const FString& Name) {
  for (MeshVariant& variant : this->_variants) {
    if (variant.name == Name) {
      this->_pCurrentVariant = &variant;
      return true;
    }
  }
  
  return false;
}

void UCesiumGltfMeshVariantsComponent::BeginDestroy() {
  this->_variants.Empty();
  super::BeginDestroy();
}

/*static*/ 
UCesiumGltfMeshVariantsComponent*
UCesiumGltfMeshVariantsBlueprintLibrary::GetMeshVariantsComponent(
    UPARAM(ref) UPrimitiveComponent* Primitive) {
  UCesiumGltfPrimitiveComponent* pGltfPrimitive = 
      Cast<UCesiumGltfPrimitiveComponent>(Primitive);
  if (!IsValid(pGltfPrimitive)) {
    return nullptr;
  }
  
  return pGltfPrimitive->pMeshVariants;   
}

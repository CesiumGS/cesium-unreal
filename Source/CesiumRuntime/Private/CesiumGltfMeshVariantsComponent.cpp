
#include "CesiumGltfMeshVariantsComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltf/ExtensionModelMaxarMeshVariants.h"
#include "CesiumGltf/ExtensionNodeMaxarMeshVariants.h"
#include <cassert>

using namespace CesiumGltf;

bool UCesiumGltfMeshVariantsComponent::InitializeVariants(
    const ExtensionModelMaxarMeshVariants* pModelExtension,
    const ExtensionNodeMaxarMeshVariants* pNodeExtension) {
  if (pModelExtension && pNodeExtension) {
    return false;
  }

  this->_pModelMeshVariants = pModelExtension;
  this->_pNodeMeshVariants = pNodeExtension;

  if (this->_pModelMeshVariants->defaultProperty < 0 || 
      this->_pModelMeshVariants->defaultProperty >= 
        this->_pModelMeshVariants->variants.size()) {
    return false;
  }

  this->_currentVariantIndex = 
      static_cast<uint32_t>(this->_pModelMeshVariants->defaultProperty);

  return true;
}

void AddMesh(uint32_t meshIndex, std::vector<UCesiumGltfPrimitiveComponent*>&& mesh) {
  assert(this->_pModelMeshVariants != nullptr);
  assert(this->_pNodeMeshVariants != nullptr);
  assert(this->_currentVariantIndex != -1);
  assert(this->_meshes.find(meshIndex) == this->_meshes.end());

  auto meshIt = this->_meshes.emplace(meshIndex, std::move(mesh)).first;
  
  // Find the mapping for this mesh.
  for (const ExtensionNodeMaxarMeshVariantsMappingsValue& mapping : 
       this->_pNodeMeshVariants->mappings) {
    if (mapping.mesh == meshIndex) {
      // Check if this mapping contains the current variant.
      for (int32_t variantIndex : mapping.variants) {
        if (variantIndex == this->_currentVariantIndex) {
          // This should be the only visible mesh for now.
          // TODO: SetVisible
          for (UCesiumGltfPrimitiveComponent* pPrimitive : meshIt->second) {
            
          }
        }
      }
      break;
    }
  }
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

static void showMesh(const std::vector<UCesiumGltfPrimitiveComponent*>& mesh) {
  for (UCesiumGltfPrimitiveComponent* pPrimitive : mesh) {
    if (pPrimitive && !pPrimitive->IsVisible()) {
      pPrimitive->SetVisibility(true, true);
      pPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
  }
}

static void hideMesh(const std::vector<UCesiumGltfPrimitiveComponent*>& mesh) {
  for (UCesiumGltfPrimitiveComponent* pPrimitive : mesh) {
    if (pPrimitive && pPrimitive->IsVisible()) {
      pPrimitive->SetVisibility(false, true);
      pPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
  }
}

void ShowCurrentVariant() {
  assert(this->_pModelMeshVariants != nullptr);
  assert(this->_pNodeMeshVariants != nullptr);
  assert(this->_currentVariantIndex != -1);
  
  if (!this->GetVisibleFlag()) {
    this->SetVisibleFlag(bNewVisibility);
    this->OnVisibilityChanged();
  }

  // Only the currently selected mesh variant should be set visible.
  bool visibleMeshFound = false;
  for (auto& meshIt : this->_meshes) {
    if (visibleMeshFound) {
      // We already found the visible mesh, so hide this mesh.
      hideMesh(meshIt.second);
    } else {
      // Need to check if this mesh contains the current variant.
      // Find the mapping for this mesh.
      for (const ExtensionNodeMaxarMeshVariantsMappingsValue& mapping :
          this->_pNodeMeshVariants->mappings) {
        if (mapping.mesh == meshIt.first) {
          // We found the mesh's mapping.
          // Check if this mesh's mapping contains the current variant.
          for (int32_t variantIndex : mapping.variants) {
            if (variantIndex == this->_currentVariantIndex) {
              visibleMeshFound = true;
              break;
            }
          }
          // Already found this mesh's mapping.
          break;
        }
      }

      if (visibleMeshFound) {
        showMesh(meshit.second);
      } else {
        hideMesh(meshIt.second);
      }
    }
  }
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

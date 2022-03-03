
#include "CesiumGltfMeshVariantsComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltf/ExtensionModelMaxarMeshVariants.h"
#include "CesiumGltf/ExtensionNodeMaxarMeshVariants.h"
#include <cassert>

using namespace CesiumGltf;

/*static*/ UCesiumGltfMeshVariantsComponent* 
UCesiumGltfMeshVariantsComponent::CreateMeshVariantsComponent(
    USceneComponent* pOutter,
    const FString& name,
    const ExtensionModelMaxarMeshVariants* pModelExtension,
    const ExtensionNodeMaxarMeshVariants* pNodeExtension) {

  if (!pModelExtension || !pNodeExtension ||
      pModelExtension->defaultProperty < 0 || 
      pModelExtension->defaultProperty >= pModelExtension->variants.size()) {
    return nullptr;
  }

  UCesiumMeshVariantsComponent* pVariantsComponent =
      NewObject<UCesiumMeshVariantsComponent>(pOutter, name);

  pVariantsComponent->_pModelMeshVariants = pModelExtension;
  pVariantsComponent->_pNodeMeshVariants = pNodeExtension;
  pVariantsComponent->_currentVariantIndex = 
      static_cast<uint32_t>(this->_pModelMeshVariants->defaultProperty);

  pVariantsComponent->SetMobility(EComponentMobility::Movable);
  pVariantsComponent->SetupAttachment(pOutter);
  pVariantsComponent->RegisterComponent();

  return pVariantsComponent;
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

FString UCesiumGltfMeshVariantsComponent::GetCurrentVariantName() const {
  if (this->_pModelMeshVariants && this->_currentVariantIndex != -1) {
    return 
        FString(UTF8_TO_TCHAR(
          this->_pModelMeshVariants->variants[this->_currentVariantIndex].name.c_str()));
  }

  return "";
}                                                     

bool UCesiumGltfMeshVariantsComponent::SetVariantByIndex(int32 VariantIndex) {
  if (VariantIndex < 0 || VariantIndex >= this->_variants.Num()) {
    return false;
  }

  this->_currentVariantIndex = VariantIndex;
  ShowCurrentVariant();
  return true;
}

bool UCesiumGltfMeshVariantsComponent::SetVariantByName(const FString& Name) {
  if (this->_pModelMeshVariants) {
    for (size_t i = 0; i < this->_pModelMeshVariants->variants.size(); ++i) {
      const std::string& variantName = 
          this->_pModelMeshVariants->variants[i].name;
      if (Name == FString(UTF8_TO_TCHAR(variantName.c_str()))) {
        this->_currentVariantIndex = static_cast<int32_t>(i);
        ShowCurrentVariant();
        return true;
      }
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

void UCesiumGltfMeshVariantsComponent::ShowCurrentVariant() {
  assert(this->_pModelMeshVariants != nullptr);
  assert(this->_pNodeMeshVariants != nullptr);
  assert(this->_currentVariantIndex != -1);
  
  if (!this->GetVisibleFlag()) {
    this->SetVisibleFlag(true);
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

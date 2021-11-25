#include "CesiumSelectionStats.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "Engine/Selection.h"

CesiumSelectionStats::CesiumSelectionStats() noexcept {
  this->_selectionChangedHandle = USelection::SelectionChangedEvent.AddLambda(
      [this](UObject* pSelectionObj) {
        USelection* pSelection = Cast<USelection>(pSelectionObj);
        if (!pSelection) {
          return;
        }

        TArray<UCesiumGltfPrimitiveComponent*> primitives;
        pSelection->GetSelectedObjects<UCesiumGltfPrimitiveComponent>(
            primitives);

        for (UCesiumGltfPrimitiveComponent* pPrimitive : primitives) {
          if (!pPrimitive->pModel) {
            continue;
          }

          const CesiumGltf::Model& model = *pPrimitive->pModel;

        }
      });
}

CesiumSelectionStats::~CesiumSelectionStats() noexcept {
  USelection::SelectionChangedEvent.Remove(this->_selectionChangedHandle);
}

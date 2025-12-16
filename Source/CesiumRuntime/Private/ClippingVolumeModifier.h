#include "Cesium3DTileset.h"
#include "CesiumRuntime.h"

#include <Cesium3DTilesSelection/GltfModifier.h>

class ACesium3DTileset;

class ClippingVolumeModifier : public Cesium3DTilesSelection::GltfModifier {
public:
  ClippingVolumeModifier(ACesium3DTileset* pTileset) : _pTileset(pTileset) {}

  virtual CesiumAsync::Future<
      std::optional<Cesium3DTilesSelection::GltfModifierOutput>>
  apply(Cesium3DTilesSelection::GltfModifierInput&& input) override;

private:
  TWeakObjectPtr<ACesium3DTileset> _pTileset;
};

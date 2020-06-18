#include "Cesium3DTileContent.h"

Cesium3DTileContent::Cesium3DTileContent(const Cesium3DTile& tile) :
    rendererResource(nullptr),
    _pTile(&tile)
{
}

Cesium3DTileContent::~Cesium3DTileContent() {

}
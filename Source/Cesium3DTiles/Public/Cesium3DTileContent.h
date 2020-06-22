#pragma once

class Cesium3DTile;

class Cesium3DTileContent {
public:
    Cesium3DTileContent(const Cesium3DTile& tile);
    virtual ~Cesium3DTileContent();

private:
    // TODO: use VectorReference instead of a raw pointer
    const Cesium3DTile* _pTile;
};

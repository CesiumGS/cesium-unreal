#include "Cesium3DTile.h"
#include "Cesium3DTileset.h"
#include "IAssetAccessor.h"
#include "IAssetResponse.h"
#include "Cesium3DTileContentFactory.h"

Cesium3DTile::Cesium3DTile(const Cesium3DTileset& tileset, VectorReference<Cesium3DTile> pParent) :
    _pTileset(&tileset),
    _pParent(pParent),
    _children(),
    _contentUri(),
    _pContentRequest(nullptr)
{
}

Cesium3DTile::~Cesium3DTile() {
}

Cesium3DTile::Cesium3DTile(Cesium3DTile&& rhs) noexcept :
    _pTileset(rhs._pTileset),
    _pParent(rhs._pParent),
    _children(rhs._children),
    _pContent(std::move(rhs._pContent)),
    _contentUri(rhs._contentUri),
    _pContentRequest(std::move(rhs._pContentRequest))
{
}

Cesium3DTile& Cesium3DTile::operator=(Cesium3DTile&& rhs) noexcept {
    if (this != &rhs) {
        this->_pTileset = rhs._pTileset;
        this->_pParent = rhs._pParent;
        this->_children = rhs._children;
        this->_pContent = std::move(rhs._pContent);
        this->_contentUri = rhs._contentUri;
        this->_pContentRequest = std::move(rhs._pContentRequest);
    }

    return *this;
}

void Cesium3DTile::children(const VectorRange<Cesium3DTile>& children) {
    this->_children = children;
}

void Cesium3DTile::contentUri(const std::optional<std::string>& value)
{
    this->_contentUri = value;
}

void Cesium3DTile::loadContent()
{
    if (this->isContentLoaded() || this->isContentLoading()) {
        return;
    }

    if (!this->contentUri().has_value()) {
        return;
    }

    IAssetAccessor* pAssetAccessor = this->_pTileset->externals().pAssetAccessor;
    this->_pContentRequest = pAssetAccessor->requestAsset(*this->_contentUri);
    this->_pContentRequest->bind(std::bind(&Cesium3DTile::contentResponseReceived, this, std::placeholders::_1));
}

void Cesium3DTile::contentResponseReceived(IAssetRequest* pRequest) {
    IAssetResponse* pResponse = pRequest->response();
    if (!pResponse) {
        // TODO: report the lack of response. Network error? Can this even happen?
        return;
    }

    if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
        // TODO: report error response.
        return;
    }

    gsl::span<const uint8_t> data = pResponse->data();

    this->_pContent = Cesium3DTileContentFactory::createContent(*this, data);

    if (!this->_pContent) {
        // Try loading this content as an external tileset (JSON).
    }
}

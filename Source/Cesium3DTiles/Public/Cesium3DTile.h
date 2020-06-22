#pragma once

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <gsl/span>
#include "IAssetRequest.h"
#include "Cesium3DTileContent.h"

class Cesium3DTileContent;

namespace Cesium3DTiles {
    class Tileset;
}

template <class T>
class VectorReference {
public:
    VectorReference() :
        _pVector(nullptr),
        _index(0)
    {
    }

    VectorReference(std::vector<T>& vector, size_t index) :
        _pVector(&vector),
        _index(index)
    {
    }

    operator bool() const { return this_ > pVector; }

    T& operator*() { return (*this->_pVector)[this->_index]; }
    const T& operator*() const { return (*this->_pVector)[this->_index]; }

    T* operator->() { return &(*this->_pVector)[this->_index]; }
    const T* operator->() const { return &(*this->_pVector)[this->_index]; }

    T* data() { return this->_pVector ? &(*this->_pVector)[this->_index] : nullptr; }
    const T* data() const { return this->_pVector ? &(*this->_pVector)[this->_index] : nullptr; }

private:
    std::vector<T>* _pVector;
    size_t _index;
};

template <class T>
class VectorRange {
public:
    VectorRange() :
        _pVector(nullptr),
        _begin(0),
        _end(0)
    {
    }

    VectorRange(std::vector<T>& vector, size_t begin, size_t end) :
        _pVector(&vector),
        _begin(begin),
        _end(end)
    {
    }

    typename std::vector<T>::iterator begin() { return this->_pVector ? this->_pVector->begin() + this->_begin : std::vector<T>::iterator(nullptr, nullptr); }
    typename std::vector<T>::const_iterator begin() const { return this->_pVector ? this->_pVector->begin() + this->_begin : std::vector<T>::const_iterator(nullptr, nullptr); }

    typename std::vector<T>::iterator end() { return this->_pVector ? this->_pVector->begin() + this->_end : std::vector<T>::iterator(nullptr, nullptr); }
    typename std::vector<T>::const_iterator end() const { return this->_pVector ? this->_pVector->begin() + this->_end : std::vector<T>::const_iterator(nullptr, nullptr); }

    size_t size() const { return this->_end - this->_begin; }

    T& operator[](size_t i) {
        return (*this->_pVector)[this->_begin + i];
    }

    const T& operator[](size_t i) const {
        return (*this->_pVector)[this->_begin + i];
    }

private:
    std::vector<T>* _pVector;
    size_t _begin;
    size_t _end;
};

class CESIUM3DTILES_API Cesium3DTile {
public:
    enum LoadState {
        /// <summary>
        /// Something went wrong while loading this tile.
        /// </summary>
        Failed = -1,

        /// <summary>
        /// The tile is not yet loaded at all, beyond the metadata in tileset.json.
        /// </summary>
        Unloaded = 0,

        /// <summary>
        /// The tile content is currently being loaded.
        /// </summary>
        ContentLoading = 1,

        /// <summary>
        /// The tile content has finished loading.
        /// </summary>
        ContentLoaded = 2,

        /// <summary>
        /// The tile's renderer resources are currently being prepared.
        /// </summary>
        RendererResourcesPreparing = 3,

        /// <summary>
        /// The tile's renderer resources are done being prepared and this
        /// tile is ready to render.
        /// </summary>
        RendererResourcesPrepared = 4
    };

    Cesium3DTile(const Cesium3DTiles::Tileset& tileset, VectorReference<Cesium3DTile> pParent = VectorReference<Cesium3DTile>());
    ~Cesium3DTile();
    Cesium3DTile(Cesium3DTile& rhs) noexcept = delete;
    Cesium3DTile(Cesium3DTile&& rhs) noexcept;
    Cesium3DTile& operator=(Cesium3DTile&& rhs) noexcept;

    Cesium3DTile* getParent() { return &*this->_pParent.data(); }
    const Cesium3DTile* getParent() const { return this->_pParent.data(); }

    VectorRange<Cesium3DTile>& getChildren() { return this->_children; }
    const VectorRange<Cesium3DTile>& getChildren() const { return this->_children; }
    void setChildren(const VectorRange<Cesium3DTile>& children);

    const std::optional<std::string>& getContentUri() const { return this->_contentUri; }
    void setContentUri(const std::optional<std::string>& value);

    Cesium3DTileContent* getContent() { return this->_pContent.get(); }
    const Cesium3DTileContent* getContent() const { return this->_pContent.get(); }

    void* getRendererResources() { return this->_pRendererResources; }

    LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }

    void loadContent();

    /// <summary>
    /// Notifies the tile that its renderer resources have been prepared and optionally stores
    /// a pointer to those resources. This method is safe to call from any thread.
    /// </summary>
    /// <param name="pResource">The renderer resources as an opaque pointer.</param>
    void finishPrepareRendererResources(void* pResource = nullptr);

protected:
    void setState(LoadState value);
    void contentResponseReceived(IAssetRequest* pRequest);

private:
    // Position in bounding-volume hierarchy.
    const Cesium3DTiles::Tileset* _pTileset;
    VectorReference<Cesium3DTile> _pParent;
    VectorRange<Cesium3DTile> _children;

    // Properties from tileset.json.
    // These are immutable after the tile leaves TileState::Unloaded.
    std::optional<std::string> _contentUri;

    // Load state and data.
    std::atomic<LoadState> _state;
    std::unique_ptr<IAssetRequest> _pContentRequest;
    std::unique_ptr<Cesium3DTileContent> _pContent;
    void* _pRendererResources;
};

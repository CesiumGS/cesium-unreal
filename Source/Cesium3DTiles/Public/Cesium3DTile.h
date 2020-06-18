#pragma once

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <gsl/span>
#include "IAssetRequest.h"
#include "Cesium3DTileContent.h"

class Cesium3DTileContent;
class Cesium3DTileset;

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
    Cesium3DTile(const Cesium3DTileset& tileset, VectorReference<Cesium3DTile> pParent = VectorReference<Cesium3DTile>());
    ~Cesium3DTile();
    Cesium3DTile(Cesium3DTile& rhs) noexcept = delete;
    Cesium3DTile(Cesium3DTile&& rhs) noexcept;
    Cesium3DTile& operator=(Cesium3DTile&& rhs) noexcept;

    Cesium3DTile* parent() { return &*this->_pParent.data(); }
    const Cesium3DTile* parent() const { return this->_pParent.data(); }

    VectorRange<Cesium3DTile>& children() { return this->_children; }
    const VectorRange<Cesium3DTile>& children() const { return this->_children; }
    void children(const VectorRange<Cesium3DTile>& children);

    const std::optional<std::string>& contentUri() const { return this->_contentUri; }
    void contentUri(const std::optional<std::string>& value);

    Cesium3DTileContent* content() { return this->_pContent.get(); }
    const Cesium3DTileContent* content() const { return this->_pContent.get(); }

    bool isContentLoading() const { return this->_pContentRequest ? true : false; }
    bool isContentLoaded() const { return this->_pContent ? true : false; }

    void loadContent();

protected:
    void contentResponseReceived(IAssetRequest* pRequest);

private:
    const Cesium3DTileset* _pTileset;
    VectorReference<Cesium3DTile> _pParent;
    VectorRange<Cesium3DTile> _children;
    std::unique_ptr<Cesium3DTileContent> _pContent;
    std::optional<std::string> _contentUri;
    std::unique_ptr<IAssetRequest> _pContentRequest;
};

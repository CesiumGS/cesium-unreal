#pragma once

#include <vector>

namespace Cesium3DTiles {

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

}
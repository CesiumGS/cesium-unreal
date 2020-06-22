#include <vector>

namespace Cesium3DTiles {

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

}

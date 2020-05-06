#pragma once

#include "tiny_gltf.h"

template <class T>
class GltfAccessor {
private:
	const unsigned char* _pBufferViewData;
	size_t _stride;
	size_t _offset;
	size_t _size;

public:
	GltfAccessor(tinygltf::Model& model, size_t accessorID)
	{
		tinygltf::Accessor& accessor = model.accessors[accessorID];
		tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
		tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

		const std::vector<unsigned char>& data = buffer.data;
		size_t bufferBytes = data.size();
		if (bufferView.byteOffset + bufferView.byteLength > bufferBytes)
		{
			throw std::runtime_error("bufferView does not fit in buffer.");
		}

		int accessorByteStride = accessor.ByteStride(bufferView);
		if (accessorByteStride == -1)
		{
			throw std::runtime_error("cannot compute accessor byteStride.");
		}

		int32_t accessorComponentElements = tinygltf::GetNumComponentsInType(accessor.type);
		int32_t accessorComponentBytes = tinygltf::GetComponentSizeInBytes(accessor.componentType);
		int32_t accessorBytesPerStride = accessorComponentElements * accessorComponentBytes;

		if (sizeof(T) != accessorBytesPerStride)
		{
			throw std::runtime_error("sizeof(T) does not much accessor bytes.");
		}

		size_t accessorBytes = accessorByteStride * accessor.count;
		if (accessorBytes > bufferView.byteLength || accessor.byteOffset + accessorBytesPerStride > accessorByteStride)
		{
			throw std::runtime_error("accessor does not fit in bufferView.");
		}

		const unsigned char* pBufferData = &data[0];
		const unsigned char* pBufferViewData = pBufferData + bufferView.byteOffset;

		this->_stride = accessorByteStride;
		this->_offset = accessor.byteOffset;
		this->_size = accessor.count;
		this->_pBufferViewData = pBufferViewData;
	}

	const T& operator[](size_t i)
	{
		if (i < 0 || i >= this->_size)
		{
			throw std::range_error("index out of range");
		}

		return *reinterpret_cast<const T*>(this->_pBufferViewData + i * this->_stride + this->_offset);
	}

	size_t size()
	{
		return this->_size;
	}
};

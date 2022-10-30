// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumDerivedDataCache.h"

//#include "Compression/lz4.h"

#include <CesiumGltfReader/GltfReader.h>
#include <CesiumGltfWriter/GltfWriter.h>

namespace CesiumDerivedDataCache {
namespace {
/*
  Format Outline:
  1. CachedGltfHeader
  2. glTF JSON
  3. CachedBufferDescription 1, 2, 3...
  4. CachedImageDescription 1, 2, 3...
  5. CachedPhysicsMeshDescription 1, 2, 3...
  5. Binary Chunk with decoded buffers / images / cooked physics meshes
 */
// TODO: Formalize the format
// TODO: Encode current "options", so we can invalidate on
// options changing.

enum ECachedPhysicsType : uint32_t {
  PHYSX = 0,
  CHAOS = 1 // TODO: not supported yet
};

struct CachedGltfHeader {
  unsigned char magic[4];
  uint32_t version;
  uint32_t gltfJsonSize;
  uint32_t cachedBuffersCount;
  uint32_t cachedImagesCount;
  uint32_t cachedPhysicsType;
  uint32_t cachedPhysicsMeshesCount;
};

struct CachedBufferDescription {
  uint32_t byteOffset;
  uint32_t byteSize;
};

struct CachedImageDescription {
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint32_t bytesPerChannel;
  uint32_t mipCount;
  uint32_t byteOffset;
  uint32_t byteSize;
};

struct CachedPhysicsMeshDescription {
  uint32_t byteOffset;
  uint32_t byteSize;
};
} // namespace

// Convenient macro for runtime "asserting" - fails the function otherwise.
#define PROCEED_IF(stmt)                                                       \
  if (!(stmt)) {                                                               \
    return std::nullopt;                                                       \
  }
std::optional<DerivedDataResult>
deserialize(const gsl::span<const std::byte>& cache) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::DeserializeGltf)

  // TODO: should be static thread-local?
  // std::vector<std::byte> cache;
  //{
  //  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LZ4_Decompress)
  //  cache.resize(4096);

  //}

  PROCEED_IF(cache.size() >= sizeof(CachedGltfHeader))

  size_t readPos = 0;

  const CachedGltfHeader& header =
      *reinterpret_cast<const CachedGltfHeader*>(&cache[readPos]);
  readPos += sizeof(CachedGltfHeader);

  // TODO: look for a more elegant magic check!
  PROCEED_IF(header.magic[0] == 'C')
  PROCEED_IF(header.magic[1] == '4')
  PROCEED_IF(header.magic[2] == 'U')
  PROCEED_IF(header.magic[3] == 'E')
  PROCEED_IF(header.version == 1)

  PROCEED_IF(cache.size() >= readPos + header.gltfJsonSize);

  gsl::span<const std::byte> gltfJsonBytes(
      &cache[readPos],
      header.gltfJsonSize);
  readPos += header.gltfJsonSize;

  CesiumGltfReader::GltfReader reader;
  CesiumGltfReader::GltfReaderResult gltfJsonResult =
      reader.readGltf(gltfJsonBytes);

  PROCEED_IF(gltfJsonResult.errors.empty() && gltfJsonResult.model)
  PROCEED_IF(
      gltfJsonResult.model->buffers.size() == header.cachedBuffersCount &&
      gltfJsonResult.model->images.size() == header.cachedImagesCount)

  size_t bufferDescriptionsSize =
      header.cachedBuffersCount * sizeof(CachedBufferDescription) +
      header.cachedImagesCount * sizeof(CachedImageDescription);

  PROCEED_IF(cache.size() >= readPos + bufferDescriptionsSize);

  for (CesiumGltf::Buffer& buffer : gltfJsonResult.model->buffers) {
    const CachedBufferDescription& description =
        *reinterpret_cast<const CachedBufferDescription*>(&cache[readPos]);
    readPos += sizeof(CachedBufferDescription);

    PROCEED_IF(cache.size() >= description.byteOffset + description.byteSize)
    buffer.cesium.data.resize(description.byteSize);
    std::memcpy(
        buffer.cesium.data.data(),
        &cache[description.byteOffset],
        description.byteSize);
  }

  for (CesiumGltf::Image& image : gltfJsonResult.model->images) {
    const CachedImageDescription& description =
        *reinterpret_cast<const CachedImageDescription*>(&cache[readPos]);
    readPos += sizeof(CachedImageDescription);

    PROCEED_IF(cache.size() >= description.byteOffset + description.byteSize)
    image.cesium.pixelData.resize(description.byteSize);
    std::memcpy(
        image.cesium.pixelData.data(),
        &cache[description.byteOffset],
        description.byteSize);

    image.cesium.width = static_cast<int32_t>(description.width);
    image.cesium.height = static_cast<int32_t>(description.height);
    image.cesium.channels = static_cast<int32_t>(description.channels);
    image.cesium.bytesPerChannel =
        static_cast<int32_t>(description.bytesPerChannel);

    image.cesium.mipPositions.resize(description.mipCount);

    size_t mipByteOffset = 0;

    // A mip count of 0 indicates there is only one mip and it is within the
    // pixelData.
    if (description.mipCount != 0) {
      CesiumGltf::ImageCesiumMipPosition& mip0 = image.cesium.mipPositions[0];
      mip0.byteOffset = mipByteOffset;
      mip0.byteSize = description.width * description.height *
                      description.channels * description.bytesPerChannel;

      mipByteOffset += mip0.byteSize;
    }

    for (uint32_t mipIndex = 1; mipIndex < description.mipCount; ++mipIndex) {
      uint32_t mipWidth = description.width >> mipIndex;
      uint32_t mipHeight = description.height >> mipIndex;

      if (mipWidth < 1) {
        mipWidth = 1;
      }

      if (mipHeight < 1) {
        mipHeight = 1;
      }

      CesiumGltf::ImageCesiumMipPosition& mip =
          image.cesium.mipPositions[mipIndex];
      mip.byteOffset = mipByteOffset;
      mip.byteSize = mipWidth * mipHeight * description.channels *
                     description.bytesPerChannel;

      mipByteOffset += mip.byteSize;
    }
  }
  // TODO: physics!!
  // https://github.com/EpicGames/UnrealEngine/blob/46544fa5e0aa9e6740c19b44b0628b72e7bbd5ce/Engine/Source/Runtime/Engine/Private/PhysicsEngine/BodySetup.cpp#L531

#if PHYSICS_INTERFACE_PHYSX
  PROCEED_IF(header.cachedPhysicsType == ECachedPhysicsType::PHYSX);
#else
  PROCEED_IF(header.cachedPhysicsType == ECachedPhysicsType::CHAOS);
#endif

  DerivedDataResult result;
  result.model = std::move(*gltfJsonResult.model);

  size_t physicsMeshDescriptionsSize =
      header.cachedPhysicsMeshesCount * sizeof(CachedPhysicsMeshDescription);
  PROCEED_IF(cache.size() >= readPos + physicsMeshDescriptionsSize);

  result.cookedPhysicsMeshViews.reserve(header.cachedPhysicsMeshesCount);

  for (uint32_t i = 0; i < header.cachedPhysicsMeshesCount; ++i) {
    const CachedPhysicsMeshDescription& description =
        *reinterpret_cast<const CachedPhysicsMeshDescription*>(&cache[readPos]);
    readPos += sizeof(CachedPhysicsMeshDescription);

    PROCEED_IF(cache.size() >= description.byteOffset + description.byteSize);

    result.cookedPhysicsMeshViews.push_back(gsl::span<const std::byte>(
        &cache[description.byteOffset],
        description.byteSize));
  }

  // TODO: Would RVO happen if I just "return result"?
  // ... it gets implicitly converted to an optional,
  // but does it implicitly "move" the result into the
  // optional? But "return std::move(result)" also seems
  // non-idiomatic...
  return std::make_optional(std::move(result));
}
#undef PROCEED_IF

std::vector<std::byte> serialize(const DerivedDataToCache& derivedData) {
  check(derivedData.pModel != nullptr);

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::serializeGltf)

  const CesiumGltf::Model& model = *derivedData.pModel;

  uint32_t bufferCount = static_cast<uint32_t>(model.buffers.size());
  uint32_t imageCount = static_cast<uint32_t>(model.images.size());
  uint32_t physicsMeshCount =
      static_cast<uint32_t>(derivedData.cookedPhysicsMeshViews.size());

  CesiumGltfWriter::GltfWriter writer;
  CesiumGltfWriter::GltfWriterResult gltfJsonResult = writer.writeGltf(model);

  if (!gltfJsonResult.errors.empty()) {
    return {};
  }

  size_t gltfJsonSize = gltfJsonResult.gltfBytes.size();

  uint32_t binaryChunkSize = 0;
  for (uint32_t i = 0; i < bufferCount; ++i) {
    binaryChunkSize += model.buffers[i].cesium.data.size();
  }

  for (uint32_t i = 0; i < imageCount; ++i) {
    binaryChunkSize += model.images[i].cesium.pixelData.size();
  }

  for (uint32_t i = 0; i < physicsMeshCount; ++i) {
    binaryChunkSize += derivedData.cookedPhysicsMeshViews[i].size();
  }

  // TODO: alignment?
  size_t binaryChunkOffset =
      sizeof(CachedGltfHeader) + gltfJsonSize +
      bufferCount * sizeof(CachedBufferDescription) +
      imageCount * sizeof(CachedImageDescription) +
      physicsMeshCount * sizeof(CachedPhysicsMeshDescription);
  size_t totalAllocation = binaryChunkOffset + binaryChunkSize;

  std::vector<std::byte> result(totalAllocation);
  std::byte* pWritePos = result.data();

  CachedGltfHeader& header = *reinterpret_cast<CachedGltfHeader*>(pWritePos);
  header.magic[0] = 'C';
  header.magic[1] = '4';
  header.magic[2] = 'U';
  header.magic[3] = 'E';
  header.version = 1;
  header.gltfJsonSize = static_cast<uint32_t>(gltfJsonSize);
  header.cachedBuffersCount = bufferCount;
  header.cachedImagesCount = imageCount;
  header.cachedPhysicsMeshesCount = physicsMeshCount;
  pWritePos += sizeof(CachedGltfHeader);

  // Copy gltf json
  std::memcpy(pWritePos, gltfJsonResult.gltfBytes.data(), gltfJsonSize);
  pWritePos += gltfJsonSize;

  size_t binaryChunkWritePos = binaryChunkOffset;

  for (const CesiumGltf::Buffer& buffer : model.buffers) {
    CachedBufferDescription& description =
        *reinterpret_cast<CachedBufferDescription*>(pWritePos);
    description.byteOffset = static_cast<uint32_t>(binaryChunkWritePos);
    description.byteSize = static_cast<uint32_t>(buffer.cesium.data.size());
    pWritePos += sizeof(CachedBufferDescription);

    std::memcpy(
        &result[binaryChunkWritePos],
        buffer.cesium.data.data(),
        description.byteSize);
    binaryChunkWritePos += description.byteSize;
  }

  for (const CesiumGltf::Image& image : model.images) {
    CachedImageDescription& description =
        *reinterpret_cast<CachedImageDescription*>(pWritePos);
    description.width = static_cast<uint32_t>(image.cesium.width);
    description.height = static_cast<uint32_t>(image.cesium.height);
    description.channels = static_cast<uint32_t>(image.cesium.channels);
    description.bytesPerChannel =
        static_cast<uint32_t>(image.cesium.bytesPerChannel);
    description.mipCount =
        static_cast<uint32_t>(image.cesium.mipPositions.size());
    description.byteOffset = static_cast<uint32_t>(binaryChunkWritePos);
    description.byteSize = static_cast<uint32_t>(image.cesium.pixelData.size());
    pWritePos += sizeof(CachedImageDescription);

    std::memcpy(
        &result[binaryChunkWritePos],
        image.cesium.pixelData.data(),
        description.byteSize);
    binaryChunkWritePos += description.byteSize;
  }

  for (const gsl::span<const std::byte>& cookedPhysicsMesh :
       derivedData.cookedPhysicsMeshViews) {
    CachedPhysicsMeshDescription& description =
        *reinterpret_cast<CachedPhysicsMeshDescription*>(pWritePos);
    description.byteOffset = static_cast<uint32_t>(binaryChunkWritePos);
    description.byteSize = static_cast<uint32_t>(cookedPhysicsMesh.size());
    pWritePos += sizeof(CachedPhysicsMeshDescription);

    std::memcpy(
        &result[binaryChunkWritePos],
        cookedPhysicsMesh.data(),
        cookedPhysicsMesh.size());
    binaryChunkWritePos += description.byteSize;
  }

  // The description and json writing should end at the start of the binary
  // chunk.
  check(pWritePos == &result[binaryChunkOffset]);

  // The written binary chunk should end as the expected at the very end of the
  // allocation.
  check(binaryChunkWritePos == totalAllocation);

  // TODO: if we are returning a separate compressed vector of bytes, then the
  // "result" vector is just a scratch vector. So we should keep around the
  // allocation. Maybe result should be static thread-local?
  /*/ std::vector<std::byte> lz4CompressedBuffer;

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LZ4_Compress)

    lz4CompressedBuffer.resize(LZ4_compressBound(result.size()));

    LZ4_compress_default(
        (const char*)result.data(),
        (char*)lz4CompressedBuffer.data(),
        result.size(),
        lz4CompressedBuffer.size());
  }*/

  return result;
  // return lz4CompressedBuffer;
}
} // namespace CesiumDerivedDataCache

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "CesiumAsync/AsyncSystem.h"
#include "ExtensionImageCesiumUnreal.h"
#include "Misc/AutomationTest.h"
#include "RenderingThread.h"
#include <CesiumGltfReader/GltfReader.h>
#include <UnrealTaskProcessor.h>
#include <memory>

using namespace CesiumGltf;
using namespace CesiumTextureUtility;
using namespace CesiumUtility;

BEGIN_DEFINE_SPEC(
    CesiumTextureUtilitySpec,
    "Cesium.Unit.CesiumTextureUtility",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter | EAutomationTestFlags::NonNullRHI)
std::vector<uint8_t> originalPixels;
std::vector<uint8_t> originalMipPixels;
std::vector<uint8_t> expectedMipPixelsIfGenerated;
SharedAsset<ImageCesium> imageCesium;

void RunTests();

void CheckPixels(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    bool requireMips = false);
void CheckSRGB(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    bool expectedSRGB);
void CheckAddress(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureAddress expectedAddressX,
    TextureAddress expectedAddressY);
void CheckFilter(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureFilter expectedFilter);
void CheckGroup(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureGroup expectedGroup);
END_DEFINE_SPEC(CesiumTextureUtilitySpec)

void CesiumTextureUtilitySpec::Define() {
  Describe("Without Mips", [this]() {
    BeforeEach([this]() {
      originalPixels = {0x20, 0x40, 0x80, 0xF0, 0x21, 0x41, 0x81, 0xF1,
                        0x22, 0x42, 0x82, 0xF2, 0x23, 0x43, 0x83, 0xF3,
                        0x24, 0x44, 0x84, 0xF4, 0x25, 0x45, 0x85, 0xF5};
      originalMipPixels.clear();

      ImageCesium image{};
      imageCesium = SharedAsset<ImageCesium>(image);
      imageCesium->width = 3;
      imageCesium->height = 2;
      TestEqual(
          "image buffer size is correct",
          originalPixels.size(),
          imageCesium->width * imageCesium->height *
              imageCesium->bytesPerChannel * imageCesium->channels);
      imageCesium->pixelData.resize(originalPixels.size());

      std::memcpy(
          imageCesium->pixelData.data(),
          originalPixels.data(),
          originalPixels.size());

      ImageCesium copy = *imageCesium;
      CesiumGltfReader::GltfReader::generateMipMaps(copy);

      expectedMipPixelsIfGenerated.clear();

      if (copy.mipPositions.size() >= 2) {
        expectedMipPixelsIfGenerated.resize(copy.mipPositions[1].byteSize);
        for (size_t iSrc = copy.mipPositions[1].byteOffset, iDest = 0;
             iDest < copy.mipPositions[1].byteSize;
             ++iSrc, ++iDest) {
          expectedMipPixelsIfGenerated[iDest] = uint8_t(copy.pixelData[iSrc]);
        }
      }
    });

    RunTests();
  });

  Describe("With Mips", [this]() {
    BeforeEach([this]() {
      imageCesium = {};
      imageCesium->width = 3;
      imageCesium->height = 2;

      // Original image (3x2)
      originalPixels = {0x20, 0x40, 0x80, 0xF0, 0x21, 0x41, 0x81, 0xF1,
                        0x22, 0x42, 0x82, 0xF2, 0x23, 0x43, 0x83, 0xF3,
                        0x24, 0x44, 0x84, 0xF4, 0x25, 0x45, 0x85, 0xF5};
      imageCesium->mipPositions.emplace_back(
          ImageCesiumMipPosition{0, originalPixels.size()});

      // Mip 1 (1x1)
      originalMipPixels = {0x26, 0x46, 0x86, 0xF6};
      imageCesium->mipPositions.emplace_back(ImageCesiumMipPosition{
          imageCesium->mipPositions[0].byteSize,
          originalMipPixels.size()});

      imageCesium->pixelData.resize(
          originalPixels.size() + originalMipPixels.size());
      std::memcpy(
          imageCesium->pixelData.data(),
          originalPixels.data(),
          originalPixels.size());
      std::memcpy(
          imageCesium->pixelData.data() + originalPixels.size(),
          originalMipPixels.data(),
          originalMipPixels.size());
    });

    RunTests();
  });
}

void CesiumTextureUtilitySpec::RunTests() {
  It("ImageCesium non-sRGB", [this]() {
    TUniquePtr<LoadedTextureResult> pHalfLoaded = loadTextureAnyThreadPart(
        *imageCesium,
        TextureAddress::TA_Mirror,
        TextureAddress::TA_Wrap,
        TextureFilter::TF_Bilinear,
        true,
        TextureGroup::TEXTUREGROUP_Cinematic,
        false,
        std::nullopt);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, true);
    CheckSRGB(pRefCountedTexture, false);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Mirror,
        TextureAddress::TA_Wrap);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Bilinear);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_Cinematic);
  });

  It("ImageCesium sRGB", [this]() {
    TUniquePtr<LoadedTextureResult> pHalfLoaded = loadTextureAnyThreadPart(
        *imageCesium,
        TextureAddress::TA_Clamp,
        TextureAddress::TA_Mirror,
        TextureFilter::TF_Trilinear,
        true,
        TextureGroup::TEXTUREGROUP_Bokeh,
        true,
        std::nullopt);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, true);
    CheckSRGB(pRefCountedTexture, true);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Clamp,
        TextureAddress::TA_Mirror);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Trilinear);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_Bokeh);
  });

  It("Image and Sampler", [this]() {
    Sampler sampler;
    sampler.minFilter = Sampler::MinFilter::NEAREST;
    sampler.magFilter = Sampler::MagFilter::NEAREST;
    sampler.wrapS = Sampler::WrapS::MIRRORED_REPEAT;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    TUniquePtr<LoadedTextureResult> pHalfLoaded =
        loadTextureFromImageAndSamplerAnyThreadPart(
            *imageCesium,
            sampler,
            false);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, false);
    CheckSRGB(pRefCountedTexture, false);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Mirror,
        TextureAddress::TA_Clamp);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Nearest);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_World);
  });

  It("Model", [this]() {
    Model model;

    Image& image = model.images.emplace_back();
    image.cesium = imageCesium;

    Sampler& sampler = model.samplers.emplace_back();
    sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
    sampler.magFilter = Sampler::MagFilter::LINEAR;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;

    Texture& texture = model.textures.emplace_back();
    texture.source = 0;
    texture.sampler = 0;

    TUniquePtr<LoadedTextureResult> pHalfLoaded =
        loadTextureFromModelAnyThreadPart(model, texture, true);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());
    TestNotNull("pHalfLoaded->pTexture", pHalfLoaded->pTexture.get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(model, pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, true);
    CheckSRGB(pRefCountedTexture, true);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Wrap,
        TextureAddress::TA_Mirror);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Default);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_World);
  });

  It("Two textures referencing one image", [this]() {
    Model model;

    Image& image = model.images.emplace_back();
    image.cesium = imageCesium;

    Sampler& sampler1 = model.samplers.emplace_back();
    sampler1.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
    sampler1.magFilter = Sampler::MagFilter::LINEAR;
    sampler1.wrapS = Sampler::WrapS::REPEAT;
    sampler1.wrapT = Sampler::WrapT::MIRRORED_REPEAT;

    Texture& texture1 = model.textures.emplace_back();
    texture1.source = 0;
    texture1.sampler = 0;

    Sampler& sampler2 = model.samplers.emplace_back();
    sampler2.minFilter = Sampler::MinFilter::NEAREST;
    sampler2.magFilter = Sampler::MagFilter::NEAREST;
    sampler2.wrapS = Sampler::WrapS::MIRRORED_REPEAT;
    sampler2.wrapT = Sampler::WrapT::REPEAT;

    Texture& texture2 = model.textures.emplace_back();
    texture2.source = 0;
    texture2.sampler = 1;

    TUniquePtr<LoadedTextureResult> pHalfLoaded1 =
        loadTextureFromModelAnyThreadPart(model, model.textures[0], true);
    TestNotNull("pHalfLoaded1", pHalfLoaded1.Get());
    TestNotNull("pHalfLoaded1->pTexture", pHalfLoaded1->pTexture.get());

    TUniquePtr<LoadedTextureResult> pHalfLoaded2 =
        loadTextureFromModelAnyThreadPart(model, model.textures[1], false);
    TestNotNull("pHalfLoaded2", pHalfLoaded2.Get());
    TestNotNull("pHalfLoaded2->pTexture", pHalfLoaded2->pTexture.get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture1 =
        loadTextureGameThreadPart(model, pHalfLoaded1.Get());
    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture2 =
        loadTextureGameThreadPart(model, pHalfLoaded2.Get());

    CheckPixels(pRefCountedTexture1, true);
    CheckSRGB(pRefCountedTexture1, true);
    CheckAddress(
        pRefCountedTexture1,
        TextureAddress::TA_Wrap,
        TextureAddress::TA_Mirror);
    CheckFilter(pRefCountedTexture1, TextureFilter::TF_Default);
    CheckGroup(pRefCountedTexture1, TextureGroup::TEXTUREGROUP_World);

    CheckPixels(pRefCountedTexture2, false);
    CheckSRGB(pRefCountedTexture2, false);
    CheckAddress(
        pRefCountedTexture2,
        TextureAddress::TA_Mirror,
        TextureAddress::TA_Wrap);
    CheckFilter(pRefCountedTexture2, TextureFilter::TF_Nearest);
    CheckGroup(pRefCountedTexture2, TextureGroup::TEXTUREGROUP_World);

    TestEqual(
        "Textures share RHI resource",
        pRefCountedTexture1->getUnrealTexture()->GetResource()->GetTextureRHI(),
        pRefCountedTexture2->getUnrealTexture()
            ->GetResource()
            ->GetTextureRHI());
  });

  It("Loading the same texture twice", [this]() {
    Model model;

    Image& image = model.images.emplace_back();
    image.cesium = imageCesium;

    Sampler& sampler = model.samplers.emplace_back();
    sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
    sampler.magFilter = Sampler::MagFilter::LINEAR;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;

    Texture& texture = model.textures.emplace_back();
    texture.source = 0;
    texture.sampler = 0;

    TUniquePtr<LoadedTextureResult> pHalfLoaded =
        loadTextureFromModelAnyThreadPart(model, texture, true);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());
    TestNotNull("pHalfLoaded->pTexture", pHalfLoaded->pTexture.get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(model, pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, true);
    CheckSRGB(pRefCountedTexture, true);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Wrap,
        TextureAddress::TA_Mirror);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Default);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_World);

    // Copy the model and load the same texture again.
    // This time there's no more pixel data, so it's necessary to use the
    // previously-created texture.
    Model model2 = model;
    TUniquePtr<LoadedTextureResult> pHalfLoaded2 =
        loadTextureFromModelAnyThreadPart(model2, model.textures[0], true);
    TestNotNull("pHalfLoaded2", pHalfLoaded2.Get());
    TestNotNull("pHalfLoaded2->pTexture", pHalfLoaded2->pTexture.get());
    TestNull(
        "pHalfLoaded2->pTexture->getTextureResource()",
        pHalfLoaded2->pTexture->getTextureResource().Get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture2 =
        loadTextureGameThreadPart(model, pHalfLoaded.Get());
    TestEqual("Same textures", pRefCountedTexture2, pRefCountedTexture);
  });

  It("Loading the same texture twice from one model", [this]() {
    Model model;

    Image& image = model.images.emplace_back();
    image.cesium = imageCesium;

    Sampler& sampler = model.samplers.emplace_back();
    sampler.minFilter = Sampler::MinFilter::LINEAR_MIPMAP_LINEAR;
    sampler.magFilter = Sampler::MagFilter::LINEAR;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;

    Texture& texture = model.textures.emplace_back();
    texture.source = 0;
    texture.sampler = 0;

    TUniquePtr<LoadedTextureResult> pHalfLoaded =
        loadTextureFromModelAnyThreadPart(model, texture, true);
    TestNotNull("pHalfLoaded", pHalfLoaded.Get());
    TestNotNull("pHalfLoaded->pTexture", pHalfLoaded->pTexture.get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture =
        loadTextureGameThreadPart(model, pHalfLoaded.Get());
    CheckPixels(pRefCountedTexture, true);
    CheckSRGB(pRefCountedTexture, true);
    CheckAddress(
        pRefCountedTexture,
        TextureAddress::TA_Wrap,
        TextureAddress::TA_Mirror);
    CheckFilter(pRefCountedTexture, TextureFilter::TF_Default);
    CheckGroup(pRefCountedTexture, TextureGroup::TEXTUREGROUP_World);

    // Load the same texture again.
    // This time there's no more pixel data, so it's necessary to use the
    // previously-created texture.
    TUniquePtr<LoadedTextureResult> pHalfLoaded2 =
        loadTextureFromModelAnyThreadPart(model, model.textures[0], true);
    TestNotNull("pHalfLoaded2", pHalfLoaded2.Get());
    TestNotNull("pHalfLoaded2->pTexture", pHalfLoaded2->pTexture.get());
    TestNull(
        "pHalfLoaded2->pTexture->getTextureResource()",
        pHalfLoaded2->pTexture->getTextureResource().Get());

    IntrusivePointer<ReferenceCountedUnrealTexture> pRefCountedTexture2 =
        loadTextureGameThreadPart(model, pHalfLoaded.Get());
    TestEqual("Same textures", pRefCountedTexture2, pRefCountedTexture);
  });
}

void CesiumTextureUtilitySpec::CheckPixels(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    bool requireMips) {
  TestNotNull("pRefCountedTexture", pRefCountedTexture.get());
  TestNotNull(
      "pRefCountedTexture->getUnrealTexture()",
      pRefCountedTexture->getUnrealTexture().Get());

  UTexture2D* pTexture = pRefCountedTexture->getUnrealTexture();
  TestNotNull("pTexture", pTexture);
  if (pTexture == nullptr)
    return;

  FTextureResource* pResource = pTexture->GetResource();
  TestNotNull("pResource", pResource);
  if (pResource == nullptr)
    return;

  TArray<FColor> readPixels;
  TArray<FColor> readPixelsMip1;

  ENQUEUE_RENDER_COMMAND(ReadSurfaceCommand)
  ([pResource, &readPixels, &readPixelsMip1](
       FRHICommandListImmediate& RHICmdList) {
    FRHITexture* pRHITexture = pResource->GetTextureRHI();
    if (pRHITexture == nullptr)
      return;

    FReadSurfaceDataFlags flags{};
    flags.SetLinearToGamma(false);
    RHICmdList
        .ReadSurfaceData(pRHITexture, FIntRect(0, 0, 3, 2), readPixels, flags);

    if (pRHITexture->GetNumMips() > 1) {
      flags.SetMip(1);
      RHICmdList.ReadSurfaceData(
          pRHITexture,
          FIntRect(0, 0, 1, 1),
          readPixelsMip1,
          flags);
    }
  });
  FlushRenderingCommands();

  TestEqual("read buffer size", readPixels.Num() * 4, originalPixels.size());
  for (size_t i = 0; i < readPixels.Num(); ++i) {
    TestEqual("pixel-red", readPixels[i].R, originalPixels[i * 4]);
    TestEqual("pixel-green", readPixels[i].G, originalPixels[i * 4 + 1]);
    TestEqual("pixel-blue", readPixels[i].B, originalPixels[i * 4 + 2]);
    TestEqual("pixel-alpha", readPixels[i].A, originalPixels[i * 4 + 3]);
  }

  if (requireMips) {
    TestTrue("Has Mips", !readPixelsMip1.IsEmpty());
  }

  if (!readPixelsMip1.IsEmpty()) {
    std::vector<uint8_t>& pixelsToMatch = originalMipPixels.empty()
                                              ? expectedMipPixelsIfGenerated
                                              : originalMipPixels;

    TestEqual(
        "read buffer size",
        readPixelsMip1.Num() * 4,
        pixelsToMatch.size());
    for (size_t i = 0;
         i < readPixelsMip1.Num() && (i * 4 + 3) < pixelsToMatch.size();
         ++i) {
      TestEqual("mip pixel-red", readPixelsMip1[i].R, pixelsToMatch[i * 4]);
      TestEqual(
          "mip pixel-green",
          readPixelsMip1[i].G,
          pixelsToMatch[i * 4 + 1]);
      TestEqual(
          "mip pixel-blue",
          readPixelsMip1[i].B,
          pixelsToMatch[i * 4 + 2]);
      TestEqual(
          "mip pixel-alpha",
          readPixelsMip1[i].A,
          pixelsToMatch[i * 4 + 3]);
    }
  }
}

void CesiumTextureUtilitySpec::CheckSRGB(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    bool expectedSRGB) {
  TestNotNull("pRefCountedTexture", pRefCountedTexture.get());
  if (!pRefCountedTexture)
    return;

  UTexture2D* pTexture = pRefCountedTexture->getUnrealTexture();
  TestNotNull("pTexture", pTexture);
  if (!pTexture)
    return;

  TestEqual("SRGB", pTexture->SRGB, expectedSRGB);

  FTextureResource* pResource = pTexture->GetResource();
  TestNotNull("pResource", pResource);
  if (!pResource)
    return;

  TestEqual("RHI sRGB", pResource->bSRGB, expectedSRGB);
}

void CesiumTextureUtilitySpec::CheckAddress(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureAddress expectedAddressX,
    TextureAddress expectedAddressY) {
  TestNotNull("pRefCountedTexture", pRefCountedTexture.get());
  if (!pRefCountedTexture)
    return;

  UTexture2D* pTexture = pRefCountedTexture->getUnrealTexture();
  TestNotNull("pTexture", pTexture);
  if (!pTexture)
    return;

  TestEqual("AddressX", pTexture->AddressX, expectedAddressX);
  TestEqual("AddressY", pTexture->AddressY, expectedAddressY);
}

void CesiumTextureUtilitySpec::CheckFilter(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureFilter expectedFilter) {
  TestNotNull("pRefCountedTexture", pRefCountedTexture.get());
  if (!pRefCountedTexture)
    return;

  UTexture2D* pTexture = pRefCountedTexture->getUnrealTexture();
  TestNotNull("pTexture", pTexture);
  if (!pTexture)
    return;

  TestEqual("Filter", pTexture->Filter, expectedFilter);
}

void CesiumTextureUtilitySpec::CheckGroup(
    const IntrusivePointer<ReferenceCountedUnrealTexture>& pRefCountedTexture,
    TextureGroup expectedGroup) {
  TestNotNull("pRefCountedTexture", pRefCountedTexture.get());
  if (!pRefCountedTexture)
    return;

  UTexture2D* pTexture = pRefCountedTexture->getUnrealTexture();
  TestNotNull("pTexture", pTexture);
  if (!pTexture)
    return;

  TestEqual("LODGroup", pTexture->LODGroup, expectedGroup);
}

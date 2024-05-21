// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumUtility/Math.h>

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumFeatureIdTextureSpec,
    "Cesium.Unit.FeatureIdTexture",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
const std::vector<glm::vec2> texCoords{
    glm::vec2(0, 0),
    glm::vec2(0.5, 0),
    glm::vec2(0, 0.5),
    glm::vec2(0.5, 0.5)};
TObjectPtr<UCesiumGltfPrimitiveComponent> pPrimitiveComponent;
CesiumPrimitiveData* pData;
END_DEFINE_SPEC(FCesiumFeatureIdTextureSpec)

void FCesiumFeatureIdTextureSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("constructs invalid instance for empty texture", [this]() {
      FCesiumFeatureIdTexture featureIDTexture;
      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::ErrorInvalidTexture);

      auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::ErrorUninitialized);
    });

    It("constructs invalid instance for nonexistent texture", [this]() {
      FeatureIdTexture texture;
      texture.index = -1;
      texture.texCoord = 0;
      texture.channels = {0};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::ErrorInvalidTexture);

      auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::ErrorInvalidTexture);
    });

    It("constructs invalid instance for texture with invalid image", [this]() {
      CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
      gltfTexture.source = -1;

      FeatureIdTexture texture;
      texture.index = 0;
      texture.texCoord = 0;
      texture.channels = {0};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::ErrorInvalidTexture);

      auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::ErrorInvalidImage);
    });

    It("constructs valid instance", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::Valid);
    });

    It("constructs valid instance for texture with nonexistent texcoord attribute",
       [this]() {
         Image& image = model.images.emplace_back();
         image.cesium.width = image.cesium.height = 1;
         image.cesium.channels = 1;
         image.cesium.pixelData.push_back(std::byte(42));

         Sampler& sampler = model.samplers.emplace_back();
         sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
         sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

         CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
         gltfTexture.source = 0;
         gltfTexture.sampler = 0;

         FeatureIdTexture texture;
         texture.index = 0;
         texture.texCoord = 0;
         texture.channels = {0};

         FCesiumFeatureIdTexture featureIDTexture(
             model,
             *pPrimitive,
             texture,
             "PropertyTableName");

         TestEqual(
             "FeatureIDTextureStatus",
             UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
                 featureIDTexture),
             ECesiumFeatureIdTextureStatus::Valid);

         auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
         TestEqual(
             "FeatureIDTextureViewStatus",
             featureIDTextureView.status(),
             FeatureIdTextureViewStatus::Valid);
       });

    It("constructs valid instance for texture with invalid texcoord accessor",
       [this]() {
         Image& image = model.images.emplace_back();
         image.cesium.width = image.cesium.height = 1;
         image.cesium.channels = 1;
         image.cesium.pixelData.push_back(std::byte(42));

         Sampler& sampler = model.samplers.emplace_back();
         sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
         sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

         CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
         gltfTexture.source = 0;
         gltfTexture.sampler = 0;

         FeatureIdTexture texture;
         texture.index = 0;
         texture.texCoord = 0;
         texture.channels = {0};

         pPrimitive->attributes.insert({"TEXCOORD_0", 0});

         FCesiumFeatureIdTexture featureIDTexture(
             model,
             *pPrimitive,
             texture,
             "PropertyTableName");

         TestEqual(
             "FeatureIDTextureStatus",
             UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
                 featureIDTexture),
             ECesiumFeatureIdTextureStatus::Valid);

         auto featureIDTextureView = featureIDTexture.getFeatureIdTextureView();
         TestEqual(
             "FeatureIDTextureViewStatus",
             featureIDTextureView.status(),
             FeatureIdTextureViewStatus::Valid);
       });
  });

  Describe("GetFeatureIDForUV", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns -1 for invalid texture", [this]() {
      CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
      gltfTexture.source = -1;

      FeatureIdTexture texture;
      texture.index = 0;
      texture.texCoord = 0;
      texture.channels = {0};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          texture,
          "PropertyTableName");

      TestNotEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      TestEqual(
          "FeatureID",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForUV(
              featureIDTexture,
              FVector2D::Zero()),
          -1);
    });

    It("returns correct value for valid attribute", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      for (size_t i = 0; i < texCoords.size(); i++) {
        const glm::vec2& texCoord = texCoords[i];
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForUV(
                featureIDTexture,
                {texCoord.x, texCoord.y});
        TestEqual("FeatureID", featureID, featureIDs[i]);
      }
    });

    It("returns correct value with KHR_texture_transform", [this]() {
      const std::vector<uint8_t> featureIDs{1, 2, 0, 7};
      const std::vector<glm::vec2> rawTexCoords{
          glm::vec2(0, 0),
          glm::vec2(1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 1)};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          rawTexCoords,
          0,
          Sampler::WrapS::REPEAT,
          Sampler::WrapT::REPEAT);

      assert(featureId.texture != std::nullopt);
      ExtensionKhrTextureTransform& textureTransform =
          featureId.texture->addExtension<ExtensionKhrTextureTransform>();
      textureTransform.offset = {0.5, -0.5};
      textureTransform.rotation = UE_DOUBLE_HALF_PI;
      textureTransform.scale = {0.5, 0.5};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
      // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
      // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
      // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
      std::vector<uint8_t> expected{7, 2, 0, 1};

      for (size_t i = 0; i < texCoords.size(); i++) {
        const glm::vec2& texCoord = rawTexCoords[i];
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForUV(
                featureIDTexture,
                {texCoord.x, texCoord.y});
        TestEqual("FeatureID", featureID, expected[i]);
      }
    });
  });

  Describe("GetFeatureIDForVertex", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns -1 for invalid texture", [this]() {
      FeatureIdTexture texture;
      texture.index = -1;
      texture.texCoord = 0;
      texture.channels = {0};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          texture,
          "PropertyTableName");

      TestNotEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
              featureIDTexture,
              0),
          -1);
    });

    It("returns -1 for out-of-bounds index", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      TestEqual(
          "FeatureIDForNegativeVertex",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
              featureIDTexture,
              -1),
          -1);

      TestEqual(
          "FeatureIDForOutOfBoundsVertex",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
              featureIDTexture,
              10),
          -1);
    });

    It("returns correct value for valid texture", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      for (size_t i = 0; i < featureIDs.size(); i++) {
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
                featureIDTexture,
                static_cast<int64>(i));
        TestEqual("FeatureIDForVertex", featureID, featureIDs[i]);
      }
    });

    It("returns correct value for primitive with multiple texcoords", [this]() {
      const std::vector<glm::vec2> texCoord0{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0, 0.5),
          glm::vec2(0.5, 0.5)};

      std::vector<std::byte> values(texCoord0.size());
      std::memcpy(values.data(), texCoord0.data(), values.size());

      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "TEXCOORD_0",
          AccessorSpec::Type::VEC2,
          AccessorSpec::ComponentType::FLOAT,
          std::move(values));

      const std::vector<glm::vec2> texCoord1{
          glm::vec2(0.5, 0.5),
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0.0, 0.5)};

      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoord1,
          1);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      const std::vector<uint8_t> expected{2, 0, 3, 1};
      for (size_t i = 0; i < featureIDs.size(); i++) {
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
                featureIDTexture,
                static_cast<int64>(i));
        TestEqual("FeatureIDForVertex", featureID, expected[i]);
      }
    });
  });

  Describe("GetFeatureIDFromHit", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pPrimitive->mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
      pPrimitiveComponent = NewObject<UCesiumGltfPrimitiveComponent>();
      pData = getPrimitiveData(pPrimitiveComponent);
      pData->pMeshPrimitive = pPrimitive;

      std::vector<glm::vec3> positions{
          glm::vec3(-1, 0, 0),
          glm::vec3(0, 1, 0),
          glm::vec3(1, 0, 0),
          glm::vec3(-1, 3, 0),
          glm::vec3(0, 4, 0),
          glm::vec3(1, 3, 0),
      };

      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "POSITION",
          AccessorSpec::Type::VEC3,
          AccessorSpec::ComponentType::FLOAT,
          positions);
    });

    It("returns -1 for invalid texture", [this]() {
      FeatureIdTexture texture;
      texture.index = -1;
      texture.texCoord = 0;
      texture.channels = {0};

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          texture,
          "PropertyTableName");

      TestNotEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.Location = FVector_NetQuantize::Zero();
      Hit.Component = pPrimitiveComponent;
      Hit.FaceIndex = 0;

      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
              featureIDTexture,
              Hit),
          -1);
    });

    It("returns -1 if hit has no valid component", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords0,
          0);

      pData->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      pData->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      Hit.FaceIndex = 0;
      Hit.Component = nullptr;

      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
              featureIDTexture,
              Hit),
          -1);
    });

    It("returns -1 if specified texcoord set does not exist", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords0,
          0);

      pData->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      pData->TexCoordAccessorMap.emplace(
          1,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
              featureIDTexture,
              Hit),
          -1);
    });

    It("returns correct value for valid texture", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords0,
          0);

      pData->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      pData->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 0, 0),
          FVector_NetQuantize(0, -1, 0),
          FVector_NetQuantize(0.0, -0.25, 0)};
      std::array<int64, 3> expected{3, 1, 0};

      for (size_t i = 0; i < locations.size(); i++) {
        Hit.Location = locations[i];
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
                featureIDTexture,
                Hit);
        TestEqual("FeatureIDFromHit", featureID, expected[i]);
      }
    });

    It("returns correct value for different face", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords0,
          0);

      pData->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      pData->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.FaceIndex = 1;
      Hit.Component = pPrimitiveComponent;

      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 3, 0),
          FVector_NetQuantize(0, -4, 0),
          FVector_NetQuantize(0.0, -3.25, 0)};
      std::array<int64, 3> expected{3, 1, 0};

      for (size_t i = 0; i < locations.size(); i++) {
        Hit.Location = locations[i];
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
                featureIDTexture,
                Hit);
        TestEqual("FeatureIDFromHit", featureID, expected[i]);
      }
    });

    It("returns correct value for primitive with multiple texcoords", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "TEXCOORD_0",
          AccessorSpec::Type::VEC2,
          AccessorSpec::ComponentType::FLOAT,
          GetValuesAsBytes(texCoords0));
      int32 texCoord0AccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      std::vector<glm::vec2> texCoords1{
          glm::vec2(0.5, 0.5),
          glm::vec2(0, 1.0),
          glm::vec2(1, 0),
          glm::vec2(0.5, 0.5),
          glm::vec2(0, 1.0),
          glm::vec2(1, 0),
      };
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords1,
          1);

      FCesiumFeatureIdTexture featureIDTexture(
          model,
          *pPrimitive,
          *featureId.texture,
          "PropertyTableName");

      pData->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      pData->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              texCoord0AccessorIndex));
      pData->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(model, 1));
      pData->TexCoordAccessorMap.emplace(
          1,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      TestEqual(
          "FeatureIDTextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              featureIDTexture),
          ECesiumFeatureIdTextureStatus::Valid);

      FHitResult Hit;
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 0, 0),
          FVector_NetQuantize(0, -1, 0),
          FVector_NetQuantize(-1, 0, 0)};
      std::array<int64, 3> expected{3, 1, 2};

      for (size_t i = 0; i < locations.size(); i++) {
        Hit.Location = locations[i];
        int64 featureID =
            UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
                featureIDTexture,
                Hit);
        TestEqual("FeatureIDFromHit", featureID, expected[i]);
      }
    });
  });
}

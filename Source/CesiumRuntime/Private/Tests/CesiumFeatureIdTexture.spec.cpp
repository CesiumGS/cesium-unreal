#include "CesiumFeatureIdTexture.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumFeatureIdTextureSpec,
    "Cesium.FeatureIdTexture",
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

         CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
         gltfTexture.source = 0;

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

         CesiumGltf::Texture& gltfTexture = model.textures.emplace_back();
         gltfTexture.source = 0;

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

  Describe("GetFeatureIDForTextureCoordinates", [this]() {
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
          UCesiumFeatureIdTextureBlueprintLibrary::
              GetFeatureIDForTextureCoordinates(featureIDTexture, 0, 0),
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
        int64 featureID = UCesiumFeatureIdTextureBlueprintLibrary::
            GetFeatureIDForTextureCoordinates(
                featureIDTexture,
                texCoord[0],
                texCoord[1]);
        TestEqual("FeatureID", featureID, featureIDs[i]);
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
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
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
          AccessorSpec::Type::SCALAR,
          AccessorSpec::ComponentType::UNSIGNED_BYTE,
          std::move(values));

      const std::vector<glm::vec2> texCoord1{
          glm::vec2(0.5, 0.5),
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0.0, 0.5)};

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
      pPrimitiveComponent = NewObject<UCesiumGltfPrimitiveComponent>();

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

      pPrimitiveComponent->PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1));

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords{
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
          texCoords);

      pPrimitiveComponent->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));
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
          "FeatureIDForVertex",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
              featureIDTexture,
              Hit),
          -1);
    });

    // It("returns -1 if hit has no valid component", [this]() {
    //  FHitResult Hit;
    //  Hit.Location = FVector_NetQuantize(0, -1, 0);
    //  Hit.FaceIndex = 0;
    //  Hit.Component = nullptr;

    //  FVector2D UV;
    //  TestFalse(
    //      "found hit",
    //      UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
    //});

    // It("returns -1 if specified texcoord set does not exist", [this]() {
    //  FHitResult Hit;
    //  Hit.Location = FVector_NetQuantize(0, -1, 0);
    //  Hit.FaceIndex = 0;
    //  Hit.Component = pPrimitiveComponent;

    //  FVector2D UV;
    //  TestFalse(
    //      "found hit",
    //      UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 1, UV));
    //});

    // It("returns correct value for valid texture", [this]() {
    //  const std::vector<uint8_t> featureIDs{0, 3, 1, 2};

    //  FeatureId& featureId = AddFeatureIDsAsTextureToModel(
    //      model,
    //      *pPrimitive,
    //      featureIDs,
    //      4,
    //      2,
    //      2,
    //      texCoords,
    //      0);

    //  FCesiumFeatureIdTexture featureIDTexture(
    //      model,
    //      *pPrimitive,
    //      *featureId.texture,
    //      "PropertyTableName");

    //  TestEqual(
    //      "FeatureIDTextureStatus",
    //      UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
    //          featureIDTexture),
    //      ECesiumFeatureIdTextureStatus::Valid);

    //  for (size_t i = 0; i < featureIDs.size(); i++) {
    //    int64 featureID =
    //        UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
    //            featureIDTexture,
    //            static_cast<int64>(i));
    //    TestEqual("FeatureIDForVertex", featureID, featureIDs[i]);
    //  }
    //});

    // It("returns correct value for primitive with multiple texcoords",
    // [this]() {
    //  const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
    //  const std::vector<glm::vec2> texCoord0{
    //      glm::vec2(0, 0),
    //      glm::vec2(0.5, 0),
    //      glm::vec2(0, 0.5),
    //      glm::vec2(0.5, 0.5)};

    //  std::vector<std::byte> values(texCoord0.size());
    //  std::memcpy(values.data(), texCoord0.data(), values.size());

    //  CreateAttributeForPrimitive(
    //      model,
    //      *pPrimitive,
    //      "TEXCOORD_0",
    //      AccessorSpec::Type::VEC2,
    //      AccessorSpec::ComponentType::FLOAT,
    //      std::move(values));

    //  const std::vector<glm::vec2> texCoord1{
    //      glm::vec2(0.5, 0.5),
    //      glm::vec2(0, 0),
    //      glm::vec2(0.5, 0),
    //      glm::vec2(0.0, 0.5)};

    //  FeatureId& featureId = AddFeatureIDsAsTextureToModel(
    //      model,
    //      *pPrimitive,
    //      featureIDs,
    //      4,
    //      2,
    //      2,
    //      texCoord1,
    //      1);

    //  FCesiumFeatureIdTexture featureIDTexture(
    //      model,
    //      *pPrimitive,
    //      *featureId.texture,
    //      "PropertyTableName");

    //  TestEqual(
    //      "FeatureIDTextureStatus",
    //      UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
    //          featureIDTexture),
    //      ECesiumFeatureIdTextureStatus::Valid);

    //  const std::vector<uint8_t> expected{2, 0, 3, 1};
    //  for (size_t i = 0; i < featureIDs.size(); i++) {
    //    int64 featureID =
    //        UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
    //            featureIDTexture,
    //            static_cast<int64>(i));
    //    TestEqual("FeatureIDForVertex", featureID, expected[i]);
    //  }
    //});

    // It("gets hit for primitive without indices", [this]() {
    //  FHitResult Hit;
    //  Hit.Location = FVector_NetQuantize(0, -1, 0);
    //  Hit.FaceIndex = 0;
    //  Hit.Component = pPrimitiveComponent;

    //  FVector2D UV = FVector2D::Zero();

    //  TestTrue(
    //      "found hit",
    //      UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
    //  TestEqual("UV at point", UV, FVector2D(0, 1));

    //  Hit.Location = FVector_NetQuantize(0, -0.5, 0);
    //  TestTrue(
    //      "found hit",
    //      UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
    //  TestTrue(
    //      "UV at point inside triangle (X)",
    //      FMath::IsNearlyEqual(UV[0], 0.0));
    //  TestTrue(
    //      "UV at point inside triangle (Y)",
    //      FMath::IsNearlyEqual(UV[1], 1.0 / 3.0));

    //  Hit.FaceIndex = 1;
    //  Hit.Location = FVector_NetQuantize(0, -4, 0);
    //  TestTrue(
    //      "found hit",
    //      UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
    //  TestEqual("UV at point", UV, FVector2D(0, 1));
    //});
  });
}

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdSet.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumFeatureIdSetSpec,
    "Cesium.Unit.FeatureIdSet",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
TObjectPtr<UCesiumGltfPrimitiveComponent> pPrimitiveComponent;
END_DEFINE_SPEC(FCesiumFeatureIdSetSpec)

void FCesiumFeatureIdSetSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("constructs from empty feature ID set", [this]() {
      // This is technically disallowed by the spec, but just make sure it's
      // handled reasonably.
      FeatureId featureId;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::None);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          0);
    });

    It("constructs implicit feature ID set", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::Implicit);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureId.featureCount));
    });

    It("constructs set with feature ID attribute", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureID = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::Attribute);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureID.featureCount));
    });

    It("constructs set with feature ID texture", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0, 0.5),
          glm::vec2(0.5, 0.5)};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::Texture);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureId.featureCount));
    });

    It("constructs with null feature ID", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;
      featureId.nullFeatureId = 0;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::Implicit);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureId.featureCount));
      TestEqual(
          "NullFeatureID",
          UCesiumFeatureIdSetBlueprintLibrary::GetNullFeatureID(featureIDSet),
          static_cast<int64>(*featureId.nullFeatureId));
    });

    It("constructs with property table index", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;
      featureId.propertyTable = 1;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
              featureIDSet),
          ECesiumFeatureIdSetType::Implicit);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureId.featureCount));
      TestEqual(
          "PropertyTableIndex",
          UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
              featureIDSet),
          static_cast<int64>(*featureId.propertyTable));
    });
  });

  Describe("GetAsFeatureIDAttribute", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns empty instance for non-attribute feature ID set", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      const FCesiumFeatureIdAttribute attribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);
      TestEqual(
          "AttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute);
      TestEqual("AttributeIndex", attribute.getAttributeIndex(), -1);
    });

    It("returns valid instance for attribute feature ID set", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureID = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
      const FCesiumFeatureIdAttribute attribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);
      TestEqual(
          "AttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::Valid);
      TestEqual(
          "AttributeIndex",
          attribute.getAttributeIndex(),
          attributeIndex);
    });
  });

  Describe("GetAsFeatureIDTexture", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns empty instance for non-texture feature ID set", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      const FCesiumFeatureIdTexture texture =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
              featureIDSet);
      TestEqual(
          "TextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              texture),
          ECesiumFeatureIdTextureStatus::ErrorInvalidTexture);

      auto featureIDTextureView = texture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::ErrorUninitialized);
    });

    It("returns valid instance for texture feature ID set", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0, 0.5),
          glm::vec2(0.5, 0.5)};

      FeatureId& featureId = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      const FCesiumFeatureIdTexture texture =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
              featureIDSet);
      TestEqual(
          "TextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              texture),
          ECesiumFeatureIdTextureStatus::Valid);

      auto featureIDTextureView = texture.getFeatureIdTextureView();
      TestEqual(
          "FeatureIDTextureViewStatus",
          featureIDTextureView.status(),
          FeatureIdTextureViewStatus::Valid);
    });
  });

  Describe("GetFeatureIDForVertex", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns -1 for empty feature ID set", [this]() {
      FCesiumFeatureIdSet featureIDSet;
      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
              featureIDSet,
              0),
          -1);
    });

    It("returns -1 for out of bounds index", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
              featureIDSet,
              -1),
          -1);
      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
              featureIDSet,
              11),
          -1);
    });

    It("returns correct value for implicit set", [this]() {
      FeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      for (int64 i = 0; i < featureId.featureCount; i++) {
        TestEqual(
            "FeatureIDForVertex",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
                featureIDSet,
                i),
            i);
      }
    });

    It("returns correct value for attribute set", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureID = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
      for (size_t i = 0; i < featureIDs.size(); i++) {
        TestEqual(
            "FeatureIDForVertex",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
                featureIDSet,
                static_cast<int64>(i)),
            featureIDs[i]);
      }
    });

    It("returns correct value for texture set", [this]() {
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(0, 0.5),
          glm::vec2(0.5, 0.5)};

      FeatureId& featureID = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
      for (size_t i = 0; i < featureIDs.size(); i++) {
        TestEqual(
            "FeatureIDForVertex",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
                featureIDSet,
                static_cast<int64>(i)),
            featureIDs[i]);
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
      pPrimitiveComponent->getPrimitiveData().pMeshPrimitive = pPrimitive;

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

    It("returns -1 for empty feature ID set", [this]() {
      FCesiumFeatureIdSet featureIDSet;
      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
              featureIDSet,
              0),
          -1);
    });

    It("returns -1 for invalid hit component", [this]() {
      FeatureId featureId;
      featureId.featureCount = 6;
      CesiumPrimitiveData& primData = pPrimitiveComponent->getPrimitiveData();
      primData.PositionAccessor = CesiumGltf::AccessorView<FVector3f>(
          model,
          static_cast<int32_t>(model.accessors.size() - 1));

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);

      FHitResult Hit;
      Hit.Component = nullptr;
      Hit.FaceIndex = 0;

      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
              featureIDSet,
              Hit),
          -1);
    });

    It("returns correct value for texture set", [this]() {
      int32 positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

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
      const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
      FeatureId& featureID = AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          2,
          2,
          texCoords,
          0);

      CesiumPrimitiveData& primData = pPrimitiveComponent->getPrimitiveData();
      primData.PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);
      primData.TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;
      Hit.FaceIndex = 0;

      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 0, 0),
          FVector_NetQuantize(0, -1, 0),
          FVector_NetQuantize(0.0, -0.25, 0)};
      std::array<int64, 3> expected{3, 1, 0};

      for (size_t i = 0; i < locations.size(); i++) {
        Hit.Location = locations[i];
        TestEqual(
            "FeatureIDFromHit",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
                featureIDSet,
                Hit),
            expected[i]);
      }
    });

    It("returns correct value for implicit set", [this]() {
      FeatureId featureId;
      featureId.featureCount = 6;

      CesiumPrimitiveData& primData = pPrimitiveComponent->getPrimitiveData();
      primData.PositionAccessor = CesiumGltf::AccessorView<FVector3f>(
          model,
          static_cast<int32_t>(model.accessors.size() - 1));

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;
      Hit.FaceIndex = 0;

      std::array<int32, 3> faceIndices{0, 1, 0};
      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 0, 0),
          FVector_NetQuantize(0, -4, 0),
          FVector_NetQuantize(-1, 0, 0)};
      std::array<int64, 3> expected{0, 3, 0};
      for (size_t i = 0; i < locations.size(); i++) {
        Hit.FaceIndex = faceIndices[i];
        Hit.Location = locations[i];
        TestEqual(
            "FeatureIDFromHit",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
                featureIDSet,
                Hit),
            expected[i]);
      }
    });

    It("returns correct value for attribute set", [this]() {
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          2,
          attributeIndex);

      CesiumPrimitiveData& primData = pPrimitiveComponent->getPrimitiveData();
      primData.PositionAccessor =
          CesiumGltf::AccessorView<FVector3f>(model, positionAccessorIndex);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;
      Hit.FaceIndex = 0;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
              featureIDSet,
              Hit),
          0);

      Hit.FaceIndex = 1;
      Hit.Location = FVector_NetQuantize(0, -4, 0);
      TestEqual(
          "FeatureIDFromHit",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
              featureIDSet,
              Hit),
          1);
    });
  });

  Describe("Deprecated", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("backwards compatibility for FCesiumFeatureIdAttribute.GetFeatureTableName",
       [this]() {
         const int64 attributeIndex = 0;
         const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
         FeatureId& featureID = AddFeatureIDsAsAttributeToModel(
             model,
             *pPrimitive,
             featureIDs,
             4,
             attributeIndex);
         featureID.propertyTable = 0;

         const std::string expectedName = "PropertyTableName";

         ExtensionModelExtStructuralMetadata& metadataExtension =
             model.addExtension<ExtensionModelExtStructuralMetadata>();
         PropertyTable& propertyTable =
             metadataExtension.propertyTables.emplace_back();
         propertyTable.name = expectedName;

         FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
         const FCesiumFeatureIdAttribute attribute =
             UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
                 featureIDSet);
         TestEqual(
             "AttributeStatus",
             UCesiumFeatureIdAttributeBlueprintLibrary::
                 GetFeatureIDAttributeStatus(attribute),
             ECesiumFeatureIdAttributeStatus::Valid);
         TestEqual(
             "GetFeatureTableName",
             UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
                 attribute),
             FString(expectedName.c_str()));
       });

    It("backwards compatibility for FCesiumFeatureIdTexture.GetFeatureTableName",
       [this]() {
         const std::vector<uint8_t> featureIDs{0, 3, 1, 2};
         const std::vector<glm::vec2> texCoords{
             glm::vec2(0, 0),
             glm::vec2(0.5, 0),
             glm::vec2(0, 0.5),
             glm::vec2(0.5, 0.5)};

         FeatureId& featureID = AddFeatureIDsAsTextureToModel(
             model,
             *pPrimitive,
             featureIDs,
             4,
             2,
             2,
             texCoords,
             0);
         featureID.propertyTable = 0;

         const std::string expectedName = "PropertyTableName";

         ExtensionModelExtStructuralMetadata& metadataExtension =
             model.addExtension<ExtensionModelExtStructuralMetadata>();
         PropertyTable& propertyTable =
             metadataExtension.propertyTables.emplace_back();
         propertyTable.name = expectedName;

         FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
         const FCesiumFeatureIdTexture texture =
             UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
                 featureIDSet);
         TestEqual(
             "TextureStatus",
             UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
                 texture),
             ECesiumFeatureIdTextureStatus::Valid);
         TestEqual(
             "GetFeatureTableName",
             UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(
                 texture),
             FString(expectedName.c_str()));
       });
  });
}

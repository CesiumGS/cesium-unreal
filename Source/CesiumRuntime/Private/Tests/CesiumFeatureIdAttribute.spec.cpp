// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdAttribute.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesiumFeatureIdAttributeSpec,
    "Cesium.Unit.FeatureIdAttribute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
CesiumGltf::Model model;
CesiumGltf::MeshPrimitive* pPrimitive;
END_DEFINE_SPEC(FCesiumFeatureIdAttributeSpec)

void FCesiumFeatureIdAttributeSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = CesiumGltf::Model();
      CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("constructs invalid instance for empty attribute", [this]() {
      FCesiumFeatureIdAttribute featureIDAttribute;

      TestEqual("AttributeIndex", featureIDAttribute.getAttributeIndex(), -1);
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute);
    });

    It("constructs invalid instance for nonexistent attribute", [this]() {
      const int64 attributeIndex = 0;
      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "AttributeIndex",
          featureIDAttribute.getAttributeIndex(),
          attributeIndex);
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute);
    });

    It("constructs invalid instance for attribute with nonexistent accessor",
       [this]() {
         const int64 attributeIndex = 0;
         pPrimitive->attributes.insert({"_FEATURE_ID_0", 0});

         FCesiumFeatureIdAttribute featureIDAttribute(
             model,
             *pPrimitive,
             attributeIndex,
             "PropertyTableName");
         TestEqual(
             "AttributeIndex",
             featureIDAttribute.getAttributeIndex(),
             attributeIndex);
         TestEqual(
             "FeatureIDAttributeStatus",
             UCesiumFeatureIdAttributeBlueprintLibrary::
                 GetFeatureIDAttributeStatus(featureIDAttribute),
             ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor);
       });

    It("constructs invalid instance for attribute with invalid accessor",
       [this]() {
         CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
         accessor.type = CesiumGltf::AccessorSpec::Type::VEC2;
         accessor.componentType =
             CesiumGltf::AccessorSpec::ComponentType::FLOAT;
         const int64 attributeIndex = 0;
         pPrimitive->attributes.insert({"_FEATURE_ID_0", 0});

         FCesiumFeatureIdAttribute featureIDAttribute(
             model,
             *pPrimitive,
             attributeIndex,
             "PropertyTableName");
         TestEqual(
             "AttributeIndex",
             featureIDAttribute.getAttributeIndex(),
             attributeIndex);
         TestEqual(
             "FeatureIDAttributeStatus",
             UCesiumFeatureIdAttributeBlueprintLibrary::
                 GetFeatureIDAttributeStatus(featureIDAttribute),
             ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor);
       });

    It("constructs valid instance", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 3, 3, 3, 1, 1, 1, 2, 2, 2};
      AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "AttributeIndex",
          featureIDAttribute.getAttributeIndex(),
          attributeIndex);
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::Valid);
    });
  });

  Describe("GetFeatureIDCount", [this]() {
    BeforeEach([this]() {
      model = CesiumGltf::Model();
      CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns 0 for invalid attribute", [this]() {
      const int64 attributeIndex = 0;
      pPrimitive->attributes.insert({"_FEATURE_ID_0", 0});

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor);
      TestEqual(
          "VertexCount",
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDCount(
              featureIDAttribute),
          0);
    });

    It("returns correct value for valid attribute", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 3, 3, 3, 1, 1, 1, 2, 2, 2};
      const int64 vertexCount = static_cast<int64>(featureIDs.size());
      AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::Valid);
      TestEqual(
          "VertexCount",
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDCount(
              featureIDAttribute),
          vertexCount);
    });
  });

  Describe("GetFeatureID", [this]() {
    BeforeEach([this]() {
      model = CesiumGltf::Model();
      CesiumGltf::Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns -1 for invalid attribute", [this]() {
      const int64 attribute = 0;
      pPrimitive->attributes.insert({"_FEATURE_ID_0", 0});

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attribute,
          "PropertyTableName");
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor);
      TestEqual(
          "FeatureIDForVertex",
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
              featureIDAttribute,
              0),
          -1);
    });

    It("returns -1 for out-of-bounds index", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          2,
          attributeIndex);

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::Valid);
      TestEqual(
          "FeatureIDForNegativeVertex",
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
              featureIDAttribute,
              -1),
          -1);
      TestEqual(
          "FeatureIDForOutOfBoundsVertex",
          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
              featureIDAttribute,
              10),
          -1);
    });

    It("returns correct value for valid attribute", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 3, 3, 3, 1, 1, 1, 2, 2, 2};
      AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs,
          4,
          attributeIndex);

      FCesiumFeatureIdAttribute featureIDAttribute(
          model,
          *pPrimitive,
          attributeIndex,
          "PropertyTableName");
      TestEqual(
          "FeatureIDAttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(featureIDAttribute),
          ECesiumFeatureIdAttributeStatus::Valid);
      for (size_t i = 0; i < featureIDs.size(); i++) {
        TestEqual(
            "FeatureIDForVertex",
            UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureID(
                featureIDAttribute,
                static_cast<int64>(i)),
            featureIDs[i]);
      }
    });
  });
}

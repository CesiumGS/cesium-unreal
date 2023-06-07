#include "CesiumFeatureIdSet.h"
#include "CesiumFeatureIdSpecUtility.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumFeatureIdSetSpec,
    "Cesium.FeatureIdSet",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;

END_DEFINE_SPEC(FCesiumFeatureIdSetSpec)

void FCesiumFeatureIdSetSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      ExtensionExtMeshFeatures& extension =
          pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("constructs from feature ID with no features", [this]() {
      // This is technically disallowed by the spec, but just make sure it's
      // handled reasonably.
      ExtensionExtMeshFeaturesFeatureId featureId;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::None);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          0);
    });

    It("constructs from implicit feature ID", [this]() {
      ExtensionExtMeshFeaturesFeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Implicit);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          featureId.featureCount);
    });

    It("constructs from feature ID attribute", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      ExtensionExtMeshFeaturesFeatureId& featureID =
          AddFeatureIDsAsAttributeToModel(
              model,
              *pPrimitive,
              featureIDs,
              4,
              attributeIndex);

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureID);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Attribute);
      TestEqual(
          "FeatureCount",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          featureID.featureCount);
    });

    // It("constructs from feature ID texture", [this]() {

    //  FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, *pFeatureId);
    //  TestEqual(
    //      "FeatureIDType",
    //      UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
    //      ECesiumFeatureIdType::Attribute);
    //  TestEqual(
    //      "FeatureCount",
    //      UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
    //      pFeatureId->featureCount);
    //});
  });

  Describe("GetAsFeatureIDAttribute", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
    });

    It("returns empty instance for non-attribute feature ID set", [this]() {
      ExtensionExtMeshFeaturesFeatureId featureId;
      featureId.featureCount = 10;

      FCesiumFeatureIdSet featureIDSet(model, *pPrimitive, featureId);
      const FCesiumFeatureIdAttribute attribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);
      TestEqual(
          "Attribute Status",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute);
      TestEqual("Attribute Index", attribute.getAttributeIndex(), -1);
    });

    It("returns valid instance for attribute feature ID set", [this]() {
      const int64 attributeIndex = 0;
      const std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      ExtensionExtMeshFeaturesFeatureId& featureID =
          AddFeatureIDsAsAttributeToModel(
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
          "Attribute Status",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::Valid);
      TestEqual(
          "Attribute Index",
          attribute.getAttributeIndex(),
          attributeIndex);
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
         ExtensionExtMeshFeaturesFeatureId& featureID =
             AddFeatureIDsAsAttributeToModel(
                 model,
                 *pPrimitive,
                 featureIDs,
                 4,
                 attributeIndex);
         featureID.propertyTable = 0;

         const std::string expectedName = "PropertyTableName";

         ExtensionModelExtStructuralMetadata& metadataExtension =
             model.addExtension<ExtensionModelExtStructuralMetadata>();
         ExtensionExtStructuralMetadataPropertyTable& propertyTable =
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
  });
}

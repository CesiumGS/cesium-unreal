#include "CesiumPrimitiveFeatures.h"
#include "CesiumFeatureIdSpecUtility.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPrimitiveFeaturesSpec,
    "Cesium.PrimitiveFeatures",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
ExtensionExtMeshFeatures* pExtension;
END_DEFINE_SPEC(FCesiumPrimitiveFeaturesSpec)

void FCesiumPrimitiveFeaturesSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("constructs with no feature ID sets", [this]() {
      // This is technically disallowed by the spec, but just make sure it's
      // handled reasonably.
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 0);
    });

    It("constructs with single feature ID set", [this]() {
      ExtensionExtMeshFeaturesFeatureId& featureID =
          pExtension->featureIds.emplace_back();
      featureID.featureCount = 10;

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet>& featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "Feature Count",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          featureID.featureCount);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Implicit);
    });

    It("handles multiple feature ID sets", [this]() {
      const std::vector<uint8_t> attributeIDs{0, 0, 0};
      AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 1, 0);

      const std::vector<uint8_t> textureIDs{1, 2, 3};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(1, 0)};
      AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          textureIDs,
          3,
          3,
          1,
          texCoords);

      ExtensionExtMeshFeaturesFeatureId implicitIDs =
          pExtension->featureIds.emplace_back();
      implicitIDs.featureCount = 3;

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet>& featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 3);

      const std::vector<ECesiumFeatureIdType> expectedTypes{
          ECesiumFeatureIdType::Attribute,
          ECesiumFeatureIdType::Texture,
          ECesiumFeatureIdType::Implicit};

      for (size_t i = 0; i < featureIDSets.Num(); i++) {
        const FCesiumFeatureIdSet& featureIDSet =
            featureIDSets[static_cast<int32>(i)];
        const ExtensionExtMeshFeaturesFeatureId& gltfFeatureID =
            pExtension->featureIds[i];
        TestEqual(
            "Feature Count",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
            gltfFeatureID.featureCount);
        TestEqual(
            "FeatureIDType",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
            expectedTypes[i]);
      }
    });
  });

  Describe("GetFeatureIDSetsOfType", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();

      const std::vector<uint8_t> attributeIDs{0, 0, 0};
      AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 1, 0);

      const std::vector<uint8_t> textureIDs{1, 2, 3};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(1, 0)};
      AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          textureIDs,
          3,
          3,
          1,
          texCoords);

      ExtensionExtMeshFeaturesFeatureId& implicitIDs =
          pExtension->featureIds.emplace_back();
      implicitIDs.featureCount = 3;
    });

    It("gets feature ID attribute", [this]() {
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
              primitiveFeatures,
              ECesiumFeatureIdType::Attribute);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Attribute);

      const FCesiumFeatureIdAttribute& attribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::Valid);
    });

    It("gets implicit feature ID", [this]() {
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
              primitiveFeatures,
              ECesiumFeatureIdType::Implicit);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Implicit);
    });
  });

}

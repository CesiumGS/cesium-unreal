#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "CesiumMetadataPickingBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumMetadataPickingSpec,
    "Cesium.MetadataPicking",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
ExtensionExtMeshFeatures* pMeshFeatures;
ExtensionModelExtStructuralMetadata* pStructuralMetadata;
PropertyTable* pPropertyTable;
TObjectPtr<UCesiumGltfComponent> pModelComponent;
TObjectPtr<UCesiumGltfPrimitiveComponent> pPrimitiveComponent;
END_DEFINE_SPEC(FCesiumMetadataPickingSpec)

void FCesiumMetadataPickingSpec::Define() {
  BeforeEach([this]() {
    model = Model();

    Mesh& mesh = model.meshes.emplace_back();
    pPrimitive = &mesh.primitives.emplace_back();

    // Two disconnected triangles.
    std::vector<glm::vec3> positions{
        glm::vec3(-1, 1, 0),
        glm::vec3(1, 1, 0),
        glm::vec3(1, -1, 0),
        glm::vec3(2, 2, 0),
        glm::vec3(-2, 2, 0),
        glm::vec3(-2, -2, 0),
    };
    std::vector<std::byte> positionData(positions.size() * sizeof(glm::vec3));
    std::memcpy(positionData.data(), positions.data(), positionData.size());
    CreateAttributeForPrimitive(
        model,
        *pPrimitive,
        "POSITION",
        AccessorSpec::Type::VEC3,
        AccessorSpec::ComponentType::FLOAT,
        std::move(positionData));

    pMeshFeatures = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    pStructuralMetadata =
        &model.addExtension<ExtensionModelExtStructuralMetadata>();

    std::string className = "testClass";
    pStructuralMetadata->schema.emplace();
    pStructuralMetadata->schema->classes[className];

    pPropertyTable = &pStructuralMetadata->propertyTables.emplace_back();
    pPropertyTable->classProperty = className;

    pModelComponent = NewObject<UCesiumGltfComponent>();
    pPrimitiveComponent =
        NewObject<UCesiumGltfPrimitiveComponent>(pModelComponent);
    pPrimitiveComponent->AttachToComponent(
        pModelComponent,
        FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
  });

  Describe("GetMetadataValuesForFace", [this]() {
    It("returns empty map for invalid face index", [this]() {
      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pStructuralMetadata->propertyTables.size() - 1);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      pModelComponent->Metadata =
          FCesiumModelMetadata(model, *pStructuralMetadata);
      pPrimitiveComponent->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      auto values =
          UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
              pPrimitiveComponent,
              -1);
      TestTrue("empty values for negative index", values.IsEmpty());

      values = UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
          pPrimitiveComponent,
          2);
      TestTrue(
          "empty values for positive out-of-range index",
          values.IsEmpty());
    });

    It("returns empty map for invalid feature ID set index", [this]() {
      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pStructuralMetadata->propertyTables.size() - 1);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      pModelComponent->Metadata =
          FCesiumModelMetadata(model, *pStructuralMetadata);
      pPrimitiveComponent->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      auto values =
          UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
              pPrimitiveComponent,
              0,
              -1);
      TestTrue("empty values for negative index", values.IsEmpty());

      values = UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
          pPrimitiveComponent,
          0,
          1);
      TestTrue(
          "empty values for positive out-of-range index",
          values.IsEmpty());
    });

    It("returns empty values if feature ID set is not associated with a property table",
       [this]() {
         std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
         AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);

         std::vector<int32_t> scalarValues{1, 2};
         pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
         const std::string scalarPropertyName("scalarProperty");
         AddPropertyTablePropertyToModel(
             model,
             *pPropertyTable,
             scalarPropertyName,
             ClassProperty::Type::SCALAR,
             ClassProperty::ComponentType::INT32,
             scalarValues);

         std::vector<glm::vec2> vec2Values{
             glm::vec2(1.0f, 2.5f),
             glm::vec2(3.1f, -4.0f)};
         const std::string vec2PropertyName("vec2Property");
         AddPropertyTablePropertyToModel(
             model,
             *pPropertyTable,
             vec2PropertyName,
             ClassProperty::Type::VEC2,
             ClassProperty::ComponentType::FLOAT32,
             vec2Values);

         pModelComponent->Metadata =
             FCesiumModelMetadata(model, *pStructuralMetadata);
         pPrimitiveComponent->Features =
             FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

         const auto values =
             UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                 pPrimitiveComponent,
                 0);
         TestTrue("values are empty", values.IsEmpty());
       });

    It("returns values for first feature ID set by default", [this]() {
      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pStructuralMetadata->propertyTables.size() - 1);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(3.1f, -4.0f)};
      const std::string vec2PropertyName("vec2Property");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      pModelComponent->Metadata =
          FCesiumModelMetadata(model, *pStructuralMetadata);
      pPrimitiveComponent->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                static_cast<int64_t>(i));
        TestEqual("number of values", values.Num(), 2);
        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            values.Contains(FString(vec2PropertyName.c_str())));

        const FCesiumMetadataValue& scalarValue =
            *values.Find(FString(scalarPropertyName.c_str()));
        TestEqual(
            "scalar value",
            UCesiumMetadataValueBlueprintLibrary::GetInteger(scalarValue, 0),
            scalarValues[i]);

        const FCesiumMetadataValue& vec2Value =
            *values.Find(FString(vec2PropertyName.c_str()));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        TestEqual(
            "vec2 value",
            UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                vec2Value,
                FVector2D::Zero()),
            expected);
      }
    });

    It("returns values for specified feature ID set", [this]() {
      std::vector<uint8_t> featureIDs0{1, 1, 1, 0, 0, 0};
      FeatureId& featureId0 = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs0,
          2,
          0);
      std::vector<uint8_t> featureIDs1{0, 0, 0, 1, 1, 1};
      FeatureId& featureId1 = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs1,
          2,
          1);
      featureId0.propertyTable = featureId1.propertyTable =
          static_cast<int64_t>(pStructuralMetadata->propertyTables.size() - 1);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(3.1f, -4.0f)};
      const std::string vec2PropertyName("vec2Property");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      pModelComponent->Metadata =
          FCesiumModelMetadata(model, *pStructuralMetadata);
      pPrimitiveComponent->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                i,
                1);
        TestEqual("number of values", values.Num(), 2);
        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            values.Contains(FString(vec2PropertyName.c_str())));

        const FCesiumMetadataValue& scalarValue =
            *values.Find(FString(scalarPropertyName.c_str()));
        TestEqual(
            "scalar value",
            UCesiumMetadataValueBlueprintLibrary::GetInteger(scalarValue, 0),
            scalarValues[i]);

        const FCesiumMetadataValue& vec2Value =
            *values.Find(FString(vec2PropertyName.c_str()));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        TestEqual(
            "vec2 value",
            UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                vec2Value,
                FVector2D::Zero()),
            expected);
      }
    });
  });

  Describe("GetMetadataValuesForFaceAsStrings", [this]() {
    It("returns values for first feature ID set by default", [this]() {
      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pStructuralMetadata->propertyTables.size() - 1);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(3.1f, -4.0f)};
      const std::string vec2PropertyName("vec2Property");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      pModelComponent->Metadata =
          FCesiumModelMetadata(model, *pStructuralMetadata);
      pPrimitiveComponent->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto strings = UCesiumMetadataPickingBlueprintLibrary::
            GetMetadataValuesForFaceAsStrings(
                pPrimitiveComponent,
                static_cast<int64_t>(i));
        TestEqual("number of strings", strings.Num(), 2);
        TestTrue(
            "contains scalar value",
            strings.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            strings.Contains(FString(vec2PropertyName.c_str())));

        const FString& scalarString =
            *strings.Find(FString(scalarPropertyName.c_str()));
        TestEqual(
            "scalar value",
            scalarString,
            FString(std::to_string(scalarValues[i]).c_str()));

        const FString& vec2Value =
            *strings.Find(FString(vec2PropertyName.c_str()));
        std::string expected(
            "X=" + std::to_string(vec2Values[i][0]) +
            " Y=" + std::to_string(vec2Values[i][1]));
        TestEqual("vec2 value", vec2Value, FString(expected.c_str()));
      }
    });
  });
}

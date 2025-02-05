// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTable.h"
#include "CesiumGltf/Enum.h"
#include "CesiumGltf/EnumValue.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace {

CesiumGltf::EnumValue makeEnumValue(const std::string& name, int64_t value) {
  CesiumGltf::EnumValue enumValue;
  enumValue.name = name;
  enumValue.value = value;
  return enumValue;
}

} // namespace

BEGIN_DEFINE_SPEC(
    FCesiumPropertyTableSpec,
    "Cesium.Unit.PropertyTable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
CesiumGltf::Model model;
CesiumGltf::ExtensionModelExtStructuralMetadata* pExtension;
CesiumGltf::PropertyTable* pPropertyTable;
END_DEFINE_SPEC(FCesiumPropertyTableSpec)

void FCesiumPropertyTableSpec::Define() {
  BeforeEach([this]() {
    model = CesiumGltf::Model();
    pExtension =
        &model.addExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
    pExtension->schema.emplace();
    pPropertyTable = &pExtension->propertyTables.emplace_back();
  });

  Describe("Constructor", [this]() {
    It("constructs invalid instance by default", [this]() {
      FCesiumPropertyTable propertyTable;
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(0));
    });

    It("constructs invalid instance for missing schema", [this]() {
      pExtension->schema.reset();

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(0));
    });

    It("constructs invalid instance for missing class", [this]() {
      pPropertyTable->classProperty = "nonexistent class";

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(0));
    });

    It("constructs valid instance with valid property", [this]() {
      pPropertyTable->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::vector<int32_t> values{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(values.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(values.size()));
    });

    It("constructs valid instance with invalid property", [this]() {
      // Even if one of its properties is invalid, the property table itself is
      // still valid.
      pPropertyTable->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::vector<int8_t> values{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(values.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32, // Incorrect
                                                           // component type
          values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(values.size()));
    });
  });

  Describe("GetProperties", [this]() {
    BeforeEach([this]() { pPropertyTable->classProperty = "testClass"; });

    It("returns no properties for invalid property table", [this]() {
      FCesiumPropertyTable propertyTable;
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      const auto properties =
          UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(scalarValues.size()));

      const auto properties =
          UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);

      TestTrue(
          "has scalar property",
          properties.Contains(FString(scalarPropertyName.c_str())));
      const FCesiumPropertyTableProperty& scalarProperty =
          *properties.Find(FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(scalarProperty),
          ECesiumPropertyTablePropertyStatus::Valid);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              scalarProperty),
          static_cast<int64>(scalarValues.size()));
      for (size_t i = 0; i < scalarValues.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        TestEqual(
            label.c_str(),
            UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger(
                scalarProperty,
                static_cast<int64>(i)),
            scalarValues[i]);
      }

      TestTrue(
          "has vec2 property",
          properties.Contains(FString(vec2PropertyName.c_str())));
      const FCesiumPropertyTableProperty& vec2Property =
          *properties.Find(FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(vec2Property),
          ECesiumPropertyTablePropertyStatus::Valid);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              vec2Property),
          static_cast<int64>(vec2Values.size()));
      for (size_t i = 0; i < vec2Values.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        TestEqual(
            label.c_str(),
            UCesiumPropertyTablePropertyBlueprintLibrary::GetVector2D(
                vec2Property,
                static_cast<int64>(i),
                FVector2D::Zero()),
            expected);
      }
    });

    It("gets invalid property", [this]() {
      // Even invalid properties should still be retrieved.
      std::vector<int8_t> values{0, 1, 2};
      pPropertyTable->count = static_cast<int64_t>(values.size());
      std::string propertyName("badProperty");

      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(values.size()));

      const auto properties =
          UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);

      TestTrue(
          "has invalid property",
          properties.Contains(FString(propertyName.c_str())));
      const FCesiumPropertyTableProperty& property =
          *properties.Find(FString(propertyName.c_str()));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(property),
          ECesiumPropertyTablePropertyStatus::ErrorInvalidPropertyData);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              property),
          0);
    });
  });

  Describe("GetPropertyNames", [this]() {
    BeforeEach([this]() { pPropertyTable->classProperty = "testClass"; });

    It("returns empty array for invalid property table", [this]() {
      FCesiumPropertyTable propertyTable;
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      const auto properties =
          UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets all property names", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      std::string invalidPropertyName("badProperty");
      std::vector<int8_t> invalidPropertyValues{0, 1, 2};
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          invalidPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32, // Incorrect
                                                           // component type
          invalidPropertyValues);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(scalarValues.size()));

      const auto propertyNames =
          UCesiumPropertyTableBlueprintLibrary::GetPropertyNames(propertyTable);
      TestEqual("number of names", propertyNames.Num(), 3);
      TestTrue(
          "has scalar property name",
          propertyNames.Contains(FString(scalarPropertyName.c_str())));
      TestTrue(
          "has vec2 property name",
          propertyNames.Contains(FString(vec2PropertyName.c_str())));
      TestTrue(
          "has invalid property name",
          propertyNames.Contains(FString(invalidPropertyName.c_str())));
    });
  });

  Describe("FindProperty", [this]() {
    BeforeEach([this]() { pPropertyTable->classProperty = "testClass"; });

    It("returns invalid instance for nonexistent property", [this]() {
      std::string propertyName("testProperty");
      std::vector<int32_t> values{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(values.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(values.size()));

      FCesiumPropertyTableProperty property =
          UCesiumPropertyTableBlueprintLibrary::FindProperty(
              propertyTable,
              FString("nonexistent property"));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(property),
          ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(0));
    });

    It("finds existing properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(scalarValues.size()));

      FCesiumPropertyTableProperty scalarProperty =
          UCesiumPropertyTableBlueprintLibrary::FindProperty(
              propertyTable,
              FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(scalarProperty),
          ECesiumPropertyTablePropertyStatus::Valid);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              scalarProperty),
          static_cast<int64>(scalarValues.size()));

      FCesiumPropertyTableProperty vec2Property =
          UCesiumPropertyTableBlueprintLibrary::FindProperty(
              propertyTable,
              FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(vec2Property),
          ECesiumPropertyTablePropertyStatus::Valid);
      TestEqual<int64>(
          "Size",
          UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
              vec2Property),
          static_cast<int64>(vec2Values.size()));
    });
  });

  Describe("GetMetadataValuesForFeature", [this]() {
    BeforeEach([this]() { pPropertyTable->classProperty = "testClass"; });

    It("returns empty map for invalid property table", [this]() {
      FCesiumPropertyTable propertyTable;

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(0));

      const auto values =
          UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
              propertyTable,
              0);
      TestTrue("values map is empty", values.IsEmpty());
    });

    It("returns empty map for out-of-bounds feature IDs", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(scalarValues.size()));

      auto values =
          UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
              propertyTable,
              -1);
      TestTrue("no values for for negative feature ID", values.IsEmpty());

      values =
          UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
              propertyTable,
              10);
      TestTrue(
          "no values for positive out-of-range feature ID",
          values.IsEmpty());
    });

    It("returns values of valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(scalarValues.size()));

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto values =
            UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
                propertyTable,
                static_cast<int64>(i));
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
        FVector2D expected(vec2Values[i][0], vec2Values[i][1]);
        TestEqual(
            "vec2 value",
            UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                vec2Value,
                FVector2D::Zero()),
            expected);
      }
    });

    It("does not return value for invalid property", [this]() {
      std::vector<int8_t> propertyValues{0, 1, 2};
      pPropertyTable->count = static_cast<int64_t>(propertyValues.size());
      std::string propertyName("badProperty");

      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          propertyValues);
      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(propertyValues.size()));

      const auto values =
          UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
              propertyTable,
              0);
      TestTrue("values map is empty", values.IsEmpty());
    });
  });

  Describe("GetMetadataValuesForFeatureAsStrings", [this]() {
    BeforeEach([this]() { pPropertyTable->classProperty = "testClass"; });

    It("returns empty map for invalid property table", [this]() {
      FCesiumPropertyTable propertyTable;

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(0));

      const auto values = UCesiumPropertyTableBlueprintLibrary::
          GetMetadataValuesForFeatureAsStrings(propertyTable, 0);
      TestTrue("values map is empty", values.IsEmpty());
    });

    It("returns empty map for out-of-bounds feature IDs", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64_t>(scalarValues.size()));

      auto values = UCesiumPropertyTableBlueprintLibrary::
          GetMetadataValuesForFeatureAsStrings(propertyTable, -1);
      TestTrue("no values for for negative feature ID", values.IsEmpty());

      values = UCesiumPropertyTableBlueprintLibrary::
          GetMetadataValuesForFeatureAsStrings(propertyTable, 10);
      TestTrue(
          "no values for positive out-of-range feature ID",
          values.IsEmpty());
    });

    It("returns values of valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int32_t> scalarValues{1, 2, 3, 4};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          scalarValues);

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(-0.7f, 4.9f),
          glm::vec2(8.0f, 2.0f),
          glm::vec2(11.0f, 0.0f),
      };
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          CesiumGltf::ClassProperty::Type::VEC2,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      std::string enumPropertyName("enumProperty");
      std::vector<int16_t> enumValues{0, 1, 2, 3};
      std::vector<std::string> enumNames{"Foo", "Bar", "Baz", "Qux"};
      CesiumGltf::PropertyTableProperty& enumTableProperty =
          AddPropertyTablePropertyToModel(
              model,
              *pPropertyTable,
              enumPropertyName,
              CesiumGltf::ClassProperty::Type::ENUM,
              std::nullopt,
              enumValues);

      CesiumGltf::Schema& schema =
          *model
               .getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>()
               ->schema;
      schema.classes[pPropertyTable->classProperty]
          .properties[enumPropertyName]
          .enumType = "TestEnum";

      CesiumGltf::Enum& enumDef = schema.enums["TestEnum"];
      enumDef.name = "Test";
      enumDef.description = "An example enum";
      enumDef.values = std::vector<CesiumGltf::EnumValue>{
          makeEnumValue("Foo", 0),
          makeEnumValue("Bar", 1),
          makeEnumValue("Baz", 2),
          makeEnumValue("Qux", 3)};
      enumDef.valueType = CesiumGltf::Enum::ValueType::INT16;

      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(scalarValues.size()));

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto values = UCesiumPropertyTableBlueprintLibrary::
            GetMetadataValuesForFeatureAsStrings(
                propertyTable,
                static_cast<int64>(i));
        TestEqual("number of values", values.Num(), 3);

        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            values.Contains(FString(vec2PropertyName.c_str())));
        TestTrue(
            "contains enum value",
            values.Contains(FString(enumPropertyName.c_str())));

        const FString& scalarValue =
            *values.Find(FString(scalarPropertyName.c_str()));
        FString expected(std::to_string(scalarValues[i]).c_str());
        TestEqual("scalar value as string", scalarValue, expected);

        const FString& vec2Value =
            *values.Find(FString(vec2PropertyName.c_str()));
        std::string expectedString(
            "X=" + std::to_string(vec2Values[i][0]) +
            " Y=" + std::to_string(vec2Values[i][1]));
        expected = FString(expectedString.c_str());
        TestEqual("vec2 value as string", vec2Value, expected);

        const FString& enumValue =
            *values.Find(FString(enumPropertyName.c_str()));
        expected = FString(enumNames[i].c_str());
        TestEqual("enum value as string", enumValue, expected);
      }
    });

    It("does not return value for invalid property", [this]() {
      std::vector<int8_t> propertyValues{0, 1, 2};
      pPropertyTable->count = static_cast<int64_t>(propertyValues.size());
      std::string propertyName("badProperty");

      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          propertyName,
          CesiumGltf::ClassProperty::Type::SCALAR,
          CesiumGltf::ClassProperty::ComponentType::INT32,
          propertyValues);
      FCesiumPropertyTable propertyTable(model, *pPropertyTable);

      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::Valid);
      TestEqual<int64>(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          static_cast<int64>(propertyValues.size()));

      const auto values = UCesiumPropertyTableBlueprintLibrary::
          GetMetadataValuesForFeatureAsStrings(propertyTable, 0);
      TestTrue("values map is empty", values.IsEmpty());
    });
  });
}

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyAttribute.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <limits>

BEGIN_DEFINE_SPEC(
    FCesiumPropertyAttributeSpec,
    "Cesium.Unit.PropertyAttribute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
CesiumGltf::Model model;
CesiumGltf::MeshPrimitive* pPrimitive;
CesiumGltf::ExtensionModelExtStructuralMetadata* pExtension;
CesiumGltf::PropertyAttribute* pPropertyAttribute;
TObjectPtr<UCesiumGltfComponent> pModelComponent;
TObjectPtr<UCesiumGltfPrimitiveComponent> pPrimitiveComponent;
END_DEFINE_SPEC(FCesiumPropertyAttributeSpec)

void FCesiumPropertyAttributeSpec::Define() {
  using namespace CesiumGltf;

  BeforeEach([this]() {
    model = Model();
    Mesh& mesh = model.meshes.emplace_back();
    pPrimitive = &mesh.primitives.emplace_back();
    pPrimitive->mode = MeshPrimitive::Mode::POINTS;
    pExtension = &model.addExtension<ExtensionModelExtStructuralMetadata>();
    pExtension->schema.emplace();
    pPropertyAttribute = &pExtension->propertyAttributes.emplace_back();
  });

  Describe("Constructor", [this]() {
    It("constructs invalid instance by default", [this]() {
      FCesiumPropertyAttribute propertyAttribute;
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttribute);
      TestTrue(
          "Properties",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .IsEmpty());
    });

    It("constructs invalid instance for missing schema", [this]() {
      pExtension->schema.reset();

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttributeClass);
      TestTrue(
          "Properties",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .IsEmpty());
    });

    It("constructs invalid instance for missing class", [this]() {
      pPropertyAttribute->classProperty = "nonexistent class";

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttributeClass);
      TestTrue(
          "Properties",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .IsEmpty());
    });

    It("constructs valid instance with valid property", [this]() {
      pPropertyAttribute->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::vector<int8_t> values{1, 2, 3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          values,
          "_TEST");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual<int64>(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          1);
    });

    It("constructs valid instance with invalid property", [this]() {
      // Even if one of its properties is invalid, the property Attribute itself
      // is still valid.
      pPropertyAttribute->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::vector<int8_t> values{1, 2, 3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32, // Incorrect component type
          values,
          "_TEST");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual<int64>(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          1);
    });
  });

  Describe("GetProperties", [this]() {
    BeforeEach([this]() { pPropertyAttribute->classProperty = "testClass"; });

    It("returns no properties for invalid property Attribute", [this]() {
      FCesiumPropertyAttribute propertyAttribute;
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttribute);
      const auto properties =
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int8_t> scalarValues{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          "_SCALAR");

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::u8vec2> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          "_VECTOR");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);

      const auto properties =
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute);

      TestTrue(
          "has scalar property",
          properties.Contains(FString(scalarPropertyName.c_str())));
      const FCesiumPropertyAttributeProperty& scalarProperty =
          *properties.Find(FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(scalarProperty),
          ECesiumPropertyAttributePropertyStatus::Valid);
      for (size_t i = 0; i < scalarValues.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        TestEqual(
            label.c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
                scalarProperty,
                int64(i)),
            scalarValues[i]);
      }

      TestTrue(
          "has vec2 property",
          properties.Contains(FString(vec2PropertyName.c_str())));
      const FCesiumPropertyAttributeProperty& vec2Property =
          *properties.Find(FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(vec2Property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      for (size_t i = 0; i < vec2Values.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        TestEqual(
            label.c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
                vec2Property,
                int64(i),
                FVector2D::Zero()),
            expected);
      }
    });

    It("gets invalid property", [this]() {
      // Even invalid properties should still be retrieved.
      std::vector<int8_t> values{0, 1, 2, 3};
      std::string propertyName("badProperty");

      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          values,
          "_TEST");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);

      const auto properties =
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute);

      TestTrue(
          "has invalid property",
          properties.Contains(FString(propertyName.c_str())));
      const FCesiumPropertyAttributeProperty& property =
          *properties.Find(FString(propertyName.c_str()));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidPropertyData);
    });
  });

  Describe("GetPropertyNames", [this]() {
    BeforeEach([this]() { pPropertyAttribute->classProperty = "testClass"; });

    It("returns empty array for invalid property Attribute", [this]() {
      FCesiumPropertyAttribute propertyAttribute;
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttribute);
      const auto properties =
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets all property names", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int8_t> scalarValues{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          "_SCALAR");

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::u8vec2> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          "_VECTOR");

      std::string invalidPropertyName("badProperty");
      std::vector<uint8_t> invalidPropertyValues{0, 1, 2, 3};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          invalidPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32, // Incorrect component type
          invalidPropertyValues,
          {0});

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);

      const auto propertyNames =
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyNames(
              propertyAttribute);
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
    BeforeEach([this]() { pPropertyAttribute->classProperty = "testClass"; });

    It("returns invalid instance for nonexistent property", [this]() {
      std::string propertyName("testProperty");
      std::vector<int8_t> values{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          values,
          "_TEST");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          1);

      FCesiumPropertyAttributeProperty property =
          UCesiumPropertyAttributeBlueprintLibrary::FindProperty(
              propertyAttribute,
              FString("nonexistent property"));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
    });

    It("finds existing properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int8_t> scalarValues{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          "_SCALAR");

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::u8vec2> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          "_VECTOR");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);
      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          2);

      FCesiumPropertyAttributeProperty scalarProperty =
          UCesiumPropertyAttributeBlueprintLibrary::FindProperty(
              propertyAttribute,
              FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(scalarProperty),
          ECesiumPropertyAttributePropertyStatus::Valid);

      FCesiumPropertyAttributeProperty vec2Property =
          UCesiumPropertyAttributeBlueprintLibrary::FindProperty(
              propertyAttribute,
              FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(vec2Property),
          ECesiumPropertyAttributePropertyStatus::Valid);
    });
  });

  Describe("GetMetadataValuesAtIndex", [this]() {
    BeforeEach([this]() { pPropertyAttribute->classProperty = "testClass"; });

    It("returns empty map for invalid property Attribute", [this]() {
      FCesiumPropertyAttribute propertyAttribute;

      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttribute);
      TestTrue(
          "Properties",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .IsEmpty());

      const auto values =
          UCesiumPropertyAttributeBlueprintLibrary::GetMetadataValuesAtIndex(
              propertyAttribute,
              0);
      TestTrue("values map is empty", values.IsEmpty());
    });

    It("returns values of valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::vector<int8_t> scalarValues{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          "_SCALAR");

      std::string vec2PropertyName("vec2Property");
      std::vector<glm::u8vec2> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          "_VECTOR");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);

      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          2);

      for (size_t i = 0; i < scalarValues.size(); i++) {
        const auto values =
            UCesiumPropertyAttributeBlueprintLibrary::GetMetadataValuesAtIndex(
                propertyAttribute,
                int64(i));
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
      std::string propertyName("badProperty");
      std::vector<int8_t> data{-1, 2, -3, 4};
      AddPropertyAttributePropertyToModel(
          model,
          *pPrimitive,
          *pPropertyAttribute,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          data,
          "_TEST");

      FCesiumPropertyAttribute propertyAttribute(
          model,
          *pPrimitive,
          *pPropertyAttribute);

      TestEqual(
          "PropertyAttributeStatus",
          UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
              propertyAttribute),
          ECesiumPropertyAttributeStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
              propertyAttribute)
              .Num(),
          1);

      const auto values =
          UCesiumPropertyAttributeBlueprintLibrary::GetMetadataValuesAtIndex(
              propertyAttribute,
              0);
      TestTrue("values map is empty", values.IsEmpty());
    });
  });
}

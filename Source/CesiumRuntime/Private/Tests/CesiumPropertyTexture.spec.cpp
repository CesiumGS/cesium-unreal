#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfSpecUtility.h"
#include "CesiumPropertyTexture.h"
#include "Misc/AutomationTest.h"
#include <limits>

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPropertyTextureSpec,
    "Cesium.PropertyTexture",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
ExtensionModelExtStructuralMetadata* pExtension;
PropertyTexture* pPropertyTexture;

const std::vector<FVector2D> texCoords{
    FVector2D(0, 0),
    FVector2D(0.5, 0),
    FVector2D(0, 0.5),
    FVector2D(0.5, 0.5)};
END_DEFINE_SPEC(FCesiumPropertyTextureSpec)

void FCesiumPropertyTextureSpec::Define() {
  BeforeEach([this]() {
    model = Model();
    pExtension = &model.addExtension<ExtensionModelExtStructuralMetadata>();
    pExtension->schema = Schema();
    pPropertyTexture = &pExtension->propertyTextures.emplace_back();
  });

  Describe("Constructor", [this]() {
    It("constructs invalid instance by default", [this]() {
      FCesiumPropertyTexture propertyTexture;
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTexture);
      TestTrue(
          "Properties",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .IsEmpty());
    });

    It("constructs invalid instance for missing schema", [this]() {
      pExtension->schema = std::nullopt;

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTextureClass);
      TestTrue(
          "Properties",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .IsEmpty());
    });

    It("constructs invalid instance for missing class", [this]() {
      pPropertyTexture->classProperty = "nonexistent class";

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTextureClass);
      TestTrue(
          "Properties",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .IsEmpty());
    });

    It("constructs valid instance with valid property", [this]() {
      pPropertyTexture->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::array<int8_t, 4> values{1, 2, 3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          values,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual<int64>(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          1);
    });

    It("constructs valid instance with invalid property", [this]() {
      // Even if one of its properties is invalid, the property table itself is
      // still valid.
      pPropertyTexture->classProperty = "testClass";
      std::string propertyName("testProperty");
      std::array<int8_t, 4> values{1, 2, 3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32, // Incorrect component type
          values,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual<int64>(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          1);
    });
  });

  Describe("GetProperties", [this]() {
    BeforeEach([this]() { pPropertyTexture->classProperty = "testClass"; });

    It("returns no properties for invalid property texture", [this]() {
      FCesiumPropertyTexture propertyTexture;
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTexture);
      const auto properties =
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(
              propertyTexture);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      std::string vec2PropertyName("vec2Property");
      std::array<glm::u8vec2, 4> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          {0, 1});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);

      const auto properties =
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(
              propertyTexture);

      TestTrue(
          "has scalar property",
          properties.Contains(FString(scalarPropertyName.c_str())));
      const FCesiumPropertyTextureProperty& scalarProperty =
          *properties.Find(FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(scalarProperty),
          ECesiumPropertyTexturePropertyStatus::Valid);
      for (size_t i = 0; i < texCoords.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        TestEqual(
            label.c_str(),
            UCesiumPropertyTexturePropertyBlueprintLibrary::GetInteger(
                scalarProperty,
                texCoords[i]),
            scalarValues[i]);
      }

      TestTrue(
          "has vec2 property",
          properties.Contains(FString(vec2PropertyName.c_str())));
      const FCesiumPropertyTextureProperty& vec2Property =
          *properties.Find(FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(vec2Property),
          ECesiumPropertyTexturePropertyStatus::Valid);
      for (size_t i = 0; i < texCoords.size(); i++) {
        std::string label("Property value " + std::to_string(i));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        TestEqual(
            label.c_str(),
            UCesiumPropertyTexturePropertyBlueprintLibrary::GetVector2D(
                vec2Property,
                texCoords[i],
                FVector2D::Zero()),
            expected);
      }
    });

    It("gets invalid property", [this]() {
      // Even invalid properties should still be retrieved.
      std::array<int8_t, 4> values{0, 1, 2, 3};
      std::string propertyName("badProperty");

      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          values,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);

      const auto properties =
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(
              propertyTexture);

      TestTrue(
          "has invalid property",
          properties.Contains(FString(propertyName.c_str())));
      const FCesiumPropertyTextureProperty& property =
          *properties.Find(FString(propertyName.c_str()));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(property),
          ECesiumPropertyTexturePropertyStatus::ErrorInvalidPropertyData);
    });
  });

  Describe("GetPropertyNames", [this]() {
    BeforeEach([this]() { pPropertyTexture->classProperty = "testClass"; });

    It("returns empty array for invalid property table", [this]() {
      FCesiumPropertyTexture propertyTexture;
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTexture);
      const auto properties =
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(
              propertyTexture);
      TestTrue("properties are empty", properties.IsEmpty());
    });

    It("gets all property names", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      std::string vec2PropertyName("vec2Property");
      std::array<glm::u8vec2, 4> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          {0, 1});

      std::string invalidPropertyName("badProperty");
      std::array<uint8_t, 4> invalidPropertyValues{0, 1, 2, 3};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          invalidPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32, // Incorrect component type
          invalidPropertyValues,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);

      const auto propertyNames =
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyNames(
              propertyTexture);
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
    BeforeEach([this]() { pPropertyTexture->classProperty = "testClass"; });

    It("returns invalid instance for nonexistent property", [this]() {
      std::string propertyName("testProperty");
      std::array<int8_t, 4> values{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          values,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          1);

      FCesiumPropertyTextureProperty property =
          UCesiumPropertyTextureBlueprintLibrary::FindProperty(
              propertyTexture,
              FString("nonexistent property"));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(property),
          ECesiumPropertyTexturePropertyStatus::ErrorInvalidProperty);
    });

    It("finds existing properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      std::string vec2PropertyName("vec2Property");
      std::array<glm::u8vec2, 4> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          {0, 1});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);
      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          2);

      FCesiumPropertyTextureProperty scalarProperty =
          UCesiumPropertyTextureBlueprintLibrary::FindProperty(
              propertyTexture,
              FString(scalarPropertyName.c_str()));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(scalarProperty),
          ECesiumPropertyTexturePropertyStatus::Valid);

      FCesiumPropertyTextureProperty vec2Property =
          UCesiumPropertyTextureBlueprintLibrary::FindProperty(
              propertyTexture,
              FString(vec2PropertyName.c_str()));
      TestEqual(
          "PropertyTexturePropertyStatus",
          UCesiumPropertyTexturePropertyBlueprintLibrary::
              GetPropertyTexturePropertyStatus(vec2Property),
          ECesiumPropertyTexturePropertyStatus::Valid);
    });
  });

  Describe("GetMetadataValuesForUV", [this]() {
    BeforeEach([this]() { pPropertyTexture->classProperty = "testClass"; });

    It("returns empty map for invalid property table", [this]() {
      FCesiumPropertyTexture propertyTexture;

      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::ErrorInvalidPropertyTexture);
      TestTrue(
          "Properties",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .IsEmpty());

      const auto values =
          UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUV(
              propertyTexture,
              FVector2D::Zero());
      TestTrue("values map is empty", values.IsEmpty());
    });

    It("returns values of valid properties", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      std::string vec2PropertyName("vec2Property");
      std::array<glm::u8vec2, 4> vec2Values{
          glm::u8vec2(1, 2),
          glm::u8vec2(0, 4),
          glm::u8vec2(8, 9),
          glm::u8vec2(11, 0),
      };
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::UINT8,
          vec2Values,
          {0, 1});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);

      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          2);

      for (size_t i = 0; i < texCoords.size(); i++) {
        const auto values =
            UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUV(
                propertyTexture,
                texCoords[i]);
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
      std::array<int8_t, 4> data{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          propertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          data,
          {0});

      FCesiumPropertyTexture propertyTexture(model, *pPropertyTexture);

      TestEqual(
          "PropertyTextureStatus",
          UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
              propertyTexture),
          ECesiumPropertyTextureStatus::Valid);
      TestEqual(
          "Property Count",
          UCesiumPropertyTextureBlueprintLibrary::GetProperties(propertyTexture)
              .Num(),
          1);

      const auto values =
          UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUV(
              propertyTexture,
              FVector2D::Zero());
      TestTrue("values map is empty", values.IsEmpty());
    });
  });
}

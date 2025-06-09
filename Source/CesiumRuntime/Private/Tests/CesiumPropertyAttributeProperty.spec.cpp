// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyAttributeProperty.h"
#include "CesiumGltfSpecUtility.h"
#include "Misc/AutomationTest.h"
#include <limits>

BEGIN_DEFINE_SPEC(
    FCesiumPropertyAttributePropertySpec,
    "Cesium.Unit.PropertyAttributeProperty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCesiumPropertyAttributePropertySpec)

void FCesiumPropertyAttributePropertySpec::Define() {
  using namespace CesiumGltf;

  Describe("Constructor", [this]() {
    It("constructs invalid instance by default", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);

      FCesiumMetadataValueType expectedType; // Invalid type
      TestTrue(
          "ValueType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
              property) == expectedType);
      TestEqual<int64_t>(
          "Size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          0);
    });

    It("constructs invalid instance from view with invalid definition",
       [this]() {
         PropertyAttributePropertyView<int8_t> propertyView(
             PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
         FCesiumPropertyAttributeProperty property(propertyView);
         TestEqual(
             "PropertyAttributePropertyStatus",
             UCesiumPropertyAttributePropertyBlueprintLibrary::
                 GetPropertyAttributePropertyStatus(property),
             ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);

         FCesiumMetadataValueType expectedType; // Invalid type
         TestTrue(
             "ValueType",
             UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
                 property) == expectedType);
         TestEqual<int64_t>(
             "Size",
             UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
                 property),
             0);
       });

    It("constructs invalid instance from view with invalid data", [this]() {
      PropertyAttributePropertyView<int8_t> propertyView(
          PropertyAttributePropertyViewStatus::
              ErrorAccessorComponentTypeMismatch);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidPropertyData);

      FCesiumMetadataValueType expectedType; // Invalid type
      TestTrue(
          "ValueType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
              property) == expectedType);
      TestEqual<int64_t>(
          "Size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          0);
    });

    It("constructs valid instance", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT32;

      std::vector<uint32_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint32_t> accessorView(
          data.data(),
          sizeof(uint32_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint32_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "Size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          int64_t(values.size()));

      FCesiumMetadataValueType expectedType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Uint32,
          false);
      TestTrue(
          "ValueType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
              property) == expectedType);
      TestEqual(
          "BlueprintType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetBlueprintType(
              property),
          ECesiumMetadataBlueprintType::Integer64);

      TestFalse(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      // Check that undefined properties return empty values
      FCesiumMetadataValue value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetOffset(property);
      TestTrue("Offset", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetScale(property);
      TestTrue("Scale", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetMaximumValue(
          property);
      TestTrue("Max", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetMinimumValue(
          property);
      TestTrue("Min", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetNoDataValue(
          property);
      TestTrue("NoData", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetDefaultValue(
          property);
      TestTrue("Default", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("constructs valid normalized instance", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      std::vector<int8_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int8_t> accessorView(
          data.data(),
          sizeof(int8_t),
          0,
          values.size());

      PropertyAttributePropertyView<int8_t, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "Size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          int64_t(values.size()));

      FCesiumMetadataValueType expectedType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Int8,
          false);
      TestTrue(
          "ValueType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
              property) == expectedType);
      TestEqual(
          "BlueprintType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetBlueprintType(
              property),
          ECesiumMetadataBlueprintType::Integer);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));
    });

    It("constructs valid instance with additional properties", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      double offset = 1.0;
      double scale = 2.0;
      double min = 1.0;
      double max = 3.0;
      int8_t noData = 1;
      double defaultValue = 12.3;

      classProperty.offset = offset;
      classProperty.scale = scale;
      classProperty.min = min;
      classProperty.max = max;
      classProperty.noData = noData;
      classProperty.defaultProperty = defaultValue;

      std::vector<int8_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int8_t> accessorView(
          data.data(),
          sizeof(int8_t),
          0,
          values.size());

      PropertyAttributePropertyView<int8_t, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "Size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          int64_t(values.size()));

      FCesiumMetadataValueType expectedType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Int8,
          false);
      TestTrue(
          "ValueType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
              property) == expectedType);
      TestEqual(
          "BlueprintType",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetBlueprintType(
              property),
          ECesiumMetadataBlueprintType::Integer);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      FCesiumMetadataValue value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetOffset(property);
      TestEqual(
          "Offset",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          offset);

      value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetScale(property);
      TestEqual(
          "Scale",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          scale);

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetMaximumValue(
          property);
      TestEqual(
          "Max",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          max);

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetMinimumValue(
          property);
      TestEqual(
          "Min",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          min);

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetNoDataValue(
          property);
      TestEqual(
          "NoData",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0.0),
          noData);

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetDefaultValue(
          property);
      TestEqual(
          "Default",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          defaultValue);
    });
  });

  Describe("GetByte", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
              property,
              0),
          0);
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;

      std::vector<uint8_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint8_t> accessorView(
          data.data(),
          sizeof(uint8_t),
          0,
          values.size());
      PropertyAttributePropertyView<uint8_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);

      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
              property,
              -1),
          0);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
              property,
              10),
          0);
    });

    It("gets from uint8 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;

      std::vector<uint8_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint8_t> accessorView(
          data.data(),
          sizeof(uint8_t),
          0,
          values.size());
      PropertyAttributePropertyView<uint8_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);

      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
                property,
                int64_t(i)),
            values[i]);
      }
    });

    It("converts compatible values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-1.0f, 2.0f, 256.0f, 4.0f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<uint8_t> expected{0, 2, 0, 4};
      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
                property,
                int64(i),
                0),
            expected[i]);
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;

      uint8_t noDataValue = 0;
      uint8_t defaultValue = 255;

      classProperty.noData = noDataValue;
      classProperty.defaultProperty = defaultValue;

      std::vector<uint8_t> values{1, 2, 3, 0};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint8> accessorView(
          data.data(),
          sizeof(uint8),
          0,
          values.size());

      PropertyAttributePropertyView<uint8> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        if (values[i] == noDataValue) {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
                  property,
                  int64(i)),
              defaultValue);
        } else {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
                  property,
                  int64(i)),
              values[i]);
        }
      }
    });
  });

  Describe("GetInteger", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
              property,
              0),
          0);
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      std::vector<int16_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
              property,
              -1),
          0);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
              property,
              10),
          0);
    });

    It("gets from int16 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      std::vector<int16_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
                property,
                int64_t(i)),
            values[i]);
      }
    });

    It("converts compatible values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{
          -1.0f,
          2.0f,
          float(std::numeric_limits<uint32_t>::max()),
          4.0f,
          2.54f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<int32_t> expected{-1, 2, 0, 4, 2};
      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
                property,
                int64_t(i)),
            expected[i]);
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      int16_t noDataValue = -1;
      int16_t defaultValue = 10;

      classProperty.noData = noDataValue;
      classProperty.defaultProperty = defaultValue;

      std::vector<int16_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        if (values[i] == noDataValue) {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
                  property,
                  int64_t(i)),
              defaultValue);
        } else {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
                  property,
                  int64_t(i)),
              values[i]);
        }
      }
    });
  });

  Describe("GetInteger64", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
              property,
              0),
          0);
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT32;

      std::vector<uint32_t>
          values{1, 2, 3, 4, std::numeric_limits<uint32_t>::max()};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint32_t> accessorView(
          data.data(),
          sizeof(uint32_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint32_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
              property,
              -1),
          0);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
              property,
              10),
          0);
    });

    It("gets from uint32 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT32;

      std::vector<uint32_t>
          values{1, 2, 3, 4, std::numeric_limits<uint32_t>::max()};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint32_t> accessorView(
          data.data(),
          sizeof(uint32_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint32_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
                property,
                int64_t(i)),
            int64_t(values[i]));
      }
    });

    It("converts compatible values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{
          float(std::numeric_limits<uint64_t>::max()),
          2.0f,
          float(std::numeric_limits<uint32_t>::max()),
          4.0f,
          2.54f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<int64_t> expected{0, 2, int64_t(values[2]), 4, 2};
      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
                property,
                int64_t(i)),
            expected[i]);
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      int32_t noDataValue = -1;
      int32_t defaultValue = 10;

      classProperty.noData = noDataValue;
      classProperty.defaultProperty = defaultValue;

      std::vector<int16_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        if (values[i] == noDataValue) {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
                  property,
                  int64_t(i)),
              defaultValue);
        } else {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
                  property,
                  int64_t(i)),
              values[i]);
        }
      }
    });
  });

  Describe("GetFloat", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
              property,
              0),
          0.0f);
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-1.54f, 52.78f, -39.0f, 4.005f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
              property,
              -1),
          0.0f);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
              property,
              10),
          0.0f);
    });

    It("gets from float property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-1.54f, 52.78f, -39.0f, 4.005f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
                property,
                i),
            values[i]);
      }
    });

    It("converts integer values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<int8_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int8_t> accessorView(
          data.data(),
          sizeof(int8_t),
          0,
          values.size());

      PropertyAttributePropertyView<int8_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
                property,
                i),
            static_cast<float>(values[i]));
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      float offset = 5.0f;
      float scale = 2.0f;

      classProperty.offset = offset;
      classProperty.scale = scale;

      std::vector<float> values{-1.1f, 2.2f, -3.3f, 4.0f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
                property,
                i),
            values[i] * scale + offset);
      }
    });
  });

  Describe("GetFloat64", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
              property,
              0),
          0.0);
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-1.1, 2.2, -3.3, 4.0};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
              property,
              -1),
          0.0);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
              property,
              10),
          0.0);
    });

    It("gets float values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-1.1, 2.2, -3.3, 4.0};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
                property,
                i),
            double(values[i]));
      }
    });

    It("gets from normalized uint8 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;
      classProperty.normalized = true;

      std::vector<uint8_t> values{0, 128, 255, 0};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint8_t> accessorView(
          data.data(),
          sizeof(uint8_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint8_t, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
                property,
                i),
            double(values[i]) / 255.0);
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;
      classProperty.normalized = true;

      float offset = 5.0;
      float scale = 2.0;

      classProperty.offset = offset;
      classProperty.scale = scale;

      std::vector<uint8_t> values{0, 128, 255, 0};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint8_t> accessorView(
          data.data(),
          sizeof(uint8_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint8, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
                property,
                i),
            (double(values[i]) / 255.0) * scale + offset);
      }
    });
  });

  Describe("GetIntPoint", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntPoint(
              property,
              0,
              FIntPoint(0)),
          FIntPoint(0));
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<glm::i8vec2> values{
          glm::i8vec2(1, 1),
          glm::i8vec2(-1, -1),
          glm::i8vec2(2, 4),
          glm::i8vec2(0, -8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec2> accessorView(
          data.data(),
          sizeof(glm::i8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
              property,
              -1,
              FIntVector(0)),
          FIntVector(0));
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
              property,
              10,
              FIntVector(0)),
          FIntVector(0));
    });

    It("gets from i8vec2 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<glm::i8vec2> values{
          glm::i8vec2(1, 1),
          glm::i8vec2(-1, -1),
          glm::i8vec2(2, 4),
          glm::i8vec2(0, -8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec2> accessorView(
          data.data(),
          sizeof(glm::i8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FIntPoint expected(values[i][0], values[i][1]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntPoint(
                property,
                int64_t(i),
                FIntPoint(0)),
            expected);
      }
    });

    It("converts compatible values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{
          1.234f,
          -24.5f,
          std::numeric_limits<float>::lowest(),
          2456.80f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<int32_t> expected{1, -24, 0, 2456};
      for (size_t i = 0; i < values.size(); i++) {
        FIntPoint expectedIntPoint(expected[i]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntPoint(
                property,
                int64_t(i),
                FIntPoint(0)),
            expectedIntPoint);
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      glm::i8vec2 noData(-1, -1);
      FIntPoint defaultValue(5, 22);

      classProperty.noData = {noData[0], noData[1]};
      classProperty.defaultProperty = {defaultValue[0], defaultValue[1]};

      std::vector<glm::i8vec2> values{
          glm::i8vec2(1, 1),
          glm::i8vec2(-1, -1),
          glm::i8vec2(2, 4),
          glm::i8vec2(0, -8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec2> accessorView(
          data.data(),
          sizeof(glm::i8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);

      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FIntPoint expected;
        if (values[i] == noData) {
          expected = defaultValue;
        } else {
          expected = FIntPoint(values[i][0], values[i][1]);
        }

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntPoint(
                property,
                int64_t(i),
                FIntPoint(0)),
            expected);
      }
    });
  });

  Describe("GetVector2D", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
              property,
              0,
              FVector2D::Zero()),
          FVector2D::Zero());
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec2> values{
          glm::vec2(1.0f, 1.1f),
          glm::vec2(-1.0f, -0.1f),
          glm::vec2(2.2f, 4.2f),
          glm::vec2(0.0f, -8.0f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec2> accessorView(
          data.data(),
          sizeof(glm::vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
              property,
              -1,
              FVector2D::Zero()),

          FVector2D::Zero());
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
              property,
              10,
              FVector2D::Zero()),
          FVector2D::Zero());
    });

    It("gets from glm::vec2 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec2> values{
          glm::vec2(1.0f, 1.1f),
          glm::vec2(-1.0f, -0.1f),
          glm::vec2(2.2f, 4.2f),
          glm::vec2(0.0f, -8.0f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec2> accessorView(
          data.data(),
          sizeof(glm::vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
                property,
                int64_t(i),
                FVector2D::Zero()),
            FVector2D(values[i][0], values[i][1]));
      }
    });

    It("gets from normalized glm::u8vec2 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;
      classProperty.normalized = true;

      std::vector<glm::u8vec2> values{
          glm::u8vec2(1, 1),
          glm::u8vec2(0, 255),
          glm::u8vec2(10, 4),
          glm::u8vec2(128, 8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::u8vec2> accessorView(
          data.data(),
          sizeof(glm::u8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::u8vec2, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec2 expected = glm::dvec2(values[i][0], values[i][1]) / 255.0;
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
                property,
                int64_t(i),
                FVector2D::Zero()),
            FVector2D(expected[0], expected[1]));
      }
    });

    It("converts unnormalized glm::u8vec2 values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;

      std::vector<glm::u8vec2> values{
          glm::u8vec2(1, 1),
          glm::u8vec2(0, 255),
          glm::u8vec2(10, 4),
          glm::u8vec2(128, 8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::u8vec2> accessorView(
          data.data(),
          sizeof(glm::u8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::u8vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
                property,
                int64_t(i),
                FVector2D::Zero()),
            FVector2D(values[i][0], values[i][1]));
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;
      classProperty.normalized = true;

      FVector2D offset(3.0, 2.4);
      FVector2D scale(2.0, -1.0);

      classProperty.offset = {offset[0], offset[1]};
      classProperty.scale = {scale[0], scale[1]};

      std::vector<glm::u8vec2> values{
          glm::u8vec2(1, 1),
          glm::u8vec2(0, 255),
          glm::u8vec2(10, 4),
          glm::u8vec2(128, 8)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::u8vec2> accessorView(
          data.data(),
          sizeof(glm::u8vec2),
          0,
          values.size());

      PropertyAttributePropertyView<glm::u8vec2, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FVector2D expected(
            double(values[i][0]) / 255.0 * scale[0] + offset[0],
            double(values[i][1]) / 255.0 * scale[1] + offset[1]);

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
                property,
                int64_t(i),
                FVector2D::Zero()),
            expected);
      }
    });
  });

  Describe("GetIntVector", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
              property,
              0,
              FIntVector(0)),
          FIntVector(0));
    });

    It("gets from glm::i8vec3 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<glm::i8vec3> values{
          glm::i8vec3(1, 1, -1),
          glm::i8vec3(-1, -1, 2),
          glm::i8vec3(0, 4, 2),
          glm::i8vec3(10, 8, 5)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec3> accessorView(
          data.data(),
          sizeof(glm::i8vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FIntVector expected(values[i][0], values[i][1], values[i][2]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
                property,
                i,
                FIntVector(0)),
            expected);
      }
    });

    It("converts compatible values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{
          1.234f,
          -24.5f,
          std::numeric_limits<float>::lowest(),
          2456.80f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<int32_t> expected{1, -24, 0, 2456};
      for (size_t i = 0; i < values.size(); i++) {
        FIntVector expectedIntVector(expected[i]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
                property,
                i,
                FIntVector(0)),
            expectedIntVector);
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      glm::i8vec3 noData(-1, -1, 2);
      FIntVector defaultValue(1, 2, 3);

      classProperty.noData = {noData[0], noData[1], noData[2]};
      classProperty.defaultProperty = {
          defaultValue[0],
          defaultValue[1],
          defaultValue[2]};

      std::vector<glm::i8vec3> values{
          glm::i8vec3(1, 1, -1),
          glm::i8vec3(-1, -1, 2),
          glm::i8vec3(0, 4, 2),
          glm::i8vec3(10, 8, 5)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec3> accessorView(
          data.data(),
          sizeof(glm::i8vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FIntVector expected;
        if (values[i] == noData) {
          expected = defaultValue;
        } else {
          expected = FIntVector(values[i][0], values[i][1], values[i][2]);
        }

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
                property,
                i,
                FIntVector(0)),
            expected);
      }
    });
  });

  Describe("GetVector3f", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
              property,
              0,
              FVector3f::Zero()),
          FVector3f::Zero());
    });

    It("returns default value for invalid index", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec3> values{
          glm::vec3(1.0f, 1.9f, -1.0f),
          glm::vec3(-1.0f, -1.8f, 2.5f),
          glm::vec3(10.0f, 4.4f, 5.4f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec3> accessorView(
          data.data(),
          sizeof(glm::vec3),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
              property,
              -1,
              FVector3f::Zero()),
          FVector3f::Zero());
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
              property,
              10,
              FVector3f::Zero()),
          FVector3f::Zero());
    });

    It("gets from glm::vec3 property", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec3> values{
          glm::vec3(1.0f, 1.9f, -1.0f),
          glm::vec3(-1.0f, -1.8f, 2.5f),
          glm::vec3(10.0f, 4.4f, 5.4f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec3> accessorView(
          data.data(),
          sizeof(glm::vec3),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FVector3f expected(values[i][0], values[i][1], values[i][2]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
                property,
                int64_t(i),
                FVector3f(0)),
            expected);
      }
    });

    It("converts vec2 values", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec2> values{
          glm::vec2(1.0f, 2.0f),
          glm::vec2(-5.9f, 8.2f),
          glm::vec2(20.5f, 2.0f),
          glm::vec2(-1.0f, -1.0f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec2> accessorView(
          data.data(),
          sizeof(glm::vec2),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<FVector3f> expected(4);
      for (size_t i = 0; i < values.size(); i++) {
        expected[i] = FVector3f(float(values[i][0]), float(values[i][1]), 0.0f);
      }

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
                property,
                int64(i),
                FVector3f::Zero()),
            expected[i]);
      }
    });

    It("gets with offset / scale", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      FVector3f offset(1.0f, 4.5f, -2.0f);
      FVector3f scale(0.5f, -1.0f, 2.2f);

      classProperty.offset = {offset[0], offset[1], offset[2]};
      classProperty.scale = {scale[0], scale[1], scale[2]};

      std::vector<glm::vec3> values{
          glm::vec3(1.0f, 1.9f, -1.0f),
          glm::vec3(-1.0f, -1.8f, 2.5f),
          glm::vec3(10.0f, 4.4f, 5.4f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec3> accessorView(
          data.data(),
          sizeof(glm::vec3),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FVector3f expected(
            values[i][0] * scale[0] + offset[0],
            values[i][1] * scale[1] + offset[1],
            values[i][2] * scale[2] + offset[2]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
                property,
                int64_t(i),
                FVector3f::Zero()),
            expected);
      }
    });
  });

  Describe("GetVector", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
              property,
              0,
              FVector::Zero()),
          FVector::Zero());
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec3> values{
          glm::vec3(1.02f, 0.1f, -1.11f),
          glm::vec3(-1.0f, -1.0f, 2.0f),
          glm::vec3(0.02f, 4.2f, 2.01f),
          glm::vec3(10.0f, 8.067f, 5.213f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec3> accessorView(
          data.data(),
          sizeof(glm::vec3),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
              property,
              -1,
              FVector::Zero()),
          FVector::Zero());
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
              property,
              10,
              FVector::Zero()),
          FVector::Zero());
    });

    It("gets from glm::vec3 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec3> values{
          glm::vec3(1.02f, 0.1f, -1.11f),
          glm::vec3(-1.0f, -1.0f, 2.0f),
          glm::vec3(0.02f, 4.2f, 2.01f),
          glm::vec3(10.0f, 8.067f, 5.213f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec3> accessorView(
          data.data(),
          sizeof(glm::vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestFalse(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec3 expected(values[i][0], values[i][1], values[i][2]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector::Zero()),
            FVector(expected[0], expected[1], expected[2]));
      }
    });

    It("gets from normalized glm::i8vec3 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      std::vector<glm::i8vec3> values{
          glm::i8vec3(1, 1, -1),
          glm::i8vec3(-1, -1, 2),
          glm::i8vec3(0, 4, 2),
          glm::i8vec3(10, 8, 5)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec3> accessorView(
          data.data(),
          sizeof(glm::i8vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec3, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec3 expected =
            glm::dvec3(values[i][0], values[i][1], values[i][2]) / 127.0;
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector::Zero()),
            FVector(expected[0], expected[1], expected[2]));
      }
    });

    It("converts unnormalized glm::i8vec3 values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<glm::i8vec3> values{
          glm::i8vec3(1, 1, -1),
          glm::i8vec3(-1, -1, 2),
          glm::i8vec3(0, 4, 2),
          glm::i8vec3(10, 8, 5)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec3> accessorView(
          data.data(),
          sizeof(glm::i8vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec3> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector::Zero()),
            FVector(values[i][0], values[i][1], values[i][2]));
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC3;
      classProperty.componentType = ClassProperty::ComponentType::UINT8;
      classProperty.normalized = true;

      FVector offset(1.0, 2.0, 3.0);
      FVector scale(0.5, -1.0, 2.0);

      classProperty.offset = {offset[0], offset[1], offset[2]};
      classProperty.scale = {scale[0], scale[1], scale[2]};

      std::vector<glm::u8vec3> values{
          glm::u8vec3(0, 128, 255),
          glm::u8vec3(255, 255, 255),
          glm::u8vec3(10, 20, 30),
          glm::u8vec3(128, 0, 0)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::u8vec3> accessorView(
          data.data(),
          sizeof(glm::u8vec3),
          0,
          values.size());

      PropertyAttributePropertyView<glm::u8vec3, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FVector expected(
            double(values[i][0]) / 255.0 * scale[0] + offset[0],
            double(values[i][1]) / 255.0 * scale[1] + offset[1],
            double(values[i][2]) / 255.0 * scale[2] + offset[2]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector::Zero()),
            expected);
      }
    });
  });

  Describe("GetVector4", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
              property,
              0,
              FVector4::Zero()),
          FVector4::Zero());
    });

    It("returns default value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC4;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec4> values{
          glm::vec4(1.02f, 0.1f, -1.11f, 1.0f),
          glm::vec4(-1.0f, -1.0f, 2.0f, 0.0f),
          glm::vec4(0.02f, 4.2f, 2.01f, 6.0f),
          glm::vec4(10.0f, 8.067f, 5.213f, 0.0f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec4> accessorView(
          data.data(),
          sizeof(glm::vec4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          static_cast<int64_t>(values.size()));

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
              property,
              -1,
              FVector4::Zero()),
          FVector4::Zero());
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
              property,
              10,
              FVector4::Zero()),
          FVector4::Zero());
    });

    It("gets from glm::vec4 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC4;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec4> values{
          glm::vec4(1.02f, 0.1f, -1.11f, 1.0f),
          glm::vec4(-1.0f, -1.0f, 2.0f, 0.0f),
          glm::vec4(0.02f, 4.2f, 2.01f, 6.0f),
          glm::vec4(10.0f, 8.067f, 5.213f, 0.0f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec4> accessorView(
          data.data(),
          sizeof(glm::vec4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec4 expected(
            values[i][0],
            values[i][1],
            values[i][2],
            values[i][3]);

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
                property,
                int64_t(i),
                FVector4::Zero()),
            FVector4(expected[0], expected[1], expected[2], expected[3]));
      }
    });

    It("gets from normalized glm::i8vec4 property", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC4;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      std::vector<glm::i8vec4> values{
          glm::i8vec4(1, 1, -1, 1),
          glm::i8vec4(-1, -1, 2, 0),
          glm::i8vec4(0, 4, 2, -8),
          glm::i8vec4(10, 8, 5, 27)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec4> accessorView(
          data.data(),
          sizeof(glm::i8vec4),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec4, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec4 expected(
            values[i][0],
            values[i][1],
            values[i][2],
            values[i][3]);
        expected /= 127.0;

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector4::Zero()),
            FVector4(expected[0], expected[1], expected[2], expected[3]));
      }
    });

    It("converts unnormalized glm::i8vec4 values", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC4;
      classProperty.componentType = ClassProperty::ComponentType::INT8;

      std::vector<glm::i8vec4> values{
          glm::i8vec4(-1, 2, 5, 8),
          glm::i8vec4(-1, -1, 2, 0),
          glm::i8vec4(3, 5, 7, 0),
          glm::i8vec4(1, -1, -2, 5)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec4> accessorView(
          data.data(),
          sizeof(glm::i8vec4),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        glm::dvec4 expected(
            values[i][0],
            values[i][1],
            values[i][2],
            values[i][3]);

        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
                property,
                int64_t(i),
                FVector4::Zero()),
            FVector4(expected[0], expected[1], expected[2], expected[3]));
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC4;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      FVector4 offset(1.0, 2.0, 3.0, -1.0);
      FVector4 scale(0.5, -1.0, 2.0, 3.5);

      classProperty.offset = {offset[0], offset[1], offset[2], offset[3]};
      classProperty.scale = {scale[0], scale[1], scale[2], scale[3]};

      std::vector<glm::i8vec4> values{
          glm::i8vec4(1, 1, -1, 1),
          glm::i8vec4(-1, -1, 2, 0),
          glm::i8vec4(0, 4, 2, -8),
          glm::i8vec4(10, 8, 5, 27)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8vec4> accessorView(
          data.data(),
          sizeof(glm::i8vec4),
          0,
          values.size());

      PropertyAttributePropertyView<glm::i8vec4, true> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        FVector4 expected(
            values[i][0] / 127.0 * scale[0] + offset[0],
            values[i][1] / 127.0 * scale[1] + offset[1],
            values[i][2] / 127.0 * scale[2] + offset[2],
            values[i][3] / 127.0 * scale[3] + offset[3]);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
                property,
                int64_t(i),
                FVector4::Zero()),
            expected);
      }
    });
  });

  Describe("GetMatrix", [this]() {
    It("returns default value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);
      TestEqual(
          "value",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
              property,
              0,
              FMatrix::Identity),
          FMatrix::Identity);
    });

    It("returns default value for invalid index", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::MAT4;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      // clang-format off
      std::vector<glm::mat4> values{
          glm::mat4(
             1.0,  2.0,  3.0,  4.0,
             5.0,  6.0,  7.0,  8.0,
             9.0, 10.0, 11.0, 12.0,
            13.0, 14.0, 15.0, 16.0),
          glm::mat4(
             1.0,  0.0, 0.0, 0.0,
             0.0, -2.5, 0.0, 0.0,
             0.0,  0.0, 0.5, 0.0,
            -1.5,  4.0, 2.0, 1.0),
      };
      // clang-format on
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::mat4> accessorView(
          data.data(),
          sizeof(glm::mat4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::mat4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestEqual(
          "negative index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
              property,
              -1,
              FMatrix::Identity),

          FMatrix::Identity);
      TestEqual(
          "out-of-range positive index",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
              property,
              10,
              FMatrix::Identity),
          FMatrix::Identity);
    });

    It("gets from glm::dmat4 property", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::MAT4;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      // clang-format off
      std::vector<glm::mat4> values{
          glm::mat4(
             1.0,  2.0,  3.0,  4.0,
             5.0,  6.0,  7.0,  8.0,
             9.0, 10.0, 11.0, 12.0,
            13.0, 14.0, 15.0, 16.0),
          glm::mat4(
             1.0,  0.0, 0.0, 0.0,
             0.0, -2.5, 0.0, 0.0,
             0.0,  0.0, 0.5, 0.0,
            -1.5,  4.0, 2.0, 1.0),
      };
      // clang-format on
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::mat4> accessorView(
          data.data(),
          sizeof(glm::mat4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::mat4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<FMatrix> expected(2);
      expected[0] = FMatrix(
          FPlane4d(1.0, 5.0, 9.0, 13.0),
          FPlane4d(2.0, 6.0, 10.0, 14.0),
          FPlane4d(3.0, 7.0, 11.0, 15.0),
          FPlane4d(4.0, 8.0, 12.0, 16.0));
      expected[1] = FMatrix(
          FPlane4d(1.0, 0.0, 0.0, -1.5),
          FPlane4d(0.0, -2.5, 0.0, 4.0),
          FPlane4d(0.0, 0.0, 0.5, 2.0),
          FPlane4d(0.0, 0.0, 0.0, 1.0));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
                property,
                int64_t(i),
                FMatrix::Identity),
            expected[i]);
      }
    });

    It("gets from glm::u8mat4x4 property", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::MAT4;
      classProperty.componentType = ClassProperty::ComponentType::INT8;
      classProperty.normalized = true;

      // clang-format off
      std::vector<glm::i8mat4x4> values{
          glm::i8mat4x4(
             127,   0,    0,    0,
               0, 127,    0,    0,
               0,   0,  127,    0,
               0,   0, -127,  127),
          glm::i8mat4x4(
               0,   -127,   0,   0,
               127,    0,   0,   0,
               0,      0, 127,   0,
               0,      0, 127, 127),
      };
      // clang-format on
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::i8mat4x4> accessorView(
          data.data(),
          sizeof(glm::i8mat4x4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::i8mat4x4, true>
          propertyView(propertyAttributeProperty, classProperty, accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      TestTrue(
          "IsNormalized",
          UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
              property));

      std::vector<FMatrix> expected(2);
      expected[0] = FMatrix(
          FPlane4d(1.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 1.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 1.0, -1.0),
          FPlane4d(0.0, 0.0, 0.0, 1.0));
      expected[1] = FMatrix(
          FPlane4d(0.0, 1.0, 0.0, 0.0),
          FPlane4d(-1.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 1.0, 1.0),
          FPlane4d(0.0, 0.0, 0.0, 1.0));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
                property,
                int64_t(i),
                FMatrix::Identity),
            expected[i]);
      }
    });

    It("converts compatible values", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<float> values{-2.0f, 10.5f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<FMatrix> expected(2);
      expected[0] = FMatrix(
          FPlane4d(-2.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, -2.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, -2.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, -2.0));
      expected[1] = FMatrix(
          FPlane4d(10.5, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 10.5, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 10.5, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 10.5));
      for (size_t i = 0; i < expected.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
                property,
                int64_t(i),
                FMatrix::Identity),
            expected[i]);
      }
    });

    It("returns default values for incompatible type", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::VEC2;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      std::vector<glm::vec2> values{
          glm::vec2(-2.0f, 10.5f),
          glm::vec2(1.5f, 0.1f)};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::vec2> accessorView(
          data.data(),
          sizeof(glm::vec2),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::vec2> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
                property,
                int64_t(i),
                FMatrix::Identity),
            FMatrix::Identity);
      }
    });

    It("gets with offset / scale", [this]() {
      CesiumGltf::PropertyAttributeProperty propertyAttributeProperty;
      CesiumGltf::ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::MAT4;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      double offset = 1.0;
      double scale = 2.0;

      CesiumUtility::JsonValue::Array offsetArray(16);
      CesiumUtility::JsonValue::Array scaleArray(16);
      for (size_t i = 0; i < 16; i++) {
        offsetArray[i] = offset;
        scaleArray[i] = scale;
      }

      classProperty.offset = offsetArray;
      classProperty.scale = scaleArray;

      // clang-format off
      std::vector<glm::mat4> values{
          glm::mat4(
             1.0,  2.0,  3.0,  4.0,
             5.0,  6.0,  7.0,  8.0,
             9.0, 10.0, 11.0, 12.0,
            13.0, 14.0, 15.0, 16.0),
          glm::mat4(
             1.0,  0.0, 0.0, 0.0,
             0.0, -2.5, 0.0, 0.0,
             0.0,  0.0, 0.5, 0.0,
            -1.5,  4.0, 2.0, 1.0),
      };
      // clang-format on
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<glm::mat4> accessorView(
          data.data(),
          sizeof(glm::mat4),
          0,
          values.size());

      CesiumGltf::PropertyAttributePropertyView<glm::mat4> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      std::vector<FMatrix> expected(2);
      expected[0] = FMatrix(
          FPlane4d(3.0, 11.0, 19.0, 27.0),
          FPlane4d(5.0, 13.0, 21.0, 29.0),
          FPlane4d(7.0, 15.0, 23.0, 31.0),
          FPlane4d(9.0, 17.0, 25.0, 33.0));
      expected[1] = FMatrix(
          FPlane4d(3.0, 1.0, 1.0, -2.0),
          FPlane4d(1.0, -4.0, 1.0, 9.0),
          FPlane4d(1.0, 1.0, 2.0, 5.0),
          FPlane4d(1.0, 1.0, 1.0, 3.0));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
                property,
                int64_t(i),
                FMatrix::Identity),
            expected[i]);
      }
    });
  });

  Describe("GetValue", [this]() {
    It("returns empty value for invalid property", [this]() {
      FCesiumPropertyAttributeProperty property;
      TestEqual(
          "PropertyAttributePropertyStatus",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty);

      FCesiumMetadataValue value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
              property,
              0);
      FCesiumMetadataValueType valueType; // Unknown type
      TestTrue(
          "value type",
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
              valueType);
    });

    It("returns empty value for invalid index", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT32;

      std::vector<uint32_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint32_t> accessorView(
          data.data(),
          sizeof(uint32_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint32_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);
      TestEqual<int64_t>(
          "size",
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
              property),
          int64_t(values.size()));

      FCesiumMetadataValue value =
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
              property,
              -1);
      TestTrue(
          "negative index",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));

      value = UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
          property,
          10);
      TestTrue(
          "out-of-range positive index",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("gets value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::UINT32;

      std::vector<uint32_t> values{1, 2, 3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<uint32_t> accessorView(
          data.data(),
          sizeof(uint32_t),
          0,
          values.size());

      PropertyAttributePropertyView<uint32_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      FCesiumMetadataValueType valueType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Uint32,
          false);
      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
                property,
                int64_t(i));
        TestTrue(
            "value type",
            UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
                valueType);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
            values[i]);
      }
    });

    It("gets with offset / scale", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

      float offset = 1.0f;
      float scale = 2.0f;

      classProperty.offset = offset;
      classProperty.scale = scale;

      std::vector<float> values{-1.1f, 2.0f, -3.5f, 4.0f};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<float> accessorView(
          data.data(),
          sizeof(float),
          0,
          values.size());

      PropertyAttributePropertyView<float> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      FCesiumMetadataValueType valueType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Float32,
          false);
      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
                property,
                i);
        TestTrue(
            "value type",
            UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
                valueType);
        TestEqual(
            std::string("value" + std::to_string(i)).c_str(),
            UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0),
            values[i] * scale + offset);
      }
    });

    It("gets with noData", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      int16_t noData = -1;
      classProperty.noData = noData;

      std::vector<int16_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      FCesiumMetadataValueType valueType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Int16,
          false);
      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
                property,
                i);
        if (values[i] == noData) {
          // Empty value indicated by invalid value type.
          TestTrue(
              "value type",
              UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
                  FCesiumMetadataValueType());
        } else {
          TestTrue(
              "value type",
              UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
                  valueType);
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
              values[i]);
        }
      }
    });

    It("gets with noData / default value", [this]() {
      PropertyAttributeProperty propertyAttributeProperty;
      ClassProperty classProperty;
      classProperty.type = ClassProperty::Type::SCALAR;
      classProperty.componentType = ClassProperty::ComponentType::INT16;

      int16_t noData = -1;
      int16_t defaultValue = 15;

      classProperty.noData = noData;
      classProperty.defaultProperty = defaultValue;

      std::vector<int16_t> values{-1, 2, -3, 4};
      std::vector<std::byte> data = GetValuesAsBytes(values);

      AccessorView<int16_t> accessorView(
          data.data(),
          sizeof(int16_t),
          0,
          values.size());

      PropertyAttributePropertyView<int16_t> propertyView(
          propertyAttributeProperty,
          classProperty,
          accessorView);
      FCesiumPropertyAttributeProperty property(propertyView);
      TestEqual(
          "status",
          UCesiumPropertyAttributePropertyBlueprintLibrary::
              GetPropertyAttributePropertyStatus(property),
          ECesiumPropertyAttributePropertyStatus::Valid);

      FCesiumMetadataValueType valueType(
          ECesiumMetadataType::Scalar,
          ECesiumMetadataComponentType::Int16,
          false);
      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
                property,
                i);
        TestTrue(
            "value type",
            UCesiumMetadataValueBlueprintLibrary::GetValueType(value) ==
                valueType);
        if (values[i] == noData) {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
              defaultValue);
        } else {
          TestEqual(
              std::string("value" + std::to_string(i)).c_str(),
              UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
              values[i]);
        }
      }
    });
  });
}

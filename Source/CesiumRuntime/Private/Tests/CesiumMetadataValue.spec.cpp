#include "CesiumMetadataValue.h"
#include "Misc/AutomationTest.h"

#include <limits>

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumMetadataValueSpec,
    "Cesium.MetadataValue",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCesiumMetadataValueSpec)

void FCesiumMetadataValueSpec::Define() {
  Describe("Constructor", [this]() {
    It("constructs value with unknown type by default", [this]() {
      FCesiumMetadataValue value;
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Invalid);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs boolean value with correct type", [this]() {
      FCesiumMetadataValue value(true);
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Boolean);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs scalar value with correct type", [this]() {
      FCesiumMetadataValue value(1.6);
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Scalar);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Float64);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs vecN value with correct type", [this]() {
      FCesiumMetadataValue value(glm::u8vec4(1, 2, 3, 4));
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Vec4);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Uint8);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs matN value with correct type", [this]() {
      FCesiumMetadataValue value(glm::imat2x2(-1, -2, 3, 0));
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Mat2);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Int32);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs string value with correct type", [this]() {
      FCesiumMetadataValue value(std::string_view("Hello"));
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::String);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs array value with correct type", [this]() {
      PropertyArrayView<uint8_t> arrayView;
      FCesiumMetadataValue value(arrayView);
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Scalar);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Uint8);
      TestTrue("IsArray", valueType.bIsArray);
    });
  });

  Describe("GetBoolean", [this]() {
    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestTrue(
          "true",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(1.0f);
      TestTrue(
          "true",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("true"));
      TestTrue(
          "true",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));
    });

    It("returns default value for incompatible types", [this]() {
      FCesiumMetadataValue value(glm::u16vec2(1, 1));
      TestFalse(
          "vecN",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));

      value = FCesiumMetadataValue(glm::mat2(1.0f, 1.0f, 1.0f, 1.0f));
      TestFalse(
          "matN",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));

      value = FCesiumMetadataValue(PropertyArrayView<bool>());
      TestFalse(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));
    });
  });

  Describe("GetByte", [this]() {
    It("gets from uint8", [this]() {
      FCesiumMetadataValue value(static_cast<uint8_t>(23));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          23);
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          1);
    });

    It("gets from in-range integers", [this]() {
      FCesiumMetadataValue value(static_cast<int32_t>(255));
      TestEqual(
          "larger signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          255);

      value = FCesiumMetadataValue(static_cast<uint64_t>(255));
      TestEqual(
          "larger unsigned integer",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          255);
    });

    It("gets from in-range floating-point numbers", [this]() {
      FCesiumMetadataValue value(254.5f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          254);

      value = FCesiumMetadataValue(0.85);
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 255),
          0);
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("123"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          123);
    });

    It("returns default value for out-of-range numbers", [this]() {
      FCesiumMetadataValue value(static_cast<int8_t>(-1));
      TestEqual(
          "negative integer",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 255),
          255);

      value = FCesiumMetadataValue(-1.0);
      TestEqual(
          "negative floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 255),
          255);

      value = FCesiumMetadataValue(256);
      TestEqual(
          "positive integer",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          0);

      value = FCesiumMetadataValue(255.5f);
      TestEqual(
          "positive floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          0);
    });

    It("returns default value for incompatible types", [this]() {
      FCesiumMetadataValue value(glm::u16vec2(1, 1));
      TestEqual(
          "vecN",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          0);

      value = FCesiumMetadataValue(glm::mat2(1.0f, 1.0f, 1.0f, 1.0f));
      TestEqual(
          "matN",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          0);

      value = FCesiumMetadataValue(PropertyArrayView<uint8_t>());
      TestEqual(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
          0);
    });
  });

  Describe("GetInteger", [this]() {
    It("gets from in-range integers", [this]() {
      FCesiumMetadataValue value(static_cast<int32_t>(123));
      TestEqual(
          "int32_t",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          123);

      value = FCesiumMetadataValue(static_cast<int16_t>(-25));
      TestEqual(
          "smaller signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          -25);

      value = FCesiumMetadataValue(static_cast<uint8_t>(5));
      TestEqual(
          "smaller unsigned integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          5);

      value = FCesiumMetadataValue(static_cast<int64_t>(-123));
      TestEqual(
          "larger signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          -123);

      value = FCesiumMetadataValue(static_cast<int64_t>(456));
      TestEqual(
          "larger unsigned integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          456);
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(false);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, -1),
          0);
    });

    It("gets from in-range floating point number", [this]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          1234);

      value = FCesiumMetadataValue(-78.9);
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          -78);
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("-1234"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          -1234);
    });

    It("returns default value for out-of-range numbers", [this]() {
      FCesiumMetadataValue value(std::numeric_limits<int64_t>::min());
      TestEqual(
          "negative integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);

      value = FCesiumMetadataValue(std::numeric_limits<float>::lowest());
      TestEqual(
          "negative floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);

      value = FCesiumMetadataValue(std::numeric_limits<int64_t>::max());
      TestEqual(
          "positive integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);

      value = FCesiumMetadataValue(std::numeric_limits<float>::max());
      TestEqual(
          "positive floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);
    });

    It("returns default value for incompatible types", [this]() {
      FCesiumMetadataValue value(glm::ivec2(1, 1));
      TestEqual(
          "vecN",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);

      value = FCesiumMetadataValue(glm::i32mat2x2(1, 1, 1, 1));
      TestEqual(
          "matN",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);

      value = FCesiumMetadataValue(PropertyArrayView<int32_t>());
      TestEqual(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          0);
    });
  });

  Describe("GetInteger64", [this]() {
    It("gets from in-range integers", [this]() {
      FCesiumMetadataValue value(std::numeric_limits<int64_t>::max() - 1);
      TestEqual(
          "int64_t",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          std::numeric_limits<int64_t>::max() - 1);

      value = FCesiumMetadataValue(static_cast<int16_t>(-12345));
      TestEqual(
          "smaller signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          static_cast<int64_t>(-12345));

      value = FCesiumMetadataValue(static_cast<uint8_t>(255));
      TestEqual(
          "smaller unsigned integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          static_cast<int64_t>(255));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, -1),
          static_cast<int64_t>(1));
    });

    It("gets from in-range floating point number", [this]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          static_cast<int64_t>(1234));

      value = FCesiumMetadataValue(-78.9);
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          static_cast<int64_t>(-78));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("-1234"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, 0),
          static_cast<int64_t>(-1234));
    });

    It("returns default value for out-of-range numbers", [this]() {
      const int64_t zero = static_cast<int64_t>(0);
      FCesiumMetadataValue value(std::numeric_limits<float>::lowest());
      TestEqual(
          "negative floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);

      value = FCesiumMetadataValue(std::numeric_limits<uint64_t>::max());
      TestEqual(
          "positive integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);

      value = FCesiumMetadataValue(std::numeric_limits<float>::max());
      TestEqual(
          "positive floating-point number",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);
    });

    It("returns default value for incompatible types", [this]() {
      const int64_t zero = static_cast<int64_t>(0);
      FCesiumMetadataValue value(glm::u64vec2(1, 1));
      TestEqual(
          "vecN",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);

      value = FCesiumMetadataValue(glm::mat2(1.0f, 1.0f, 1.0f, 1.0f));
      TestEqual(
          "matN",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);

      value = FCesiumMetadataValue(PropertyArrayView<int64_t>());
      TestEqual(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(value, zero),
          zero);
    });
  });

  Describe("FString", [this]() {
    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("Hello"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("Hello"));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "true",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("true"));

      value = FCesiumMetadataValue(false);
      TestEqual(
          "false",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("false"));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(1234);
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("1234"));

      value = FCesiumMetadataValue(1.2345f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString(std::to_string(1.2345f).c_str()));
    });

    It("gets from vecN", [this]() {
      FCesiumMetadataValue value(glm::ivec4(1, 2, 3, 4));
      TestEqual(
          "vec4",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("X=1 Y=2 Z=3 W=4"));
    });

    It("returns default value for incompatible types", [this]() {
      FCesiumMetadataValue value =
          FCesiumMetadataValue(PropertyArrayView<uint8_t>());
      TestEqual(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString(""));
    });
  });
}

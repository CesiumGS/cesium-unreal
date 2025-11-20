// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValue.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

#include <limits>

BEGIN_DEFINE_SPEC(
    FCesiumMetadataValueSpec,
    "Cesium.Unit.MetadataValue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
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

    It("constructs enum value with correct type", [this]() {
      FCesiumMetadataValue value(
          0,
          MakeShared<FCesiumMetadataEnum>(
              StaticEnum<ECesiumMetadataBlueprintType>()));
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("Type", valueType.Type, ECesiumMetadataType::Enum);
      TestEqual(
          "ComponentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Int32);
      TestFalse("IsArray", valueType.bIsArray);
    });

    It("constructs array value with correct type", [this]() {
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView;
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

    It("constructs from existing array value", [this]() {
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView;
      FCesiumPropertyArray array(arrayView);

      FCesiumMetadataValue value(std::move(array));
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
  });

  Describe("GetInteger", [this]() {
    It("gets from in-range integers", [this]() {
      FCesiumMetadataValue value(static_cast<int32_t>(123));
      TestEqual(
          "int32_t",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          123);

      value = FCesiumMetadataValue(static_cast<int64_t>(-123));
      TestEqual(
          "larger signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger(value, 0),
          -123);

      value = FCesiumMetadataValue(static_cast<uint64_t>(456));
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
  });

  Describe("GetInteger64", [this]() {
    const int64_t defaultValue = static_cast<int64_t>(0);

    It("gets from in-range integers", [this, defaultValue]() {
      FCesiumMetadataValue value(std::numeric_limits<int64_t>::max() - 1);
      TestEqual<int64>(
          "int64_t",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          std::numeric_limits<int64>::max() - 1);

      value = FCesiumMetadataValue(static_cast<int16_t>(-12345));
      TestEqual<int64>(
          "smaller signed integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(-12345));

      value = FCesiumMetadataValue(static_cast<uint8_t>(255));
      TestEqual<int64>(
          "smaller unsigned integer",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(255));
    });

    It("gets from boolean", [this, defaultValue]() {
      FCesiumMetadataValue value(true);
      TestEqual<int64>(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(1));
    });

    It("gets from in-range floating point number", [this, defaultValue]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual<int64>(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(1234));

      value = FCesiumMetadataValue(-78.9);
      TestEqual<int64>(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(-78));
    });

    It("gets from string", [this, defaultValue]() {
      FCesiumMetadataValue value(std::string_view("-1234"));
      TestEqual<int64>(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetInteger64(
              value,
              defaultValue),
          static_cast<int64_t>(-1234));
    });

    It("returns default value for out-of-range numbers",
       [this, defaultValue]() {
         FCesiumMetadataValue value(std::numeric_limits<float>::lowest());
         TestEqual<int64>(
             "negative floating-point number",
             UCesiumMetadataValueBlueprintLibrary::GetInteger64(
                 value,
                 defaultValue),
             defaultValue);

         value = FCesiumMetadataValue(std::numeric_limits<uint64_t>::max());
         TestEqual<int64>(
             "positive integer",
             UCesiumMetadataValueBlueprintLibrary::GetInteger64(
                 value,
                 defaultValue),
             defaultValue);

         value = FCesiumMetadataValue(std::numeric_limits<float>::max());
         TestEqual<int64>(
             "positive floating-point number",
             UCesiumMetadataValueBlueprintLibrary::GetInteger64(
                 value,
                 defaultValue),
             defaultValue);
       });
  });

  Describe("GetFloat", [this]() {
    It("gets from in-range floating point number", [this]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f),
          1234.56f);

      double doubleValue = -78.9;
      value = FCesiumMetadataValue(doubleValue);
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f),
          static_cast<float>(doubleValue));
    });

    It("gets from integer", [this]() {
      FCesiumMetadataValue value(-12345);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f),
          static_cast<float>(-12345));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, -1.0f),
          1.0f);
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("-123.01"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f),
          static_cast<float>(-123.01));
    });

    It("returns default value for out-of-range numbers", [this]() {
      FCesiumMetadataValue value(std::numeric_limits<double>::lowest());
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetFloat(value, 0.0f),
          0.0f);
    });
  });

  Describe("GetFloat64", [this]() {
    It("gets from floating point number", [this]() {
      FCesiumMetadataValue value(78.91);
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          78.91);

      value = FCesiumMetadataValue(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          static_cast<double>(1234.56f));
    });

    It("gets from integer", [this]() {
      FCesiumMetadataValue value(-12345);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0f),
          static_cast<double>(-12345));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, -1.0),
          1.0);
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("-1234.05"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, 0.0),
          -1234.05);
    });
  });

  Describe("GetIntPoint", [this]() {
    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::ivec2(1, -2));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(1, -2));

      value = FCesiumMetadataValue(glm::vec2(-5.2f, 6.68f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(-5, 6));
    });

    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::u8vec3(4, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(4, 5));

      value = FCesiumMetadataValue(glm::vec3(-5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(-5, 6));
    });

    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::i16vec4(4, 2, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(4, 2));

      value = FCesiumMetadataValue(glm::vec4(1.01f, -5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(1, -5));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(123);
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(123));

      value = FCesiumMetadataValue(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(1234));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(-1)),
          FIntPoint(1));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1 Y=2"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(1, 2));
    });
  });

  Describe("GetVector2D", [this]() {
    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::ivec2(1, -2));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(static_cast<double>(1), static_cast<double>(-2)));

      value = FCesiumMetadataValue(glm::dvec2(-5.2, 6.68));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(-5.2, 6.68));
    });

    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::u8vec3(4, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(static_cast<double>(4), static_cast<double>(5)));

      value = FCesiumMetadataValue(glm::dvec3(-5.2, 6.68, -23));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(-5.2, 6.68));
    });

    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::i16vec4(4, 2, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(static_cast<double>(4), static_cast<double>(2)));

      value = FCesiumMetadataValue(glm::dvec4(1.01, -5.2, 6.68, -23.8));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(1.01, -5.2));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(123);
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(static_cast<double>(123)));

      value = FCesiumMetadataValue(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(static_cast<double>(1234.56f)));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D(-1.0)),
          FVector2D(1.0));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1.5 Y=2.5"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector2D(
              value,
              FVector2D::Zero()),
          FVector2D(1.5, 2.5));
    });
  });

  Describe("GetIntVector", [this]() {
    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::u8vec3(4, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(4, 5, 12));

      value = FCesiumMetadataValue(glm::vec3(-5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(-5, 6, -23));
    });

    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::ivec2(1, -2));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(1, -2, 0));

      value = FCesiumMetadataValue(glm::vec2(-5.2f, 6.68f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(-5, 6, 0));
    });

    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::i16vec4(4, 2, 5, 12));
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(4, 2, 5));

      value = FCesiumMetadataValue(glm::vec4(1.01f, -5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(1, -5, 6));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(123);
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(123));

      value = FCesiumMetadataValue(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(0)),
          FIntVector(1234));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetIntVector(
              value,
              FIntVector(-1)),
          FIntVector(1));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1 Y=2"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
              value,
              FIntPoint(0)),
          FIntPoint(1, 2));
    });
  });

  Describe("GetVector3f", [this]() {
    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::vec3(-5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f::Zero()),
          FVector3f(-5.2f, 6.68f, -23.8f));
    });

    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::vec2(-5.2f, 6.68f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f::Zero()),
          FVector3f(-5.2f, 6.68f, 0.0f));
    });

    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::vec4(1.01f, -5.2f, 6.68f, -23.8f));
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f::Zero()),
          FVector3f(1.01f, -5.2f, 6.68f));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f::Zero()),
          FVector3f(1234.56f));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f(-1.0f)),
          FVector3f(1.0f));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1 Y=2 Z=3"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector3f(
              value,
              FVector3f::Zero()),
          FVector3f(1, 2, 3));
    });
  });

  Describe("GetVector", [this]() {
    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::dvec3(-5.2, 6.68, -23.8));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector(
              value,
              FVector::Zero()),
          FVector(-5.2, 6.68, -23.8));
    });

    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::dvec2(-5.2, 6.68));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector(
              value,
              FVector::Zero()),
          FVector(-5.2, 6.68, 0.0));
    });

    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::dvec4(1.01, -5.2, 6.68, -23.8));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector(
              value,
              FVector::Zero()),
          FVector(1.01, -5.2, 6.68));
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(12345);
      TestEqual(
          "integer",
          UCesiumMetadataValueBlueprintLibrary::GetVector(
              value,
              FVector::Zero()),
          FVector(static_cast<double>(12345)));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(true);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector(value, FVector(-1.0)),
          FVector(1.0));
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1.5 Y=2.5 Z=3.5"));
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector(
              value,
              FVector::Zero()),
          FVector(1.5, 2.5, 3.5));
    });
  });

  Describe("GetVector4", [this]() {
    It("gets from vec4", [this]() {
      FCesiumMetadataValue value(glm::dvec4(1.01, -5.2, 6.68, -23.8));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(1.01, -5.2, 6.68, -23.8));
    });

    It("gets from vec3", [this]() {
      FCesiumMetadataValue value(glm::dvec3(-5.2, 6.68, -23.8));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(-5.2, 6.68, -23.8, 0.0));
    });

    It("gets from vec2", [this]() {
      FCesiumMetadataValue value(glm::dvec2(-5.2, 6.68));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(-5.2, 6.68, 0.0, 0.0));
    });

    It("gets from scalar", [this]() {
      float floatValue = 7.894f;
      double doubleValue = static_cast<double>(floatValue);
      FCesiumMetadataValue value(floatValue);
      TestEqual(
          "float",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(doubleValue, doubleValue, doubleValue, doubleValue));
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(false);
      TestEqual(
          "value",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4(-1.0)),
          FVector4::Zero());
    });

    It("gets from string", [this]() {
      FCesiumMetadataValue value(std::string_view("X=1.5 Y=2.5 Z=3.5 W=4.5"));
      TestEqual(
          "value without W-component",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 4.5));

      value = FCesiumMetadataValue(std::string_view("X=1.5 Y=2.5 Z=3.5"));
      TestEqual(
          "value without W-component",
          UCesiumMetadataValueBlueprintLibrary::GetVector4(
              value,
              FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 1.0));
    });
  });

  Describe("GetMatrix", [this]() {
    It("gets from mat4", [this]() {
      // clang-format off
      glm::dmat4 input = glm::dmat4(
           1.0,  2.0, 3.0, 4.0,
           5.0,  6.0, 7.0, 8.0,
           9.0, 11.0, 4.0, 1.0,
          10.0, 12.0, 3.0, 1.0);
      // clang-format on
      input = glm::transpose(input);

      FCesiumMetadataValue value(input);
      FMatrix expected(
          FPlane4d(1.0, 2.0, 3.0, 4.0),
          FPlane4d(5.0, 6.0, 7.0, 8.0),
          FPlane4d(9.0, 11.0, 4.0, 1.0),
          FPlane4d(10.0, 12.0, 3.0, 1.0));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetMatrix(
              value,
              FMatrix::Identity),
          expected);
    });

    It("gets from mat3", [this]() {
      // clang-format off
      glm::dmat3 input = glm::dmat3(
          1.0, 2.0, 3.0,
          4.0, 5.0, 6.0,
          7.0, 8.0, 9.0);
      // clang-format on
      input = glm::transpose(input);

      FCesiumMetadataValue value(input);
      FMatrix expected(
          FPlane4d(1.0, 2.0, 3.0, 0.0),
          FPlane4d(4.0, 5.0, 6.0, 0.0),
          FPlane4d(7.0, 8.0, 9.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetMatrix(
              value,
              FMatrix::Identity),
          expected);
    });

    It("gets from mat2", [this]() {
      // clang-format off
      glm::dmat2 input = glm::dmat2(
          1.0, 2.0,
          3.0, 4.0);
      // clang-format on
      input = glm::transpose(input);

      FCesiumMetadataValue value(input);
      FMatrix expected(
          FPlane4d(1.0, 2.0, 0.0, 0.0),
          FPlane4d(3.0, 4.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetMatrix(
              value,
              FMatrix::Identity),
          expected);
    });

    It("gets from scalar", [this]() {
      FCesiumMetadataValue value(7.894);
      FMatrix expected(
          FPlane4d(7.894, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 7.894, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 7.894, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 7.894));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetMatrix(
              value,
              FMatrix::Identity),
          expected);
    });

    It("gets from boolean", [this]() {
      FCesiumMetadataValue value(false);
      FMatrix expected(
          FPlane4d(0.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0));
      TestEqual(
          "double",
          UCesiumMetadataValueBlueprintLibrary::GetMatrix(
              value,
              FMatrix::Identity),
          expected);
    });
  });

  Describe("GetFString", [this]() {
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

    It("gets from matN", [this]() {
      // clang-format off
      FCesiumMetadataValue value(
          glm::i32mat4x4(
            1,   2,  3, -7,
            4,   5,  6, 88,
            0,  -1, -4,  4,
            2 , 70,  8,  9));
      // clang-format on
      std::string expected("[1 4 0 2] [2 5 -1 70] [3 6 -4 8] [-7 88 4 9]");
      TestEqual(
          "mat4",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString(expected.c_str()));
    });

    It("gets from enum", [this]() {
      TSharedPtr<FCesiumMetadataEnum> enumDef = MakeShared<FCesiumMetadataEnum>(
          StaticEnum<ECesiumMetadataBlueprintType>());
      FCesiumMetadataValue value(
          int32(ECesiumMetadataBlueprintType::Byte),
          enumDef);
      TestEqual(
          "enum",
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString("")),
          FString("Byte"));
    });
  });

  Describe("GetArray", [this]() {
    It("gets empty array from non-array value", [this]() {
      FCesiumMetadataValue value(std::string_view("not an array"));
      FCesiumPropertyArray array =
          UCesiumMetadataValueBlueprintLibrary::GetArray(value);
      TestEqual(
          "array size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(0));

      FCesiumMetadataValueType elementType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual(
          "array element type",
          elementType.Type,
          ECesiumMetadataType::Invalid);
      TestEqual(
          "array element component type",
          elementType.ComponentType,
          ECesiumMetadataComponentType::None);
    });

    It("gets array from array value", [this]() {
      std::vector<uint8_t> arrayValues{1, 2};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView =
          std::vector(arrayValues);

      FCesiumMetadataValue value(arrayView);
      FCesiumPropertyArray array =
          UCesiumMetadataValueBlueprintLibrary::GetArray(value);
      TestEqual(
          "array size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(arrayValues.size()));

      FCesiumMetadataValueType elementType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual(
          "array element type",
          elementType.Type,
          ECesiumMetadataType::Scalar);
      TestEqual(
          "array element component type",
          elementType.ComponentType,
          ECesiumMetadataComponentType::Uint8);
    });
  });

  Describe("IsEmpty", [this]() {
    It("returns true for default value", [this]() {
      FCesiumMetadataValue value;
      TestTrue("IsEmpty", UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for boolean value", [this]() {
      FCesiumMetadataValue value(true);
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for scalar value", [this]() {
      FCesiumMetadataValue value(1.6);
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for vecN value", [this]() {
      FCesiumMetadataValue value(glm::u8vec4(1, 2, 3, 4));
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for matN value", [this]() {
      FCesiumMetadataValue value(glm::imat2x2(-1, -2, 3, 0));
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for string value", [this]() {
      FCesiumMetadataValue value(std::string_view("Hello"));
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });

    It("returns false for array value", [this]() {
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView;
      FCesiumMetadataValue value(arrayView);
      TestFalse(
          "IsEmpty",
          UCesiumMetadataValueBlueprintLibrary::IsEmpty(value));
    });
  });

  Describe("GetValuesAsStrings", [this]() {
    It("returns empty map if input is empty", [this]() {
      TMap<FString, FCesiumMetadataValue> values;
      const auto strings =
          UCesiumMetadataValueBlueprintLibrary::GetValuesAsStrings(values);
      TestTrue("values map is empty", strings.IsEmpty());
    });

    It("returns values as strings", [this]() {
      TMap<FString, FCesiumMetadataValue> values;
      values.Add({"scalar", FCesiumMetadataValue(-1)});
      values.Add({"vec2", FCesiumMetadataValue(glm::u8vec2(2, 3))});
      values.Add(
          {"array",
           FCesiumMetadataValue(
               CesiumGltf::PropertyArrayCopy<uint8>({1, 2, 3}))});

      const auto strings =
          UCesiumMetadataValueBlueprintLibrary::GetValuesAsStrings(values);
      TestEqual("map count", values.Num(), strings.Num());

      const FString* pString = strings.Find(FString("scalar"));
      TestTrue("has scalar value", pString != nullptr);
      TestEqual("scalar value as string", *pString, FString("-1"));

      pString = strings.Find(FString("vec2"));
      TestTrue("has vec2 value", pString != nullptr);
      TestEqual("vec2 value as string", *pString, FString("X=2 Y=3"));

      pString = strings.Find(FString("array"));
      TestTrue("has array value", pString != nullptr);
      TestEqual("array value as string", *pString, FString());
    });
  });

  Describe("GetUnsignedInteger64", [this]() {
    const uint64_t defaultValue = static_cast<uint64_t>(0);

    It("gets from in-range integers", [this, defaultValue]() {
      FCesiumMetadataValue value(std::numeric_limits<uint64_t>::max() - 1);
      TestEqual<uint64_t>(
          "uint64_t",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          std::numeric_limits<uint64_t>::max() - 1);

      value = FCesiumMetadataValue(std::numeric_limits<int64_t>::max() - 1);
      TestEqual<uint64_t>(
          "int64_t",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(std::numeric_limits<int64_t>::max() - 1));

      value = FCesiumMetadataValue(static_cast<int16_t>(12345));
      TestEqual<uint64_t>(
          "smaller signed integer",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(12345));

      value = FCesiumMetadataValue(static_cast<uint8_t>(255));
      TestEqual<uint64_t>(
          "smaller unsigned integer",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(255));
    });

    It("gets from boolean", [this, defaultValue]() {
      FCesiumMetadataValue value(true);
      TestEqual<uint64_t>(
          "value",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(1));
    });

    It("gets from in-range floating point number", [this, defaultValue]() {
      FCesiumMetadataValue value(1234.56f);
      TestEqual<uint64_t>(
          "float",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(1234));
    });

    It("gets from string", [this, defaultValue]() {
      FCesiumMetadataValue value(std::string_view("1234"));
      TestEqual<uint64_t>(
          "value",
          CesiumMetadataValueAccess::GetUnsignedInteger64(value, defaultValue),
          static_cast<uint64_t>(1234));
    });

    It("returns default value for out-of-range numbers",
       [this, defaultValue]() {
         FCesiumMetadataValue value(-5);
         TestEqual<uint64_t>(
             "negative integer",
             CesiumMetadataValueAccess::GetUnsignedInteger64(
                 value,
                 defaultValue),
             defaultValue);

         value = FCesiumMetadataValue(-59.62f);
         TestEqual<uint64_t>(
             "negative floating-point number",
             CesiumMetadataValueAccess::GetUnsignedInteger64(
                 value,
                 defaultValue),
             defaultValue);

         value = FCesiumMetadataValue(std::numeric_limits<float>::max());
         TestEqual<uint64_t>(
             "positive floating-point number",
             CesiumMetadataValueAccess::GetUnsignedInteger64(
                 value,
                 defaultValue),
             defaultValue);
       });
  });
}

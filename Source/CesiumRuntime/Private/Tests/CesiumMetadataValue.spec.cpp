#include "CesiumMetadataValue.h"
#include "Misc/AutomationTest.h"

#include <vector>

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
      PropertyArrayView<uint8_t> arrayView(std::vector<std::byte>());
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

      //value = FCesiumMetadataValue(PropertyArrayView<bool>());
      TestFalse(
          "array",
          UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, false));
    });
  });

  // Describe("integer", [this]() {
  //  It("converts from integer", [this]() {
  //    TestEqual(
  //        "same type",
  //        CesiumMetadataConversions<int32_t, int32_t>::convert(50, 0),
  //        50);

  //    TestEqual(
  //        "different sign",
  //        CesiumMetadataConversions<int32_t, uint32_t>::convert(50, 0),
  //        50);
  //  });

  //  It("converts from in-range floating point number", [this]() {
  //    TestEqual(
  //        "single-precision",
  //        CesiumMetadataConversions<int32_t, float>::convert(50.125f, 0),
  //        50);

  //    TestEqual(
  //        "double-precision",
  //        CesiumMetadataConversions<int32_t, double>::convert(1234.05678f, 0),
  //        1234);
  //  });
  //});

  // Describe("FString", [this]() {
  //  It("converts from string", [this]() {
  //    TestEqual(
  //        "FString",
  //        CesiumMetadataConversions<FString, FString>::convert(
  //            FString("Hello"),
  //            FString("")),
  //        FString("Hello"));
  //    TestEqual(
  //        "std::string_view",
  //        CesiumMetadataConversions<FString, std::string_view>::convert(
  //            std::string_view("Hello"),
  //            FString("")),
  //        FString("Hello"));
  //  });

  //  It("converts from boolean", [this]() {
  //    TestEqual(
  //        "true",
  //        CesiumMetadataConversions<FString, bool>::convert(true,
  //        FString("")), FString("true"));
  //    TestEqual(
  //        "float",
  //        CesiumMetadataConversions<FString, bool>::convert(false,
  //        FString("")), FString("false"));
  //  });

  //  It("converts from scalar", [this]() {
  //    TestEqual(
  //        "integer",
  //        CesiumMetadataConversions<FString, uint16_t>::convert(
  //            1234,
  //            FString("")),
  //        FString("1234"));
  //    TestEqual(
  //        "float",
  //        CesiumMetadataConversions<FString, float>::convert(
  //            1.2345f,
  //            FString("")),
  //        FString(std::to_string(1.2345f).c_str()));
  //  });

  //  It("uses default value for incompatible types", [this]() {
  //    TestEqual(
  //        "default value for array",
  //        CesiumMetadataConversions<FString, PropertyArrayView<uint8_t>>::
  //            convert(
  //                PropertyArrayView<uint8_t>(std::vector<std::byte>()),
  //                FString("")),
  //        FString(""));
  //  });
  //});
}

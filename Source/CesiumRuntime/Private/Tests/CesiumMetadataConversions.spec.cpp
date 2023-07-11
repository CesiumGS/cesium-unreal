#include "CesiumMetadataConversions.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumMetadataConversionsSpec,
    "Cesium.MetadataConversions",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCesiumMetadataConversionsSpec)

void FCesiumMetadataConversionsSpec::Define() {
  Describe("boolean", [this]() {
    It("converts from boolean", [this]() {
      TestTrue(
          "true",
          CesiumMetadataConversions<bool, bool>::convert(true, false));
      TestFalse(
          "false",
          CesiumMetadataConversions<bool, bool>::convert(false, true));
    });

    It("converts from scalar", [this]() {
      TestTrue(
          "true for nonzero value",
          CesiumMetadataConversions<bool, int8_t>::convert(10, false));
      TestFalse(
          "false for zero value",
          CesiumMetadataConversions<bool, int8_t>::convert(0, true));
    });

    It("converts from string", [this]() {
      std::string_view stringView("true");
      TestTrue(
          "true ('true')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              false));

      stringView = std::string_view("yes");
      TestTrue(
          "true ('yes')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              true));

      stringView = std::string_view("1");
      TestTrue(
          "true ('1')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              true));

      stringView = std::string_view("false");
      TestFalse(
          "false ('false')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              true));

      stringView = std::string_view("no");
      TestFalse(
          "false ('no')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              true));

      stringView = std::string_view("0");
      TestFalse(
          "false ('0')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              true));
    });

    It("uses default value for incompatible types", [this]() {
      TestFalse(
          "default value for vecN",
          CesiumMetadataConversions<bool, glm::vec3>::convert(
              glm::vec3(1, 2, 3),
              false));
      TestFalse(
          "default value for matN",
          CesiumMetadataConversions<bool, glm::mat2>::convert(
              glm::mat2(),
              false));
      TestFalse(
          "default value for array",
          CesiumMetadataConversions<bool, PropertyArrayView<int8_t>>::convert(
              PropertyArrayView<int8_t>(),
              false));
    });
  });

  Describe("integer", [this]() {
    It("converts from integer", [this]() {
      TestEqual(
          "same type",
          CesiumMetadataConversions<int32_t, int32_t>::convert(50, 0),
          50);

      TestEqual(
          "different sign",
          CesiumMetadataConversions<int32_t, uint32_t>::convert(50, 0),
          50);
    });

    It("converts from in-range floating point number", [this]() {
      TestEqual(
          "single-precision",
          CesiumMetadataConversions<int32_t, float>::convert(50.125f, 0),
          50);

      TestEqual(
          "double-precision",
          CesiumMetadataConversions<int32_t, double>::convert(1234.05678f, 0),
          1234);
    });
  });

  Describe("FString", [this]() {
    It("converts from string", [this]() {
      TestEqual(
          "FString",
          CesiumMetadataConversions<FString, FString>::convert(
              FString("Hello"),
              FString("")),
          FString("Hello"));
      TestEqual(
          "std::string_view",
          CesiumMetadataConversions<FString, std::string_view>::convert(
              std::string_view("Hello"),
              FString("")),
          FString("Hello"));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "true",
          CesiumMetadataConversions<FString, bool>::convert(true, FString("")),
          FString("true"));
      TestEqual(
          "float",
          CesiumMetadataConversions<FString, bool>::convert(false, FString("")),
          FString("false"));
    });

    It("converts from scalar", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FString, uint16_t>::convert(
              1234,
              FString("")),
          FString("1234"));
      TestEqual(
          "float",
          CesiumMetadataConversions<FString, float>::convert(
              1.2345f,
              FString("")),
          FString(std::to_string(1.2345f).c_str()));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "default value for array",
          CesiumMetadataConversions<FString, PropertyArrayView<uint8_t>>::
              convert(PropertyArrayView<uint8_t>(), FString("")),
          FString(""));
    });
  });
}

#include "CesiumMetadataConversions.h"
#include "Misc/AutomationTest.h"

#include <limits>

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
          "vecN",
          CesiumMetadataConversions<bool, glm::vec3>::convert(
              glm::vec3(1, 2, 3),
              false));
      TestFalse(
          "matN",
          CesiumMetadataConversions<bool, glm::mat2>::convert(
              glm::mat2(),
              false));
      TestFalse(
          "array",
          CesiumMetadataConversions<bool, PropertyArrayView<bool>>::convert(
              PropertyArrayView<bool>(),
              false));
    });
  });

  Describe("integer", [this]() {
    It("converts from in-range integer", [this]() {
      TestEqual(
          "same type",
          CesiumMetadataConversions<int32_t, int32_t>::convert(50, 0),
          50);

      TestEqual(
          "different size",
          CesiumMetadataConversions<int32_t, int64_t>::convert(50, 0),
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

    It("converts from boolean", [this]() {
      TestEqual(
          "true",
          CesiumMetadataConversions<int32_t, bool>::convert(true, -1),
          1);

      TestEqual(
          "false",
          CesiumMetadataConversions<int32_t, bool>::convert(false, -1),
          0);
    });

    It("converts from string", [this]() {
      std::string_view value("-123");
      TestEqual(
          "integer string",
          CesiumMetadataConversions<int32_t, std::string_view>::convert(
              value,
              0),
          -123);

      value = std::string_view("123.456");
      TestEqual(
          "double string",
          CesiumMetadataConversions<int32_t, std::string_view>::convert(
              value,
              0),
          123);
    });

    It("uses default value for out-of-range numbers", [this]() {
      TestEqual(
          "out-of-range unsigned int",
          CesiumMetadataConversions<int32_t, uint32_t>::convert(
              std::numeric_limits<uint32_t>::max(),
              0),
          0);
      TestEqual(
          "out-of-range signed int",
          CesiumMetadataConversions<int32_t, int64_t>::convert(
              std::numeric_limits<int64_t>::min(),
              0),
          0);
      TestEqual(
          "out-of-range float",
          CesiumMetadataConversions<uint8_t, float>::convert(1234.56f, 0),
          0);
      TestEqual(
          "out-of-range double",
          CesiumMetadataConversions<int32_t, double>::convert(
              std::numeric_limits<double>::max(),
              0),
          0);
    });

    It("uses default value for invalid strings", [this]() {
      TestEqual(
          "out-of-range number",
          CesiumMetadataConversions<int8_t, std::string_view>::convert(
              std::string_view("-255"),
              0),
          0);
      TestEqual(
          "mixed number and non-number input",
          CesiumMetadataConversions<int8_t, std::string_view>::convert(
              std::string_view("10 hello"),
              0),
          0);
      TestEqual(
          "non-number input",
          CesiumMetadataConversions<int8_t, std::string_view>::convert(
              std::string_view("not a number"),
              0),
          0);
      TestEqual(
          "empty input",
          CesiumMetadataConversions<int8_t, std::string_view>::convert(
              std::string_view(),
              0),
          0);
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "vecN",
          CesiumMetadataConversions<int32_t, glm::ivec3>::convert(
              glm::ivec3(1, 2, 3),
              0),
          0);
      TestEqual(
          "matN",
          CesiumMetadataConversions<int32_t, glm::imat2x2>::convert(
              glm::imat2x2(),
              0),
          0);

      PropertyArrayView<int32_t> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<int32_t, PropertyArrayView<int32_t>>::
              convert(arrayView, 0),
          0);
    });
  });

  Describe("float", [this]() {
    It("converts from in-range floating point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<float, float>::convert(123.45f, 0.0f),
          123.45f);

      TestEqual(
          "double",
          CesiumMetadataConversions<float, double>::convert(123.45, 0.0f),
          static_cast<float>(123.45));
    });

    It("converts from integer", [this]() {
      int32_t int32Value = -1234;
      TestEqual(
          "int32_t",
          CesiumMetadataConversions<float, int32_t>::convert(int32Value, 0.0f),
          static_cast<float>(int32Value));

      uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
      TestEqual(
          "uint64_t",
          CesiumMetadataConversions<float, uint64_t>::convert(
              uint64Value,
              0.0f),
          static_cast<float>(uint64Value));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "true",
          CesiumMetadataConversions<float, bool>::convert(true, -1.0f),
          1.0f);
      TestEqual(
          "false",
          CesiumMetadataConversions<float, bool>::convert(false, -1.0f),
          0.0f);
    });

    It("converts from string", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view("123.456"),
              0),
          static_cast<float>(123.456));
    });

    It("uses default value for invalid strings", [this]() {
      TestEqual(
          "out-of-range number",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view(
                  std::to_string(std::numeric_limits<double>::max())),
              0.0f),
          0.0f);
      TestEqual(
          "mixed number and non-number input",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view("10.00f hello"),
              0.0f),
          0.0f);
      TestEqual(
          "non-number input",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view("not a number"),
              0.0f),
          0.0f);
      TestEqual(
          "empty input",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view(),
              0.0f),
          0.0f);
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "vecN",
          CesiumMetadataConversions<float, glm::vec3>::convert(
              glm::vec3(1, 2, 3),
              0.0f),
          0.0f);
      TestEqual(
          "matN",
          CesiumMetadataConversions<float, glm::mat2>::convert(
              glm::mat2(),
              0.0f),
          0.0f);

      PropertyArrayView<float> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<float, PropertyArrayView<float>>::convert(
              arrayView,
              0.0f),
          0.0f);
    });
  });

  Describe("double", [this]() {
    It("converts from floating point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<double, float>::convert(123.45f, 0.0),
          static_cast<double>(123.45f));

      TestEqual(
          "double",
          CesiumMetadataConversions<double, double>::convert(123.45, 0.0),
          123.45);
    });

    It("converts from integer", [this]() {
      int32_t int32Value = -1234;
      TestEqual(
          "int32_t",
          CesiumMetadataConversions<double, int32_t>::convert(int32Value, 0.0),
          static_cast<double>(int32Value));

      uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
      TestEqual(
          "uint64_t",
          CesiumMetadataConversions<double, uint64_t>::convert(
              uint64Value,
              0.0),
          static_cast<double>(uint64Value));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "true",
          CesiumMetadataConversions<double, bool>::convert(true, -1.0),
          1.0);
      TestEqual(
          "false",
          CesiumMetadataConversions<double, bool>::convert(false, -1.0),
          0.0);
    });

    It("converts from string", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("123.456"),
              0),
          123.456);
    });

    It("uses default value for invalid strings", [this]() {
      TestEqual(
          "out-of-range number",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view(
                  std::to_string(std::numeric_limits<double>::max()) + "1"),
              0.0),
          0.0);
      TestEqual(
          "mixed number and non-number input",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("10.00 hello"),
              0.0f),
          0.0);
      TestEqual(
          "non-number input",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("not a number"),
              0.0),
          0.0);
      TestEqual(
          "empty input",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view(),
              0.0),
          0.0);
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "vecN",
          CesiumMetadataConversions<double, glm::dvec3>::convert(
              glm::dvec3(1, 2, 3),
              0.0),
          0.0);
      TestEqual(
          "matN",
          CesiumMetadataConversions<double, glm::dmat2>::convert(
              glm::dmat2(),
              0.0),
          0.0);

      PropertyArrayView<double> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<double, PropertyArrayView<double>>::convert(
              arrayView,
              0.0),
          0.0);
    });
  });

  // TODO: see if this works, and if so, templatize the other vecNs
  Describe("FVector3f", [this]() {
    It("converts from glm::vec3", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector3f, glm::vec3>::convert(
              glm::vec3(1.0f, 2.3f, 4.56f),
              FVector3f(0.0f)),
          FVector3f(1.0f, 2.3f, 4.56f));
    });
  });

  Describe("FString", [this]() {
    It("converts from std::string_view", [this]() {
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
          "false",
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

    It("converts from vecN", [this]() {
      TestEqual(
          "vec2",
          CesiumMetadataConversions<FString, glm::u8vec2>::convert(
              glm::u8vec2(1, 2),
              FString("")),
          FString("X=1 Y=2"));

      std::string expectedVec3 = "X=" + std::to_string(4.5f) +
                                 " Y=" + std::to_string(3.21f) +
                                 " Z=" + std::to_string(123.0f);
      TestEqual(
          "vec3",
          CesiumMetadataConversions<FString, glm::vec3>::convert(
              glm::vec3(4.5f, 3.21f, 123.0f),
              FString("")),
          FString(expectedVec3.c_str()));
    });

    It("uses default value for incompatible types", [this]() {
      PropertyArrayView<std::string_view> arrayView;
      TestEqual(
          "rray",
          CesiumMetadataConversions<
              FString,
              PropertyArrayView<std::string_view>>::
              convert(arrayView, FString("")),
          FString(""));
    });
  });
}

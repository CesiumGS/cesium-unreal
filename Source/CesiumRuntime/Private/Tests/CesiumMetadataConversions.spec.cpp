#include "CesiumMetadataConversions.h"
#include "CesiumTestHelpers.h"
#include "Misc/AutomationTest.h"

#include <limits>

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumMetadataConversionsSpec,
    "Cesium.Unit.MetadataConversions",
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
              false));

      stringView = std::string_view("1");
      TestTrue(
          "true ('1')",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              false));

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

    It("uses default value for incompatible strings", [this]() {
      std::string_view stringView("11");
      TestFalse(
          "invalid number",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              false));

      stringView = std::string_view("this is true");
      TestFalse(
          "invalid word",
          CesiumMetadataConversions<bool, std::string_view>::convert(
              stringView,
              false));
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
          "float",
          CesiumMetadataConversions<int32_t, float>::convert(50.125f, 0),
          50);

      TestEqual(
          "double",
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
          "32-bit",
          CesiumMetadataConversions<float, int32_t>::convert(int32Value, 0.0f),
          static_cast<float>(int32Value));

      uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
      TestEqual(
          "64-bit",
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
          "integer value",
          CesiumMetadataConversions<float, std::string_view>::convert(
              std::string_view("123"),
              0),
          static_cast<float>(123));
      TestEqual(
          "floating-point value",
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
      uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
      TestEqual(
          "64-bit",
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
          "integer value",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("123"),
              0),
          static_cast<double>(123));
      TestEqual(
          "floating-point value",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("123.456"),
              0),
          123.456);
    });

    It("uses default value for invalid strings", [this]() {
      TestEqual(
          "mixed number and non-number input",
          CesiumMetadataConversions<double, std::string_view>::convert(
              std::string_view("10.00 hello"),
              0.0),
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
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
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

  Describe("FIntPoint", [this]() {
    It("converts from glm::ivec2", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntPoint, glm::ivec2>::convert(
              glm::ivec2(-1, 2),
              FIntPoint(0)),
          FIntPoint(-1, 2));
    });

    It("converts from other vec2 types", [this]() {
      TestEqual(
          "uint8_t",
          CesiumMetadataConversions<FIntPoint, glm::u8vec2>::convert(
              glm::u8vec2(12, 76),
              FIntPoint(0)),
          FIntPoint(12, 76));
      TestEqual(
          "int64_t",
          CesiumMetadataConversions<FIntPoint, glm::i64vec2>::convert(
              glm::i64vec2(-28, 44),
              FIntPoint(0)),
          FIntPoint(-28, 44));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntPoint, glm::dvec2>::convert(
              glm::dvec2(-3.5, 1.23456),
              FIntPoint(0)),
          FIntPoint(-3, 1));
    });

    It("converts from vec3 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntPoint, glm::ivec3>::convert(
              glm::ivec3(-84, 5, 25),
              FIntPoint(0)),
          FIntPoint(-84, 5));
      TestEqual(
          "float",
          CesiumMetadataConversions<FIntPoint, glm::vec3>::convert(
              glm::vec3(4.5f, -2.345f, 81.0f),
              FIntPoint(0)),
          FIntPoint(4, -2));
    });

    It("converts from vec4 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntPoint, glm::i16vec4>::convert(
              glm::i16vec4(-42, 278, 23, 1),
              FIntPoint(0)),
          FIntPoint(-42, 278));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntPoint, glm::dvec4>::convert(
              glm::dvec4(-3.5, 1.23456, 26.0, 8.0),
              FIntPoint(0)),
          FIntPoint(-3, 1));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntPoint, bool>::convert(
              true,
              FIntPoint(-1)),
          FIntPoint(1));
    });

    It("converts from in-range integer", [this]() {
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FIntPoint, int32_t>::convert(
              -12345,
              FIntPoint(0)),
          FIntPoint(-12345));

      TestEqual(
          "64-bit",
          CesiumMetadataConversions<FIntPoint, int64_t>::convert(
              static_cast<int64_t>(12345),
              FIntPoint(0)),
          FIntPoint(static_cast<int32_t>(12345)));
    });

    It("converts from in-range floating-point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<FIntPoint, float>::convert(
              1234.56f,
              FIntPoint(0)),
          FIntPoint(1234));

      TestEqual(
          "double",
          CesiumMetadataConversions<FIntPoint, double>::convert(
              789.12,
              FIntPoint(0)),
          FIntPoint(789));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntPoint, std::string_view>::convert(
              str,
              FIntPoint(0)),
          FIntPoint(1, 2));
    });

    It("uses default value for out-of-range scalars", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntPoint, uint64_t>::convert(
              static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
              FIntPoint(0)),
          FIntPoint(0));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntPoint, double>::convert(
              std::numeric_limits<double>::max(),
              FIntPoint(0)),
          FIntPoint(0));
    });

    It("uses default value for vecNs with out-of-range components", [this]() {
      TestEqual(
          "vec2",
          CesiumMetadataConversions<FIntPoint, glm::dvec2>::convert(
              glm::dvec2(1.0, std::numeric_limits<double>::max()),
              FIntPoint(0)),
          FIntPoint(0));
      TestEqual(
          "vec3",
          CesiumMetadataConversions<FIntPoint, glm::vec3>::convert(
              glm::vec3(1.0, std::numeric_limits<float>::max(), -1.0),
              FIntPoint(0)),
          FIntPoint(0));
      TestEqual(
          "vec4",
          CesiumMetadataConversions<FIntPoint, glm::u64vec4>::convert(
              glm::u64vec4(std::numeric_limits<uint64_t>::max(), 1, 1, 1),
              FIntPoint(0)),
          FIntPoint(0));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FIntPoint, std::string_view>::convert(
              str,
              FIntPoint(0)),
          FIntPoint(0));

      str = std::string_view("R=0.5 G=0.5");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FIntPoint, std::string_view>::convert(
              str,
              FIntPoint(0)),
          FIntPoint(0));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FIntPoint, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FIntPoint(0)),
          FIntPoint(0));

      PropertyArrayView<glm::ivec2> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FIntPoint, PropertyArrayView<glm::ivec2>>::
              convert(arrayView, FIntPoint(0)),
          FIntPoint(0));
    });
  });

  Describe("FVector2D", [this]() {
    It("converts from glm::dvec2", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector2D, glm::dvec2>::convert(
              glm::dvec2(-1.0, 2.0),
              FVector2D(0.0)),
          FVector2D(-1.0, 2.0));
    });

    It("converts from other vec2 types", [this]() {
      TestEqual(
          "int32_t",
          CesiumMetadataConversions<FVector2D, glm::ivec2>::convert(
              glm::ivec2(12, 76),
              FVector2D(0.0)),
          FVector2D(static_cast<double>(12), static_cast<double>(76)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector2D, glm::vec2>::convert(
              glm::vec2(-3.5f, 1.234f),
              FVector2D(0.0)),
          FVector2D(static_cast<double>(-3.5f), static_cast<double>(1.234f)));
    });

    It("converts from vec3 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector2D, glm::ivec3>::convert(
              glm::ivec3(-84, 5, 25),
              FVector2D(0.0)),
          FVector2D(static_cast<double>(-84), static_cast<double>(5)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector2D, glm::vec3>::convert(
              glm::vec3(4.5f, -2.345f, 81.0f),
              FVector2D(0)),
          FVector2D(static_cast<double>(4.5f), static_cast<double>(-2.345f)));
    });

    It("converts from vec4 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector2D, glm::i16vec4>::convert(
              glm::i16vec4(-42, 278, 23, 1),
              FVector2D(0.0)),
          FVector2D(static_cast<double>(-42), static_cast<double>(278)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector2D, glm::vec4>::convert(
              glm::vec4(4.5f, 2.345f, 8.1f, 1038.0f),
              FVector2D(0.0)),
          FVector2D(static_cast<double>(4.5f), static_cast<double>(2.345f)));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector2D, bool>::convert(
              true,
              FVector2D(-1.0)),
          FVector2D(1.0));
    });

    It("converts from integer", [this]() {
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FVector2D, int32_t>::convert(
              -12345,
              FVector2D(0.0)),
          FVector2D(static_cast<double>(-12345)));
    });

    It("converts from in-range floating-point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector2D, float>::convert(
              1234.56f,
              FVector2D(0.0)),
          FVector2D(static_cast<double>(1234.56f)));

      TestEqual(
          "double",
          CesiumMetadataConversions<FVector2D, double>::convert(
              789.12,
              FVector2D(0.0)),
          FVector2D(789.12));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5");
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector2D, std::string_view>::convert(
              str,
              FVector2D(0.0)),
          FVector2D(1.5, 2.5));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FVector2D, std::string_view>::convert(
              str,
              FVector2D(0.0)),
          FVector2D(0.0));

      str = std::string_view("R=0.5 G=0.5");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FVector2D, std::string_view>::convert(
              str,
              FVector2D(0.0)),
          FVector2D(0.0));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FVector2D, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FVector2D(0.0)),
          FVector2D(0.0));

      PropertyArrayView<glm::dvec2> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FVector2D, PropertyArrayView<glm::dvec2>>::
              convert(arrayView, FVector2D(0.0)),
          FVector2D(0.0));
    });
  });

  Describe("FIntVector", [this]() {
    It("converts from glm::ivec3", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntVector, glm::ivec3>::convert(
              glm::ivec3(-1, 2, 4),
              FIntVector(0)),
          FIntVector(-1, 2, 4));
    });

    It("converts from other vec3 types", [this]() {
      TestEqual(
          "uint8_t",
          CesiumMetadataConversions<FIntVector, glm::u8vec3>::convert(
              glm::u8vec3(12, 76, 23),
              FIntVector(0)),
          FIntVector(12, 76, 23));
      TestEqual(
          "int64_t",
          CesiumMetadataConversions<FIntVector, glm::i64vec3>::convert(
              glm::i64vec3(-28, 44, -7),
              FIntVector(0)),
          FIntVector(-28, 44, -7));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntVector, glm::dvec3>::convert(
              glm::dvec3(-3.5, 1.23456, 82.9),
              FIntVector(0)),
          FIntVector(-3, 1, 82));
    });

    It("converts from vec2 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntVector, glm::ivec2>::convert(
              glm::ivec2(-84, 5),
              FIntVector(0)),
          FIntVector(-84, 5, 0));
      TestEqual(
          "float",
          CesiumMetadataConversions<FIntVector, glm::vec2>::convert(
              glm::vec2(4.5f, -2.345f),
              FIntVector(0)),
          FIntVector(4, -2, 0));
    });

    It("converts from vec4 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntVector, glm::i16vec4>::convert(
              glm::i16vec4(-42, 278, 23, 1),
              FIntVector(0)),
          FIntVector(-42, 278, 23));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntVector, glm::dvec4>::convert(
              glm::dvec4(-3.5, 1.23456, 26.0, 8.0),
              FIntVector(0)),
          FIntVector(-3, 1, 26));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntVector, bool>::convert(
              true,
              FIntVector(-1)),
          FIntVector(1));
    });

    It("converts from in-range integer", [this]() {
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FIntVector, int32_t>::convert(
              -12345,
              FIntVector(0)),
          FIntVector(-12345));

      TestEqual(
          "64-bit",
          CesiumMetadataConversions<FIntVector, int64_t>::convert(
              static_cast<int64_t>(12345),
              FIntVector(0)),
          FIntVector(static_cast<int32_t>(12345)));
    });

    It("converts from in-range floating-point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<FIntVector, float>::convert(
              1234.56f,
              FIntVector(0)),
          FIntVector(1234));

      TestEqual(
          "double",
          CesiumMetadataConversions<FIntVector, double>::convert(
              789.12,
              FIntVector(0)),
          FIntVector(789));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2 Z=4");
      TestEqual(
          "value",
          CesiumMetadataConversions<FIntVector, std::string_view>::convert(
              str,
              FIntVector(0)),
          FIntVector(1, 2, 4));
    });

    It("uses default value for out-of-range scalars", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FIntVector, uint64_t>::convert(
              static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()),
              FIntVector(0)),
          FIntVector(0));
      TestEqual(
          "double",
          CesiumMetadataConversions<FIntVector, double>::convert(
              std::numeric_limits<double>::max(),
              FIntVector(0)),
          FIntVector(0));
    });

    It("uses default value for vecNs with out-of-range components", [this]() {
      TestEqual(
          "vec2",
          CesiumMetadataConversions<FIntVector, glm::dvec2>::convert(
              glm::dvec2(1.0, std::numeric_limits<double>::max()),
              FIntVector(0)),
          FIntVector(0));
      TestEqual(
          "vec3",
          CesiumMetadataConversions<FIntVector, glm::vec3>::convert(
              glm::vec3(1.0, std::numeric_limits<float>::max(), -1.0),
              FIntVector(0)),
          FIntVector(0));
      TestEqual(
          "vec4",
          CesiumMetadataConversions<FIntVector, glm::u64vec4>::convert(
              glm::u64vec4(std::numeric_limits<uint64_t>::max(), 1, 1, 1),
              FIntVector(0)),
          FIntVector(0));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FIntVector, std::string_view>::convert(
              str,
              FIntVector(0)),
          FIntVector(0));

      str = std::string_view("R=0.5 G=0.5 B=1");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FIntVector, std::string_view>::convert(
              str,
              FIntVector(0)),
          FIntVector(0));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FIntVector, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FIntVector(0)),
          FIntVector(0));

      PropertyArrayView<glm::ivec3> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FIntVector, PropertyArrayView<glm::ivec3>>::
              convert(arrayView, FIntVector(0)),
          FIntVector(0));
    });
  });

  Describe("FVector3f", [this]() {
    It("converts from glm::vec3", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector3f, glm::vec3>::convert(
              glm::vec3(1.0f, 2.3f, 4.56f),
              FVector3f(0.0f)),
          FVector3f(1.0f, 2.3f, 4.56f));
    });

    It("converts from other vec3 types", [this]() {
      TestEqual(
          "int8_t",
          CesiumMetadataConversions<FVector3f, glm::i8vec3>::convert(
              glm::i8vec3(-11, 2, 53),
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(-11),
              static_cast<float>(2),
              static_cast<float>(53)));
      TestEqual(
          "uint32_t",
          CesiumMetadataConversions<FVector3f, glm::uvec3>::convert(
              glm::uvec3(0, 44, 160),
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(0),
              static_cast<float>(44),
              static_cast<float>(160)));
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector3f, glm::dvec3>::convert(
              glm::dvec3(-3.5, 1.23456, 88.08),
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(-3.5),
              static_cast<float>(1.23456),
              static_cast<float>(88.08)));
    });

    It("converts from vec2 types", [this]() {
      glm::ivec2 intVec2(-84, 5);
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector3f, glm::ivec2>::convert(
              intVec2,
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(intVec2[0]),
              static_cast<float>(intVec2[1]),
              0.0f));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector3f, glm::vec2>::convert(
              glm::vec2(4.5f, 2.345f),
              FVector3f(0.0f)),
          FVector3f(4.5f, 2.345f, 0.0f));

      glm::dvec2 doubleVec2(-3.5, 1.23456);
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector3f, glm::dvec2>::convert(
              doubleVec2,
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(doubleVec2[0]),
              static_cast<float>(doubleVec2[1]),
              0.0f));
    });

    It("converts from vec4 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector3f, glm::i16vec4>::convert(
              glm::i16vec4(-42, 278, 23, 1),
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(-42),
              static_cast<float>(278),
              static_cast<float>(23)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector3f, glm::vec4>::convert(
              glm::vec4(4.5f, 2.345f, 8.1f, 1038.0f),
              FVector3f(0.0f)),
          FVector3f(4.5f, 2.345f, 8.1f));
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector3f, glm::dvec4>::convert(
              glm::dvec4(-3.5, 1.23456, 26.0, 8.0),
              FVector3f(0.0f)),
          FVector3f(
              static_cast<float>(-3.5),
              static_cast<float>(1.23456),
              static_cast<float>(26.0)));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector3f, bool>::convert(
              true,
              FVector3f(-1.0f)),
          FVector3f(1.0f));
    });

    It("converts from integer", [this]() {
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FVector3f, uint32_t>::convert(
              12345,
              FVector3f(0.0f)),
          FVector3f(static_cast<float>(12345)));

      TestEqual(
          "64-bit",
          CesiumMetadataConversions<FVector3f, int64_t>::convert(
              -12345,
              FVector3f(0.0f)),
          FVector3f(static_cast<float>(-12345)));
    });

    It("converts from in-range floating-point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector3f, float>::convert(
              1234.56f,
              FVector3f(0.0f)),
          FVector3f(1234.56f));

      // Seems like calling static_cast<float>(789.12) directly results in a
      // different value... use a variable to guarantee correctness.
      double value = 789.12;
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector3f, double>::convert(
              value,
              FVector3f(0.0f)),
          FVector3f(static_cast<float>(value)));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2 Z=3");
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector3f, std::string_view>::convert(
              str,
              FVector3f(0.0f)),
          FVector3f(1.0f, 2.0f, 3.0f));
    });

    It("uses default value for out-of-range scalars", [this]() {
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector3f, double>::convert(
              std::numeric_limits<double>::max(),
              FVector3f(0.0f)),
          FVector3f(0.0f));
    });

    It("uses default value for vecNs with out-of-range components", [this]() {
      TestEqual(
          "vec2",
          CesiumMetadataConversions<FVector3f, glm::dvec2>::convert(
              glm::dvec2(1.0, std::numeric_limits<double>::max()),
              FVector3f(0.0f)),
          FVector3f(0.0f));
      TestEqual(
          "vec3",
          CesiumMetadataConversions<FVector3f, glm::dvec3>::convert(
              glm::dvec3(1.0, -1.0, std::numeric_limits<double>::max()),
              FVector3f(0.0f)),
          FVector3f(0.0f));
      TestEqual(
          "vec4",
          CesiumMetadataConversions<FVector3f, glm::dvec4>::convert(
              glm::dvec4(1.0, -1.0, std::numeric_limits<double>::max(), 1.0),
              FVector3f(0.0f)),
          FVector3f(0.0f));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FVector3f, std::string_view>::convert(
              str,
              FVector3f(0.0f)),
          FVector3f(0.0f));

      str = std::string_view("R=0.5 G=0.5 B=0.5");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FVector3f, std::string_view>::convert(
              str,
              FVector3f(0.0f)),
          FVector3f(0.0f));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FVector3f, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FVector3f(0.0f)),
          FVector3f(0.0f));

      PropertyArrayView<glm::vec3> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FVector3f, PropertyArrayView<glm::vec3>>::
              convert(arrayView, FVector3f(0.0f)),
          FVector3f(0.0f));
    });
  });

  Describe("FVector", [this]() {
    It("converts from glm::dvec3", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector, glm::dvec3>::convert(
              glm::dvec3(1.0, 2.3, 4.56),
              FVector(0.0)),
          FVector(1.0, 2.3, 4.56));
    });

    It("converts from other vec3 types", [this]() {
      TestEqual(
          "uint32_t",
          CesiumMetadataConversions<FVector, glm::uvec3>::convert(
              glm::uvec3(0, 44, 160),
              FVector(0.0)),
          FVector(
              static_cast<double>(0),
              static_cast<double>(44),
              static_cast<double>(160)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector, glm::vec3>::convert(
              glm::vec3(-3.5f, 1.23456f, 88.08f),
              FVector(0.0)),
          FVector(
              static_cast<double>(-3.5f),
              static_cast<double>(1.23456f),
              static_cast<double>(88.08f)));
    });

    It("converts from vec2 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector, glm::ivec2>::convert(
              glm::ivec2(-84, 5),
              FVector(0.0)),
          FVector(static_cast<double>(-84), static_cast<double>(5), 0.0));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector, glm::vec2>::convert(
              glm::vec2(4.5f, 2.345f),
              FVector(0.0)),
          FVector(static_cast<double>(4.5f), static_cast<double>(2.345f), 0.0));
    });

    It("converts from vec4 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector, glm::i16vec4>::convert(
              glm::i16vec4(-42, 278, 23, 1),
              FVector(0.0)),
          FVector(
              static_cast<double>(-42),
              static_cast<double>(278),
              static_cast<double>(23)));
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector, glm::dvec4>::convert(
              glm::dvec4(4.5, 2.34, 8.1, 1038.0),
              FVector(0.0)),
          FVector(4.5, 2.34, 8.1));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector, bool>::convert(
              true,
              FVector(-1.0)),
          FVector(1.0));
    });

    It("converts from integer", [this]() {
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FVector, uint32_t>::convert(
              12345,
              FVector(0.0)),
          FVector(static_cast<double>(12345)));

      TestEqual(
          "64-bit",
          CesiumMetadataConversions<FVector, int64_t>::convert(
              -12345,
              FVector(0.0)),
          FVector(static_cast<double>(-12345)));
    });

    It("converts from floating-point number", [this]() {
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector, float>::convert(
              1234.56f,
              FVector(0.0)),
          FVector(static_cast<double>(1234.56f)));

      TestEqual(
          "double",
          CesiumMetadataConversions<FVector, double>::convert(
              4.56,
              FVector(0.0)),
          FVector(4.56));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5 Z=3.5");
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector, std::string_view>::convert(
              str,
              FVector(0.0)),
          FVector(1.5f, 2.5f, 3.5f));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FVector, std::string_view>::convert(
              str,
              FVector(0.0)),
          FVector(0.0));

      str = std::string_view("R=0.5 G=0.5 B=0.5");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FVector, std::string_view>::convert(
              str,
              FVector(0.0)),
          FVector(0.0));
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FVector, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FVector(0.0)),
          FVector(0.0));

      PropertyArrayView<glm::dvec3> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FVector, PropertyArrayView<glm::dvec3>>::
              convert(arrayView, FVector(0.0)),
          FVector(0.0));
    });
  });

  Describe("FVector4", [this]() {
    It("converts from glm::dvec4", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector, glm::dvec4>::convert(
              glm::dvec4(1.0, 2.3, 4.56, 7.89),
              FVector4::Zero()),
          FVector4(1.0, 2.3, 4.56, 7.89));
    });

    It("converts from other vec4 types", [this]() {
      TestEqual(
          "uint32_t",
          CesiumMetadataConversions<FVector4, glm::uvec4>::convert(
              glm::uvec4(0, 44, 160, 1),
              FVector4::Zero()),
          FVector4(
              static_cast<double>(0),
              static_cast<double>(44),
              static_cast<double>(160),
              static_cast<double>(1)));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector4, glm::vec4>::convert(
              glm::vec4(-3.5f, 1.23456f, 88.08f, 1.0f),
              FVector4::Zero()),
          FVector4(
              static_cast<double>(-3.5f),
              static_cast<double>(1.23456f),
              static_cast<double>(88.08f),
              static_cast<double>(1.0f)));
    });

    It("converts from vec2 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector4, glm::ivec2>::convert(
              glm::ivec2(-84, 5),
              FVector4::Zero()),
          FVector4(static_cast<double>(-84), static_cast<double>(5), 0.0, 0.0));
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector4, glm::vec2>::convert(
              glm::vec2(4.5f, 2.345f),
              FVector4::Zero()),
          FVector4(
              static_cast<double>(4.5f),
              static_cast<double>(2.345f),
              0.0,
              0.0));
    });

    It("converts from vec3 types", [this]() {
      TestEqual(
          "integer",
          CesiumMetadataConversions<FVector4, glm::i16vec3>::convert(
              glm::i16vec3(-42, 278, 23),
              FVector4::Zero()),
          FVector4(
              static_cast<double>(-42),
              static_cast<double>(278),
              static_cast<double>(23),
              0.0));
      TestEqual(
          "double",
          CesiumMetadataConversions<FVector4, glm::dvec3>::convert(
              glm::dvec3(4.5, 2.34, 8.1),
              FVector4::Zero()),
          FVector4(4.5, 2.34, 8.1, 0.0));
    });

    It("converts from boolean", [this]() {
      TestEqual(
          "value",
          CesiumMetadataConversions<FVector4, bool>::convert(
              true,
              FVector4(-1.0, -1.0, -1.0, -1.0)),
          FVector4::One());
    });

    It("converts from integer", [this]() {
      uint32_t uint32Value = static_cast<uint32_t>(12345);
      double expected = static_cast<double>(uint32Value);
      TestEqual(
          "32-bit",
          CesiumMetadataConversions<FVector4, uint32_t>::convert(
              12345,
              FVector4::Zero()),
          FVector4(expected, expected, expected, expected));

      int64_t int64Value = static_cast<int64_t>(-12345);
      expected = static_cast<double>(int64Value);
      TestEqual(
          "64-bit",
          CesiumMetadataConversions<FVector4, int64_t>::convert(
              int64Value,
              FVector4::Zero()),
          FVector4(expected, expected, expected, expected));
    });

    It("converts from floating-point number", [this]() {
      float value = 1234.56f;
      double expected = static_cast<double>(1234.56f);
      TestEqual(
          "float",
          CesiumMetadataConversions<FVector4, float>::convert(
              1234.56f,
              FVector4::Zero()),
          FVector4(expected, expected, expected, expected));

      TestEqual(
          "double",
          CesiumMetadataConversions<FVector4, double>::convert(
              4.56,
              FVector4::Zero()),
          FVector4(4.56, 4.56, 4.56, 4.56));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5 Z=3.5 W=4.5");
      TestEqual(
          "with W component",
          CesiumMetadataConversions<FVector4, std::string_view>::convert(
              str,
              FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 4.5));

      str = std::string_view("X=1.5 Y=2.5 Z=3.5");
      TestEqual(
          "without W component",
          CesiumMetadataConversions<FVector4, std::string_view>::convert(
              str,
              FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 1.0));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          CesiumMetadataConversions<FVector4, std::string_view>::convert(
              str,
              FVector4::Zero()),
          FVector4::Zero());

      str = std::string_view("R=0.5 G=0.5 B=0.5 A=1.0");
      TestEqual(
          "bad format",
          CesiumMetadataConversions<FVector4, std::string_view>::convert(
              str,
              FVector4::Zero()),
          FVector4::Zero());
    });

    It("uses default value for incompatible types", [this]() {
      TestEqual(
          "matN",
          CesiumMetadataConversions<FVector, glm::dmat2>::convert(
              glm::dmat2(1.0, 2.0, 3.0, 4.0),
              FVector4::Zero()),
          FVector4::Zero());

      PropertyArrayView<glm::dvec4> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FVector, PropertyArrayView<glm::dvec4>>::
              convert(arrayView, FVector4::Zero()),
          FVector4::Zero());
    });
  });

  Describe("FMatrix", [this]() {
    It("converts from glm::dmat4", [this]() {
      // clang-format off
      glm::dmat4 input = glm::dmat4(
          1.0, 2.0, 3.0, 4.0,
          5.0, 6.0, 7.0, 8.0,
          0.0, 1.0, 0.0, 1.0,
          1.0, 0.0, 0.0, 1.0);
      // clang-format on
      input = glm::transpose(input);

      FMatrix expected(
          FPlane4d(1.0, 2.0, 3.0, 4.0),
          FPlane4d(5.0, 6.0, 7.0, 8.0),
          FPlane4d(0.0, 1.0, 0.0, 1.0),
          FPlane4d(1.0, 0.0, 0.0, 1.0));
      TestEqual(
          "value",
          CesiumMetadataConversions<FMatrix, glm::dmat4>::convert(
              input,
              FMatrix::Identity),
          expected);
    });

    It("converts from mat2", [this]() {
      // clang-format off
      glm::dmat2 input = glm::dmat2(
          1.0, 2.0,
          3.0, 4.0);
      // clang-format on
      input = glm::transpose(input);

      FMatrix expected(
          FPlane4d(1.0, 2.0, 0.0, 0.0),
          FPlane4d(3.0, 4.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0));
      TestEqual(
          "value",
          CesiumMetadataConversions<FMatrix, glm::dmat2>::convert(
              input,
              FMatrix::Identity),
          expected);
    });

    It("converts from mat3", [this]() {
      // clang-format off
      glm::dmat3 input = glm::dmat3(
          1.0, 2.0, 3.0,
          4.0, 5.0, 6.0,
          7.0, 8.0, 9.0);
      // clang-format on
      input = glm::transpose(input);

      FMatrix expected(
          FPlane4d(1.0, 2.0, 3.0, 0.0),
          FPlane4d(4.0, 5.0, 6.0, 0.0),
          FPlane4d(7.0, 8.0, 9.0, 0.0),
          FPlane4d(0.0, 0.0, 0.0, 0.0));
      TestEqual(
          "value",
          CesiumMetadataConversions<FMatrix, glm::dmat3>::convert(
              input,
              FMatrix::Identity),
          expected);
    });

    It("converts from boolean", [this]() {
      FMatrix zeroMatrix(ZeroPlane, ZeroPlane, ZeroPlane, ZeroPlane);

      TestEqual(
          "true",
          CesiumMetadataConversions<FMatrix, bool>::convert(true, zeroMatrix),
          FMatrix::Identity);
      TestEqual(
          "false",
          CesiumMetadataConversions<FMatrix, bool>::convert(
              false,
              FMatrix::Identity),
          zeroMatrix);
    });

    It("converts from scalar", [this]() {
      int32_t integerValue = 10;
      double value = static_cast<double>(integerValue);
      FMatrix expected(
          FPlane4d(value, 0.0, 0.0, 0.0),
          FPlane4d(0.0, value, 0.0, 0.0),
          FPlane4d(0.0, 0.0, value, 0.0),
          FPlane4d(0.0, 0.0, 0.0, value));
      TestEqual(
          "integer",
          CesiumMetadataConversions<FMatrix, int32_t>::convert(
              integerValue,
              FMatrix::Identity),
          expected);

      float floatValue = -3.45f;
      value = static_cast<double>(floatValue);
      expected = FMatrix(
          FPlane4d(value, 0.0, 0.0, 0.0),
          FPlane4d(0.0, value, 0.0, 0.0),
          FPlane4d(0.0, 0.0, value, 0.0),
          FPlane4d(0.0, 0.0, 0.0, value));
      TestEqual(
          "float",
          CesiumMetadataConversions<FMatrix, float>::convert(
              floatValue,
              FMatrix::Identity),
          expected);
    });

    It("uses default value for incompatible types", [this]() {
      // This is unsupported because there's no FMatrix::InitFromString method.
      std::string_view str("[0 1 2 3] [4 5 6 7] [8 9 10 11] [12 13 14 15]");
      TestEqual(
          "string",
          CesiumMetadataConversions<FMatrix, std::string_view>::convert(
              str,
              FMatrix::Identity),
          FMatrix::Identity);

      TestEqual(
          "vecN",
          CesiumMetadataConversions<FMatrix, glm::vec3>::convert(
              glm::vec3(1.0f, 2.0f, 3.0f),
              FMatrix::Identity),
          FMatrix::Identity);

      PropertyArrayView<glm::dmat4> arrayView;
      TestEqual(
          "array",
          CesiumMetadataConversions<FMatrix, PropertyArrayView<glm::dmat4>>::
              convert(arrayView, FMatrix::Identity),
          FMatrix::Identity);
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
      std::string expected("X=1 Y=2");
      TestEqual(
          "vec2",
          CesiumMetadataConversions<FString, glm::u8vec2>::convert(
              glm::u8vec2(1, 2),
              FString("")),
          FString(expected.c_str()));

      expected = "X=" + std::to_string(4.5f) + " Y=" + std::to_string(3.21f) +
                 " Z=" + std::to_string(123.0f);
      TestEqual(
          "vec3",
          CesiumMetadataConversions<FString, glm::vec3>::convert(
              glm::vec3(4.5f, 3.21f, 123.0f),
              FString("")),
          FString(expected.c_str()));

      expected = "X=" + std::to_string(1.0f) + " Y=" + std::to_string(2.0f) +
                 " Z=" + std::to_string(3.0f) + " W=" + std::to_string(4.0f);
      TestEqual(
          "vec4",
          CesiumMetadataConversions<FString, glm::vec4>::convert(
              glm::vec4(1.0f, 2.0f, 3.0f, 4.0f),
              FString("")),
          FString(expected.c_str()));
    });

    It("converts from matN", [this]() {
      // clang-format off
      glm::mat2 mat2(
        0.0f, 1.0f,
        2.0f, 3.0f);
      mat2 = glm::transpose(mat2);

      std::string expected =
          "[" + std::to_string(0.0f) + " " + std::to_string(1.0f) + "] " +
          "[" + std::to_string(2.0f) + " " + std::to_string(3.0f) + "]";
      // clang-format on
      TestEqual(
          "mat2",
          CesiumMetadataConversions<FString, glm::mat2>::convert(
              mat2,
              FString("")),
          FString(expected.c_str()));

      // This is written as transposed because glm::transpose only compiles for
      // floating point types.
      // clang-format off
      glm::i8mat3x3 mat3(
        -1,  4,  7,
         2, -5,  8,
         3,  6, -9);
      // clang-format on

      expected = "[-1 2 3] [4 -5 6] [7 8 -9]";
      TestEqual(
          "mat3",
          CesiumMetadataConversions<FString, glm::i8mat3x3>::convert(
              mat3,
              FString("")),
          FString(expected.c_str()));

      // This is written as transposed because glm::transpose only compiles for
      // floating point types.
      // clang-format off
      glm::u8mat4x4 mat4(
         0,  4,  8, 12,
         1,  5,  9, 13,
         2,  6, 10, 14,
         3,  7, 11, 15);
      // clang-format on

      expected = "[0 1 2 3] [4 5 6 7] [8 9 10 11] [12 13 14 15]";
      TestEqual(
          "vec4",
          CesiumMetadataConversions<FString, glm::u8mat4x4>::convert(
              mat4,
              FString("")),
          FString(expected.c_str()));
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

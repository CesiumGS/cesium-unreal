// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "UnrealMetadataConversions.h"
#include "CesiumTestHelpers.h"
#include "Misc/AutomationTest.h"

#include <limits>

BEGIN_DEFINE_SPEC(
    FUnrealMetadataConversionsSpec,
    "Cesium.Unit.MetadataConversions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FUnrealMetadataConversionsSpec)

void FUnrealMetadataConversionsSpec::Define() {

  Describe("FIntPoint", [this]() {
    It("converts from glm::ivec2", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toIntPoint(glm::ivec2(-1, 2)),
          FIntPoint(-1, 2));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "value",
          UnrealMetadataConversions::toIntPoint(str, FIntPoint(0)),
          FIntPoint(1, 2));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1");
      TestEqual(
          "partial input",
          UnrealMetadataConversions::toIntPoint(str, FIntPoint(0)),
          FIntPoint(0));

      str = std::string_view("R=0.5 G=0.5");
      TestEqual(
          "bad format",
          UnrealMetadataConversions::toIntPoint(str, FIntPoint(0)),
          FIntPoint(0));
    });
  });

  Describe("FVector2D", [this]() {
    It("converts from glm::dvec2", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector2D(glm::dvec2(-1.0, 2.0)),
          FVector2D(-1.0, 2.0));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5");
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector2D(str, FVector2D::Zero()),
          FVector2D(1.5, 2.5));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1");
      TestEqual(
          "partial input",
          UnrealMetadataConversions::toVector2D(str, FVector2D::Zero()),
          FVector2D::Zero());

      str = std::string_view("R=0.5 G=0.5");
      TestEqual(
          "bad format",
          UnrealMetadataConversions::toVector2D(str, FVector2D::Zero()),
          FVector2D::Zero());
    });
  });

  Describe("FIntVector", [this]() {
    It("converts from glm::ivec3", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toIntVector(glm::ivec3(-1, 2, 4)),
          FIntVector(-1, 2, 4));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2 Z=4");
      TestEqual(
          "value",
          UnrealMetadataConversions::toIntVector(str, FIntVector(0)),
          FIntVector(1, 2, 4));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          UnrealMetadataConversions::toIntVector(str, FIntVector(0)),
          FIntVector(0));

      str = std::string_view("R=0.5 G=0.5 B=1");
      TestEqual(
          "bad format",
          UnrealMetadataConversions::toIntVector(str, FIntVector(0)),
          FIntVector(0));
    });
  });

  Describe("FVector3f", [this]() {
    It("converts from glm::vec3", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector3f(glm::vec3(1.0f, 2.3f, 4.56f)),
          FVector3f(1.0f, 2.3f, 4.56f));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1 Y=2 Z=3");
      TestEqual(
          "value",
          UnrealMetadataConversions ::toVector3f(str, FVector3f::Zero()),
          FVector3f(1.0f, 2.0f, 3.0f));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          UnrealMetadataConversions ::toVector3f(str, FVector3f::Zero()),
          FVector3f(0.0f));

      str = std::string_view("R=0.5 G=0.5 B=0.5");
      TestEqual(
          "bad format",
          UnrealMetadataConversions ::toVector3f(str, FVector3f::Zero()),
          FVector3f(0.0f));
    });
  });

  Describe("FVector", [this]() {
    It("converts from glm::dvec3", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector(glm::dvec3(1.0, 2.3, 4.56)),
          FVector(1.0, 2.3, 4.56));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5 Z=3.5");
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector(str, FVector::Zero()),
          FVector(1.5f, 2.5f, 3.5f));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          UnrealMetadataConversions::toVector(str, FVector::Zero()),
          FVector::Zero());

      str = std::string_view("R=0.5 G=0.5 B=0.5");
      TestEqual(
          "bad format",
          UnrealMetadataConversions::toVector(str, FVector::Zero()),
          FVector::Zero());
    });
  });

  Describe("FVector4", [this]() {
    It("converts from glm::dvec4", [this]() {
      TestEqual(
          "value",
          UnrealMetadataConversions::toVector4(
              glm::dvec4(1.0, 2.3, 4.56, 7.89)),
          FVector4(1.0, 2.3, 4.56, 7.89));
    });

    It("converts from string", [this]() {
      std::string_view str("X=1.5 Y=2.5 Z=3.5 W=4.5");
      TestEqual(
          "with W component",
          UnrealMetadataConversions::toVector4(str, FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 4.5));

      str = std::string_view("X=1.5 Y=2.5 Z=3.5");
      TestEqual(
          "without W component",
          UnrealMetadataConversions::toVector4(str, FVector4::Zero()),
          FVector4(1.5, 2.5, 3.5, 1.0));
    });

    It("uses default value for invalid string", [this]() {
      std::string_view str("X=1 Y=2");
      TestEqual(
          "partial input",
          UnrealMetadataConversions::toVector4(str, FVector4::Zero()),
          FVector4::Zero());

      str = std::string_view("R=0.5 G=0.5 B=0.5 A=1.0");
      TestEqual(
          "bad format",
          UnrealMetadataConversions::toVector4(str, FVector4::Zero()),
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
      TestEqual("value", UnrealMetadataConversions::toMatrix(input), expected);
    });
  });

  Describe("FString", [this]() {
    It("converts from std::string_view", [this]() {
      TestEqual(
          "std::string_view",
          UnrealMetadataConversions::toString(std::string_view("Hello")),
          FString("Hello"));
    });

    It("converts from vecN", [this]() {
      std::string expected("X=1 Y=2");
      TestEqual(
          "vec2",
          UnrealMetadataConversions::toString(glm::u8vec2(1, 2)),
          FString(expected.c_str()));

      expected = "X=" + std::to_string(4.5f) + " Y=" + std::to_string(3.21f) +
                 " Z=" + std::to_string(123.0f);
      TestEqual(
          "vec3",
          UnrealMetadataConversions::toString(glm::vec3(4.5f, 3.21f, 123.0f)),
          FString(expected.c_str()));

      expected = "X=" + std::to_string(1.0f) + " Y=" + std::to_string(2.0f) +
                 " Z=" + std::to_string(3.0f) + " W=" + std::to_string(4.0f);
      TestEqual(
          "vec4",
          UnrealMetadataConversions::toString(
              glm::vec4(1.0f, 2.0f, 3.0f, 4.0f)),
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
          UnrealMetadataConversions::toString(mat2),
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
          UnrealMetadataConversions::toString(mat3),
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
          "mat4",
          UnrealMetadataConversions::toString(mat4),
          FString(expected.c_str()));
    });
  });
}

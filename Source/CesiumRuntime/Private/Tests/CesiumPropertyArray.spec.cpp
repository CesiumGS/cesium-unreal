// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValue.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesiumPropertyArraySpec,
    "Cesium.Unit.PropertyArray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCesiumPropertyArraySpec)

void FCesiumPropertyArraySpec::Define() {
  Describe("Constructor", [this]() {
    It("constructs empty array by default", [this]() {
      FCesiumPropertyArray array;
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          0);

      FCesiumMetadataValueType valueType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Invalid);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);

      TestEqual(
          "blueprint type",
          UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(array),
          ECesiumMetadataBlueprintType::None);
    });

    It("constructs empty array from empty view", [this]() {
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView;
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          0);

      FCesiumMetadataValueType valueType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Scalar);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Uint8);

      TestEqual(
          "blueprint type",
          UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(array),
          ECesiumMetadataBlueprintType::Byte);
    });

    It("constructs non-empty array from view", [this]() {
      std::vector<uint8_t> values{1, 2, 3, 4};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FCesiumMetadataValueType valueType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Scalar);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Uint8);

      TestEqual(
          "blueprint type",
          UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(array),
          ECesiumMetadataBlueprintType::Byte);
    });

    It("constructs empty array from invalid TArray", [this]() {
      TArray<FCesiumMetadataValue> values{
          FCesiumMetadataValue(10),
          FCesiumMetadataValue(false)};
      FCesiumPropertyArray array(std::move(values));
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          0);

      FCesiumMetadataValueType valueType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Invalid);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);

      TestEqual(
          "blueprint type",
          UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(array),
          ECesiumMetadataBlueprintType::None);
    });

    It("constructs non-empty array from valid TArray", [this]() {
      TArray<FCesiumMetadataValue> values{
          FCesiumMetadataValue(11.50),
          FCesiumMetadataValue(-0.1),
          FCesiumMetadataValue(-20.8)};
      int64 expectedSize = values.Num();

      FCesiumPropertyArray array(std::move(values));
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          expectedSize);

      FCesiumMetadataValueType valueType =
          UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(array);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Scalar);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::Float64);

      TestEqual(
          "blueprint type",
          UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(array),
          ECesiumMetadataBlueprintType::Float64);
    });
  });

  Describe("GetValue", [this]() {
    It("gets bogus value for out-of-bounds index", [this]() {
      std::vector<uint8_t> values{1};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FCesiumMetadataValue value =
          UCesiumPropertyArrayBlueprintLibrary::GetValue(array, -1);
      FCesiumMetadataValueType valueType =
          UCesiumMetadataValueBlueprintLibrary::GetValueType(value);

      TestEqual("type", valueType.Type, ECesiumMetadataType::Invalid);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);

      value = UCesiumPropertyArrayBlueprintLibrary::GetValue(array, 1);
      valueType = UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
      TestEqual("type", valueType.Type, ECesiumMetadataType::Invalid);
      TestEqual(
          "componentType",
          valueType.ComponentType,
          ECesiumMetadataComponentType::None);
    });

    It("gets value for valid index with view array", [this]() {
      std::vector<uint8_t> values{1, 2, 3, 4};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyArrayBlueprintLibrary::GetValue(array, int64(i));

        FCesiumMetadataValueType valueType =
            UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
        TestEqual("type", valueType.Type, ECesiumMetadataType::Scalar);
        TestEqual(
            "componentType",
            valueType.ComponentType,
            ECesiumMetadataComponentType::Uint8);

        TestEqual(
            "byte value",
            UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
            values[i]);
      }
    });

    It("gets value for valid index with TArray", [this]() {
      std::vector<uint8> values{1, 2, 3, 4};
      TArray<FCesiumMetadataValue> valuesArray{
          FCesiumMetadataValue(values[0]),
          FCesiumMetadataValue(values[1]),
          FCesiumMetadataValue(values[2]),
          FCesiumMetadataValue(values[3])};

      FCesiumPropertyArray array(std::move(valuesArray));
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyArrayBlueprintLibrary::GetValue(array, int64(i));

        FCesiumMetadataValueType valueType =
            UCesiumMetadataValueBlueprintLibrary::GetValueType(value);
        TestEqual("type", valueType.Type, ECesiumMetadataType::Scalar);
        TestEqual(
            "componentType",
            valueType.ComponentType,
            ECesiumMetadataComponentType::Uint8);

        TestEqual(
            "byte value",
            UCesiumMetadataValueBlueprintLibrary::GetByte(value, 0),
            values[i]);
      }
    });
  });

  Describe("ToString", [this]() {
    It("handles bool elements", [this]() {
      std::vector<bool> values{true, false, false, true, true};
      // Extraneous bool constructor is needed to avoid implicit conversion
      // under the hood.
      TArray<FCesiumMetadataValue> valuesArray{
          FCesiumMetadataValue(bool(values[0])),
          FCesiumMetadataValue(bool(values[1])),
          FCesiumMetadataValue(bool(values[2])),
          FCesiumMetadataValue(bool(values[3])),
          FCesiumMetadataValue(bool(values[4]))};

      FCesiumPropertyArray array(std::move(valuesArray));
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FString expected("[true, false, false, true, true]");
      TestEqual(
          "ToString",
          UCesiumPropertyArrayBlueprintLibrary::ToString(array),
          expected);
    });

    It("handles int elements", [this]() {
      std::vector<int32> values{1, 2, 3, -1};
      CesiumGltf::PropertyArrayCopy<int32> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FString expected("[1, 2, 3, -1]");
      TestEqual(
          "ToString",
          UCesiumPropertyArrayBlueprintLibrary::ToString(array),
          expected);
    });

    It("handles enum elements", [this]() {
      TSharedPtr<FCesiumMetadataEnum> enumDef = MakeShared<FCesiumMetadataEnum>(
          StaticEnum<ECesiumMetadataBlueprintType>());
      std::vector<int32> values{
          static_cast<int32>(ECesiumMetadataBlueprintType::Boolean),
          static_cast<int32>(ECesiumMetadataBlueprintType::Byte),
          static_cast<int32>(ECesiumMetadataBlueprintType::Integer),
          static_cast<int32>(ECesiumMetadataBlueprintType::Integer64)};
      std::vector<std::string> valueNames{
          "Boolean",
          "Byte",
          "Integer",
          "Integer64"};
      CesiumGltf::PropertyArrayCopy<int32> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView, enumDef);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FString expected("[Boolean, Byte, Integer, Integer64]");
      TestEqual(
          "ToString",
          UCesiumPropertyArrayBlueprintLibrary::ToString(array),
          expected);
    });

    It("handles string elements", [this]() {
      std::vector<std::string_view> values{"Test", "These", "Strings"};
      TArray<FCesiumMetadataValue> valuesArray{
          FCesiumMetadataValue(values[0]),
          FCesiumMetadataValue(values[1]),
          FCesiumMetadataValue(values[2])};

      FCesiumPropertyArray array(std::move(valuesArray));
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          int64(values.size()));

      FString expected("[Test, These, Strings]");
      TestEqual(
          "ToString",
          UCesiumPropertyArrayBlueprintLibrary::ToString(array),
          expected);
    });
  });
}

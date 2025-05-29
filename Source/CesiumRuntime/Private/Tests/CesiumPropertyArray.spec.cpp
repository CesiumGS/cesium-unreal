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

    It("constructs non-empty array", [this]() {
      std::vector<uint8_t> values{1, 2, 3, 4};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(values.size()));

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
  });

  Describe("GetValue", [this]() {
    It("gets bogus value for out-of-bounds index", [this]() {
      std::vector<uint8_t> values{1};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(values.size()));

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

    It("gets value for valid index", [this]() {
      std::vector<uint8_t> values{1, 2, 3, 4};
      CesiumGltf::PropertyArrayCopy<uint8_t> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        FCesiumMetadataValue value =
            UCesiumPropertyArrayBlueprintLibrary::GetValue(
                array,
                static_cast<int64>(i));

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

  Describe("UCesiumPropertyArrayBlueprintLibrary::GetString", [this]() {
    It("handles an int array correctly", [this]() {
      std::vector<int32> values{1, 2, 3, -1};
      CesiumGltf::PropertyArrayCopy<int32> arrayView = std::vector(values);
      FCesiumPropertyArray array(arrayView);
      TestEqual(
          "size",
          UCesiumPropertyArrayBlueprintLibrary::GetSize(array),
          static_cast<int64>(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            "GetString",
            UCesiumPropertyArrayBlueprintLibrary::GetString(array, i),
            FString(UTF8_TO_TCHAR(std::to_string(values[i]).c_str())));
      }
    });

    It("handles an enum array correctly", [this]() {
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
          static_cast<int64>(values.size()));

      for (size_t i = 0; i < values.size(); i++) {
        TestEqual(
            "GetString",
            UCesiumPropertyArrayBlueprintLibrary::GetString(array, i),
            FString(UTF8_TO_TCHAR(valueNames[i].c_str())));
      }
    });
  });
}

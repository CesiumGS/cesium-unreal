#include "CesiumMetadataValue.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPropertyArraySpec,
    "Cesium.Unit.PropertyArray",
    EAutomationTestFlags::ApplicationContextMask |
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
      PropertyArrayView<uint8_t> arrayView;
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
      PropertyArrayView<uint8_t> arrayView(std::move(values));
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
      PropertyArrayView<uint8_t> arrayView(std::move(values));
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
      PropertyArrayView<uint8_t> arrayView(std::move(values));
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
}

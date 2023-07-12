#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumPropertyTableProperty.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPropertyTablePropertySpec,
    "Cesium.PropertyTableProperty",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
ExtensionModelExtStructuralMetadata* pExtension;
END_DEFINE_SPEC(FCesiumPropertyTablePropertySpec)

void FCesiumPropertyTablePropertySpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pExtension = &model.addExtension<ExtensionModelExtStructuralMetadata>();
    });

    It("constructs invalid instance from empty property view", [this]() {
      FCesiumPropertyTableProperty property;
      TestEqual(
          "PropertyTablePropertyStatus",
          UCesiumPropertyTablePropertyBlueprintLibrary::
              GetPropertyTablePropertyStatus(property),
          ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty);
    });

    //It("constructs invalid instance from empty property view", [this]() {
      //PropertyTablePropertyView<int8_t> propertyView(
      //    PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
      //FCesiumPropertyTableProperty property;
      //TestEqual(
      //    "PropertyTablePropertyStatus",
      //    UCesiumPropertyTablePropertyBlueprintLibrary::
      //        GetPropertyTablePropertyStatus(property),
      //    ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty);
    //});
  });
}

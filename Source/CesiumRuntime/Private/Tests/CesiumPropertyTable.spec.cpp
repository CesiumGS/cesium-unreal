#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfSpecUtility.h"
#include "CesiumPropertyTable.h"
#include "Misc/AutomationTest.h"
#include <limits>

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPropertyTableSpec,
    "Cesium.PropertyTable",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
PropertyTable* pPropertyTable;
END_DEFINE_SPEC(FCesiumPropertyTableSpec)

void FCesiumPropertyTableSpec::Define() {
  BeforeEach([this]() {
    model = Model();
    ExtensionModelExtStructuralMetadata& extension =
        model.addExtension<ExtensionModelExtStructuralMetadata>();
    pPropertyTable = &extension.propertyTables.emplace_back();
  });

  Describe("Constructor", [this]() {
    It("constructs invalid instance by default", [this]() {
      FCesiumPropertyTable propertyTable;
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTable);
      TestEqual(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          0);
    });

    It("constructs invalid instance for missing schema", [this]() {
      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass);
      TestEqual(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          0);
    });

    It("constructs invalid instance for missing schema", [this]() {
      FCesiumPropertyTable propertyTable(model, *pPropertyTable);
      TestEqual(
          "PropertyTableStatus",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
              propertyTable),
          ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass);
      TestEqual(
          "Count",
          UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
              propertyTable),
          0);
    });
  });
}

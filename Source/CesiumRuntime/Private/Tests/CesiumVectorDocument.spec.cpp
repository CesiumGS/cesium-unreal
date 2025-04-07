// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumVectorDocument.h"
#include "CesiumVectorNode.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesiumVectorDocumentSpec,
    "Cesium.Unit.CesiumVectorDocument",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ProductFilter | EAutomationTestFlags::NonNullRHI)
END_DEFINE_SPEC(FCesiumVectorDocumentSpec)

void FCesiumVectorDocumentSpec::Define() {
  Describe("UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString", [this]() {
    It("loads a valid GeoJSON document", [this]() {
      FCesiumVectorDocument Document;
      const bool bSuccess =
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Point", "coordinates": [1, 2, 3] })==",
              Document);
      TestTrue("LoadGeoJsonFromString Success", bSuccess);
    });
    It("fails to load an invalid GeoJSON document", [this]() {
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Failed to parse GeoJSON: Missing a name for object member.");
      FCesiumVectorDocument Document;
      bool bSuccess =
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ type: "Point", coordinates: [1, 2, 3] })==",
              Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Unknown GeoJSON object type: 'Invalid'");
      bSuccess = UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
          R"==({ "type": "Invalid", "coordinates": [] })==",
          Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Failed to parse GeoJSON: Invalid value.");
      bSuccess = UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
          R"==(<some_xml_idk />)==",
          Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
    });
  });
}

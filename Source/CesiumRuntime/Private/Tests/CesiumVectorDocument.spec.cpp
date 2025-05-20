// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumGeoJsonDocument.h"
#include "CesiumGeoJsonObject.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(
    FCesiumVectorDocumentSpec,
    "Cesium.Unit.CesiumVectorDocument",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ProductFilter | EAutomationTestFlags::NonNullRHI)
END_DEFINE_SPEC(FCesiumVectorDocumentSpec)

void FCesiumVectorDocumentSpec::Define() {
  Describe("UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString", [this]() {
    It("loads a valid GeoJSON document", [this]() {
      FCesiumGeoJsonDocument Document;
      const bool bSuccess =
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Point", "coordinates": [1, 2, 3] })==",
              Document);
      TestTrue("LoadGeoJsonFromString Success", bSuccess);
    });
    It("fails to load an invalid GeoJSON document", [this]() {
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Failed to parse GeoJSON: Missing a name for object member.");
      FCesiumGeoJsonDocument Document;
      bool bSuccess =
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ type: "Point", coordinates: [1, 2, 3] })==",
              Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Unknown GeoJSON object type: 'Invalid'");
      bSuccess = UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
          R"==({ "type": "Invalid", "coordinates": [] })==",
          Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Failed to parse GeoJSON: Invalid value.");
      bSuccess = UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
          R"==(<some_xml_idk />)==",
          Document);
      TestFalse("LoadGeoJsonFromString Success", bSuccess);
    });
  });

  Describe("UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsInteger", [this]() {
    It("correctly interprets an integer ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": 10, "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsInteger",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsInteger(
              UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
                  UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
                      Document))),
          10);
    });
    It("returns -1 when the ID is missing", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsInteger",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsInteger(
              UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
                  UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
                      Document))),
          -1);
    });
  });

  Describe("UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString", [this]() {
    It("correctly interprets a string ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": "test", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(
              UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
                  UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
                      Document))),
          "test");
    });
    It("stringifies an integer ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": 10, "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(
              UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
                  UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
                      Document))),
          "10");
    });
    It("returns an empty string when the ID is missing", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(
              UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
                  UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
                      Document))),
          "");
    });
  });

  Describe("UCesiumGeoJsonFeatureBlueprintLibrary::GetChildren", [this]() {
    It("returns an array of children", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({
                  "type": "FeatureCollection",
                  "features": [
                    { "type": "Feature", "id": "test", "geometry": null, "properties": null },
                    { "type": "Feature", "id": "test2", "geometry": null, "properties": null }
                  ]
                 })==",
              Document));
      TArray<FCesiumGeoJsonFeature> Children =
          UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeatureCollection(
              UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(Document));
      TestEqual("Children.Num()", Children.Num(), 2);
      TestEqual(
          "Children[0] Id",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(Children[0]),
          "test");
      TestEqual(
          "Children[1] Id",
          UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(Children[1]),
          "test2");
    });
  });
}

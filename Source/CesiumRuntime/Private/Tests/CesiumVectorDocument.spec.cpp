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
  Describe("UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString", [this]() {
    It("loads a valid GeoJSON document", [this]() {
      FCesiumGeoJsonDocument Document;
      const bool bSuccess =
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Point", "coordinates": [1, 2, 3] })==",
              Document);
      TestTrue("LoadGeoJsonFromString Success", bSuccess);
    });
    It("fails to load an invalid GeoJSON document", [this]() {
      AddExpectedError(
          "Errors while loading GeoJSON from string:\n- Failed to parse GeoJSON: Missing a name for object member.");
      FCesiumGeoJsonDocument Document;
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

  Describe("UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger", [this]() {
    It("correctly interprets an integer ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": 10, "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsInteger",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document)),
          10);
    });
    It("returns -1 when the ID is missing", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsInteger",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document)),
          -1);
    });
  });

  Describe("UCesiumVectorNodeBlueprintLibrary::GetIdAsString", [this]() {
    It("correctly interprets a string ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": "test", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document)),
          "test");
    });
    It("stringifies an integer ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "id": 10, "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document)),
          "10");
    });
    It("returns an empty string when the ID is missing", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({ "type": "Feature", "geometry": null, "properties": null })==",
              Document));
      TestEqual(
          "GetIdAsString",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document)),
          "");
    });
  });

  Describe("UCesiumVectorNodeBlueprintLibrary::GetChildren", [this]() {
    It("returns an array of children", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({
                  "type": "FeatureCollection",
                  "features": [
                    { "type": "Feature", "id": "test", "geometry": null, "properties": null },
                    { "type": "Feature", "id": "test2", "geometry": null, "properties": null }
                  ]
                 })==",
              Document));
      TArray<FCesiumGeoJsonObject> Children =
          UCesiumVectorNodeBlueprintLibrary::GetChildren(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document));
      TestEqual("Children.Num()", Children.Num(), 2);
      TestEqual(
          "Children[0] Id",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(Children[0]),
          "test");
      TestEqual(
          "Children[1] Id",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(Children[1]),
          "test2");
    });
  });

  Describe("UCesiumVectorNodeBlueprintLibrary::GetPrimitives", [this]() {
    It("returns an array of primitives", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({
                  "type": "MultiPoint",
                  "coordinates": [
                    [ 1, 2, 3 ],
                    [ 4, 5, 6 ]
                  ]
                 })==",
              Document));
      TArray<FCesiumVectorPrimitive> Primitives =
          UCesiumVectorNodeBlueprintLibrary::GetPrimitives(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document));
      TestEqual("Primitives.Num()", Primitives.Num(), 2);
      TestEqual(
          "Primitives[0] as point",
          UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
              Primitives[0]),
          FVector(1, 2, 3));
      TestEqual(
          "Primitives[1] as point",
          UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
              Primitives[1]),
          FVector(4, 5, 6));
    });
  });

  Describe(
      "UCesiumVectorNodeBlueprintLibrary::GetPrimitivesOfTypeRecursively",
      [this]() {
        It("returns all primitives of a given type in the document", [this]() {
          FCesiumGeoJsonDocument Document;
          TestTrue(
              "LoadGeoJsonFromString Success",
              UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
                  R"==({
                  "type": "GeometryCollection",
                  "geometries": [
                    {
                      "type": "GeometryCollection",
                      "geometries": [
                        { "type": "Point", "coordinates": [ -2, -1, 0 ] }
                      ]
                    },
                    { "type": "Point", "coordinates": [ 1, 2, 3 ] },
                    { "type": "LineString", "coordinates": [ [ 1, 2, 3 ], [ 4, 5, 6 ] ] },
                    { "type": "Point", "coordinates": [ 7, 8, 9 ] }
                  ]
                 })==",
                  Document));
          TArray<FCesiumVectorPrimitive> Primitives =
              UCesiumVectorNodeBlueprintLibrary::GetPrimitivesOfTypeRecursively(
                  UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document),
                  ECesiumGeoJsonGeometryType::Point);
          TestEqual("Primitives.Num()", Primitives.Num(), 3);
          TestEqual(
              "Primitives[0] as point",
              UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
                  Primitives[0]),
              FVector(-2, -1, 0));
          TestEqual(
              "Primitives[0] as point",
              UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
                  Primitives[1]),
              FVector(1, 2, 3));
          TestEqual(
              "Primitives[1] as point",
              UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
                  Primitives[2]),
              FVector(7, 8, 9));
        });
      });

  Describe("UCesiumVectorNodeBlueprintLibrary::FindNodeByStringId", [this]() {
    It("obtains a node with the given ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({
                  "type": "FeatureCollection",
                  "features": [
                    { "type": "Feature", "id": "test1", "geometry": null, "properties": null },
                    { "type": "Feature", "id": "test2", "geometry": null, "properties": null },
                    { "type": "Feature", "id": "test3", "geometry": null, "properties": null },
                    {
                      "type": "Feature",
                      "id": "test4",
                      "geometry": {
                        "type": "Point",
                        "coordinates": [ 1, 2, 3 ]
                      },
                      "properties": null
                    },
                    { "type": "Feature", "id": "test5", "geometry": null, "properties": null },
                    { "type": "Feature", "id": "test6", "geometry": null, "properties": null }
                  ]
                 })==",
              Document));
      FCesiumGeoJsonObject Node;
      TestTrue(
          "FindNodeByStringId Success",
          UCesiumVectorNodeBlueprintLibrary::FindNodeByStringId(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document),
              "test4",
              Node));
      TestEqual(
          "Node.Id",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsString(Node),
          "test4");
      TArray<FCesiumVectorPrimitive> Primitives =
          UCesiumVectorNodeBlueprintLibrary::GetPrimitives(
              UCesiumVectorNodeBlueprintLibrary::GetChildren(Node)[0]);
      TestEqual("Primitives.Num()", Primitives.Num(), 1);
      TestEqual(
          "Primitives[0] as point",
          UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
              Primitives[0]),
          FVector(1, 2, 3));
    });
  });

  Describe("UCesiumVectorNodeBlueprintLibrary::FindNodeByIntId", [this]() {
    It("obtains a node with the given ID", [this]() {
      FCesiumGeoJsonDocument Document;
      TestTrue(
          "LoadGeoJsonFromString Success",
          UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
              R"==({
                  "type": "FeatureCollection",
                  "features": [
                    { "type": "Feature", "id": 1, "geometry": null, "properties": null },
                    { "type": "Feature", "id": 2, "geometry": null, "properties": null },
                    { "type": "Feature", "id": 3, "geometry": null, "properties": null },
                    {
                      "type": "Feature",
                      "id": 4,
                      "geometry": {
                        "type": "Point",
                        "coordinates": [ 1, 2, 3 ]
                      },
                      "properties": null
                    },
                    { "type": "Feature", "id": 5, "geometry": null, "properties": null },
                    { "type": "Feature", "id": 6, "geometry": null, "properties": null }
                  ]
                 })==",
              Document));
      FCesiumGeoJsonObject Node;
      TestTrue(
          "FindNodeByStringId Success",
          UCesiumVectorNodeBlueprintLibrary::FindNodeByIntId(
              UCesiumVectorDocumentBlueprintLibrary::GetRootNode(Document),
              4,
              Node));
      TestEqual(
          "Node.Id",
          UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger(Node),
          4);
      TArray<FCesiumVectorPrimitive> Primitives =
          UCesiumVectorNodeBlueprintLibrary::GetPrimitives(
              UCesiumVectorNodeBlueprintLibrary::GetChildren(Node)[0]);
      TestEqual("Primitives.Num()", Primitives.Num(), 1);
      TestEqual(
          "Primitives[0] as point",
          UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
              Primitives[0]),
          FVector(1, 2, 3));
    });
  });
}

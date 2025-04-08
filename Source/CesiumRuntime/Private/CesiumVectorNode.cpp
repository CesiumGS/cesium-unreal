#include "CesiumVectorNode.h"

#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/CompositeCartographicPolygon.h"
#include "Dom/JsonObject.h"

#include <utility>
#include <variant>
#include <vector>

int64 UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger(
    const FCesiumVectorNode& InVectorNode) {
  const int64_t* pId = std::get_if<int64_t>(&InVectorNode._node.id);
  if (!pId) {
    return -1;
  }

  return *pId;
}

FString UCesiumVectorNodeBlueprintLibrary::GetIdAsString(
    const FCesiumVectorNode& InVectorNode) {
  struct GetIdVisitor {
    FString operator()(const int64_t& id) { return FString::FromInt(id); }
    FString operator()(const std::string& id) {
      return UTF8_TO_TCHAR(id.c_str());
    }
    FString operator()(const std::monostate& id) { return FString(); }
  };

  return std::visit(GetIdVisitor{}, InVectorNode._node.id);
}

TArray<FCesiumVectorNode> UCesiumVectorNodeBlueprintLibrary::GetChildren(
    const FCesiumVectorNode& InVectorNode) {
  TArray<FCesiumVectorNode> children;
  children.Reserve(InVectorNode._node.children.size());
  for (const CesiumVectorData::VectorNode& child :
       InVectorNode._node.children) {
    children.Emplace(child);
  }
  return children;
}

namespace {
TSharedPtr<FJsonValue>
jsonValueToUnrealJsonValue(const CesiumUtility::JsonValue& value) {
  struct JsonValueVisitor {
    TSharedPtr<FJsonValue> operator()(const CesiumUtility::JsonValue::Null&) {
      return MakeShared<FJsonValueNull>();
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Bool& value) {
      return MakeShared<FJsonValueBoolean>(value);
    }
    TSharedPtr<FJsonValue> operator()(const std::string& value) {
      return MakeShared<FJsonValueString>(UTF8_TO_TCHAR(value.c_str()));
    }
    TSharedPtr<FJsonValue> operator()(const double& value) {
      return MakeShared<FJsonValueNumber>(value);
    }
    TSharedPtr<FJsonValue> operator()(const std::uint64_t& value) {
      return MakeShared<FJsonValueNumberString>(FString::FromInt(value));
    }
    TSharedPtr<FJsonValue> operator()(const std::int64_t& value) {
      return MakeShared<FJsonValueNumberString>(FString::FromInt(value));
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Array& value) {
      TArray<TSharedPtr<FJsonValue>> values;
      values.Reserve(value.size());
      for (const CesiumUtility::JsonValue& v : value) {
        values.Emplace(jsonValueToUnrealJsonValue(v));
      }
      return MakeShared<FJsonValueArray>(MoveTemp(values));
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Object& value) {
      FJsonObject obj;
      for (const auto& [k, v] : value) {
        obj.SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
      }
      return MakeShared<FJsonValueObject>(MoveTemp(obj));
    };
  };

  return std::visit(JsonValueVisitor{}, value.value);
}
} // namespace

FJsonObjectWrapper UCesiumVectorNodeBlueprintLibrary::GetProperties(
    const FCesiumVectorNode& InVectorNode) {
  TSharedPtr<FJsonObject> object = MakeShared<FJsonObject>();
  if (InVectorNode._node.properties) {
    for (const auto& [k, v] : *InVectorNode._node.properties) {
      object->SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
    }
  }

  FJsonObjectWrapper wrapper;
  wrapper.JsonObject = object;
  return wrapper;
}

TArray<FCesiumVectorPrimitive> UCesiumVectorNodeBlueprintLibrary::GetPrimitives(
    const FCesiumVectorNode& InVectorNode) {
  TArray<FCesiumVectorPrimitive> primitives;
  primitives.Reserve(InVectorNode._node.primitives.size());
  for (const CesiumVectorData::VectorPrimitive& primitive :
       InVectorNode._node.primitives) {
    primitives.Emplace(primitive);
  }
  return primitives;
}

namespace {
TArray<FCesiumVectorPrimitive> GetPrimitivesOfTypeInternal(
    const CesiumVectorData::VectorNode& node,
    ECesiumVectorPrimitiveType InType) {
  struct CheckTypeVisitor {
    ECesiumVectorPrimitiveType IntendedType;
    bool operator()(const CesiumGeospatial::Cartographic&) {
      return IntendedType == ECesiumVectorPrimitiveType::Point;
    }
    bool operator()(const std::vector<CesiumGeospatial::Cartographic>&) {
      return IntendedType == ECesiumVectorPrimitiveType::Line;
    }
    bool operator()(const CesiumGeospatial::CompositeCartographicPolygon&) {
      return IntendedType == ECesiumVectorPrimitiveType::Polygon;
    }
  };

  TArray<FCesiumVectorPrimitive> primitives;
  for (const CesiumVectorData::VectorPrimitive& primitive : node.primitives) {
    if (std::visit(CheckTypeVisitor{InType}, primitive)) {
      primitives.Emplace(primitive);
    }
  }

  return primitives;
}

TArray<FCesiumVectorPrimitive> GetPrimitivesOfTypeRecursivelyInternal(
    const CesiumVectorData::VectorNode& node,
    ECesiumVectorPrimitiveType InType) {
  TArray<FCesiumVectorPrimitive> primitives =
      GetPrimitivesOfTypeInternal(node, InType);
  for (const CesiumVectorData::VectorNode& child : node.children) {
    TArray<FCesiumVectorPrimitive> childPrimitives =
        GetPrimitivesOfTypeRecursivelyInternal(child, InType);
    for (FCesiumVectorPrimitive& childPrimitive : childPrimitives) {
      primitives.Emplace(MoveTemp(childPrimitive));
    }
  }
  return primitives;
}
} // namespace

TArray<FCesiumVectorPrimitive>
UCesiumVectorNodeBlueprintLibrary::GetPrimitivesOfType(
    const FCesiumVectorNode& InVectorNode,
    ECesiumVectorPrimitiveType InType) {
  return GetPrimitivesOfTypeInternal(InVectorNode._node, InType);
}

TArray<FCesiumVectorPrimitive>
UCesiumVectorNodeBlueprintLibrary::GetPrimitivesOfTypeRecursively(
    const FCesiumVectorNode& InVectorNode,
    ECesiumVectorPrimitiveType InType) {
  return GetPrimitivesOfTypeRecursivelyInternal(InVectorNode._node, InType);
}

namespace {
template <typename T>
bool FindNodeById(
    const CesiumVectorData::VectorNode& node,
    const T& InNodeId,
    FCesiumVectorNode& OutNode) {
  for (const CesiumVectorData::VectorNode& child : node.children) {
    const T* nodeId = std::get_if<T>(&child.id);
    if (nodeId && *nodeId == InNodeId) {
      OutNode = child;
      return true;
    }

    // Check children.
    if (FindNodeById(child, InNodeId, OutNode)) {
      return true;
    }
  }

  return false;
}
} // namespace

bool UCesiumVectorNodeBlueprintLibrary::FindNodeByStringId(
    const FCesiumVectorNode& InVectorNode,
    const FString& InNodeId,
    FCesiumVectorNode& OutNode) {
  const std::string& id = TCHAR_TO_UTF8(*InNodeId);
  return FindNodeById<std::string>(InVectorNode._node, id, OutNode);
}

bool UCesiumVectorNodeBlueprintLibrary::FindNodeByIntId(
    const FCesiumVectorNode& InVectorNode,
    int64 InNodeId,
    FCesiumVectorNode& OutNode) {
  return FindNodeById<int64_t>(InVectorNode._node, InNodeId, OutNode);
}

ECesiumVectorPrimitiveType
UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveType(
    const FCesiumVectorPrimitive& InPrimitive) {
  struct GetTypeVisitor {
    ECesiumVectorPrimitiveType
    operator()(const CesiumGeospatial::Cartographic&) {
      return ECesiumVectorPrimitiveType::Point;
    }
    ECesiumVectorPrimitiveType
    operator()(const std::vector<CesiumGeospatial::Cartographic>&) {
      return ECesiumVectorPrimitiveType::Line;
    }
    ECesiumVectorPrimitiveType
    operator()(const CesiumGeospatial::CompositeCartographicPolygon&) {
      return ECesiumVectorPrimitiveType::Polygon;
    }
  };

  return std::visit(GetTypeVisitor{}, InPrimitive._primitive);
}

FVector UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
    const FCesiumVectorPrimitive& InPrimitive) {
  const CesiumGeospatial::Cartographic* pCartographic =
      std::get_if<CesiumGeospatial::Cartographic>(&InPrimitive._primitive);
  if (!pCartographic) {
    return FVector::ZeroVector;
  }

  return FVector(
      CesiumUtility::Math::radiansToDegrees(pCartographic->longitude),
      CesiumUtility::Math::radiansToDegrees(pCartographic->latitude),
      pCartographic->height);
}

TArray<FVector> UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsLine(
    const FCesiumVectorPrimitive& InPrimitive) {
  const std::vector<CesiumGeospatial::Cartographic>* pCartographicLine =
      std::get_if<std::vector<CesiumGeospatial::Cartographic>>(
          &InPrimitive._primitive);
  if (!pCartographicLine) {
    return TArray<FVector>();
  }

  TArray<FVector> line;
  line.Reserve(pCartographicLine->size());
  for (const CesiumGeospatial::Cartographic& point : *pCartographicLine) {
    line.Emplace(
        CesiumUtility::Math::radiansToDegrees(point.longitude),
        CesiumUtility::Math::radiansToDegrees(point.latitude),
        point.height);
  }

  return line;
}

FCesiumCompositeCartographicPolygon
UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPolygon(
    const FCesiumVectorPrimitive& InPrimitive) {
  const CesiumGeospatial::CompositeCartographicPolygon* pPolygon =
      std::get_if<CesiumGeospatial::CompositeCartographicPolygon>(
          &InPrimitive._primitive);
  if (!pPolygon) {
    return FCesiumCompositeCartographicPolygon(
        CesiumGeospatial::CompositeCartographicPolygon({}));
  }

  return FCesiumCompositeCartographicPolygon(*pPolygon);
}

FCesiumPolygonLinearRing::FCesiumPolygonLinearRing(TArray<FVector>&& InPoints)
    : Points(MoveTemp(InPoints)) {}

bool UCesiumCompositeCartographicPolygonBlueprintLibrary::PolygonContainsPoint(
    const FCesiumCompositeCartographicPolygon& InPolygon,
    const FVector& InPoint) {
  return InPolygon._polygon.contains(CesiumGeospatial::Cartographic(
      CesiumUtility::Math::degreesToRadians(InPoint.X),
      CesiumUtility::Math::degreesToRadians(InPoint.Y),
      InPoint.Z));
}

TArray<FCesiumPolygonLinearRing>
UCesiumCompositeCartographicPolygonBlueprintLibrary::GetPolygonRings(
    const FCesiumCompositeCartographicPolygon& InPolygon) {
  const std::vector<CesiumGeospatial::CartographicPolygon>& polygons =
      InPolygon._polygon.getLinearRings();
  TArray<FCesiumPolygonLinearRing> linearRings;
  linearRings.Reserve(polygons.size());
  for (const CesiumGeospatial::CartographicPolygon& polygon : polygons) {
    TArray<FVector> points;
    points.Reserve(polygon.getVertices().size());
    for (const glm::dvec2& point : polygon.getVertices()) {
      points.Emplace(FVector(
          CesiumUtility::Math::radiansToDegrees(point.x),
          CesiumUtility::Math::radiansToDegrees(point.y),
          0));
    }
    linearRings.Emplace(MoveTemp(points));
  }
  return linearRings;
}

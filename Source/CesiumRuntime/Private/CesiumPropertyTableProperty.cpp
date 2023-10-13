// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTableProperty.h"
#include "CesiumGltf/GenericPropertyTableViewVisitor.h"
#include "CesiumGltf/PropertyTableView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"
#include <utility>

using namespace CesiumGltf;

FCesiumPropertyTableProperty::FCesiumPropertyTableProperty() noexcept
    : _propertyTable(), _propertyId() {}

FCesiumPropertyTableProperty::FCesiumPropertyTableProperty(
    const CesiumGltf::PropertyTableView& propertyTable,
    const std::string& propertyName) noexcept
    : _propertyTable(propertyTable),
      _propertyId(propertyTable.findProperty(propertyName)) {}

FCesiumPropertyTableProperty::~FCesiumPropertyTableProperty() noexcept =
    default;
FCesiumPropertyTableProperty::FCesiumPropertyTableProperty(
    FCesiumPropertyTableProperty&& rhs) noexcept = default;
FCesiumPropertyTableProperty::FCesiumPropertyTableProperty(
    const FCesiumPropertyTableProperty& rhs) noexcept = default;
FCesiumPropertyTableProperty& FCesiumPropertyTableProperty::operator=(
    FCesiumPropertyTableProperty&& rhs) noexcept = default;
FCesiumPropertyTableProperty& FCesiumPropertyTableProperty::operator=(
    const FCesiumPropertyTableProperty& rhs) noexcept = default;

template <typename TResult, typename TCallback>
TResult FCesiumPropertyTableProperty::invoke(TCallback&& callback) const {
  TResult result{};

  GenericPropertyTableViewVisitor visitor(
      [&result, &callback](
          // const std::string& propertyName,
          const auto& propertyView) {
        result = callback(std::string(), propertyView);
      });

  IPropertyTableViewVisitor* pVisitor = &visitor;

  this->_propertyTable.getPropertyViewDynamic(this->_propertyId, *pVisitor);
  return result;
}

ECesiumPropertyTablePropertyStatus
UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertyTablePropertyStatus(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<
      ECesiumPropertyTablePropertyStatus>([](const std::string& propertyName,
                                             const auto& propertyView) {
    switch (propertyView.status()) {
    case CesiumGltf::PropertyTablePropertyViewStatus::Valid:
      return ECesiumPropertyTablePropertyStatus::Valid;
    case CesiumGltf::PropertyTablePropertyViewStatus::EmptyPropertyWithDefault:
      return ECesiumPropertyTablePropertyStatus::EmptyPropertyWithDefault;
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorNonexistentProperty:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::
        ErrorComponentTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidNormalization:
    case CesiumGltf::PropertyTablePropertyViewStatus::
        ErrorNormalizationMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidOffset:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidScale:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidMax:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidMin:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidNoDataValue:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidDefaultValue:
      return ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty;
    default:
      return ECesiumPropertyTablePropertyStatus::ErrorInvalidPropertyData;
    }
  });
}

namespace {

template <typename TPropertyView>
FCesiumMetadataValueType GetMetadataValueType(const TPropertyView&) {
  return TypeToMetadataValueType<typename TPropertyView::PropertyType>();
}

} // namespace

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<ECesiumMetadataBlueprintType>(
      [](const std::string& propertyName, const auto& propertyView) {
        return CesiumMetadataValueTypeToBlueprintType(
            GetMetadataValueType(propertyView));
      });
}

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  const ClassProperty* pClassProperty = Property._propertyId.getClassProperty();
  if (!pClassProperty || !pClassProperty->array) {
    return ECesiumMetadataBlueprintType::None;
  }

  return Property.invoke<ECesiumMetadataBlueprintType>(
      [](const std::string& propertyName, const auto& propertyView) {
        FCesiumMetadataValueType valueType = GetMetadataValueType(propertyView);
        valueType.bIsArray = false;
        return CesiumMetadataValueTypeToBlueprintType(valueType);
      });
}

FCesiumMetadataValueType
UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValueType>(
      [](const std::string& propertyName, const auto& propertyView) {
        return GetMetadataValueType(propertyView);
      });
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<int64>(
      [](const std::string& propertyName, const auto& propertyView) {
        return propertyView.size();
      });
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<int64>(
      [](const std::string& propertyName, const auto& propertyView) {
        return propertyView.arrayCount();
      });
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    bool DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetBoolean(
      GetValue(Property, FeatureID),
      DefaultValue);
}

uint8 UCesiumPropertyTablePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    uint8 DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetByte(
      GetValue(Property, FeatureID),
      DefaultValue);
}

int32 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int32 DefaultValue) {
  // return Property.invoke<int32>([FeatureID, DefaultValue](
  //                                   const std::string& propertyName,
  //                                   const auto& propertyView) {
  //   if (FeatureID >= 0 && FeatureID < propertyView.size()) {
  //     auto maybeValue = propertyView.get(FeatureID);
  //     if (maybeValue) {
  //       using ValueType =
  //           std::remove_cv_t<std::remove_reference_t<decltype(*maybeValue)>>;
  //       return CesiumMetadataConversions<int32, ValueType>::convert(
  //           *maybeValue,
  //           DefaultValue);
  //     }
  //   }
  //   return DefaultValue;
  // });

  return UCesiumMetadataValueBlueprintLibrary::GetInteger(
      GetValue(Property, FeatureID),
      DefaultValue);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int64 DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetInteger64(
      GetValue(Property, FeatureID),
      DefaultValue);
}

float UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    float DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetFloat(
      GetValue(Property, FeatureID),
      DefaultValue);
}

double UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    double DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetFloat64(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FIntPoint UCesiumPropertyTablePropertyBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FIntPoint& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FVector2D UCesiumPropertyTablePropertyBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector2D& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetVector2D(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FIntVector UCesiumPropertyTablePropertyBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FIntVector& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetIntVector(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FVector3f UCesiumPropertyTablePropertyBlueprintLibrary::GetVector3f(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector3f& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetVector3f(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FVector UCesiumPropertyTablePropertyBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetVector(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FVector4 UCesiumPropertyTablePropertyBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector4& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetVector4(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FMatrix UCesiumPropertyTablePropertyBlueprintLibrary::GetMatrix(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FMatrix& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetMatrix(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FString UCesiumPropertyTablePropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FString& DefaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetString(
      GetValue(Property, FeatureID),
      DefaultValue);
}

FCesiumPropertyArray UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return UCesiumMetadataValueBlueprintLibrary::GetArray(
      GetValue(Property, FeatureID));
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return Property.invoke<FCesiumMetadataValue>(
      [FeatureID](const std::string& propertyName, const auto& propertyView) {
        // size() returns zero if the view is invalid.
        if (FeatureID >= 0 && FeatureID < propertyView.size()) {
          return FCesiumMetadataValue(propertyView.get(FeatureID));
        }
        return FCesiumMetadataValue();
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return Property.invoke<FCesiumMetadataValue>(
      [FeatureID](const std::string& propertyName, const auto& propertyView) {
        // Return an empty value if the property is empty.
        if (propertyView.status() ==
            PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
          return FCesiumMetadataValue();
        }

        // size() returns zero if the view is invalid.
        if (FeatureID >= 0 && FeatureID < propertyView.size()) {
          return FCesiumMetadataValue(propertyView.getRaw(FeatureID));
        }

        return FCesiumMetadataValue();
      });
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<bool>(
      [](const std::string& propertyName, const auto& propertyView) {
        return propertyView.normalized();
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no offset is specified.
        return FCesiumMetadataValue(propertyView.offset());
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetScale(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no scale is specified.
        return FCesiumMetadataValue(propertyView.scale());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetMinimumValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no min is specified.
        return FCesiumMetadataValue(propertyView.min());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetMaximumValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no max is specified.
        return FCesiumMetadataValue(propertyView.max());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetNoDataValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no "no data" value is specified.
        return FCesiumMetadataValue(propertyView.noData());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property.invoke<FCesiumMetadataValue>(
      [](const std::string& propertyName, const auto& propertyView) {
        // Returns an empty value if no default value is specified.
        return FCesiumMetadataValue(propertyView.defaultValue());
      });
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::
      GetArrayElementBlueprintType(Property);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyTablePropertyBlueprintLibrary::GetTrueType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return CesiumMetadataValueTypeToTrueType(GetValueType(Property));
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyTablePropertyBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  FCesiumMetadataValueType type = GetValueType(Property);
  type.bIsArray = false;
  return CesiumMetadataValueTypeToTrueType(type);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
      Property);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(Property);
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
      Property,
      FeatureID);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

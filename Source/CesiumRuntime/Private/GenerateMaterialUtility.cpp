#include "GenerateMaterialUtility.h"

#include "CesiumMetadataEncodingDetails.h"
#include "Containers/Map.h"
#include "EncodedFeaturesMetadata.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ComponentReregisterContext.h"
#include "ContentBrowserModule.h"
#include "Factories/MaterialFunctionMaterialLayerFactory.h"
#include "IContentBrowserSingleton.h"
#include "IMaterialEditor.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureProperty.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"

extern UNREALED_API class UEditorEngine* GEditor;

namespace GenerateMaterialUtility {


UMaterialFunctionMaterialLayer*
CreateMaterialLayer(const FString& PackageName, const FString& MaterialName) {
  UPackage* Package = CreatePackage(*PackageName);
  UMaterialFunctionMaterialLayerFactory* MaterialFactory =
      NewObject<UMaterialFunctionMaterialLayerFactory>();

  UMaterialFunctionMaterialLayer* pMaterialLayer =
      (UMaterialFunctionMaterialLayer*)MaterialFactory->FactoryCreateNew(
          UMaterialFunctionMaterialLayer::StaticClass(),
          Package,
          *MaterialName,
          RF_Public | RF_Standalone | RF_Transactional,
          NULL,
          GWarn);

  FAssetRegistryModule::AssetCreated(pMaterialLayer);
  Package->FullyLoad();
  Package->SetDirtyFlag(true);

  return pMaterialLayer;
}

float GetNameLengthScalar(const FName& Name) {
  return FMath::Max(static_cast<float>(Name.GetStringLength()) / 24, 1.0f);
}

float GetNameLengthScalar(const FString& Name) {
  return FMath::Max(static_cast<float>(Name.Len()) / 24, 1.0f);
}

ECustomMaterialOutputType
GetOutputTypeForEncodedType(ECesiumEncodedMetadataType Type) {
  switch (Type) {
  case ECesiumEncodedMetadataType::Vec2:
    return ECustomMaterialOutputType::CMOT_Float2;
  case ECesiumEncodedMetadataType::Vec3:
    return ECustomMaterialOutputType::CMOT_Float3;
  case ECesiumEncodedMetadataType::Vec4:
    return ECustomMaterialOutputType::CMOT_Float4;
  case ECesiumEncodedMetadataType::Scalar:
  default:
    return ECustomMaterialOutputType::CMOT_Float1;
  };
}

FString GetHlslTypeForEncodedType(
    ECesiumEncodedMetadataType Type,
    ECesiumEncodedMetadataComponentType ComponentType) {
  if (ComponentType == ECesiumEncodedMetadataComponentType::Uint8) {
    switch (Type) {
    case ECesiumEncodedMetadataType::Scalar:
      return "uint";
    case ECesiumEncodedMetadataType::Vec2:
      return "uint2";
    case ECesiumEncodedMetadataType::Vec3:
      return "uint3";
    case ECesiumEncodedMetadataType::Vec4:
      return "uint4";
    default:
      break;
    }
  }

  if (ComponentType == ECesiumEncodedMetadataComponentType::Float) {
    switch (Type) {
    case ECesiumEncodedMetadataType::Scalar:
      return "float";
    case ECesiumEncodedMetadataType::Vec2:
      return "float2";
    case ECesiumEncodedMetadataType::Vec3:
      return "float3";
    case ECesiumEncodedMetadataType::Vec4:
      return "float4";
    default:
      break;
    }
  }

  return FString();
}

FString GetSwizzleForEncodedType(ECesiumEncodedMetadataType Type) {
  switch (Type) {
  case ECesiumEncodedMetadataType::Scalar:
    return ".r";
  case ECesiumEncodedMetadataType::Vec2:
    return ".rg";
  case ECesiumEncodedMetadataType::Vec3:
    return ".rgb";
  case ECesiumEncodedMetadataType::Vec4:
    return ".rgba";
  default:
    return FString();
  };
}

UMaterialExpressionParameter* GenerateParameterNode(
    UMaterialFunctionMaterialLayer* TargetMaterialLayer,
    const ECesiumEncodedMetadataType Type,
    const FString& Name,
    int32 NodeX,
    int32 NodeY) {
  UMaterialExpressionParameter* Parameter = nullptr;
  int32 NodeHeight = 0;

  if (Type == ECesiumEncodedMetadataType::Scalar) {
    UMaterialExpressionScalarParameter* ScalarParameter =
        NewObject<UMaterialExpressionScalarParameter>(TargetMaterialLayer);
    ScalarParameter->DefaultValue = 0.0f;
    Parameter = ScalarParameter;
  }

  if (Type == ECesiumEncodedMetadataType::Vec2 ||
      Type == ECesiumEncodedMetadataType::Vec3 ||
      Type == ECesiumEncodedMetadataType::Vec4) {
    UMaterialExpressionVectorParameter* VectorParameter =
        NewObject<UMaterialExpressionVectorParameter>(TargetMaterialLayer);
    VectorParameter->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    Parameter = VectorParameter;
  }

  if (!Parameter) {
    return nullptr;
  }

  Parameter->ParameterName = FName(Name);
  Parameter->MaterialExpressionEditorX = NodeX;
  Parameter->MaterialExpressionEditorY = NodeY;

  return Parameter;
}
} // namespace GenerateMaterialUtility

#endif

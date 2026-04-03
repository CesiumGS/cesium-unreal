// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceBase.h"
#include "RHIUtilities.h"

#include "CesiumGaussianSplatDataInterface.generated.h"

class UCesiumGaussianSplatSubsystem;
class UCesiumGltfGaussianSplatComponent;

struct FNiagaraDataInterfaceProxyCesiumGaussianSplats
    : public FNiagaraDataInterfaceProxy {
  virtual ~FNiagaraDataInterfaceProxyCesiumGaussianSplats();

  virtual int32 PerInstanceDataPassedToRenderThreadSize() const override {
    return 0;
  }

  FReadBuffer TileIndicesBuffer;
  FReadBuffer TileTransformsBuffer;
  FReadBuffer SplatSHDegreesBuffer;
  FReadBuffer PositionsBuffer;
  FReadBuffer ScalesBuffer;
  FReadBuffer OrientationsBuffer;
  FReadBuffer ColorsBuffer;
  FReadBuffer SHNonZeroCoeffsBuffer;
};

struct FNDICesiumGaussianSplats_InstanceData {
  TArray<const UCesiumGltfGaussianSplatComponent*> Components;
  int32 ShCoefficientCount;
  int32 SplatCount;

  void Reset();
};

BEGIN_SHADER_PARAMETER_STRUCT(FGaussianSplatShaderParams, )
SHADER_PARAMETER_SRV(Buffer<uint>, TileIndices)
SHADER_PARAMETER_SRV(Buffer<float4>, TileTransforms)
SHADER_PARAMETER_SRV(Buffer<float4>, Positions)
SHADER_PARAMETER_SRV(Buffer<float3>, Scales)
SHADER_PARAMETER_SRV(Buffer<float4>, Orientations)
SHADER_PARAMETER_SRV(Buffer<float4>, Colors)
SHADER_PARAMETER_SRV(Buffer<uint>, SplatSHDegrees)
SHADER_PARAMETER_SRV(Buffer<float3>, SHNonZeroCoeffs)
END_SHADER_PARAMETER_STRUCT();

UCLASS()
class UCesiumGaussianSplatDataInterface : public UNiagaraDataInterface {
  GENERATED_BODY()

  UCesiumGaussianSplatDataInterface(const FObjectInitializer& Initializer);

public:
  virtual bool InitPerInstanceData(
      void* PerInstanceData,
      FNiagaraSystemInstance* SystemInstance) override;
  virtual void DestroyPerInstanceData(
      void* PerInstanceData,
      FNiagaraSystemInstance* SystemInstance) override;
  virtual bool PerInstanceTick(
      void* PerInstanceData,
      FNiagaraSystemInstance* SystemInstance,
      float DeltaSeconds) override;
  virtual int32 PerInstanceDataSize() const override;

  virtual bool HasPreSimulateTick() const override { return true; }
  virtual bool CanExecuteOnTarget(ENiagaraSimTarget target) const override;

  void MarkDirty();
  void MarkMatricesDirty();

protected:
#if WITH_EDITORONLY_DATA
  virtual void GetParameterDefinitionHLSL(
      const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
      FString& OutHLSL) override;
  virtual bool GetFunctionHLSL(
      const FNiagaraDataInterfaceGPUParamInfo& ParamInfo,
      const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo,
      int FunctionInstanceIndex,
      FString& OutHLSL) override;
  virtual bool
  AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const override;

  virtual void
  GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
#endif

  virtual void BuildShaderParameters(
      FNiagaraShaderParametersBuilder& Builder) const override;
  virtual void SetShaderParameters(
      const FNiagaraDataInterfaceSetShaderParametersContext& Context)
      const override;

  virtual void PostInitProperties() override;

private:
  bool _splatsDirty = true;
  bool _matricesDirty = true;

  TMap<FNiagaraSystemInstanceID, FNDICesiumGaussianSplats_InstanceData*>
      _systemInstancesToProxyData_GT;
};

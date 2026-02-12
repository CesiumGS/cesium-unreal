// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceBase.h"
#include "RHIUtilities.h"

#include "CesiumGaussianSplatDataInterface.generated.h"

class UCesiumGaussianSplatSubsystem;

struct FNDIGaussianSplatProxy : public FNiagaraDataInterfaceProxy {
  FNDIGaussianSplatProxy(class UCesiumGaussianSplatDataInterface* InOwner);

  TObjectPtr<class UCesiumGaussianSplatDataInterface> Owner = nullptr;
  FCriticalSection BufferLock;

  FReadBuffer SplatIndicesBuffer;
  FReadBuffer TileTransformsBuffer;
  FReadBuffer SplatSHDegreesBuffer;
  FReadBuffer PositionsBuffer;
  FReadBuffer ScalesBuffer;
  FReadBuffer OrientationsBuffer;
  FReadBuffer ColorsBuffer;
  FReadBuffer SHNonZeroCoeffsBuffer;

  bool bNeedsUpdate = true;
  bool bMatricesNeedUpdate = true;

  virtual int32 PerInstanceDataPassedToRenderThreadSize() const override {
    return 0;
  }

  void UploadToGPU(UCesiumGaussianSplatSubsystem* SplatSystem);
};

BEGIN_SHADER_PARAMETER_STRUCT(FGaussianSplatShaderParams, )
SHADER_PARAMETER(int, SplatsCount)
SHADER_PARAMETER_SRV(Buffer<uint>, SplatIndices)
SHADER_PARAMETER_SRV(Buffer<float4>, TileTransforms)
SHADER_PARAMETER_SRV(Buffer<float4>, Positions)
SHADER_PARAMETER_SRV(Buffer<float3>, Scales)
SHADER_PARAMETER_SRV(Buffer<float4>, Orientations)
SHADER_PARAMETER_SRV(Buffer<float4>, Colors)
SHADER_PARAMETER_SRV(Buffer<uint>, SplatSHDegrees)
SHADER_PARAMETER_SRV(Buffer<float3>, SHNonZeroCoeffs)
END_SHADER_PARAMETER_STRUCT()

UCLASS()
class UCesiumGaussianSplatDataInterface : public UNiagaraDataInterface {
  GENERATED_BODY()

  UCesiumGaussianSplatDataInterface(const FObjectInitializer& Initializer);

  virtual bool CanExecuteOnTarget(ENiagaraSimTarget target) const override;

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

  UCesiumGaussianSplatSubsystem* GetSubsystem() const;

public:
  void Refresh();
  void RefreshMatrices();

  FScopeLock LockGaussianBuffers();
};

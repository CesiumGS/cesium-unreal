// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceBase.h"
#include "CesiumGaussianSplatDataInterface.generated.h"

struct FNDIGaussianSplatProxy : public FNiagaraDataInterfaceProxy {
  FNDIGaussianSplatProxy(class UCesiumGaussianSplatDataInterface* InOwner);

  TObjectPtr<class UCesiumGaussianSplatDataInterface> Owner = nullptr;
  FCriticalSection BufferLock;

  FReadBuffer SplatIndicesBuffer;
  FReadBuffer SplatMatricesBuffer;
  FReadBuffer DataBuffer;
  FReadBuffer SplatSHInfoBuffer;

  bool bNeedsUpdate = true;
  bool bMatricesNeedUpdate = true;

  virtual int32 PerInstanceDataPassedToRenderThreadSize() const override {
    return 0;
  }

  void UploadToGPU();
};

BEGIN_SHADER_PARAMETER_STRUCT(FGaussianSplatShaderParams, )
SHADER_PARAMETER(int, SplatsCount)
SHADER_PARAMETER_SRV(Buffer<int>, SplatIndices)
SHADER_PARAMETER_SRV(Buffer<float4>, SplatMatrices)
SHADER_PARAMETER_SRV(Buffer<float4>, Positions)
SHADER_PARAMETER_SRV(Buffer<float4>, Scales)
SHADER_PARAMETER_SRV(Buffer<float4>, Orientations)
SHADER_PARAMETER_SRV(Buffer<float4>, SHZeroCoeffsAndOpacity)
SHADER_PARAMETER_SRV(Buffer<int>, SplatSHDegrees)
SHADER_PARAMETER_SRV(Buffer<float3>, SHNonZeroCoeffs)
END_SHADER_PARAMETER_STRUCT()

UCLASS()
class UCesiumGaussianSplatDataInterface
    : public UNiagaraDataInterface {
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
  virtual bool
  CopyToInternal(UNiagaraDataInterface* Destination) const override;
#endif

  virtual void BuildShaderParameters(
      FNiagaraShaderParametersBuilder& Builder) const override;
  virtual void SetShaderParameters(
      const FNiagaraDataInterfaceSetShaderParametersContext& Context)
      const override;

  virtual void PostInitProperties() override;

  virtual bool Equals(const UNiagaraDataInterface* Other) const override;

#if WITH_EDITOR
  virtual void PostEditChangeProperty(
      struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
  void Refresh() {
    this->GetProxyAs<FNDIGaussianSplatProxy>()->bNeedsUpdate = true;
    this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
  }

  void RefreshMatrices() {
    this->GetProxyAs<FNDIGaussianSplatProxy>()->bMatricesNeedUpdate = true;
  }
};

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
  FReadBuffer SplatOffsetsBuffer;
  FReadBuffer SplatMatricesBuffer;
  FReadBuffer DataBuffer;

  bool bNeedsUpdate = true;
  bool bMatricesNeedUpdate = true;

  virtual int32 PerInstanceDataPassedToRenderThreadSize() const override {
    return 0;
  }

  void UploadToGPU(UWorld* World);
};

BEGIN_SHADER_PARAMETER_STRUCT(FGaussianSplatShaderParams, )
SHADER_PARAMETER(int, SplatsCount)
SHADER_PARAMETER_SRV(Buffer<int>, SplatIndices)
SHADER_PARAMETER_SRV(Buffer<int>, SplatOffsets)
SHADER_PARAMETER_SRV(Buffer<float4>, SplatMatrices)
SHADER_PARAMETER_SRV(Buffer<float4>, Data)
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

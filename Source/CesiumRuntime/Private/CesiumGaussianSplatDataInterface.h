// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceBase.h"
#include "RHIUtilities.h"

#include "CesiumGaussianSplatDataInterface.generated.h"

class UCesiumGaussianSplatSubsystem;
class UCesiumGltfGaussianSplatComponent;

struct FNDICesiumGaussianSplats_InstanceData_RenderThread {
  ~FNDICesiumGaussianSplats_InstanceData_RenderThread();

  FReadBuffer TileIndicesBuffer;
  FReadBuffer TileTransformsBuffer;
  FReadBuffer SplatSHDegreesBuffer;
  FReadBuffer PositionsBuffer;
  FReadBuffer ScalesBuffer;
  FReadBuffer OrientationsBuffer;
  FReadBuffer ColorsBuffer;
  FReadBuffer SHNonZeroCoeffsBuffer;
};

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
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumTileContentInspector.generated.h"

UCLASS()
class ACesiumTileContentInspectorActor : public AActor {
  GENERATED_BODY()

public:
  ACesiumTileContentInspectorActor();

  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetUrl,
      BlueprintSetter = SetUrl,
      Category = "Cesium")
  FString Url;

  UFUNCTION(BlueprintGetter, Category = "Cesium|Debug")
  const FString &GetUrl() const;

  UFUNCTION(BlueprintSetter, Category = "Cesium|Debug")
  void SetUrl(const FString &InUrl);

  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;

  // UObject overrides
#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
  void LoadContent();

  USceneComponent* _pComponent;
  bool _isInit;
};
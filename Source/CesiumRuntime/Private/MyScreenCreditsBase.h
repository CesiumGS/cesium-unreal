// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CesiumCreditSystem.h"
#include "CoreMinimal.h"
#include "Engine/Texture2DDynamic.h"
#include "MyScreenCreditsBase.generated.h"

namespace Cesium3DTilesSelection {
class CreditSystem;
struct Credit;
} // namespace Cesium3DTilesSelection

UCLASS()
class UMyScreenCreditsBase : public UUserWidget {
  GENERATED_BODY()

  UMyScreenCreditsBase(const FObjectInitializer& ObjectInitializer);
  virtual void NativeConstruct() override;
  UFUNCTION(BlueprintCallable)
  void Update();

  UPROPERTY(meta = (BindWidget))
  class URichTextBlock* RichTextBlock_127;

private:
  FString ConvertCreditToRTF(const Cesium3DTilesSelection::Credit* credit);

  std::shared_ptr<Cesium3DTilesSelection::CreditSystem> _pCreditSystem;
  size_t _lastCreditsCount;
  TMap<const Cesium3DTilesSelection::Credit*, FString> _creditToRTF;
  UMyRichTextBlockDecorator* _imageDecorator;
};

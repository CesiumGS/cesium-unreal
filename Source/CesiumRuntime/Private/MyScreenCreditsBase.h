// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include <memory>
#include <string>
#include "MyScreenCreditsBase.generated.h"

namespace Cesium3DTilesSelection {
class CreditSystem;
struct Credit;
} // namespace Cesium3DTilesSelection

DECLARE_DELEGATE(FOnPopupClicked)

UCLASS()
class UMyScreenCreditsBase : public UUserWidget {
  GENERATED_BODY()
public:
  std::string LoadImage(const std::string& url);

private:
  UMyScreenCreditsBase(const FObjectInitializer& ObjectInitializer);
  virtual void NativeConstruct() override;
  UFUNCTION(BlueprintCallable)
  void Update();

  void OnPopupClicked();

  UPROPERTY(meta = (BindWidget))
  class URichTextBlock* RichTextBlock_127;

  FString ConvertCreditToRTF(const Cesium3DTilesSelection::Credit* credit);
  void HandleImageRequest(
      FHttpRequestPtr HttpRequest,
      FHttpResponsePtr HttpResponse,
      bool bSucceeded,
      int32 id);
  std::shared_ptr<Cesium3DTilesSelection::CreditSystem> _pCreditSystem;
  size_t _lastCreditsCount;
  TMap<const Cesium3DTilesSelection::Credit*, FString> _creditToRTF;
  class UMyRichTextBlockDecorator* _imageDecorator;
  FString _output;
  bool _showPopup;
};

UCLASS()
class UMyRichTextBlockDecorator : public URichTextBlockDecorator {
  GENERATED_BODY()

public:
  UMyRichTextBlockDecorator(const FObjectInitializer& ObjectInitializer);

  virtual TSharedPtr<ITextDecorator>
  CreateDecorator(URichTextBlock* InOwner) override;

  virtual const FSlateBrush* FindImageBrush(int32 id);

private:
  TArray<FSlateDynamicImageBrush*> _textureResources;
  FOnPopupClicked EventHandler;
  friend class UMyScreenCreditsBase;
  friend class FRichInlineImage;
};

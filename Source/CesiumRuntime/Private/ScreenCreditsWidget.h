// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include <memory>
#include <string>
#include "ScreenCreditsWidget.generated.h"

DECLARE_DELEGATE(FOnPopupClicked)

UCLASS()
class UScreenCreditsWidget : public UUserWidget {
  GENERATED_BODY()
public:
  /**
   * Attempts to load an image from the given URL and returns the name of the
   * image to be referenced in RTF.
   */
  std::string LoadImage(const std::string& url);

  void SetCredits(const FString& InCredits, const FString& InOnScreenCredits);

private:
  UScreenCreditsWidget(const FObjectInitializer& ObjectInitializer);
  ~UScreenCreditsWidget();
  virtual void NativeConstruct() override;

  void OnPopupClicked();

  void HandleImageRequest(
      FHttpRequestPtr HttpRequest,
      FHttpResponsePtr HttpResponse,
      bool bSucceeded,
      int32 id);

  UPROPERTY(meta = (BindWidget))
  class URichTextBlock* RichTextOnScreen;

  UPROPERTY(meta = (BindWidget))
  class URichTextBlock* RichTextPopup;

  UPROPERTY(meta = (BindWidget))
  class UBackgroundBlur* BackgroundBlur;

  UPROPERTY()
  TArray<UTexture2D*> _textures;

  FString _credits = "";
  FString _onScreenCredits = "";
  bool _showPopup = false;
  class UCreditsDecorator* _decoratorOnScreen;
  class UCreditsDecorator* _decoratorPopup;
  int32 _numImagesLoading;
  FSlateFontInfo _font;
  TArray<FSlateBrush*> _creditImages;
  friend class UCreditsDecorator;
};

UCLASS()
class UCreditsDecorator : public URichTextBlockDecorator {
  GENERATED_BODY()

public:
  UCreditsDecorator(const FObjectInitializer& ObjectInitializer);

  virtual TSharedPtr<ITextDecorator>
  CreateDecorator(URichTextBlock* InOwner) override;

  virtual const FSlateBrush* FindImageBrush(int32 id);

  UScreenCreditsWidget* CreditsWidget;
  FOnPopupClicked PopupClicked;
};

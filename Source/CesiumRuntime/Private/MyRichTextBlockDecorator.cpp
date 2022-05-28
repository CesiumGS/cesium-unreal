// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "MyRichTextBlockDecorator.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/DefaultValueHelper.h"
#include "Modules/ModuleManager.h"
#include "Rendering/DrawElements.h"
#include "Runtime/Engine/Public/ImageUtils.h"
#include "Runtime/Online/HTTP/Public/HttpModule.h"
#include "Slate/SlateGameResources.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SCompoundWidget.h"

class SRichInlineImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SRichInlineImage) {}
  SLATE_END_ARGS()

public:
  void Construct(const FArguments& InArgs, const FSlateBrush* Brush) {
    if (ensure(Brush)) {
      ChildSlot[SNew(SImage).Image(Brush)];
    }
  }
};

class FRichInlineImage : public FRichTextDecorator {
public:
  FRichInlineImage(
      URichTextBlock* InOwner,
      UMyRichTextBlockDecorator* InDecorator)
      : FRichTextDecorator(InOwner), Decorator(InDecorator) {}

  virtual bool Supports(
      const FTextRunParseResults& RunParseResult,
      const FString& Text) const override {
    if (RunParseResult.Name == TEXT("img") &&
        RunParseResult.MetaData.Contains(TEXT("id"))) {
      return true;
    }
    return false;
  }

protected:
  virtual TSharedPtr<SWidget> CreateDecoratorWidget(
      const FTextRunInfo& RunInfo,
      const FTextBlockStyle& TextStyle) const override {

    int32 id = FCString::Atoi(*RunInfo.MetaData[TEXT("id")]);
    const FSlateBrush* Brush = Decorator->FindImageBrush(id);

    if (Brush) {
      return SNew(SRichInlineImage, Brush);
    }
    return TSharedPtr<SWidget>();
  }

private:
  UMyRichTextBlockDecorator* Decorator;
};

UMyRichTextBlockDecorator::UMyRichTextBlockDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

TSharedPtr<ITextDecorator>
UMyRichTextBlockDecorator::CreateDecorator(URichTextBlock* InOwner) {
  return MakeShareable(new FRichInlineImage(InOwner, this));
}

const FSlateBrush* UMyRichTextBlockDecorator::FindImageBrush(int32 id) {
  if (_textureResources.Num() > id) {
    return _textureResources[id];
  }
  return nullptr;
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "MyRichTextBlockDecorator.h"
#include "Blueprint/AsyncTaskDownloadImage.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Base64.h"
#include "Misc/DefaultValueHelper.h"
#include "Rendering/DrawElements.h"
#include "Slate/SlateGameResources.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SCompoundWidget.h"
#include <Runtime/Engine/Public/ImageUtils.h>

class SRichInlineImage : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SRichInlineImage) {}
  SLATE_END_ARGS()

public:
  void Construct(
      const FArguments& InArgs,
      const FSlateBrush* Brush,
      const FTextBlockStyle& TextStyle,
      TOptional<int32> Width,
      TOptional<int32> Height,
      EStretch::Type Stretch) {
    if (ensure(Brush)) {
      const TSharedRef<FSlateFontMeasure> FontMeasure =
          FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
      float IconHeight = FMath::Min(
          (float)FontMeasure->GetMaxCharacterHeight(TextStyle.Font, 1.0f),
          Brush->ImageSize.Y);
      float IconWidth = IconHeight;

      if (Width.IsSet()) {
        IconWidth = Width.GetValue();
      }

      if (Height.IsSet()) {
        IconHeight = Height.GetValue();
      }

      ChildSlot[SNew(SBox)
                    .HeightOverride(IconHeight)
                    .WidthOverride(IconWidth)
                        [SNew(SScaleBox)
                             .Stretch(Stretch)
                             .StretchDirection(EStretchDirection::DownOnly)
                             .VAlign(
                                 VAlign_Center)[SNew(SImage).Image(Brush)]]];
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
      const FTextRange& IdRange = RunParseResult.MetaData[TEXT("id")];
      const FString TagId =
          Text.Mid(IdRange.BeginIndex, IdRange.EndIndex - IdRange.BeginIndex);

      const bool bWarnIfMissing = false;
      return true;
      /// return Decorator->FindImageBrush(*TagId, bWarnIfMissing) != nullptr;
    }

    return false;
  }

protected:
  virtual TSharedPtr<SWidget> CreateDecoratorWidget(
      const FTextRunInfo& RunInfo,
      const FTextBlockStyle& TextStyle) const override {
    const bool bWarnIfMissing = true;
    const FSlateBrush* Brush = Decorator->FindImageBrush(
        *RunInfo.MetaData[TEXT("id")],
        bWarnIfMissing);

    if (Brush) {
      TOptional<int32> Width;
      if (const FString* WidthString = RunInfo.MetaData.Find(TEXT("width"))) {
        int32 WidthTemp;
        if (FDefaultValueHelper::ParseInt(*WidthString, WidthTemp)) {
          Width = WidthTemp;
        } else {
          if (FCString::Stricmp(GetData(*WidthString), TEXT("desired")) == 0) {
            Width = Brush->ImageSize.X;
          }
        }
      }

      TOptional<int32> Height;
      if (const FString* HeightString = RunInfo.MetaData.Find(TEXT("height"))) {
        int32 HeightTemp;
        if (FDefaultValueHelper::ParseInt(*HeightString, HeightTemp)) {
          Height = HeightTemp;
        } else {
          if (FCString::Stricmp(GetData(*HeightString), TEXT("desired")) == 0) {
            Height = Brush->ImageSize.Y;
          }
        }
      }

      return SNew(
          SRichInlineImage,
          Brush,
          TextStyle,
          Width,
          Height,
          EStretch::ScaleToFit);
    }
    return TSharedPtr<SWidget>();
  }

private:
  UMyRichTextBlockDecorator* Decorator;
};

void UMyRichTextBlockDecorator::OnImageSuccess(UTexture2DDynamic* Texture) {
  UE_LOG(LogTemp, Warning, TEXT("Image loading successful"));
}
UFUNCTION()
void UMyRichTextBlockDecorator::OnImageFailure(UTexture2DDynamic* Texture) {
  UE_LOG(LogTemp, Warning, TEXT("Image loading failed"));
}

UMyRichTextBlockDecorator::UMyRichTextBlockDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

void UMyRichTextBlockDecorator::LoadImage(const std::string& url) {

  const std::string base_64_prefix = "data:image/png;base64,";

  if (url.rfind(base_64_prefix, 0) == 0) {
    TArray<uint8> data_buffer;
    FString base64 = UTF8_TO_TCHAR(url.c_str() + base_64_prefix.length());
    if (FBase64::Decode(base64, data_buffer)) {
      UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(data_buffer);
      Texture->MipGenSettings = TMGS_NoMipmaps;
      Texture->CompressionSettings =
          TextureCompressionSettings::TC_VectorDisplacementmap;
      Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
      Texture->SRGB = false;
      Texture->Filter = TextureFilter::TF_Nearest;
      Texture->UpdateResource();
    }
  } else {
    UAsyncTaskDownloadImage* asyncTask =
        UAsyncTaskDownloadImage::DownloadImage(UTF8_TO_TCHAR(url.c_str()));

    UE_LOG(LogTemp, Warning, TEXT("adding the delegate!"));

    FScriptDelegate DelegateSuccess;
    DelegateSuccess.BindUFunction(this, TEXT("OnImageSuccess"));
    asyncTask->OnSuccess.Add(DelegateSuccess);

    FScriptDelegate DelegateFailure;
    DelegateFailure.BindUFunction(this, TEXT("OnImageFailure"));
    asyncTask->OnFail.Add(DelegateFailure);
  }
}

TSharedPtr<ITextDecorator>
UMyRichTextBlockDecorator::CreateDecorator(URichTextBlock* InOwner) {
  return MakeShareable(new FRichInlineImage(InOwner, this));
}

const FSlateBrush*
UMyRichTextBlockDecorator::FindImageBrush(FName TagOrId, bool bWarnIfMissing) {
  const FRichImageRow* ImageRow = FindImageRow(TagOrId, bWarnIfMissing);
  if (ImageRow) {
    return &ImageRow->Brush;
  }

  return nullptr;
}

FRichImageRow*
UMyRichTextBlockDecorator::FindImageRow(FName TagOrId, bool bWarnIfMissing) {
  if (ImageSet) {
    FString ContextString;
    return ImageSet->FindRow<FRichImageRow>(
        TagOrId,
        ContextString,
        bWarnIfMissing);
  }

  return nullptr;
}

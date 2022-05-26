// Fill out your copyright notice in the Description page of Project Settings.

#include "MyRichTextBlockDecorator.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "Math/UnrealMathUtility.h"
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

UMyRichTextBlockDecorator::UMyRichTextBlockDecorator(
    const FObjectInitializer& ObjectInitializer)
    : URichTextBlockDecorator(ObjectInitializer) {}

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

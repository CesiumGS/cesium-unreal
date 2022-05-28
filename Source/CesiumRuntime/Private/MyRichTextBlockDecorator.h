// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/ITextDecorator.h"
#include "Framework/Text/TextLayout.h"
#include "Interfaces/IHttpRequest.h"
#include "Styling/SlateTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "MyRichTextBlockDecorator.generated.h"

class ISlateStyle;
struct FRichImageRow;
struct FSlateDynamicImageBrush;

/**
 *
 */
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

  friend class UMyScreenCreditsBase;
};

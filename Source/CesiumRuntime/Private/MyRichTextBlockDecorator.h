// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/RichTextBlockDecorator.h"
#include "CoreMinimal.h"
#include "MyRichTextBlockDecorator.generated.h"

struct FSlateDynamicImageBrush;

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

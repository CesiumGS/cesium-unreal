// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include <functional>
#include "SampleHeightCallbackReceiver.generated.h"

UCLASS()
class USampleHeightCallbackReceiver : public UObject {
  GENERATED_BODY()

public:
  using TFunction = std::function<
      void(const TArray<FCesiumSampleHeightResult>&, const TArray<FString>&)>;

  static void
  Bind(FCesiumSampleHeightMostDetailedComplete& delegate, TFunction callback) {
    USampleHeightCallbackReceiver* p =
        NewObject<USampleHeightCallbackReceiver>();
    p->_callback = callback;
    p->AddToRoot();

    delegate.AddUniqueDynamic(p, &USampleHeightCallbackReceiver::Receiver);
  }

private:
  UFUNCTION()
  void Receiver(
      const TArray<FCesiumSampleHeightResult>& Result,
      const TArray<FString>& Warnings) {
    this->_callback(Result, Warnings);
    this->RemoveFromRoot();
  }

  TFunction _callback;
};

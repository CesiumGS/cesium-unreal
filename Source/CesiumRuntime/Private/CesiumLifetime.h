// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once
#include "Components/SceneComponent.h"
#include "Containers/Array.h"
#include "Tickable.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UObject;
class UTexture;

class AmortizedDestructor : FTickableGameObject {
public:
  void Tick(float DeltaTime) override;
  ETickableTickType GetTickableTickType() const override;
  bool IsTickableWhenPaused() const override;
  bool IsTickableInEditor() const override;
  TStatId GetStatId() const;
  void destroy(UObject* pObject);

private:
  bool runDestruction(UObject* pObject) const;
  void addToPending(UObject* pObject);
  void processPending();
  void finalizeDestroy(UObject* pObject) const;

  TArray<TWeakObjectPtr<UObject>> _pending;
  TArray<TWeakObjectPtr<UObject>> _nextPending;
};

class CesiumLifetime {
public:
  static void destroy(UObject* pObject);
  static void destroyComponentRecursively(USceneComponent* pComponent);

private:
  static AmortizedDestructor amortizedDestructor;
};

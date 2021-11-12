// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

class UObject;
class UTexture;

class CesiumLifetime {
public:
  static void destroy(UObject* pObject);

private:
  static bool runDestruction(UObject* pObject);
  static void addToPending(UObject* pObject);
  static void processPending();
  static void finalizeDestroy(UObject* pObject);

  static TArray<TWeakObjectPtr<UObject>> _pending;
  static TArray<TWeakObjectPtr<UObject>> _nextPending;
  static bool _isScheduled;
};

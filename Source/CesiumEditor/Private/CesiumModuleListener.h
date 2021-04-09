// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

class ICesiumModuleListener {
public:
  virtual void OnStartupModule(){};
  virtual void OnShutdownModule(){};
};
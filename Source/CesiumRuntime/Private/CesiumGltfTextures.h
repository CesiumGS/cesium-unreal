// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include <CesiumAsync/Future.h>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumGltf {
struct Model;
}

class CesiumGltfTextures {
public:
  /**
   * Creates all of the textures that are required by the given glTF, and adds
   * `ExtensionUnrealTexture` to each.
   */
  static CesiumAsync::Future<void> createInWorkerThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      CesiumGltf::Model& model);
};

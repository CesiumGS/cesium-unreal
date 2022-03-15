from conan import ConanFile
import os

class CesiumUnrealConan(ConanFile):
    name = "cesium-unreal"
    version = "0.0.0"
    license = "Apache-2.0"
    author = "CesiumGS, Inc. and Contributors"
    url = "https://github.com/CesiumGS/cesium-unreal"
    description = "Top-level package for installing dependencies needed by Cesium for Unreal"
    topics = () # TODO
    settings = "os", "compiler", "build_type", "arch"
    options = []
    generators = "CMakeDeps"

    def requirements(self):
      self.requires("cesium-native/0.12.0@user/dev")
      self.requires("openssl/UE" + os.getenv("UNREAL_ENGINE_VERSION"), override=True)

    def layout(self):
      self.folders.source = "."
      self.folders.build = "build"
      self.folders.generators = self.folders.build + "/conan"
      self.cpp.source.includedirs = []
      self.cpp.source.libdirs = []

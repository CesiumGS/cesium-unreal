from conans import ConanFile, tools
import json
import os

class OpenSSLConan(ConanFile):
    name = "openssl"
    description = "Allow the version of OpenSSL bundled with Unreal Engine 4.26 to be used by cesium-native"
    version = "UE4.26"
    settings = ["os", "arch", "compiler"]

    def layout(self):
        # See OpenSSL.Build.cs from the Unreal Engine source code.
        # This recipe is meant to mirror that file, but make it useable by cesium-native.

        UEThirdPartySourceDirectory = os.path.join(os.getenv("UNREAL_ENGINE_DIR"), "Engine", "Source", "ThirdParty")
        OpenSSL101sPath = os.path.join(UEThirdPartySourceDirectory, "OpenSSL", "1_0_1s")
        OpenSSL111Path = os.path.join(UEThirdPartySourceDirectory, "OpenSSL", "1.1.1")
        OpenSSL111cPath = os.path.join(UEThirdPartySourceDirectory, "OpenSSL", "1.1.1c")

        # TODO: macOS and iOS

        if self.settings.os == "Windows":
            if self.settings.arch == "x86_64":
                platformSubdir = "Win64"
            elif self.settings.arch == "x86":
                platformSubdir = "Win32"
            else:
                platformSubdir = self.settings.arch # Probably won't work, but let's try.

            # VS2015 through VS2022 can all link against libs built for VS2015.
            vsVersion = "VS2015"

            self.cpp.source.includedirs = [
                os.path.join(OpenSSL111Path, "include", platformSubdir, vsVersion)
            ]

            if self.settings.compiler.runtime == "MDd":
                configFolder = "Debug"
            else:
                configFolder = "Release"

            self.cpp.source.libdirs = [
                os.path.join(OpenSSL111Path, "lib", platformSubdir, vsVersion, configFolder)
            ]
            self.cpp.source.libs = [
                "libssl.lib",
                "libcrypto.lib"
            ]

            self.cpp.source.system_libs = ["crypt32.lib"]

        elif self.settings.os == "Linux":
            # TODO
            pass

    def package_id(self):
        # We'll use the same pre-built binaries no matter the build_type or runtime.
        del self.info.settings.build_type
        del self.info.settings.compiler.runtime

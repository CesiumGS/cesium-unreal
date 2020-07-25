#define TINYGLTF_USE_CPP14 
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#define TINYGLTF_ENABLE_DRACO

#ifdef _MSC_VER
#pragma warning(disable:4996 4946 4100 4189 4127)
#endif

#include "tiny_gltf.h"

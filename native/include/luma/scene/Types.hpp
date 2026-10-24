#pragma once
#include <cstdint>
namespace luma {
struct EntityId {
    std::uint32_t value = 0;
};
struct MeshId {
    std::uint32_t value = 0;
};
struct TextureId {
    std::uint32_t value = 0;
};
struct MaterialId {
    std::uint32_t value = 0;
};
enum class Primitive { Cube, Plane, Triangle };
enum class AlphaMode { Opaque, Mask, Blend };
enum class CameraMode { Orbit, Fly };
} // namespace luma

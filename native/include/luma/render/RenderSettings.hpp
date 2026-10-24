#pragma once
#include <glm/glm.hpp>
#include <luma/scene/Types.hpp>
namespace luma {
enum class StyleKind { Basic, Wireframe };
struct RenderOptions {
    bool depth = true, cull = true, blend = true, outline = true;
    EntityId selected{};
    glm::vec4 background{0.025f, 0.035f, 0.06f, 1};
};
} // namespace luma

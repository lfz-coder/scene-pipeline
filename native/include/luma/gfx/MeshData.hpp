#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>
namespace luma {
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color{1};
};
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    void validate() const;
};
struct PrimitiveFactory {
    static MeshData cube();
    static MeshData plane();
    static MeshData triangle();
};
} // namespace luma

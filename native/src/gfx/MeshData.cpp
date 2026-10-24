#include <cmath>
#include <limits>
#include <luma/gfx/MeshData.hpp>
#include <stdexcept>
namespace luma {
void MeshData::validate() const {
    if (vertices.empty() || indices.empty() || indices.size() % 3 ||
        indices.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument(
            "mesh requires complete indexed triangles within GLsizei range");
    for (auto i : indices)
        if (i >= vertices.size())
            throw std::invalid_argument("mesh index outside vertex array");
    for (const auto& v : vertices) {
        for (int i = 0; i < 3; ++i)
            if (!std::isfinite(v.position[i]) || !std::isfinite(v.normal[i]))
                throw std::invalid_argument("nonfinite mesh vertex");
    }
}
MeshData PrimitiveFactory::cube() {
    MeshData m;
    const glm::vec3 normals[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                 {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    for (auto n : normals) {
        glm::vec3 u = std::abs(n.y) > 0.5f ? glm::vec3(1, 0, 0)
                                           : glm::normalize(glm::cross(glm::vec3(0, 1, 0), n));
        auto v = glm::cross(n, u);
        auto base = static_cast<std::uint32_t>(m.vertices.size());
        for (auto uv : {glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)})
            m.vertices.push_back(
                {(n + u * (uv.x * 2 - 1) + v * (uv.y * 2 - 1)) * 0.5f, n, uv, {1, 1, 1, 1}});
        for (auto i : {0u, 1u, 2u, 2u, 3u, 0u})
            m.indices.push_back(base + i);
    }
    return m;
}
MeshData PrimitiveFactory::plane() {
    return {{{{-0.5f, 0, -0.5f}, {0, 1, 0}, {0, 0}},
             {{-0.5f, 0, 0.5f}, {0, 1, 0}, {0, 1}},
             {{0.5f, 0, 0.5f}, {0, 1, 0}, {1, 1}},
             {{0.5f, 0, -0.5f}, {0, 1, 0}, {1, 0}}},
            {0, 1, 2, 2, 3, 0}};
}
MeshData PrimitiveFactory::triangle() {
    return {{{{-0.7f, -0.5f, 0}, {0, 0, 1}, {0, 0}, {1, 0.25f, 0.25f, 1}},
             {{0.7f, -0.5f, 0}, {0, 0, 1}, {1, 0}, {0.3f, 1, 0.4f, 1}},
             {{0, 0.7f, 0}, {0, 0, 1}, {0.5f, 1}, {0.4f, 0.5f, 1, 1}}},
            {0, 1, 2}};
}
} // namespace luma

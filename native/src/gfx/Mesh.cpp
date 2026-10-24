#include <cstddef>
#include <luma/gfx/Mesh.hpp>
#include <luma/gfx/RenderState.hpp>
#include <stdexcept>
#include <utility>
namespace luma {
void Mesh::release() noexcept {
    if (ebo_)
        glDeleteBuffers(1, &ebo_);
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = ebo_ = 0;
    count_ = 0;
}
Mesh::~Mesh() {
    release();
}
Mesh::Mesh(Mesh&& o) noexcept
    : vao_(std::exchange(o.vao_, 0)), vbo_(std::exchange(o.vbo_, 0)),
      ebo_(std::exchange(o.ebo_, 0)), count_(std::exchange(o.count_, 0)) {}
Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        release();
        vao_ = std::exchange(o.vao_, 0);
        vbo_ = std::exchange(o.vbo_, 0);
        ebo_ = std::exchange(o.ebo_, 0);
        count_ = std::exchange(o.count_, 0);
    }
    return *this;
}
Mesh Mesh::upload(const MeshData& d) {
    requireContext();
    d.validate();
    Mesh m;
    glGenVertexArrays(1, &m.vao_);
    glGenBuffers(1, &m.vbo_);
    glGenBuffers(1, &m.ebo_);
    glBindVertexArray(m.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo_);
    glBufferData(GL_ARRAY_BUFFER, d.vertices.size() * sizeof(Vertex), d.vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, d.indices.size() * sizeof(std::uint32_t),
                 d.indices.data(), GL_STATIC_DRAW);
    const GLint sizes[] = {3, 3, 2, 4};
    const size_t offsets[] = {offsetof(Vertex, position), offsetof(Vertex, normal),
                              offsetof(Vertex, uv), offsetof(Vertex, color)};
    for (unsigned i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(offsets[i]));
    }
    m.count_ = static_cast<int>(d.indices.size());
    glBindVertexArray(0);
    checkGl("mesh upload");
    return m;
}
void Mesh::draw() const {
    requireContext();
    if (!vao_)
        throw std::logic_error("draw empty Mesh");
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
}
} // namespace luma

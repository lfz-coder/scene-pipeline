#pragma once
#include <luma/gfx/MeshData.hpp>
namespace luma {
// Move-only VAO/VBO/EBO ownership. upload validates CPU data before GL calls.
class Mesh {
  public:
    Mesh() = default;
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;
    static Mesh upload(const MeshData& data);
    void draw() const;
    unsigned handle() const {
        return vao_;
    }

  private:
    unsigned vao_ = 0, vbo_ = 0, ebo_ = 0;
    int count_ = 0;
    void release() noexcept;
};
} // namespace luma

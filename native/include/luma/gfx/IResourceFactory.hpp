#pragma once
#include <filesystem>
#include <luma/gfx/MeshData.hpp>
#include <luma/gfx/TextureOptions.hpp>
#include <memory>
#include <string_view>
namespace luma {
// A draw pass calls use() before typed uniform writes; adapters may also bind in set().
class IShader {
  public:
    virtual ~IShader() = default;
    virtual void use() const = 0;
    virtual void set(std::string_view, int) = 0;
    virtual void set(std::string_view, float) = 0;
    virtual void set(std::string_view, const glm::vec3&) = 0;
    virtual void set(std::string_view, const glm::vec4&) = 0;
    virtual void set(std::string_view, const glm::mat3&) = 0;
    virtual void set(std::string_view, const glm::mat4&) = 0;
};
class IMesh {
  public:
    virtual ~IMesh() = default;
    virtual void draw() const = 0;
};
class ITexture {
  public:
    virtual ~ITexture() = default;
    virtual void bind(unsigned unit) const = 0;
};
// Factory outlives its consumers; owned resources die before their GL context.
// Failure throws or returns null; consumers reject null before publication.
class IResourceFactory {
  public:
    virtual ~IResourceFactory() = default;
    virtual std::unique_ptr<IShader> createShader(const std::filesystem::path& vertex,
                                                  const std::filesystem::path& fragment) = 0;
    virtual std::unique_ptr<IMesh> createMesh(const MeshData&) = 0;
    virtual std::unique_ptr<ITexture> createTexture(const std::filesystem::path&,
                                                    const TextureOptions&) = 0;
};
} // namespace luma

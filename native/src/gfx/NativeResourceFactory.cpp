#include <luma/gfx/Mesh.hpp>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/ShaderProgram.hpp>
#include <luma/gfx/Texture2D.hpp>
namespace luma {
namespace {
class NativeShader final : public IShader {
    ShaderProgram resource_;

  public:
    explicit NativeShader(ShaderProgram resource) : resource_(std::move(resource)) {}
    void use() const override {
        resource_.use();
    }
    void set(std::string_view name, int value) override {
        resource_.set(name, value);
    }
    void set(std::string_view name, float value) override {
        resource_.set(name, value);
    }
    void set(std::string_view name, const glm::vec3& value) override {
        resource_.set(name, value);
    }
    void set(std::string_view name, const glm::vec4& value) override {
        resource_.set(name, value);
    }
    void set(std::string_view name, const glm::mat3& value) override {
        resource_.set(name, value);
    }
    void set(std::string_view name, const glm::mat4& value) override {
        resource_.set(name, value);
    }
};
class NativeMesh final : public IMesh {
    Mesh resource_;

  public:
    explicit NativeMesh(Mesh resource) : resource_(std::move(resource)) {}
    void draw() const override {
        resource_.draw();
    }
};
class NativeTexture final : public ITexture {
    Texture2D resource_;

  public:
    explicit NativeTexture(Texture2D resource) : resource_(std::move(resource)) {}
    void bind(unsigned unit) const override {
        resource_.bind(unit);
    }
};
} // namespace
std::unique_ptr<IShader> NativeResourceFactory::createShader(const std::filesystem::path& v,
                                                             const std::filesystem::path& f) {
    return std::make_unique<NativeShader>(ShaderProgram::fromFiles(v, f));
}
std::unique_ptr<IMesh> NativeResourceFactory::createMesh(const MeshData& data) {
    return std::make_unique<NativeMesh>(Mesh::upload(data));
}
std::unique_ptr<ITexture> NativeResourceFactory::createTexture(const std::filesystem::path& path,
                                                               const TextureOptions& options) {
    return std::make_unique<NativeTexture>(Texture2D::fromFile(path, options));
}
} // namespace luma

#include <iostream>
#include <luma/gfx/RenderState.hpp>
#include <luma/render/Renderer.hpp>
#include <luma/scene/AssetStore.hpp>
#include <stdexcept>
using namespace luma;
void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejectsMessage(F f, std::string_view expected) {
    try {
        f();
    } catch (const std::exception& error) {
        check(std::string(error.what()).find(expected) != std::string::npos,
              "wrong resource failure");
        return;
    }
    throw std::runtime_error("expected resource failure");
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, "expected resource failure");
}
struct FakeMesh : IMesh {
    int& live;
    explicit FakeMesh(int& n) : live(n) {
        ++live;
    }
    ~FakeMesh() {
        --live;
    }
    void draw() const override {}
};
struct FakeTexture : ITexture {
    int& live;
    explicit FakeTexture(int& n) : live(n) {
        ++live;
    }
    ~FakeTexture() {
        --live;
    }
    void bind(unsigned) const override {}
};
struct FakeShader : IShader {
    int& live;
    explicit FakeShader(int& n) : live(n) {
        ++live;
    }
    ~FakeShader() {
        --live;
    }
    void use() const override {}
    void set(std::string_view, int) override {}
    void set(std::string_view, float) override {}
    void set(std::string_view, const glm::vec3&) override {}
    void set(std::string_view, const glm::vec4&) override {}
    void set(std::string_view, const glm::mat3&) override {}
    void set(std::string_view, const glm::mat4&) override {}
};
struct FakeFactory : IResourceFactory {
    int meshes = 0, textures = 0, liveMeshes = 0, liveTextures = 0, shaders = 0, liveShaders = 0,
        nullShader = 1;
    bool fail = false, null = false;
    std::unique_ptr<IShader> createShader(const std::filesystem::path&,
                                          const std::filesystem::path&) override {
        ++shaders;
        if (shaders == nullShader)
            return {};
        return std::make_unique<FakeShader>(liveShaders);
    }
    std::unique_ptr<IMesh> createMesh(const MeshData& data) override {
        ++meshes;
        data.validate();
        if (fail)
            throw std::runtime_error("upload");
        if (null)
            return {};
        return std::make_unique<FakeMesh>(liveMeshes);
    }
    std::unique_ptr<ITexture> createTexture(const std::filesystem::path&,
                                            const TextureOptions&) override {
        ++textures;
        if (fail)
            throw std::runtime_error("decode");
        if (null)
            return {};
        return std::make_unique<FakeTexture>(liveTextures);
    }
};
int main() {
    rejects([] { requireContext(); });
    FakeFactory shaderFactory;
    {
        Renderer renderer(shaderFactory);
        rejectsMessage([&] { renderer.initialize("unused"); }, "null shader");
    }
    check(shaderFactory.shaders == 1 && shaderFactory.liveShaders == 0,
          "null renderer shader not rejected before display");
    shaderFactory.shaders = 0;
    shaderFactory.nullShader = 2;
    {
        Renderer renderer(shaderFactory);
        rejectsMessage([&] { renderer.initialize("unused"); }, "null display shader");
    }
    check(shaderFactory.shaders == 2 && shaderFactory.liveShaders == 0,
          "display failure leaked first shader");
    shaderFactory.shaders = 0;
    shaderFactory.nullShader = 1;
    {
        DisplayTransfer transfer(shaderFactory);
        rejectsMessage([&] { transfer.initialize("unused"); }, "null display shader");
    }
    check(shaderFactory.liveShaders == 0, "null display shader leak");
    TextureOptions invalid;
    invalid.wrap = static_cast<Wrap>(99);
    rejects([&] { invalid.validate(); });
    FakeFactory factory;
    {
        AssetStore assets(factory);
        factory.fail = true;
        rejects([&] { assets.primitive(Primitive::Cube); });
        factory.fail = false;
        factory.null = true;
        rejects([&] { assets.primitive(Primitive::Cube); });
        factory.null = false;
        auto cube = assets.primitive(Primitive::Cube);
        check(assets.primitive(Primitive::Cube).value == cube.value && factory.meshes == 3,
              "mesh cached failure or missed success");
        factory.fail = true;
        rejects([&] { assets.loadTexture("missing.png"); });
        factory.fail = false;
        factory.null = true;
        rejects([&] { assets.loadTexture("missing.png"); });
        factory.null = false;
        auto texture = assets.loadTexture("missing.png");
        check(assets.loadTexture("./missing.png").value == texture.value && factory.textures == 3,
              "texture cache");
        TextureOptions options;
        options.flipY = false;
        assets.loadTexture("missing.png", options);
        check(factory.textures == 4, "options cache identity");
        check(factory.liveMeshes == 1 && factory.liveTextures == 2, "resource ownership");
        rejectsMessage([&] { assets.material({999}); }, "MaterialId 999");
        auto material = assets.createMaterial(MaterialDesc{});
        check(assets.materials().material(material).baseColor ==
                  assets.material(material).baseColor,
              "material forwarding");
        rejects([&] { assets.mesh({999}); });
        rejects([&] { assets.texture({999}); });
    }
    check(factory.liveMeshes == 0 && factory.liveTextures == 0, "resources leaked");
    std::cout << "resource factory behaviors passed\n";
}

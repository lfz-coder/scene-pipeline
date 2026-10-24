#include <glad/gl.h>
#include <array>
#include <cmath>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/render/LightingPipeline.hpp>
#include <luma/scene/AssetStore.hpp>
#include <luma/scene/Camera.hpp>
#include <luma/scene/Scene.hpp>
#include <stdexcept>

namespace {
// Supply a slanted face: axis-aligned cube normals would not expose a wrong
// normal matrix under local nonuniform scaling.
class SlantedFaceFactory final : public luma::IResourceFactory {
    luma::NativeResourceFactory native_;
  public:
    std::unique_ptr<luma::IShader> createShader(const std::filesystem::path& vertex,
                                               const std::filesystem::path& fragment) override {
        return native_.createShader(vertex, fragment);
    }
    std::unique_ptr<luma::ITexture> createTexture(const std::filesystem::path& path,
                                                 const luma::TextureOptions& options) override {
        return native_.createTexture(path, options);
    }
    std::unique_ptr<luma::IMesh> createMesh(const luma::MeshData&) override {
        const auto n = glm::normalize(glm::vec3(1, 0, 1));
        luma::MeshData face;
        face.vertices = {{{-1, -1, 1}, n, {0, 0}},
                         {{1, -1, -1}, n, {1, 0}}, {{0, 1, 0}, n, {.5f, 1}}};
        face.indices = {0, 1, 2};
        return native_.createMesh(face);
    }
};
} // namespace

void lightingGeometryChecks(const std::filesystem::path& shaders) {
    using namespace luma;
    SlantedFaceFactory resources;
    AssetStore assets(resources);
    Scene scene;
    const auto material = assets.createMaterial({});
    const auto entity = scene.create("slanted face", assets.primitive(Primitive::Triangle), material);
    scene.transform(entity).setScale({4, 1, 1});
    Camera camera;
    camera.lookAt({0, 0, 4}, {0, 0, 0});
    camera.setAspect(1);
    LightingPipeline pipeline;
    pipeline.initialize(resources, shaders);
    LightingSettings settings;
    settings.model = ShadingModel::BlinnPhong;
    settings.visualizeNormals = true;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_FRAMEBUFFER_SRGB);
    RenderState state;
    state.width = state.height = 64;
    state.apply();
    RenderState::prepareClear();
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    pipeline.draw(scene.renderItems().front(), scene, assets, camera, {}, settings, false);
    std::array<unsigned char, 4> actual{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, actual.data());
    const auto expected = glm::normalize(glm::vec3(.25f, 0, 1)) * .5f + .5f;
    for (int i = 0; i < 3; ++i)
        if (std::abs(int(actual[i]) - int(std::round(expected[i] * 255))) > 3)
            throw std::runtime_error("nonuniform scale must use inverse-transpose normal matrix");
}

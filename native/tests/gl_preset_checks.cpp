#include <luma/app/Application.hpp>
#include <luma/render/Renderer.hpp>
#include <array>
#include <stdexcept>
using namespace luma;
namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::array<unsigned char,4> center() {
    std::array<unsigned char,4> pixel{};
    glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel.data());
    return pixel;
}
}
void presetExamples(Renderer& renderer, AssetStore& assets, DisplayTarget target) {
    Scene scene;
    Camera camera;
    CameraControls controls;
    LightRig lights;
    LightingSettings lighting;
    PostProcessSettings postProcess;
    SceneContext context{assets, scene, camera, controls, lights, lighting, postProcess};
    auto quad = makePreset("quad");
    quad->onCreate(context);
    check(scene.renderItems().size() == 1, "quad preset must create one textured square");
    const auto& item = scene.renderItems().front();
    check(assets.material(item.material).baseColorTexture.has_value(),
          "quad learning example must use a texture");
    renderer.options().outline = false;
    renderer.render(scene, assets, camera, target);
    const auto pixel = center();
    check(pixel[0] > 80 || pixel[1] > 80 || pixel[2] > 80,
          "camera-facing quad must cover the viewport center");
    quad->onDestroy(context);
    scene.clear();
    auto textures = makePreset("texture");
    textures->onCreate(context);
    bool dual = false;
    for (const auto& entry : scene.renderItems()) {
        const auto& material = assets.material(entry.material);
        dual = dual || (material.baseColorTexture && material.secondaryColorTexture &&
                        material.textureMix > 0 && material.textureMix < 1);
    }
    check(dual, "texture studio must contain an actual mixed two-texture material");
    renderer.render(scene, assets, camera, target);
    textures->onDestroy(context);
}

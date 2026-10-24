#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <glad/gl.h>
#include <iostream>
#include <luma/app/Application.hpp>
#include <luma/core/GlfwWindow.hpp>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <luma/render/LightingPipeline.hpp>
#include <luma/render/Renderer.hpp>
#include <stdexcept>

using namespace luma;
void postProcessChecks(IResourceFactory&, const std::filesystem::path&);
void lightingGeometryChecks(const std::filesystem::path&);

namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
std::array<unsigned char, 4> center() {
    std::array<unsigned char, 4> result{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result.data());
    return result;
}
void clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    RenderState state;
    state.width = 64;
    state.height = 64;
    state.apply();
    RenderState::prepareClear();
    glClearDepth(1);
    glClearStencil(0);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
} // namespace

int main(int argc, char** argv) {
    try {
        const bool hidden = argc > 1 && std::string(argv[1]) == "--hidden";
        GlfwRuntime runtime;
        GlfwWindow window("Luma lighting checks", 64, 64, hidden);
        NativeResourceFactory resources;
        postProcessChecks(resources, assetRoot() / "shaders");
        lightingGeometryChecks(assetRoot() / "shaders");
        AssetStore assets(resources);
        LightingPipeline pipeline;
        pipeline.initialize(resources, assetRoot() / "shaders");
        const auto material = assets.createMaterial(MaterialDesc::unlit({.8f, .2f, .1f, 1}));
        Scene scene;
        const auto entity = scene.create("lit cube", assets.primitive(Primitive::Cube), material);
        Camera camera;
        camera.lookAt({0, 0, 4}, {0, 0, 0});
        camera.setAspect(1);
        LightRig rig;
        DirectionalLight sun;
        sun.direction = {0, 0, -1};
        const auto sunId = rig.addDirectional(sun);
        LightingSettings settings;
        settings.model = ShadingModel::BlinnPhong;
        settings.ambientStrength = 0;
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, rig, settings, false);
        auto lit = center();
        auto disabledRig = rig;
        disabledRig.entry(sunId).value = DirectionalLight{sun.direction, sun.color, sun.intensity, false};
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, disabledRig, settings, false);
        const auto ambientOnly = center();
        check(ambientOnly[0] < 5 && ambientOnly[1] < 5 && ambientOnly[2] < 5,
              "disabling the only directional light must remove direct illumination");
        check(lit[0] > 25 && lit[1] < lit[0] && lit[2] < lit[0],
              "directional light must illuminate the front face");
        LightRig emptyRig;
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, rig, settings, false);
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, emptyRig, settings, false);
        check(center()[0] < 5, "removing the directional light must clear its GPU state");
        auto point = PointLight{};
        point.position = {0, 0, 3};
        point.intensity = 3;
        rig.addPoint(point);
        auto spot = SpotLight{};
        spot.position = {0, 0, 3};
        spot.direction = {0, 0, -1};
        spot.innerCutoffDegrees = 8;
        spot.outerCutoffDegrees = 20;
        rig.addSpot(spot);
        rig.validate();
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, rig, settings, false);
        auto multi = center();
        check(multi[0] >= lit[0], "point and spot lights must contribute to the result");
        LightRig pointRig;
        pointRig.addPoint(point);
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, pointRig, settings, false);
        check(center()[0] > 25, "point light must illuminate without a directional light");
        const auto nearPoint = center();
        LightRig distantRig;
        auto distantPoint = point;
        distantPoint.position.z = 12;
        distantRig.addPoint(distantPoint);
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, distantRig, settings, false);
        check(center()[0] + 25 < nearPoint[0], "point light must attenuate with distance");
        LightRig spotRig;
        auto spotId = spotRig.addSpot(spot);
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, spotRig, settings, false);
        check(center()[0] > 25, "inside the spotlight cone must receive light");
        spot.direction = {1, 0, 0};
        spotRig.update(spotId, spot);
        clear();
        pipeline.draw(scene.renderItems().front(), scene, assets, camera, spotRig, settings, false);
        check(center()[0] < 5, "outside the spotlight cone must receive no direct light");
        Renderer renderer(resources);
        renderer.initialize(assetRoot() / "shaders");
        renderer.options().outline = false;
        renderer.setLighting(settings, rig);
        renderer.render(scene, assets, camera, {0, 64, 64, window.srgbAttachment()});
        check(center()[0] > 25, "Renderer must dispatch to the Phong pipeline");
        PostProcessSettings grayscale;
        grayscale.enabled = true;
        grayscale.effect = PostProcessEffect::Grayscale;
        grayscale.intensity = 1;
        renderer.setPostProcess(grayscale);
        renderer.render(scene, assets, camera, {0, 64, 64, window.srgbAttachment()});
        const auto filtered = center();
        check(std::abs(int(filtered[0]) - int(filtered[1])) < 6 &&
                  std::abs(int(filtered[1]) - int(filtered[2])) < 6,
              "grayscale post-process must converge output channels");
        LightRig labLights;
        LightingSettings labSettings;
        PostProcessSettings labPost;
        Scene labScene;
        Camera labCamera;
        CameraControls labControls;
        SceneContext labContext{assets, labScene, labCamera, labControls,
                                labLights, labSettings, labPost};
        auto lab = makePreset("lighting");
        lab->onCreate(labContext);
        check(labScene.renderItems().size() == 4 && labLights.entries().size() == 3,
              "lighting laboratory must create scene and three light types");
        check(labSettings.model == ShadingModel::BlinnPhong,
              "lighting laboratory must select Phong shading");
        std::cout << "PASS GL lighting: shader, nonuniform normals, directional/point/spot lights, attenuation\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL GL lighting: " << error.what() << '\n';
        return 1;
    }
}

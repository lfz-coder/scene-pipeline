#include <glm/gtc/quaternion.hpp>
#include <luma/app/Application.hpp>
#include <luma/app/SceneLayer.hpp>
#include <stdexcept>
#include <tuple>

namespace luma {
namespace {

class Preset final : public SceneLayer {
  public:
    explicit Preset(std::string name) : name_(std::move(name)) {}

    void onCreate(SceneContext& ctx) override {
        ctx.lights.clear();
        ctx.lighting = {};
        ctx.postProcess = {};
        ctx.camera.lookAt({5, 3.3f, 7}, {0, 0.7f, 0});
        ctx.controls.setMode(CameraMode::Orbit);
        if (name_ == "window")
            return;
        if (name_ == "triangle") {
            add(ctx, "Vertex colors", Primitive::Triangle, {0, 0, 0}, {2, 2, 2}, {1, 1, 1, 1});
            ctx.camera.lookAt({0, 0, 4}, {0, 0, 0});
            return;
        }
        if (name_ == "quad") {
            auto material = MaterialDesc{};
            material.baseColorTexture = ctx.assets.loadTexture(assetRoot() / "textures/checker.png");
            auto square = addWithMaterial(ctx, "Textured square", Primitive::Plane, {0, 0, 0},
                                           {2.5f, 1, 2.5f}, ctx.assets.createMaterial(material));
            ctx.scene.transform(square).setRotation(
                glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0)));
            ctx.camera.lookAt({0, 0, 4}, {0, 0, 0});
            return;
        }
        if (name_ == "lighting") {
            createLightingLab(ctx);
            return;
        }
        add(ctx, "Ground", Primitive::Plane, {0, -1, 0}, {10, 1, 10}, {0.075f, 0.1f, 0.15f, 1});
        if (name_ == "robot" || name_ == "camera")
            createRobot(ctx);
        else if (name_ == "texture")
            createTextureStudio(ctx);
        else if (name_ == "state")
            createStateLab(ctx);
        else
            throw std::invalid_argument("unknown preset: " + name_);
    }

    void onUpdate(SceneContext& ctx, const FrameInfo& frame) override {
        // The camera learning example animates. The editor presets remain static
        // so editing an object's rotation has lasting, immediately visible effect.
        if (name_ == "camera" && ctx.scene.contains(animated_)) {
            ctx.scene.transform(animated_).setRotation(glm::angleAxis(
                static_cast<float>(frame.elapsedSeconds) * 0.3f, glm::vec3(0, 1, 0)));
        }
    }

  private:
    std::string name_;
    EntityId animated_{};

    EntityId addWithMaterial(SceneContext& ctx, const std::string& name, Primitive shape,
                             glm::vec3 position, glm::vec3 scale, MaterialId material) {
        auto id = ctx.scene.create(name, ctx.assets.primitive(shape), material);
        ctx.scene.transform(id).setPosition(position);
        ctx.scene.transform(id).setScale(scale);
        return id;
    }

    EntityId add(SceneContext& ctx, const std::string& name, Primitive shape, glm::vec3 position,
                 glm::vec3 scale, glm::vec4 color) {
        return addWithMaterial(ctx, name, shape, position, scale,
                               ctx.assets.createMaterial(MaterialDesc::unlit(color)));
    }

    void createRobot(SceneContext& ctx) {
        animated_ = add(ctx, "Robot / head", Primitive::Cube, {0, 1.7f, 0}, {1.35f, 1, 1.05f},
                        {0.48f, 0.27f, 0.92f, 1});
        add(ctx, "Robot / body", Primitive::Cube, {0, 0.45f, 0}, {1.65f, 1.3f, 0.9f},
            {0.13f, 0.52f, 0.8f, 1});
        // Pairwise sharing is deliberate and visible in the inspector's usage
        // count. Clone one material there to make the corresponding limb unique.
        auto arms = ctx.assets.createMaterial(MaterialDesc::unlit({0.75f, 0.45f, 0.15f, 1}));
        auto feet = ctx.assets.createMaterial(MaterialDesc::unlit({0.3f, 0.25f, 0.65f, 1}));
        auto eyes = ctx.assets.createMaterial(MaterialDesc::unlit({0.45f, 1, 0.9f, 1}));
        for (float side : {-1.0f, 1.0f}) {
            addWithMaterial(ctx, side < 0 ? "Arm L" : "Arm R", Primitive::Cube,
                            {side * 1.15f, 0.5f, 0}, {0.45f, 1.25f, 0.5f}, arms);
            addWithMaterial(ctx, side < 0 ? "Foot L" : "Foot R", Primitive::Cube,
                            {side * 0.47f, -0.65f, 0.15f}, {0.6f, 0.7f, 1}, feet);
            addWithMaterial(ctx, side < 0 ? "Eye L" : "Eye R", Primitive::Cube,
                            {side * 0.33f, 1.8f, 0.56f}, {0.23f, 0.22f, 0.08f}, eyes);
        }
        add(ctx, "Pedestal", Primitive::Cube, {0, -1.1f, 0}, {3.4f, 0.25f, 2.6f},
            {0.15f, 0.2f, 0.28f, 1});
    }

    void createTextureStudio(SceneContext& ctx) {
        auto checker = ctx.assets.loadTexture(assetRoot() / "textures/checker.png");
        auto cutout = ctx.assets.loadTexture(assetRoot() / "textures/cutout.png");
        for (int i = 0; i < 3; ++i) {
            auto color = i == 1 ? glm::vec4(0.45f, 0.8f, 1, 1) : glm::vec4(1);
            add(ctx, i == 0 ? "sRGB checker" : i == 1 ? "Two textures / linear mix" : "Alpha cutout",
                Primitive::Cube, {(i - 1) * 2.1f, 0.2f, 0}, {1.5f, 1.8f, 1.5f}, color);
            auto& material = ctx.assets.material(ctx.scene.renderItems().back().material);
            material.baseColorTexture = i == 2 ? cutout : checker;
            if (i == 1)
                material.secondaryColorTexture = cutout;
            if (i == 2) {
                material.alphaMode = AlphaMode::Mask;
                material.doubleSided = true;
            }
        }
    }

    void createLightingLab(SceneContext& ctx) {
        add(ctx, "Ground", Primitive::Plane, {0, -1, 0}, {10, 1, 10}, {0.075f, 0.1f, 0.15f, 1});
        for (const auto& spec : {std::tuple{"Ruby", glm::vec3{-2, 0, 0}, glm::vec4{.72f, .08f, .1f, 1}, 24.0f},
                                 std::tuple{"Gold", glm::vec3{0, 0, 0}, glm::vec4{.75f, .45f, .08f, 1}, 96.0f},
                                 std::tuple{"Azure", glm::vec3{2, 0, 0}, glm::vec4{.08f, .32f, .85f, 1}, 180.0f}}) {
            MaterialDesc material = MaterialDesc::unlit(std::get<2>(spec));
            material.specularColor = {1, .9f, .8f};
            material.shininess = std::get<3>(spec);
            if (std::string(std::get<0>(spec)) == "Gold") {
                material.baseColorTexture = ctx.assets.loadTexture(assetRoot() / "textures/checker.png");
                TextureOptions specularOptions;
                specularOptions.colorSpace = ColorSpace::Linear;
                material.specularTexture = ctx.assets.loadTexture(
                    assetRoot() / "textures/specular-grid.png", specularOptions);
            }
            addWithMaterial(ctx, std::get<0>(spec), Primitive::Cube, std::get<1>(spec), glm::vec3{1.4f},
                            ctx.assets.createMaterial(material));
        }
        ctx.camera.lookAt({6, 3.8f, 8}, {0, 0.2f, 0});
        ctx.lighting.model = ShadingModel::BlinnPhong;
        ctx.lighting.ambientColor = {1, 1, 1};
        ctx.lighting.ambientStrength = .08f;
        ctx.lights.clear();
        ctx.lights.addDirectional({{-0.4f, -1, -0.3f}, {1, .92f, .8f}, 1.4f, true});
        ctx.lights.addPoint({{-2, 2.5f, 2}, {1, .18f, .1f}, 2.2f, 1, .09f, .032f, true});
        ctx.lights.addSpot({{2, 3.5f, 3}, {-0.2f, -1, -1}, {.25f, .5f, 1}, 3, 1, .09f,
                            .032f, 10, 24, true});
    }

    void createStateLab(SceneContext& ctx) {
        add(ctx, "Opaque back box", Primitive::Cube, {0, 0, 0}, {2.6f, 2.2f, 1},
            {0.55f, 0.24f, 0.8f, 1});
        for (int i = 0; i < 3; ++i) {
            auto color =
                i == 1 ? glm::vec4(0.1f, 0.85f, 0.5f, 0.45f) : glm::vec4(0.15f, 0.5f, 1, 0.45f);
            add(ctx, "Alpha layer " + std::to_string(i + 1), Primitive::Cube,
                {-1.3f + i * 1.25f, 0.15f, 1.2f + i * 0.45f}, {1.5f, 2, 0.13f}, color);
            auto& material = ctx.assets.material(ctx.scene.renderItems().back().material);
            material.alphaMode = AlphaMode::Blend;
            material.doubleSided = true;
        }
        auto back = add(ctx, "Back-facing triangle (toggle cull)", Primitive::Triangle,
                        {-3, 0.3f, 0}, {1.8f, 1.8f, 1.8f}, {1, 1, 1, 1});
        ctx.scene.transform(back).setRotation(
            glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0)));
    }
};

} // namespace

std::unique_ptr<SceneLayer> makePreset(const std::string& name) {
    return std::make_unique<Preset>(name);
}
} // namespace luma

#include <glm/gtc/quaternion.hpp>
#include <luma/app/Application.hpp>

// This file is a complete client. It contains no window, shader, or GL calls.
class CubeStudio final : public luma::SceneLayer {
  public:
    void onCreate(luma::SceneContext& ctx) override {
        const auto cube = ctx.assets.primitive(luma::Primitive::Cube);
        const auto lavender =
            ctx.assets.createMaterial(luma::MaterialDesc::unlit({0.55f, 0.35f, 0.80f, 1.0f}));
        cube_ = ctx.scene.create("Reusable cube", cube, lavender);
        ctx.camera.lookAt({4.f, 3.f, 6.f}, {0.f, 0.f, 0.f});
        ctx.controls.setMode(luma::CameraMode::Orbit);
    }

    void onUpdate(luma::SceneContext& ctx, const luma::FrameInfo& frame) override {
        ctx.scene.transform(cube_).setRotation(glm::angleAxis(
            static_cast<float>(frame.elapsedSeconds) * 0.4f, glm::vec3{0.f, 1.f, 0.f}));
    }

  private:
    luma::EntityId cube_{};
};

int main() {
    luma::Application app{"Cube Studio", 1280, 800};
    return app.run(std::make_unique<CubeStudio>());
}

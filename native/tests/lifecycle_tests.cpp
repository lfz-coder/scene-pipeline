#include <iostream>
#include <luma/app/Application.hpp>
#include <stdexcept>

namespace {
struct Counts {
    int create = 0, update = 0, destroy = 0;
};
enum class Failure { None, Create, Update, Destroy };
class LifecycleScene final : public luma::SceneLayer {
  public:
    LifecycleScene(Counts& counts, Failure failure) : counts_(counts), failure_(failure) {}
    void onCreate(luma::SceneContext& ctx) override {
        ++counts_.create;
        // Real GPU resources exist even when the callback subsequently fails.
        auto mesh = ctx.assets.primitive(luma::Primitive::Cube);
        auto material = ctx.assets.createMaterial(luma::MaterialDesc::unlit({1, 0, 0, 1}));
        ctx.scene.create("Lifecycle cube", mesh, material);
        ctx.lighting.model = luma::ShadingModel::BlinnPhong;
        ctx.lights.addPoint({});
        ctx.postProcess.enabled = true;
        ctx.postProcess.effect = luma::PostProcessEffect::Blur;
        if (failure_ == Failure::Create)
            throw std::runtime_error("deliberate create failure");
    }
    void onUpdate(luma::SceneContext&, const luma::FrameInfo&) override {
        ++counts_.update;
        if (failure_ == Failure::Update)
            throw std::runtime_error("deliberate update failure");
    }
    void onDestroy(luma::SceneContext&) override {
        ++counts_.destroy;
        if (failure_ == Failure::Destroy)
            throw std::runtime_error("deliberate destroy failure");
    }

  private:
    Counts& counts_;
    Failure failure_;
};
void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        for (auto failure : {Failure::Create, Failure::Update, Failure::Destroy, Failure::None}) {
            Counts counts;
            luma::RunOptions options;
            options.hidden = true;
            options.editor = false;
            options.frames = 1;
            options.deterministic = true;
            luma::Application app("Lifecycle integration", 64, 64);
            const auto status = app.run(std::make_unique<LifecycleScene>(counts, failure), options);
            check(counts.create == 1, "create must run once");
            check(counts.destroy == (failure == Failure::Create ? 0 : 1),
                  "destroy runs exactly once only after successful create");
            check(counts.update == (failure == Failure::Create ? 0 : 1),
                  "update must follow create");
            check((status == 0) == (failure == Failure::None),
                  "callback failures must return nonzero");
            check(app.run(std::make_unique<LifecycleScene>(counts, Failure::None), options) != 0,
                  "Application must reject a second run");
        }
        std::cout << "PASS lifecycle: create/update/destroy failures, cleanup, run-once\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL lifecycle: " << e.what() << '\n';
        return 1;
    }
}

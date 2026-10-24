#pragma once
#include <luma/gfx/IResourceFactory.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <luma/render/DisplayTransfer.hpp>
#include <luma/render/LightingPipeline.hpp>
#include <luma/render/PostProcessPipeline.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <luma/render/PostProcessTarget.hpp>
#include <luma/render/RenderSettings.hpp>
#include <luma/render/RenderTarget.hpp>
#include <luma/scene/AssetStore.hpp>
#include <luma/scene/Camera.hpp>
#include <luma/scene/Scene.hpp>
namespace luma {
// Reads scene data, validates resources, orders alpha draws and applies pass
// state. It borrows the output framebuffer and never swaps window buffers.
class Renderer {
  public:
    explicit Renderer(IResourceFactory& factory) : factory_(factory), display_(factory) {}
    void initialize(const std::filesystem::path& shaderDirectory);
    void setStyle(StyleKind style) {
        style_ = style;
    }
    void setLighting(const LightingSettings& settings, const LightRig& lights) {
        settings.validate();
        lights.validate();
        lightingSettings_ = settings;
        lights_ = lights;
    }
    void setPostProcess(const PostProcessSettings& settings) {
        settings.validate();
        postProcess_ = settings;
    }
    StyleKind style() const {
        return style_;
    }
    RenderOptions& options() {
        return options_;
    }
    const RenderOptions& options() const {
        return options_;
    }
    void render(const Scene& scene, const AssetStore& assets, const Camera& camera,
                const DisplayTarget& output);

  private:
    IResourceFactory& factory_;
    std::unique_ptr<IShader> shader_;
    LightingPipeline lighting_;
    LightingSettings lightingSettings_;
    LightRig lights_;
    PostProcessSettings postProcess_;
    PostProcessPipeline postProcessPipeline_;
    PostProcessTarget postProcessSource_, postProcessScratch_;
    DisplayTransfer display_;
    StyleKind style_ = StyleKind::Basic;
    RenderOptions options_;
    void draw(const RenderItem&, const Scene&, const AssetStore&, const Camera&,
              bool manualEncoding, bool outline = false);
};
} // namespace luma

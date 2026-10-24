#pragma once
#include <filesystem>
#include <luma/gfx/IResourceFactory.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <memory>

namespace luma {
class AssetStore;
class Camera;
class Scene;
struct RenderItem;

class LightingPipeline {
  public:
    void initialize(IResourceFactory& factory, const std::filesystem::path& shaderDirectory);
    void draw(const RenderItem&, const Scene&, const AssetStore&, const Camera&, const LightRig&,
              const LightingSettings&, bool manualEncoding);

  private:
    IResourceFactory* factory_ = nullptr;
    std::unique_ptr<IShader> shader_;
    void uploadLights(const LightRig& rig, int budget);
};
} // namespace luma

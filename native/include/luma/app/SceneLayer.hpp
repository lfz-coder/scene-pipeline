#pragma once
#include <luma/scene/AssetStore.hpp>
#include <luma/scene/CameraControls.hpp>
#include <luma/scene/Scene.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <memory>
#include <string>
namespace luma {
struct FrameInfo {
    float deltaSeconds;
    double elapsedSeconds;
    const InputSnapshot& input;
};
struct SceneContext {
    AssetStore& assets;
    Scene& scene;
    Camera& camera;
    CameraControls& controls;
    LightRig& lights;
    LightingSettings& lighting;
    PostProcessSettings& postProcess;
};
// Store CPU values and entity IDs here. GPU resources belong to AssetStore.
// onDestroy runs once only after onCreate returns successfully.
class SceneLayer {
  public:
    virtual ~SceneLayer() = default;
    virtual void onCreate(SceneContext&) = 0;
    virtual void onUpdate(SceneContext&, const FrameInfo&) {};
    virtual void onDestroy(SceneContext&) {};
};
std::unique_ptr<SceneLayer> makePreset(const std::string& name);
} // namespace luma

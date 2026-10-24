#pragma once
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <luma/render/RenderSettings.hpp>
#include <luma/scene/CameraControls.hpp>
#include <luma/scene/MaterialLibrary.hpp>
#include <luma/scene/Scene.hpp>
namespace luma {
class EditorController;
// Borrows CPU state; the application keeps dependencies alive for this model.
// Views receive const state. All editor writes go through EditorController.
class EditorModel {
  public:
    EditorModel(Scene& scene, MaterialLibrary& materials, Camera& camera, CameraControls& controls)
        : scene_(scene), materials_(materials), camera_(camera), controls_(controls) {}
    const Scene& scene() const {
        return scene_;
    }
    const MaterialLibrary& materials() const {
        return materials_;
    }
    const Camera& camera() const {
        return camera_;
    }
    const CameraControls& controls() const {
        return controls_;
    }
    EntityId selected() const {
        return options_.selected;
    }
    const RenderOptions& options() const {
        return options_;
    }
    StyleKind style() const {
        return style_;
    }
    const LightRig& lights() const {
        return lights_;
    }
    LightRig& lights() {
        return lights_;
    }
    const LightingSettings& lighting() const {
        return lighting_;
    }
    LightingSettings& lighting() {
        return lighting_;
    }
    const PostProcessSettings& postProcess() const {
        return postProcess_;
    }
    PostProcessSettings& postProcess() {
        return postProcess_;
    }
    MaterialId selectedMaterial() const;

  private:
    friend class EditorController;
    Scene& scene_;
    MaterialLibrary& materials_;
    Camera& camera_;
    CameraControls& controls_;
    RenderOptions options_;
    StyleKind style_ = StyleKind::Basic;
    LightRig lights_;
    LightingSettings lighting_;
    PostProcessSettings postProcess_;
};
} // namespace luma

#pragma once
#include <luma/editor/EditorModel.hpp>
namespace luma {
// Synchronous CPU operations; validation happens before committing state.
class EditorController {
  public:
    explicit EditorController(EditorModel& model) : model_(model) {}
    void select(EntityId entity);
    void clearSelection();
    void reconcileSelection();
    void setTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale);
    void setMaterial(MaterialDesc material);
    MaterialId cloneSelectedMaterial();
    void assignMaterial(MaterialId material);
    void setCameraMode(CameraMode mode);
    void setCameraPose(glm::vec3 eye, glm::vec3 target, glm::vec3 up = {0, 1, 0});
    void setStyle(StyleKind style);
    void setRenderOptions(RenderOptions options);
    void setLightingSettings(LightingSettings settings);
    LightId addDirectionalLight(DirectionalLight light);
    LightId addPointLight(PointLight light);
    LightId addSpotLight(SpotLight light);
    void updateLight(LightId id, LightValue value);
    void removeLight(LightId id);
    void clearLights();
    void setPostProcessSettings(PostProcessSettings settings);

  private:
    EditorModel& model_;
};
} // namespace luma

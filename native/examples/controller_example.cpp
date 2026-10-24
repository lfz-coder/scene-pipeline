#include <iostream>
#include <luma/editor/EditorController.hpp>
#include <stdexcept>
using namespace luma;
int main() {
    Scene scene;
    MaterialLibrary materials;
    Camera camera;
    CameraControls controls;
    auto shared = materials.createMaterial(MaterialDesc::unlit({1, 0, 0, 1}));
    auto first = scene.create("First", {1}, shared);
    auto second = scene.create("Second", {1}, shared);
    EditorModel model(scene, materials, camera, controls);
    EditorController controller(model);
    controller.select(first);
    controller.setTransform({1, 2, 0}, {1, 0, 0, 0}, {2, 2, 2});
    auto material = model.materials().material(model.selectedMaterial());
    material.baseColor = {0, 1, 0, 1};
    controller.setMaterial(material);
    if (materials.material(shared).baseColor.g != 1)
        throw std::runtime_error("shared edit was not visible");
    auto independent = controller.cloneSelectedMaterial();
    material.baseColor = {0, 0, 1, 1};
    controller.setMaterial(material);
    if (materials.material(shared).baseColor.g != 1 ||
        materials.material(independent).baseColor.b != 1 ||
        scene.transform(second).position() != glm::vec3(0))
        throw std::runtime_error("independent edit changed another object");
    std::cout
        << "CPU controller: selection, transform, shared color and independent clone passed\n";
}

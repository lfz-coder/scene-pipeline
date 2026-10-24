#include <cmath>
#include <iostream>
#include <luma/scene/CameraControls.hpp>
#include <stdexcept>

using namespace luma;
namespace {
void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool sameMatrix(const glm::mat4& a, const glm::mat4& b) {
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (std::abs(a[column][row] - b[column][row]) > 0.00001f)
                return false;
    return true;
}
void idleCustomUp() {
    for (auto mode : {CameraMode::Orbit, CameraMode::Fly}) {
        Camera camera;
        camera.lookAt({0, 5, 0}, {0, 0, 0}, {0, 0, -1});
        const auto view = camera.view();
        CameraControls controls;
        controls.setMode(mode);
        controls.update(camera, {}, 0);
        check(sameMatrix(view, camera.view()), "idle top-down pose must preserve custom up");
    }
}
void idleRolledUp() {
    Camera camera;
    camera.lookAt({0, 0, 5}, {0, 0, 0}, {1, 1, 0});
    const auto view = camera.view();
    CameraControls controls;
    for (auto mode : {CameraMode::Fly, CameraMode::Orbit}) {
        controls.setMode(mode);
        controls.update(camera, {}, 0);
        check(sameMatrix(view, camera.view()), "mode switch must preserve camera roll");
    }
}
void modeSwitchDistance() {
    for (float distance : {100.f, 0.1f}) {
        Camera camera;
        camera.lookAt({0, 0, distance}, {0, 0, 0});
        const auto before = camera.eye();
        CameraControls controls;
        controls.setMode(CameraMode::Fly);
        controls.update(camera, {}, 0);
        controls.setMode(CameraMode::Orbit);
        controls.update(camera, {}, 0);
        check(camera.eye() == before, "idle Orbit must not clamp existing distance");
        InputSnapshot dolly;
        dolly.scroll = 1;
        controls.update(camera, dolly, 0);
        const auto changedDistance = glm::distance(camera.eye(), camera.target());
        check(changedDistance >= 0.4999f && changedDistance <= 60.0001f,
              "actual dolly must enforce supported distance bounds");
    }
}
void customUpMovement() {
    Camera camera;
    camera.lookAt({0, 5, 0}, {0, 0, 0}, {0, 0, -1});
    CameraControls controls;
    controls.setMode(CameraMode::Fly);
    InputSnapshot input;
    input.held[static_cast<unsigned>(Key::D)] = true;
    controls.update(camera, input, 1);
    check(camera.eye().x > 2.9f && std::abs(camera.eye().y - 5) < 0.0001f,
          "Fly right must use custom-up camera basis");
    input = {};
    input.held[static_cast<unsigned>(Key::E)] = true;
    controls.update(camera, input, 1);
    check(camera.eye().z < -2.9f, "Fly E must move along supplied up reference");
}
void customUpRotation() {
    for (auto mode : {CameraMode::Orbit, CameraMode::Fly}) {
        Camera camera;
        camera.lookAt({0, 5, 0}, {0, 0, 0}, {0, 0, -1});
        CameraControls controls;
        controls.setMode(mode);
        InputSnapshot input;
        input.orbitDrag = input.flyDrag = true;
        input.mouseX = 20;
        controls.update(camera, input, 0);
        auto forward = glm::normalize(camera.target() - camera.eye());
        check(forward.x > 0.1f && std::abs(forward.z) < 0.0001f,
              "yaw must rotate around custom up rather than world Y");
        input.mouseX = 0;
        input.mouseY = -20;
        controls.update(camera, input, 0);
        forward = glm::normalize(camera.target() - camera.eye());
        check(forward.z < -0.1f, "pitch must move toward the supplied up direction");
    }
}
} // namespace
int main() {
    int failures = 0;
    const auto run = [&](const char* name, auto test) {
        try {
            test();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };
    run("idle custom up", idleCustomUp);
    run("idle roll", idleRolledUp);
    run("mode switch distance", modeSwitchDistance);
    run("custom up movement", customUpMovement);
    run("custom up rotation", customUpRotation);
    return failures ? 1 : 0;
}

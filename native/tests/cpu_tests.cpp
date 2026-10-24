#include <functional>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <limits>
#include <luma/gfx/MeshData.hpp>
#include <luma/scene/CameraControls.hpp>
#include <luma/scene/MaterialDesc.hpp>
#include <luma/scene/Scene.hpp>
#include <luma/scene/Transform.hpp>
#include <stdexcept>
using namespace luma;
void check(bool ok, const char* what) {
    if (!ok)
        throw std::runtime_error(what);
}
void rejects(const std::function<void()>& f) {
    bool threw = false;
    try {
        f();
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, "invalid value must throw");
}
int main() {
    try {
        Transform t;
        t.setPosition({3, 2, 1});
        t.setRotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1)));
        t.setScale({2, 3, 4});
        auto p = t.matrix() * glm::vec4(1, 0, 0, 1);
        check(glm::length(glm::vec3(p) - glm::vec3(3, 4, 1)) < 0.0001f,
              "Transform must apply scale, rotation, then translation");
        Transform tiny;
        tiny.setRotation({1e-20f, 0, 0, 0});
        check(std::abs(tiny.rotation().w - 1) < 0.0001f,
              "finite nonzero tiny quaternion must normalize");
        rejects([&] { t.setScale({0, 1, 1}); });
        rejects([&] { t.setScale({-1, 1, 1}); });
        rejects([&] { t.setRotation({0, 0, 0, 0}); });
        rejects([&] { t.setPosition({NAN, 0, 0}); });
        Camera c;
        rejects([&] {
            float large = std::numeric_limits<float>::max();
            c.lookAt({large, 0, 0}, {-large, 0, 0});
        });
        rejects([&] { c.setPerspective(180, 0.1f, 100); });
        rejects([&] { c.setPerspective(45, 1, 0.5f); });
        rejects([&] { c.setAspect(0); });
        rejects([&] { c.setPerspective(1e-38f, 0.1f, 100); });
        rejects([&] { c.setAspect(1e-40f); });
        rejects([&] { c.lookAt({0, 0, 0}, {0, 0, 0}); });
        rejects([&] { c.lookAt({0, 1, 0}, {0, 0, 0}); });
        c.lookAt({0, 0, 5}, {0, 0, 0});
        c.setAspect(2);
        auto proj = c.projection();
        check(std::abs(proj[1][1] / proj[0][0] - 2) < 0.001f,
              "framebuffer aspect must affect projection");
        CameraControls controls;
        controls.setMode(CameraMode::Fly);
        InputSnapshot in;
        in.held[0] = true;
        controls.update(c, in, 0.5f);
        check(c.eye().z < 5, "W must move camera forward");
        auto eye = c.eye();
        in.focused = false;
        controls.update(c, in, 1);
        check(c.eye() == eye, "unfocused inputs must not move camera");
        controls.setMode(CameraMode::Orbit);
        in = {};
        in.scroll = 1;
        auto distance = glm::distance(c.eye(), c.target());
        controls.update(c, in, 0);
        check(glm::distance(c.eye(), c.target()) < distance,
              "scroll must dolly without delta multiplication");
        for (auto mesh :
             {PrimitiveFactory::cube(), PrimitiveFactory::plane(), PrimitiveFactory::triangle()}) {
            mesh.validate();
            check(!mesh.indices.empty(), "primitive must contain triangles");
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                auto a = mesh.vertices.at(mesh.indices[i]),
                     b = mesh.vertices.at(mesh.indices[i + 1]),
                     d = mesh.vertices.at(mesh.indices[i + 2]);
                check(glm::dot(glm::cross(b.position - a.position, d.position - a.position),
                               a.normal) > 0,
                      "triangle winding must agree with outward normal");
            }
        }
        MeshData bad;
        rejects([&] { bad.validate(); });
        bad = PrimitiveFactory::cube();
        bad.indices[0] = 9999;
        rejects([&] { bad.validate(); });
        Scene s;
        auto a = s.create("A", {1}, {1});
        auto b = s.create("B", {1}, {1});
        s.transform(a).setPosition({1, 0, 0});
        check(s.transform(b).position() == glm::vec3(0), "shared mesh must not share transform");
        s.setMaterial(a, {2});
        check(s.renderItems()[1].material.value == 1, "material reassignment must be per entity");
        s.erase(a);
        check(!s.contains(a) && s.contains(b), "erase must invalidate only removed ID");
        s.clear();
        auto d = s.create("C", {1}, {1});
        check(d.value > b.value && !s.contains(b), "clear must never reuse IDs");
        rejects([&] { s.transform(a); });
        auto m = MaterialDesc::unlit({0.2f, 0.5f, 0.9f, 1});
        m.validate();
        m.baseColor.r = 2;
        rejects([&] { m.validate(); });
        m = MaterialDesc{};
        for (float mix : {-0.01f, 1.01f, NAN, INFINITY}) {
            m.textureMix = mix;
            rejects([&] { m.validate(); });
        }
        m.textureMix = 0.5f;
        m.secondaryColorTexture = TextureId{2};
        rejects([&] { m.validate(); });
        m.baseColorTexture = TextureId{1};
        for (float mix : {0.0f, 0.5f, 1.0f}) {
            m.textureMix = mix;
            m.validate();
        }
        m.secondaryColorTexture = TextureId{0};
        rejects([&] { m.validate(); });
        std::cout << "PASS CPU: TRS, validation, camera inputs, projection, geometry winding, "
                     "IDs/sharing, materials\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL CPU: " << e.what() << '\n';
        return 1;
    }
}

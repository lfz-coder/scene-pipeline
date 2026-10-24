#include <cmath>
#include <iostream>
#include <luma/lighting/LightTypes.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <luma/scene/MaterialDesc.hpp>
#include <stdexcept>

using namespace luma;

namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

template <class F> void rejects(F&& action) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, "invalid lighting value accepted");
}
} // namespace

int main() {
    try {
        DirectionalLight directional;
        directional.validate();
        check(directional.enabled && directional.intensity > 0, "directional default");
        rejects([] {
            DirectionalLight value;
            value.direction = {0, 0, 0};
            value.validate();
        });
        rejects([] {
            DirectionalLight value;
            value.intensity = -1;
            value.validate();
        });

        PointLight point;
        point.validate();
        rejects([] {
            PointLight value;
            value.quadratic = -0.1f;
            value.validate();
        });
        rejects([] {
            PointLight value;
            value.color.x = NAN;
            value.validate();
        });

        SpotLight spot;
        spot.validate();
        rejects([] {
            SpotLight value;
            value.innerCutoffDegrees = 30;
            value.outerCutoffDegrees = 20;
            value.validate();
        });
        rejects([] {
            SpotLight value;
            value.outerCutoffDegrees = 100;
            value.validate();
        });

        LightingSettings lighting;
        lighting.validate();
        check(lighting.maxLights == 13, "default budget must cover all supported light types");
        auto fullBudget = lighting;
        fullBudget.maxLights = 13;
        fullBudget.validate();
        rejects([] {
            LightingSettings value;
            value.ambientStrength = -0.01f;
            value.validate();
        });
        rejects([] {
            LightingSettings value;
            value.maxLights = 0;
            value.validate();
        });

        MaterialDesc material = MaterialDesc::unlit({.2f, .3f, .4f, 1});
        material.specularColor = {.8f, .8f, .8f};
        material.shininess = 64;
        material.validate();
        auto invalid = material;
        invalid.shininess = 0;
        rejects([&] { invalid.validate(); });
        invalid = material;
        invalid.specularColor.y = 2;
        rejects([&] { invalid.validate(); });
        invalid = material;
        invalid.specularTexture = TextureId{0};
        rejects([&] { invalid.validate(); });

        LightRig rig;
        const auto sun = rig.addDirectional(directional);
        check(sun.value == 1 && rig.entries().size() == 1, "directional ID");
        rejects([&] { rig.addDirectional(directional); });
        for (int i = 0; i < 8; ++i)
            rig.addPoint(point);
        rejects([&] { rig.addPoint(point); });
        const auto spotId = rig.addSpot(spot);
        check(rig.entry(spotId).kind == LightKind::Spot, "spot kind");
        auto updated = spot;
        updated.intensity = 2;
        rig.update(spotId, updated);
        check(std::get<SpotLight>(rig.entry(spotId).value).intensity == 2, "spot update");
        rig.remove(sun);
        rejects([&] { rig.entry(sun); });

        PostProcessSettings post;
        post.validate();
        post.effect = PostProcessEffect::Blur;
        post.radius = 8;
        post.validate();
        rejects([] {
            PostProcessSettings value;
            value.radius = 0;
            value.validate();
        });
        rejects([] {
            PostProcessSettings value;
            value.intensity = 2;
            value.validate();
        });
        std::cout << "PASS lighting values: defaults, light bounds, material specular\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL lighting values: " << error.what() << '\n';
        return 1;
    }
}

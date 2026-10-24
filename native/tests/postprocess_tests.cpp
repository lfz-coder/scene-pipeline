#include <iostream>
#include <luma/render/PostProcessSettings.hpp>
#include <luma/render/PostProcessTarget.hpp>
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
    check(failed, "invalid post-process target accepted");
}
} // namespace

int main() {
    try {
        PostProcessSettings value;
        value.validate();
        value.enabled = true;
        value.effect = PostProcessEffect::Sharpen;
        value.intensity = .8f;
        value.radius = 5;
        value.validate();
        PostProcessTarget target;
        rejects([&] { target.resize(0, 64); });
        rejects([&] { target.resize(64, 0); });
        std::cout << "PASS postprocess values: effects, bounds, target dimensions\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL postprocess values: " << error.what() << '\n';
        return 1;
    }
}

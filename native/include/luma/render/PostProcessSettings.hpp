#pragma once

namespace luma {

enum class PostProcessEffect { None, Invert, Grayscale, Sharpen, Blur };

struct PostProcessSettings {
    bool enabled = false;
    PostProcessEffect effect = PostProcessEffect::None;
    float intensity = 1.0f;
    int radius = 3;
    void validate() const;
};

} // namespace luma

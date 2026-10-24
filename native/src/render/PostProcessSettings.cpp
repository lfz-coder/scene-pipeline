#include <cmath>
#include <luma/render/PostProcessSettings.hpp>
#include <stdexcept>

namespace luma {
void PostProcessSettings::validate() const {
    if (effect != PostProcessEffect::None && effect != PostProcessEffect::Invert &&
        effect != PostProcessEffect::Grayscale && effect != PostProcessEffect::Sharpen &&
        effect != PostProcessEffect::Blur)
        throw std::invalid_argument("invalid post-process effect");
    if (!std::isfinite(intensity) || intensity < 0 || intensity > 1)
        throw std::invalid_argument("post-process intensity must be in [0,1]");
    if (radius < 1 || radius > 15)
        throw std::invalid_argument("post-process radius must be in [1,15]");
    if (!enabled && effect == PostProcessEffect::None)
        return;
}
} // namespace luma

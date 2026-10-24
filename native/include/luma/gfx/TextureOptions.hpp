#pragma once
#include <string>
namespace luma {
enum class ColorSpace { Linear, SRGB };
enum class Wrap { Repeat, Clamp };
enum class Filter { Nearest, Linear, LinearMipmapLinear };
struct TextureOptions {
    ColorSpace colorSpace = ColorSpace::SRGB;
    bool flipY = true;
    Wrap wrap = Wrap::Repeat;
    Filter minFilter = Filter::LinearMipmapLinear, magFilter = Filter::Linear;
    bool mipmaps = true;
    void validate() const;
    std::string cacheSuffix() const;
};
} // namespace luma

#include <luma/gfx/TextureOptions.hpp>
#include <stdexcept>
namespace luma {
void TextureOptions::validate() const {
    if ((colorSpace != ColorSpace::Linear && colorSpace != ColorSpace::SRGB) ||
        (wrap != Wrap::Repeat && wrap != Wrap::Clamp) ||
        (minFilter != Filter::Nearest && minFilter != Filter::Linear &&
         minFilter != Filter::LinearMipmapLinear) ||
        (magFilter != Filter::Nearest && magFilter != Filter::Linear &&
         magFilter != Filter::LinearMipmapLinear))
        throw std::invalid_argument("invalid texture option enum");
    if (magFilter == Filter::LinearMipmapLinear)
        throw std::invalid_argument("magnification filter cannot require mipmaps");
    if (minFilter == Filter::LinearMipmapLinear && !mipmaps)
        throw std::invalid_argument("minification filter requires mipmaps");
}
std::string TextureOptions::cacheSuffix() const {
    return ":" + std::to_string(static_cast<int>(colorSpace)) + ":" + std::to_string(flipY) + ":" +
           std::to_string(static_cast<int>(wrap)) + ":" +
           std::to_string(static_cast<int>(minFilter)) + ":" +
           std::to_string(static_cast<int>(magFilter)) + ":" + std::to_string(mipmaps);
}
} // namespace luma

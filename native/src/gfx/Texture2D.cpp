#include <luma/gfx/RenderState.hpp>
#include <luma/gfx/Texture2D.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <algorithm>
#include <memory>
#include <stb_image.h>
#include <stdexcept>
#include <utility>
namespace luma {
ImageData ImageData::decode(const std::filesystem::path& p, bool flip) {
    ImageData d;
    int channels;
    std::unique_ptr<stbi_uc, void (*)(void*)> pixels(
        stbi_load(p.string().c_str(), &d.width, &d.height, &channels, 4), stbi_image_free);
    if (!pixels)
        throw std::runtime_error(
            "decode " + p.string() + ": " +
            (stbi_failure_reason() ? stbi_failure_reason() : "unknown image error"));
    size_t row = static_cast<size_t>(d.width) * 4;
    d.rgba.assign(pixels.get(), pixels.get() + row * d.height);
    if (flip)
        for (int y = 0; y < d.height / 2; ++y)
            std::swap_ranges(d.rgba.begin() + y * row, d.rgba.begin() + (y + 1) * row,
                             d.rgba.begin() + (d.height - 1 - y) * row);
    return d;
}
Texture2D::~Texture2D() {
    if (handle_)
        glDeleteTextures(1, &handle_);
}
Texture2D::Texture2D(Texture2D&& o) noexcept : handle_(std::exchange(o.handle_, 0)) {}
Texture2D& Texture2D::operator=(Texture2D&& o) noexcept {
    if (this != &o) {
        if (handle_)
            glDeleteTextures(1, &handle_);
        handle_ = std::exchange(o.handle_, 0);
    }
    return *this;
}
Texture2D Texture2D::fromFile(const std::filesystem::path& p, const TextureOptions& options) {
    requireContext();
    options.validate();
    auto image = ImageData::decode(p, options.flipY);
    Texture2D t;
    GLint active, old, alignment;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGenTextures(1, &t.handle_);
    glBindTexture(GL_TEXTURE_2D, t.handle_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 options.colorSpace == ColorSpace::SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8, image.width,
                 image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
    auto filter = [](Filter f) {
        return f == Filter::Nearest  ? GL_NEAREST
               : f == Filter::Linear ? GL_LINEAR
                                     : GL_LINEAR_MIPMAP_LINEAR;
    };
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    options.wrap == Wrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    options.wrap == Wrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter(options.minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter(options.magFilter));
    if (options.mipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glBindTexture(GL_TEXTURE_2D, old);
    glActiveTexture(active);
    checkGl("texture upload");
    return t;
}
void Texture2D::bind(unsigned unit) const {
    requireContext();
    GLint count;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &count);
    if (unit >= static_cast<unsigned>(count))
        throw std::out_of_range("texture unit outside GL implementation limit");
    if (!handle_)
        throw std::logic_error("bind empty Texture2D");
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, handle_);
}
} // namespace luma

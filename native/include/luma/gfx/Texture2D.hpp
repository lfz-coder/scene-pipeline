#pragma once
#include <cstdint>
#include <filesystem>
#include <luma/gfx/TextureOptions.hpp>
#include <string>
#include <vector>
namespace luma {
// Decoding is CPU-only; row flipping never changes stb global decode state.
struct ImageData {
    int width = 0, height = 0;
    std::vector<std::uint8_t> rgba;
    static ImageData decode(const std::filesystem::path& path, bool flipY);
};
class Texture2D {
  public:
    Texture2D() = default;
    ~Texture2D();
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) noexcept;
    Texture2D& operator=(Texture2D&&) noexcept;
    static Texture2D fromFile(const std::filesystem::path& path, const TextureOptions& options);
    void bind(unsigned unit) const;
    unsigned handle() const {
        return handle_;
    }

  private:
    unsigned handle_ = 0;
};
} // namespace luma

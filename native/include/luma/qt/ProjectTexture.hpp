#pragma once
#include <luma/scene/AssetStore.hpp>
#include <optional>

namespace luma {
// Portable built-in asset description. Runtime IDs belong only to AssetStore.
struct ProjectTexture {
    std::string asset;
    TextureOptions options;
    void validate() const;
    static std::optional<ProjectTexture> capture(const AssetStore&, std::optional<TextureId>);
    TextureId resolve(AssetStore&) const;
    bool operator==(const ProjectTexture& other) const {
        return asset == other.asset && options.cacheSuffix() == other.options.cacheSuffix();
    }
    bool operator!=(const ProjectTexture& other) const { return !(*this == other); }
};
} // namespace luma

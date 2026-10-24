#pragma once
#include <luma/gfx/IResourceFactory.hpp>
#include <luma/scene/MaterialLibrary.hpp>
#include <map>
#include <unordered_map>
namespace luma {
struct TextureSource {
    std::filesystem::path path;
    TextureOptions options;
};
// Owns resources until application shutdown; entities borrow typed IDs.
// A shared MaterialId deliberately shares edits; cloneMaterial separates them.
class AssetStore {
  public:
    explicit AssetStore(IResourceFactory& factory) : factory_(factory) {}
    MaterialLibrary& materials() {
        return materials_;
    }
    const MaterialLibrary& materials() const {
        return materials_;
    }
    MeshId primitive(Primitive primitive);
    TextureId loadTexture(const std::filesystem::path&, const TextureOptions& options = {});
    MaterialId createMaterial(MaterialDesc material);
    MaterialId cloneMaterial(MaterialId source);
    MaterialDesc& material(MaterialId material);
    const MaterialDesc& material(MaterialId material) const;
    const IMesh& mesh(MeshId mesh) const;
    const ITexture& texture(TextureId texture) const;
    const TextureSource& textureSource(TextureId texture) const;

  private:
    IResourceFactory& factory_;
    std::uint32_t nextMesh_ = 1, nextTexture_ = 1;
    std::map<std::uint32_t, std::unique_ptr<IMesh>> meshes_;
    std::map<std::uint32_t, std::unique_ptr<ITexture>> textures_;
    std::map<std::uint32_t, TextureSource> textureSources_;
    MaterialLibrary materials_;
    std::map<Primitive, MeshId> primitives_;
    std::unordered_map<std::string, TextureId> textureCache_;
};
} // namespace luma

#pragma once
#include <luma/gfx/IResourceFactory.hpp>
namespace luma {
class NativeResourceFactory final : public IResourceFactory {
  public:
    std::unique_ptr<IShader> createShader(const std::filesystem::path&,
                                          const std::filesystem::path&) override;
    std::unique_ptr<IMesh> createMesh(const MeshData&) override;
    std::unique_ptr<ITexture> createTexture(const std::filesystem::path&,
                                            const TextureOptions&) override;
};
} // namespace luma

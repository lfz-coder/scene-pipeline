#include <limits>
#include <luma/scene/AssetStore.hpp>
#include <stdexcept>
namespace luma {
namespace {
template <class Map> decltype(auto) get(Map& map, std::uint32_t id, const char* type) {
    auto it = map.find(id);
    if (it == map.end())
        throw std::out_of_range(std::string("invalid ") + type + " " + std::to_string(id));
    return (it->second);
}
std::uint32_t allocate(std::uint32_t& n) {
    if (n == std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("resource ID exhausted");
    return n++;
}
} // namespace
MeshId AssetStore::primitive(Primitive p) {
    auto i = primitives_.find(p);
    if (i != primitives_.end())
        return i->second;
    auto data = p == Primitive::Cube    ? PrimitiveFactory::cube()
                : p == Primitive::Plane ? PrimitiveFactory::plane()
                                        : PrimitiveFactory::triangle();
    auto mesh = factory_.createMesh(data);
    if (!mesh)
        throw std::runtime_error("resource factory returned null mesh");
    MeshId id{allocate(nextMesh_)};
    meshes_.emplace(id.value, std::move(mesh));
    primitives_.emplace(p, id);
    return id;
}
TextureId AssetStore::loadTexture(const std::filesystem::path& p, const TextureOptions& options) {
    options.validate();
    auto path = std::filesystem::weakly_canonical(std::filesystem::absolute(p));
    auto key = path.generic_string() + options.cacheSuffix();
    auto it = textureCache_.find(key);
    if (it != textureCache_.end())
        return it->second;
    auto texture = factory_.createTexture(path, options);
    if (!texture)
        throw std::runtime_error("resource factory returned null texture");
    TextureId id{allocate(nextTexture_)};
    textures_.emplace(id.value, std::move(texture));
    textureSources_.emplace(id.value, TextureSource{path, options});
    textureCache_.emplace(std::move(key), id);
    return id;
}
MaterialId AssetStore::createMaterial(MaterialDesc m) {
    return materials_.createMaterial(std::move(m));
}
MaterialId AssetStore::cloneMaterial(MaterialId id) {
    return materials_.cloneMaterial(id);
}
MaterialDesc& AssetStore::material(MaterialId id) {
    return materials_.material(id);
}
const MaterialDesc& AssetStore::material(MaterialId id) const {
    return materials_.material(id);
}
const IMesh& AssetStore::mesh(MeshId id) const {
    return *get(meshes_, id.value, "MeshId");
}
const ITexture& AssetStore::texture(TextureId id) const {
    return *get(textures_, id.value, "TextureId");
}
const TextureSource& AssetStore::textureSource(TextureId id) const {
    return get(textureSources_, id.value, "TextureId");
}
} // namespace luma

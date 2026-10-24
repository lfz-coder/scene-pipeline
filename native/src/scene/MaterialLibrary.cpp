#include <limits>
#include <luma/scene/MaterialLibrary.hpp>
#include <stdexcept>
namespace luma {
MaterialId MaterialLibrary::createMaterial(MaterialDesc material) {
    material.validate();
    if (next_ == std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("material ID exhausted");
    MaterialId id{next_};
    materials_.emplace(id.value, std::move(material));
    ++next_;
    return id;
}
MaterialId MaterialLibrary::cloneMaterial(MaterialId source) {
    return createMaterial(material(source));
}
MaterialDesc& MaterialLibrary::material(MaterialId id) {
    auto found = materials_.find(id.value);
    if (found == materials_.end())
        throw std::out_of_range("invalid MaterialId " + std::to_string(id.value));
    return found->second;
}
const MaterialDesc& MaterialLibrary::material(MaterialId id) const {
    auto found = materials_.find(id.value);
    if (found == materials_.end())
        throw std::out_of_range("invalid MaterialId " + std::to_string(id.value));
    return found->second;
}
} // namespace luma

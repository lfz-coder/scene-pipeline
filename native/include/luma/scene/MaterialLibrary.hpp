#pragma once
#include <luma/scene/MaterialDesc.hpp>
#include <map>
namespace luma {
// CPU ownership only. Sharing an ID shares edits; cloning separates values.
class MaterialLibrary {
  public:
    MaterialId createMaterial(MaterialDesc material);
    MaterialId cloneMaterial(MaterialId source);
    MaterialDesc& material(MaterialId id);
    const MaterialDesc& material(MaterialId id) const;

  private:
    std::uint32_t next_ = 1;
    std::map<std::uint32_t, MaterialDesc> materials_;
};
} // namespace luma

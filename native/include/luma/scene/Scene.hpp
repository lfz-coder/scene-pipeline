#pragma once
#include <luma/scene/Transform.hpp>
#include <luma/scene/Types.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace luma {
struct RenderItem {
    EntityId entity;
    MeshId mesh;
    MaterialId material;
};
// IDs are monotonic within this scene, including after erase/clear.
// Keep IDs across frames; use references and name views only immediately.
class Scene {
  public:
    EntityId create(std::string name, MeshId mesh, MaterialId material);
    bool contains(EntityId entity) const;
    Transform& transform(EntityId entity);
    const Transform& transform(EntityId entity) const;
    std::string_view name(EntityId entity) const;
    void setMaterial(EntityId entity, MaterialId material);
    void erase(EntityId entity);
    void clear();
    const std::vector<RenderItem>& renderItems() const {
        return items_;
    }

  private:
    struct Object {
        std::string name;
        Transform transform;
    };
    std::uint32_t next_ = 1;
    std::unordered_map<std::uint32_t, Object> objects_;
    std::vector<RenderItem> items_;
};
} // namespace luma

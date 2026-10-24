#include <algorithm>
#include <limits>
#include <luma/scene/Scene.hpp>
#include <stdexcept>
namespace luma {
namespace {
std::out_of_range missing(EntityId id) {
    return std::out_of_range("invalid EntityId " + std::to_string(id.value));
}
} // namespace
EntityId Scene::create(std::string name, MeshId mesh, MaterialId mat) {
    if (!mesh.value || !mat.value)
        throw std::invalid_argument("entity requires nonzero mesh and material IDs");
    if (next_ == std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("EntityId exhausted");
    EntityId id{next_++};
    objects_.emplace(id.value, Object{std::move(name), {}});
    try {
        items_.push_back({id, mesh, mat});
    } catch (...) {
        objects_.erase(id.value);
        throw;
    }
    return id;
}
bool Scene::contains(EntityId e) const {
    return objects_.count(e.value) != 0;
}
Transform& Scene::transform(EntityId e) {
    auto it = objects_.find(e.value);
    if (it == objects_.end())
        throw missing(e);
    return it->second.transform;
}
const Transform& Scene::transform(EntityId e) const {
    auto it = objects_.find(e.value);
    if (it == objects_.end())
        throw missing(e);
    return it->second.transform;
}
std::string_view Scene::name(EntityId e) const {
    auto it = objects_.find(e.value);
    if (it == objects_.end())
        throw missing(e);
    return it->second.name;
}
void Scene::setMaterial(EntityId e, MaterialId m) {
    if (!m.value)
        throw std::invalid_argument("MaterialId 0");
    for (auto& i : items_)
        if (i.entity.value == e.value) {
            i.material = m;
            return;
        }
    throw missing(e);
}
void Scene::erase(EntityId e) {
    if (!objects_.erase(e.value))
        throw missing(e);
    items_.erase(std::remove_if(items_.begin(), items_.end(),
                                [&](const auto& i) { return i.entity.value == e.value; }),
                 items_.end());
}
void Scene::clear() {
    items_.clear();
    objects_.clear();
}
} // namespace luma

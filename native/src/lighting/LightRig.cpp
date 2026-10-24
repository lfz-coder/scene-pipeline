#include <algorithm>
#include <luma/lighting/LightRig.hpp>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace luma {
namespace {
LightKind kindOf(const LightValue& value) {
    return std::visit(
        [](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, DirectionalLight>)
                return LightKind::Directional;
            if constexpr (std::is_same_v<T, PointLight>)
                return LightKind::Point;
            return LightKind::Spot;
        },
        value);
}
void validateValue(const LightValue& value) {
    std::visit([](const auto& item) { item.validate(); }, value);
}
} // namespace

LightId LightRig::append(LightKind kind, LightValue value) {
    validateValue(value);
    if (kind == LightKind::Directional &&
        std::any_of(entries_.begin(), entries_.end(), [](const auto& e) {
            return e.kind == LightKind::Directional;
        }))
        throw std::invalid_argument("only one directional light is supported");
    const auto count = static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(), [kind](const auto& e) { return e.kind == kind; }));
    if ((kind == LightKind::Point && count >= maxPoints) ||
        (kind == LightKind::Spot && count >= maxSpots))
        throw std::invalid_argument("light type limit reached");
    if (next_ == std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("light ID exhausted");
    LightId id{next_++};
    entries_.push_back({id, kind, std::move(value)});
    return id;
}

LightId LightRig::addDirectional(DirectionalLight value) {
    return append(LightKind::Directional, std::move(value));
}
LightId LightRig::addPoint(PointLight value) {
    return append(LightKind::Point, std::move(value));
}
LightId LightRig::addSpot(SpotLight value) {
    return append(LightKind::Spot, std::move(value));
}

std::vector<LightEntry>::iterator LightRig::find(LightId id) {
    return std::find_if(entries_.begin(), entries_.end(), [id](const auto& e) { return e.id == id; });
}
std::vector<LightEntry>::const_iterator LightRig::find(LightId id) const {
    return std::find_if(entries_.begin(), entries_.end(), [id](const auto& e) { return e.id == id; });
}

LightEntry& LightRig::entry(LightId id) {
    auto it = find(id);
    if (it == entries_.end())
        throw std::out_of_range("invalid light ID");
    return *it;
}
const LightEntry& LightRig::entry(LightId id) const {
    auto it = find(id);
    if (it == entries_.end())
        throw std::out_of_range("invalid light ID");
    return *it;
}

void LightRig::update(LightId id, LightValue value) {
    validateValue(value);
    auto& current = entry(id);
    if (current.kind != kindOf(value))
        throw std::invalid_argument("light type cannot change during update");
    current.value = std::move(value);
}

void LightRig::remove(LightId id) {
    auto it = find(id);
    if (it == entries_.end())
        throw std::out_of_range("invalid light ID");
    entries_.erase(it);
}

void LightRig::validate() const {
    std::size_t directional = 0, points = 0, spots = 0;
    for (const auto& light : entries_) {
        validateValue(light.value);
        if (light.kind != kindOf(light.value))
            throw std::invalid_argument("light entry type mismatch");
        if (light.kind == LightKind::Directional)
            ++directional;
        if (light.kind == LightKind::Point)
            ++points;
        if (light.kind == LightKind::Spot)
            ++spots;
    }
    if (directional > 1 || points > maxPoints || spots > maxSpots)
        throw std::invalid_argument("light rig limit exceeded");
}
} // namespace luma

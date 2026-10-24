#pragma once
#include <luma/lighting/LightTypes.hpp>
#include <variant>
#include <vector>

namespace luma {

enum class LightKind { Directional, Point, Spot };
using LightValue = std::variant<DirectionalLight, PointLight, SpotLight>;

struct LightEntry {
    LightId id;
    LightKind kind;
    LightValue value;
};

class LightRig {
  public:
    static constexpr std::size_t maxPoints = 8;
    static constexpr std::size_t maxSpots = 4;

    LightId addDirectional(DirectionalLight value);
    LightId addPoint(PointLight value);
    LightId addSpot(SpotLight value);
    void update(LightId id, LightValue value);
    void remove(LightId id);
    void clear() {
        entries_.clear();
    }
    LightEntry& entry(LightId id);
    const LightEntry& entry(LightId id) const;
    const std::vector<LightEntry>& entries() const {
        return entries_;
    }
    void validate() const;

  private:
    LightId append(LightKind kind, LightValue value);
    std::vector<LightEntry>::iterator find(LightId id);
    std::vector<LightEntry>::const_iterator find(LightId id) const;
    std::uint32_t next_ = 1;
    std::vector<LightEntry> entries_;
};

} // namespace luma

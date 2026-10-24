#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace luma {
struct TimelineClip {
    std::string id, assetId, title;
    std::int64_t inMs = 0, outMs = 3000;
    std::int64_t duration() const { return outMs - inMs; }
};
struct TimelinePosition { std::size_t index; std::int64_t sourceMs; };
class TimelineModel {
  public:
    const std::vector<TimelineClip>& clips() const { return clips_; }
    std::int64_t duration() const;
    void add(TimelineClip clip);
    void trim(std::size_t index, std::int64_t in, std::int64_t out, std::int64_t sourceDuration);
    void split(std::size_t index, std::int64_t localMs, std::string newId);
    void move(std::size_t from, std::size_t to);
    void remove(std::size_t index);
    TimelinePosition locate(std::int64_t globalMs) const;
    void clear() { clips_.clear(); }
  private:
    std::vector<TimelineClip> clips_;
    void validate(const TimelineClip& clip) const;
};
} // namespace luma

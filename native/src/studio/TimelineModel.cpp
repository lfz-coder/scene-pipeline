#include <luma/studio/TimelineModel.hpp>
#include <algorithm>
#include <stdexcept>
namespace luma {
std::int64_t TimelineModel::duration() const {
    std::int64_t sum = 0;
    for (const auto& clip : clips_) sum += clip.duration();
    return sum;
}
void TimelineModel::validate(const TimelineClip& clip) const {
    if (clip.id.empty() || clip.assetId.empty() || clip.title.empty() ||
        clip.title.size() > 480 || clip.inMs < 0 || clip.outMs > 3600000 || clip.duration() < 100)
        throw std::invalid_argument("片段须有名称，时长至少 0.1 秒且源范围不超过一小时");
}
void TimelineModel::add(TimelineClip clip) {
    validate(clip);
    if (clips_.size() >= 100 || duration() + clip.duration() > 3600000)
        throw std::invalid_argument("时间线最多 100 个片段，总时长不超过一小时");
    for (const auto& previous : clips_)
        if (previous.id == clip.id) throw std::invalid_argument("片段 ID 重复");
    clips_.push_back(std::move(clip));
}
void TimelineModel::trim(std::size_t index, std::int64_t in, std::int64_t out,
                         std::int64_t sourceDuration) {
    auto candidate = clips_.at(index);
    candidate.inMs = in; candidate.outMs = out;
    validate(candidate);
    if (out > sourceDuration || duration() - clips_[index].duration() + candidate.duration() > 3600000)
        throw std::invalid_argument("裁剪超出素材或时间线时长");
    clips_[index] = std::move(candidate);
}
void TimelineModel::split(std::size_t index, std::int64_t localMs, std::string newId) {
    const auto original = clips_.at(index);
    if (localMs < 100 || localMs > original.duration() - 100 || clips_.size() >= 100)
        throw std::invalid_argument("拆分点两侧须至少保留 0.1 秒");
    for (const auto& clip : clips_)
        if (clip.id == newId) throw std::invalid_argument("片段 ID 重复");
    auto right = original;
    right.id = std::move(newId); right.inMs += localMs;
    validate(right);
    clips_.insert(clips_.begin() + index + 1, right);
    clips_[index].outMs = right.inMs;
}
void TimelineModel::move(std::size_t from, std::size_t to) {
    auto clip = clips_.at(from);
    clips_.at(to); // Validate both before mutation.
    clips_.erase(clips_.begin() + from);
    clips_.insert(clips_.begin() + to, std::move(clip));
}
void TimelineModel::remove(std::size_t index) {
    clips_.at(index);
    clips_.erase(clips_.begin() + index);
}
TimelinePosition TimelineModel::locate(std::int64_t globalMs) const {
    if (clips_.empty()) throw std::logic_error("时间线为空");
    auto remaining = std::clamp<std::int64_t>(globalMs, 0, duration() - 1);
    for (std::size_t i = 0; i < clips_.size(); ++i) {
        if (remaining < clips_[i].duration()) return {i, clips_[i].inMs + remaining};
        remaining -= clips_[i].duration();
    }
    throw std::logic_error("时间线定位失败");
}
} // namespace luma

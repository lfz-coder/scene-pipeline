#pragma once
#include <array>
namespace luma {
enum class PipelineNodeState { Pending, Running, Waiting, Done, Failed, Cancelled };
class PipelineState {
  public:
    const std::array<PipelineNodeState, 4>& nodes() const { return nodes_; }
    bool active() const;
    bool succeeded() const { return nodes_[3] == PipelineNodeState::Done; }
    void start();
    void prepared();
    void analyzed();
    void approve();
    void assembled();
    void fail();
    void cancel();
    void reset();
    // Validate persisted states; running work is restored as interrupted.
    void restore(const std::array<PipelineNodeState, 4>&);
  private:
    std::array<PipelineNodeState, 4> nodes_{};
    void advance(int from, int to, PipelineNodeState expected, PipelineNodeState next);
};
} // namespace luma

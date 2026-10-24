#include <luma/studio/PipelineState.hpp>
#include <stdexcept>
namespace luma {
bool PipelineState::active() const {
    for (auto state : nodes_)
        if (state == PipelineNodeState::Running || state == PipelineNodeState::Waiting) return true;
    return false;
}
void PipelineState::start() {
    if (active()) throw std::logic_error("已有编排正在执行");
    reset(); nodes_[0] = PipelineNodeState::Running;
}
void PipelineState::advance(int from, int to, PipelineNodeState expected, PipelineNodeState next) {
    if (nodes_[from] != expected || nodes_[to] != PipelineNodeState::Pending)
        throw std::logic_error("编排节点的前置条件未满足");
    nodes_[from] = PipelineNodeState::Done;
    nodes_[to] = next;
}
void PipelineState::prepared() { advance(0, 1, PipelineNodeState::Running, PipelineNodeState::Running); }
void PipelineState::analyzed() { advance(1, 2, PipelineNodeState::Running, PipelineNodeState::Waiting); }
void PipelineState::approve() { advance(2, 3, PipelineNodeState::Waiting, PipelineNodeState::Running); }
void PipelineState::assembled() {
    if (nodes_[3] != PipelineNodeState::Running) throw std::logic_error("请先确认镜头方案");
    nodes_[3] = PipelineNodeState::Done;
}
void PipelineState::fail() {
    for (auto& state : nodes_)
        if (state == PipelineNodeState::Running || state == PipelineNodeState::Waiting)
            state = PipelineNodeState::Failed;
}
void PipelineState::cancel() {
    for (auto& state : nodes_)
        if (state != PipelineNodeState::Done) state = PipelineNodeState::Cancelled;
}
void PipelineState::reset() { nodes_.fill(PipelineNodeState::Pending); }
void PipelineState::restore(const std::array<PipelineNodeState, 4>& states) {
    PipelineState phase;
    bool reachable = false;
    for (int i = 0; i < 6; ++i) {
        auto failed = phase; failed.fail();
        auto cancelled = phase; cancelled.cancel();
        reachable |= states == phase.nodes_ || states == failed.nodes_ || states == cancelled.nodes_;
        if (i == 0) phase.start();
        else if (i == 1) phase.prepared();
        else if (i == 2) phase.analyzed();
        else if (i == 3) phase.approve();
        else if (i == 4) phase.assembled();
    }
    if (!reachable) throw std::invalid_argument("编排状态不符合节点执行顺序");
    nodes_ = states;
    for (auto& state : nodes_)
        if (state == PipelineNodeState::Running) state = PipelineNodeState::Failed;
}
} // namespace luma

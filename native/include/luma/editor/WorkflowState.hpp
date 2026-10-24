#pragma once
#include <array>
#include <stdexcept>
namespace luma {
enum class WorkflowStage { Scene, Material, Lighting, Camera, Review, PostProcess, Export };
// Approval state only. Human and future automated callers use the same rules.
class WorkflowState {
  public:
    WorkflowStage current() const {
        return current_;
    }
    bool completed(WorkflowStage stage) const {
        return completed_[index(stage)];
    }
    bool canComplete(WorkflowStage stage) const {
        auto n = index(stage);
        for (std::size_t i = 0; i < n; ++i)
            if (!completed_[i])
                return false;
        return true;
    }
    void setCurrent(WorkflowStage stage) {
        index(stage);
        current_ = stage;
    }
    void complete(WorkflowStage stage) {
        if (!canComplete(stage))
            throw std::logic_error("请先确认前面的创作步骤");
        completed_[index(stage)] = true;
    }
    void invalidateFrom(WorkflowStage stage) {
        for (auto i = index(stage); i < completed_.size(); ++i)
            completed_[i] = false;
    }
    void reset() {
        completed_.fill(false);
        current_ = WorkflowStage::Scene;
    }

  private:
    static std::size_t index(WorkflowStage stage) {
        auto value = static_cast<int>(stage);
        if (value < 0 || value >= 7)
            throw std::out_of_range("invalid workflow stage");
        return static_cast<std::size_t>(value);
    }
    WorkflowStage current_ = WorkflowStage::Scene;
    std::array<bool, 7> completed_{};
};
} // namespace luma

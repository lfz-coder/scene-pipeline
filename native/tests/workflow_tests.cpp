#include <iostream>
#include <luma/editor/WorkflowState.hpp>
#include <stdexcept>
using namespace luma;
static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
int main() {
    try {
        WorkflowState state;
        check(state.current() == WorkflowStage::Scene, "starts at scene");
        check(!state.canComplete(WorkflowStage::Export), "cannot approve export before review");
        for (int i = 0; i < 7; ++i) {
            auto stage = static_cast<WorkflowStage>(i);
            check(state.canComplete(stage), "previous stages permit approval");
            state.complete(stage);
        }
        state.invalidateFrom(WorkflowStage::Material);
        check(state.completed(WorkflowStage::Scene), "upstream approval survives edits");
        for (int i = 1; i < 7; ++i)
            check(!state.completed(static_cast<WorkflowStage>(i)),
                  "downstream approval invalidated");
        for (int i = 0; i < 7; ++i)
            state.complete(static_cast<WorkflowStage>(i));
        state.invalidateFrom(WorkflowStage::Lighting);
        check(state.completed(WorkflowStage::Material), "material approval survives lighting edit");
        check(!state.completed(WorkflowStage::Lighting) &&
                  !state.completed(WorkflowStage::PostProcess),
              "lighting edit invalidates lighting and postprocess");
        state.setCurrent(WorkflowStage::Camera);
        check(state.current() == WorkflowStage::Camera, "users may navigate freely");
        bool rejected = false;
        try {
            state.complete(WorkflowStage::Review);
        } catch (const std::logic_error&) {
            rejected = true;
        }
        check(rejected, "out of order approval rejected");
        rejected = false;
        try {
            state.setCurrent(static_cast<WorkflowStage>(8));
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        check(rejected, "invalid stages rejected");
        state.reset();
        check(!state.completed(WorkflowStage::Scene), "reset clears approvals");
        std::cout << "PASS workflow: sequential approval, downstream invalidation, free "
                     "navigation, validation\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include <iostream>
#include <luma/studio/TimelineModel.hpp>
#include <luma/studio/PipelineState.hpp>
#include <stdexcept>
using namespace luma;
static int failures = 0;
static void check(bool value, const char* name) {
    std::cout << (value ? "PASS " : "FAIL ") << name << '\n';
    failures += !value;
}
template <class F> void rejected(F action, const char* name) {
    try { action(); check(false, name); }
    catch (const std::exception&) { check(true, name); }
}
int main() {
    TimelineModel timeline;
    timeline.add({"a", "asset", "first", 0, 3000});
    timeline.add({"b", "asset", "second", 1000, 5000});
    check(timeline.duration() == 7000, "duration sums trimmed intervals");
    check(timeline.locate(3000).index == 1 && timeline.locate(3000).sourceMs == 1000,
          "cut boundary selects next source interval");
    check(timeline.locate(7000).index == 1 && timeline.locate(7000).sourceMs == 4999,
          "final boundary clamps to final frame");
    rejected([&] { timeline.trim(0, 4000, 1000, 5000); }, "invalid trim is rejected");
    check(timeline.duration() == 7000, "rejected trim is atomic");
    timeline.split(0, 1500, "c");
    check(timeline.duration() == 7000 && timeline.clips().size() == 3 &&
          timeline.clips()[1].inMs == 1500, "split preserves total duration and source offsets");
    timeline.move(2, 0);
    check(timeline.clips()[0].id == "b", "move reorders clips");
    timeline.trim(0, 1500, 4000, 5000);
    check(timeline.duration() == 5500, "trim recomputes duration");
    timeline.remove(0);
    check(timeline.duration() == 3000, "remove recomputes duration");
    rejected([&] { timeline.add({"a", "asset", "duplicate", 0, 1000}); }, "clip IDs are unique");
    PipelineState pipeline;
    rejected([&] { pipeline.approve(); }, "approval requires completed analysis");
    pipeline.start(); pipeline.prepared(); pipeline.analyzed();
    check(pipeline.nodes()[2] == PipelineNodeState::Waiting, "analysis waits for human review");
    pipeline.approve(); pipeline.assembled();
    check(pipeline.succeeded(), "approval permits assembly");
    pipeline.reset(); pipeline.start(); pipeline.prepared(); pipeline.cancel();
    rejected([&] { pipeline.analyzed(); }, "cancelled pipeline rejects late analysis");
    check(pipeline.nodes()[1] == PipelineNodeState::Cancelled, "cancel marks active nodes");
    return failures ? 1 : 0;
}

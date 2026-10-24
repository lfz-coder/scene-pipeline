#pragma once
#include <filesystem>
#include <luma/app/SceneLayer.hpp>
namespace luma {
struct RunOptions {
    bool hidden = false, editor = true;
    int frames = 0;
    bool deterministic = false;
    std::filesystem::path screenshot;
};
// Construction validates CPU configuration. run() owns context creation and
// guarantees resource cleanup while that context is still current. Run once.
class Application {
  public:
    Application(std::string title, int logicalWidth, int logicalHeight);
    int run(std::unique_ptr<SceneLayer> layer, const RunOptions& options = {});

  private:
    std::string title_;
    int width_, height_;
    bool ran_ = false;
};
std::filesystem::path assetRoot();
} // namespace luma

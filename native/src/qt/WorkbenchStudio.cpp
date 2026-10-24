#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/StudioWorkspace.hpp>
namespace luma {
QWidget* WorkbenchWindow::buildStudio() {
    studio_ = new StudioWorkspace(this);
    studio_->captureScene = [this] {
        if (!viewport_->model() || !viewport_->isValid()) return QImage{};
        auto image = viewport_->grabFramebuffer(); image.setDevicePixelRatio(1); return image;
    };
    studio_->onChanged = [this] { changed(WorkflowStage::Export); };
    studio_->onError = [this](const QString& text) { notify(text, true); };
    studio_->onReviewReady = [this] { showPage(5); };
    studio_->onAssembled = [this] { showPage(4); };
    studio_->onLocateAsset = [this] { showPage(4); };
    return studio_;
}
} // namespace luma

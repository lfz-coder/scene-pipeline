#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <luma/qt/WorkbenchTheme.hpp>
#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/MediaGallery.hpp>
#include <luma/qt/StudioWorkspace.hpp>
namespace luma {
using namespace qtui;
WorkbenchWindow::WorkbenchWindow() {
    setWindowTitle("光场工坊 · AI 创作工作台");
    resize(1480, 920);
    setMinimumSize(1100, 720);
    setStyleSheet(workbenchStyleSheet());
    buildShell();
    viewport_->onSceneChanged = [this] {
        QSignalBlocker block(presets_);
        presets_->setCurrentIndex(
            presets_->findData(QString::fromStdString(viewport_->currentPreset())));
        if (auto* model = viewport_->model(); model && model->scene().renderItems().size() > 1) {
            const auto& items = model->scene().renderItems();
            if (model->scene().name(items.front().entity) == "Ground")
                viewport_->controller()->select(items[1].entity);
        }
        rebuildObjects();
        refreshInspector();
        if (!restoring_) {
            projectName_->setText(presets_->currentText() + " · 新作品");
            workflow_.reset();
            shotData_.clear();
            shotGallery_->clear();
            studio_->restoreState({});
            projectPath_.clear();
            dirty_ = false;
            saveState_->setText("本地创作");
        }
        refreshWorkflow();
        notify("场景就绪 · 拖动视口开始创作");
    };
    viewport_->onError = [this](const QString& message) { notify("操作失败：" + message, true); };
    viewport_->onCameraChanged = [this] { changed(WorkflowStage::Camera); };
    auto shortcut = [this](const QKeySequence& key, const std::function<void()>& action) {
        auto* s = new QShortcut(key, this);
        connect(s, &QShortcut::activated, this, action);
    };
    shortcut(QKeySequence::Save, [this] { chooseSaveProject(); });
    shortcut(QKeySequence::Open, [this] { chooseLoadProject(); });
    shortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), [this] { chooseExport(); });
    shortcut(QKeySequence::SaveAs, [this] { chooseSaveProject(true); });
    auto* cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, [this] { updateCameraInfo(); });
    cameraTimer->start(350);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->showMessage("正在准备渲染视口…");
    auto* sessionLabel = label("本地工程 · AI 辅助 / 手动编辑", this, "muted");
    sessionLabel->setWordWrap(false);
    statusBar()->addPermanentWidget(sessionLabel);
    showPage(1);
    selectStage(0);
}
} // namespace luma

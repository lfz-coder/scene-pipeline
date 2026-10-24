#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/MediaGallery.hpp>
#include <luma/qt/StudioWorkspace.hpp>
#include <QTabWidget>
#include <algorithm>
#include "QtUiHelpers.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildShots() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);
    layout->addWidget(label("镜头 / 静帧集合", page, "eyebrow"));
    layout->addWidget(label("镜头集合", page, "pageTitle"));
    layout->addWidget(label("在工作台的「镜头构图」保存快照。双击预览，恢复机位时使用当前场景与材质。仅本次会话保留。",
                            page, "muted"));
    shotGallery_ = new MediaGallery("shots", true, page);
    shotGallery_->onRestore = [this](int) { restoreShot(); };
    shotGallery_->onRename = [this](int id, const QString& name) {
        for (auto& shot : shotData_) if (shot.id == id) shot.name = name;
    };
    shotGallery_->onRemove = [this](int id) {
        shotData_.erase(std::remove_if(shotData_.begin(), shotData_.end(),
                                     [id](const auto& shot) { return shot.id == id; }), shotData_.end());
        updateCameraInfo();
    };
    shotGallery_->onMessage = [this](const QString& text, bool error) { notify(text, error); };
    layout->addWidget(shotGallery_, 1);
    return page;
}
QWidget* WorkbenchWindow::buildExports() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);
    layout->addWidget(label("项目 / 素材与输出", page, "eyebrow"));
    layout->addWidget(label("项目媒体", page, "pageTitle"));
    layout->addWidget(label("项目素材与时间线共享；导出记录保留本次会话的 PNG 输出。",
                            page, "muted"));
    exportGallery_ = new MediaGallery("exports", false, page);
    exportGallery_->onMessage = [this](const QString& text, bool error) { notify(text, error); };
    auto* tabs = new QTabWidget(page);
    tabs->setObjectName("mediaTabs");
    tabs->addTab(studio_->assetsView(), "项目素材");
    tabs->addTab(exportGallery_, "导出记录");
    layout->addWidget(tabs, 1);
    return page;
}
}

#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/MediaGallery.hpp>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QStatusBar>
#include <algorithm>
namespace luma {
bool WorkbenchWindow::exportImage(const QString& path) {
    const bool success = viewport_->lastError().empty() && viewport_->savePng(path);
    const auto time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    exportGallery_->addEntry({nextMediaId_++, QFileInfo(path).fileName(), presets_->currentText(),
                              (success ? "已导出 · " : "失败 · ") + time,
                              absolutePath, success ? QImage(path) : QImage{}, success});
    if (success) {
        if (workflow_.canComplete(WorkflowStage::Export)) workflow_.complete(WorkflowStage::Export);
        refreshWorkflow();
    }
    addTask((success ? "PNG 导出完成 · " : "PNG 导出失败 · ") + path, success);
    statusBar()->showMessage(success ? "已导出：" + path
                                    : "导出失败，请检查文件夹是否存在以及写入权限。", 12000);
    return success;
}
void WorkbenchWindow::chooseExport() {
    QFileDialog dialog(this, "导出当前画面", projectName_->text() + ".png");
    dialog.setNameFilter("PNG 图片 (*.png)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("png");
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) return;
    const auto path = dialog.selectedFiles().first();
    if (!path.endsWith(".png", Qt::CaseInsensitive)) {
        notify("请使用 .png 扩展名导出图片。", true);
        return;
    }
    exportImage(path);
}
void WorkbenchWindow::captureShot() {
    if (!viewport_->model() || !viewport_->isValid()) {
        notify("视口尚未就绪，请稍后保存镜头。", true);
        return;
    }
    auto image = viewport_->grabFramebuffer();
    image.setDevicePixelRatio(1);
    if (image.isNull()) { notify("镜头快照失败，视口尚未就绪", true); return; }
    const auto& camera = viewport_->model()->camera();
    const int id = nextMediaId_++;
    auto name = QString("镜头 %1 · %2").arg(id, 2, 10, QChar('0')).arg(presets_->currentText());
    shotData_.push_back({id, name, camera.eye(), camera.target(), camera.up(),
                         viewport_->model()->controls().mode(), image, viewport_->currentPreset()});
    shotGallery_->addEntry({id, name, presets_->currentText(),
                            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), {}, image});
    addTask("保存镜头快照 · " + name, true);
    notify("镜头已保存。在镜头集合中可预览画面或恢复机位。");
    updateCameraInfo();
}
void WorkbenchWindow::restoreShot() {
    const auto* selected = shotGallery_->selected();
    if (!selected) { notify("请先选择一个镜头"); return; }
    const auto found = std::find_if(shotData_.begin(), shotData_.end(),
                                   [selected](const auto& shot) { return shot.id == selected->id; });
    if (found == shotData_.end()) return;
    const auto shot = *found;
    if (viewport_->currentPreset() != shot.preset) switchPreset(shot.preset);
    if (!viewport_->lastError().empty() || viewport_->currentPreset() != shot.preset) return;
    if (invoke([shot](auto& c) {
            c.setCameraPose(shot.eye, shot.target, shot.up);
            c.setCameraMode(shot.mode);
        })) {
        changed(WorkflowStage::Camera);
        showPage(1);
        selectStage(3);
        refreshInspector();
        notify("已恢复机位 · 使用当前场景的最新材质");
    }
}
} // namespace luma

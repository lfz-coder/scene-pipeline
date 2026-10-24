#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/MediaGallery.hpp>
#include <luma/qt/StudioWorkspace.hpp>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
namespace luma {
bool WorkbenchWindow::chooseSaveProject(bool saveAs) {
    if (!saveAs && !projectPath_.isEmpty()) return saveProject(projectPath_);
    QFileDialog dialog(this, "保存可编辑工程",
                       projectPath_.isEmpty() ? projectName_->text() + ".luma" : projectPath_);
    dialog.setNameFilter("光场工坊工程 (*.luma)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("luma"); // Overwrite checks must use the final filename.
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) return false;
    const auto path = dialog.selectedFiles().first();
    if (!path.endsWith(".luma", Qt::CaseInsensitive)) {
        notify("请使用 .luma 扩展名保存工程。", true);
        return false;
    }
    return saveProject(path);
}
bool WorkbenchWindow::confirmLeave() {
    if (!dirty_) return true;
    QMessageBox dialog(QMessageBox::Question, "保存当前作品？",
                      "当前工程有未保存的修改。是否先保存，再继续操作？\n"
                      "工程文件不包含本次会话的镜头快照。", QMessageBox::NoButton, this);
    dialog.setObjectName("unsavedChanges");
    auto* save = dialog.addButton("保存并继续", QMessageBox::AcceptRole);
    auto* discard = dialog.addButton("放弃修改", QMessageBox::DestructiveRole);
    dialog.addButton("取消", QMessageBox::RejectRole);
    dialog.setDefaultButton(save);
    dialog.exec();
    if (dialog.clickedButton() == save) return chooseSaveProject();
    return dialog.clickedButton() == discard;
}
void WorkbenchWindow::chooseLoadProject() {
    const auto path = QFileDialog::getOpenFileName(this, "打开光场工坊工程", {},
                                                  "光场工坊工程 (*.luma)");
    if (!path.isEmpty() && confirmLeave()) loadProject(path);
}
bool WorkbenchWindow::createProject(const std::string& preset) {
    if (!viewport_->controller()) { notify("渲染视口尚未就绪，请稍后重试。", true); return false; }
    if (!confirmLeave()) return false;
    restoring_ = true;
    viewport_->loadPreset(preset);
    restoring_ = false;
    if (!viewport_->lastError().empty()) return false;
    viewport_->controller()->setStyle(StyleKind::Basic);
    viewport_->controller()->setRenderOptions(RenderOptions{});
    drafts_.clear();
    shotData_.clear();
    shotGallery_->clear();
    studio_->restoreState({});
    projectPath_.clear();
    projectName_->setText(presets_->currentText() + " · 新作品");
    workflow_.reset();
    changed(WorkflowStage::Scene);
    rebuildObjects();
    showPage(1);
    selectStage(0);
    addTask("新建工程 · " + presets_->currentText(), true);
    return true;
}
void WorkbenchWindow::buildProjectMenu(QPushButton* button) {
    auto* menu = new QMenu(button);
    menu->setObjectName("projectActions");
    auto* fresh = menu->addAction("新建当前场景工程…", this,
                                 [this] { createProject(viewport_->currentPreset()); });
    fresh->setObjectName("newCurrentProject");
    menu->addSeparator();
    auto* saveAs = menu->addAction("另存为…", this, [this] { chooseSaveProject(true); });
    saveAs->setObjectName("saveProjectAs");
    button->setMenu(menu);
}
void WorkbenchWindow::closeEvent(QCloseEvent* event) {
    if (confirmLeave()) event->accept();
    else event->ignore();
}
} // namespace luma

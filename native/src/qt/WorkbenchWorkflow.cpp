#include <luma/qt/WorkbenchWindow.hpp>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <algorithm>
#include <exception>
#include "WorkbenchWorkflowText.hpp"
namespace luma {
void WorkbenchWindow::showPage(int index) {
    if (index < 0 || index >= int(nav_.size()))
        return;
    if (index == 0)
        updateHomePreview();
    shellPages_->setCurrentIndex(index == 0 ? 0 : 1);
    projectTabs_->setVisible(index != 0);
    findChild<QPushButton*>("toggleAssistant")->setVisible(index != 0);
    projectName_->setVisible(index != 0);
    if (index != 0) pages_->setCurrentIndex(index - 1);
    nav_[index]->setChecked(true);
    if (index == 1)
        viewport_->update();
}
void WorkbenchWindow::selectStage(int index) {
    workflow_.setCurrent(WorkflowStage(index));
    stagePages_->setCurrentIndex(index);
    stageTitle_->setText(titles[index]);
    stageHelp_->setText(descriptions[index]);
    refreshWorkflow();
    refreshInspector();
}
void WorkbenchWindow::refreshWorkflow() {
    int current = int(workflow_.current());
    for (int i = 0; i < 7; ++i) {
        steps_[i]->setText(
            QString("%1   %2")
                .arg(workflow_.completed(WorkflowStage(i)) ? "✓" : QString::number(i + 1))
                .arg(titles[i]));
        steps_[i]->setChecked(i == current);
    }
    previous_->setEnabled(current > 0);
    next_->setEnabled(current == 6 || workflow_.canComplete(WorkflowStage(current)));
    next_->setText(current == 6 ? "导出 PNG  ↗" : "确认并继续  →");
    next_->setAccessibleName(next_->text());
    next_->setToolTip(current == 6 ? "选择保存位置，导出当前画面的 PNG 图片"
                         : workflow_.canComplete(WorkflowStage(current))
                               ? "确认当前阶段并继续"
                               : "请先完成前面的步骤；也可自由返回编辑");
}
void WorkbenchWindow::completeStage() {
    if (workflow_.current() == WorkflowStage::Export) {
        chooseExport();
        return;
    }
    try {
        workflow_.complete(workflow_.current());
        addTask(titles[int(workflow_.current())] + " · 已确认", true);
        selectStage(std::min(6, int(workflow_.current()) + 1));
    } catch (const std::exception&) {
        notify("请先确认前面的步骤；点击顶部步骤可以返回。", true);
    }
}
bool WorkbenchWindow::invoke(const std::function<void(EditorController&)>& action) {
    if (!viewport_->controller())
        return false;
    try {
        action(*viewport_->controller());
        viewport_->update();
        return true;
    } catch (const std::exception& e) {
        notify(QString("编辑失败：") + e.what(), true);
        refreshInspector();
        return false;
    }
}
void WorkbenchWindow::changed(WorkflowStage stage) {
    if (restoring_)
        return;
    workflow_.invalidateFrom(stage);
    dirty_ = true;
    saveState_->setText("● 未保存");
    refreshWorkflow();
}
} // namespace luma

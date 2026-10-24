#include <luma/qt/MediaGallery.hpp>
#include <QDesktopServices>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
namespace luma {
void MediaGallery::preview() {
    const auto* current = selected();
    if (!current || !current->success || current->image.isNull()) return;
    std::vector<MediaEntry> visible;
    int index = 0;
    for (int i = 0; i < list_->count(); ++i) {
        const auto* item = list_->item(i);
        const auto& entry = entries_.at(item->data(Qt::UserRole).toInt());
        if (item->isHidden() || !entry.success || entry.image.isNull()) continue;
        if (entry.id == current->id) index = int(visible.size());
        visible.push_back(entry);
    }
    MediaPreviewDialog(std::move(visible), index, this).exec();
}
void MediaGallery::rename() {
    const auto* current = selected();
    if (!current) return;
    const int id = current->id;
    QInputDialog dialog(this);
    dialog.setWindowTitle("重命名镜头");
    dialog.setLabelText("镜头名称（1–120 个字符）");
    dialog.setTextValue(current->title);
    dialog.setOkButtonText("保存名称");
    dialog.setCancelButtonText("取消");
    if (dialog.exec() != QDialog::Accepted) return;
    const auto name = dialog.textValue().trimmed();
    if (name.isEmpty() || name.size() > 120) {
        if (onMessage) onMessage("名称须为 1–120 个字符，未修改原名称。", true);
        return;
    }
    auto& entry = entries_.at(id);
    entry.title = name;
    list_->currentItem()->setText(name + "\n" + entry.detail.section('\n', 0, 0));
    list_->currentItem()->setToolTip(name + "\n" + entry.detail);
    if (onRename) onRename(id, name);
    filter();
}
void MediaGallery::remove() {
    const auto* current = selected();
    if (!current) return;
    const int id = current->id;
    QMessageBox dialog(QMessageBox::Question, "移除这条记录？",
                      "将从本次会话中移除「" + current->title + "」。磁盘文件会保留。",
                      QMessageBox::NoButton, this);
    auto* confirm = dialog.addButton("移除记录", QMessageBox::AcceptRole);
    dialog.addButton("取消", QMessageBox::RejectRole);
    dialog.exec();
    if (dialog.clickedButton() != confirm) return;
    delete list_->takeItem(list_->currentRow());
    entries_.erase(id);
    if (onRemove) onRemove(id);
    refreshGroups();
    filter();
}
void MediaGallery::open(bool directory) {
    const auto* entry = selected();
    if (!entry || !entry->success || entry->path.isEmpty()) return;
    QFileInfo file(entry->path);
    if (!file.exists() || !file.isFile()) {
        if (onMessage) onMessage("原文件已不存在，请重新导出：" + entry->path, true);
        return;
    }
    const auto path = directory ? file.absolutePath() : file.absoluteFilePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)) && onMessage)
        onMessage("无法打开：" + path, true);
}
void MediaGallery::contextMenu(const QPoint& position) {
    auto* item = list_->itemAt(position);
    if (!item) return;
    list_->setCurrentItem(item);
    QMenu menu(this);
    auto action = [&](QPushButton* button) {
        if (!button) return;
        auto* itemAction = menu.addAction(button->text(), button, &QPushButton::click);
        itemAction->setEnabled(button->isEnabled());
    };
    action(preview_);
    action(primary_);
    action(rename_);
    action(folder_);
    menu.addSeparator();
    action(remove_);
    menu.exec(list_->viewport()->mapToGlobal(position));
}
} // namespace luma

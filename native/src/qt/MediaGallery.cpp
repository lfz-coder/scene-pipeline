#include <luma/qt/MediaGallery.hpp>
#include <luma/qt/WorkbenchTheme.hpp>
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace luma {
MediaGallery::MediaGallery(const QString& prefix, bool snapshots, QWidget* parent)
    : QWidget(parent), prefix_(prefix), snapshots_(snapshots) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    auto* filters = new QHBoxLayout;
    count_ = qtui::label("", this, "muted");
    count_->setObjectName(prefix + "Count");
    filters->addWidget(count_);
    filters->addStretch();
    group_ = new QComboBox(this);
    group_->setObjectName(prefix + "Group");
    group_->addItem("全部场景");
    group_->setMinimumWidth(150);
    filters->addWidget(group_);
    if (!snapshots) {
        status_ = new QComboBox(this);
        status_->setObjectName(prefix + "Status");
        status_->addItems({"全部状态", "已导出", "失败"});
        filters->addWidget(status_);
        connect(status_, &QComboBox::currentIndexChanged, this, [this] { filter(); });
    }
    search_ = new QLineEdit(this);
    search_->setObjectName(prefix + "Search");
    search_->setPlaceholderText(snapshots ? "搜索镜头名称…" : "搜索作品或路径…");
    search_->setClearButtonEnabled(true);
    search_->setMaximumWidth(280);
    filters->addWidget(search_);
    layout->addLayout(filters);
    empty_ = qtui::label("", this, "muted");
    empty_->setObjectName(prefix + "Empty");
    empty_->setAlignment(Qt::AlignCenter);
    layout->addWidget(empty_);
    list_ = new QListWidget(this);
    list_->setObjectName(prefix);
    list_->setViewMode(QListView::IconMode);
    list_->setResizeMode(QListView::Adjust);
    list_->setMovement(QListView::Static);
    list_->setIconSize({240, 155});
    list_->setGridSize({264, 224});
    list_->setSpacing(10);
    list_->setWordWrap(true);
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(list_, 1);
    auto* actions = new QHBoxLayout;
    auto action = [&](const QString& title, const QString& suffix) {
        auto* result = qtui::button(title, this);
        result->setObjectName(prefix + suffix);
        actions->addWidget(result);
        return result;
    };
    preview_ = action("预览画面", "Preview");
    primary_ = action(snapshots ? "恢复选中镜头" : "打开文件", "Open");
    if (snapshots) {
        primary_->setObjectName("restoreShot");
        rename_ = action("重命名", "Rename");
        connect(rename_, &QPushButton::clicked, this, [this] { rename(); });
    } else {
        folder_ = action("打开所在目录", "Folder");
        connect(folder_, &QPushButton::clicked, this, [this] { open(true); });
    }
    remove_ = action("移除记录", "Remove");
    remove_->setToolTip("只移除本次会话中的记录，保留磁盘文件");
    actions->addStretch();
    layout->addLayout(actions);
    connect(search_, &QLineEdit::textChanged, this, [this] { filter(); });
    connect(group_, &QComboBox::currentIndexChanged, this, [this] { filter(); });
    connect(list_, &QListWidget::currentRowChanged, this, [this] { refreshActions(); });
    connect(list_, &QListWidget::itemSelectionChanged, this, [this] { refreshActions(); });
    connect(list_, &QListWidget::customContextMenuRequested, this, &MediaGallery::contextMenu);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this] { preview(); });
    connect(preview_, &QPushButton::clicked, this, [this] { preview(); });
    connect(remove_, &QPushButton::clicked, this, [this] { remove(); });
    connect(primary_, &QPushButton::clicked, this, [this] {
        const auto* entry = selected();
        if (entry && snapshots_ && onRestore) onRestore(entry->id);
        else if (!snapshots_) open(false);
    });
    filter();
}
const MediaEntry* MediaGallery::selected() const {
    const auto* item = list_->currentItem();
    if (!item || item->isHidden() || !item->isSelected()) return nullptr;
    const auto found = entries_.find(item->data(Qt::UserRole).toInt());
    return found == entries_.end() ? nullptr : &found->second;
}
void MediaGallery::addEntry(MediaEntry entry) {
    auto* item = new QListWidgetItem;
    item->setText(entry.title + "\n" + entry.detail.section('\n', 0, 0));
    item->setToolTip(entry.title + "\n" + entry.detail + "\n" + entry.path);
    item->setData(Qt::UserRole, entry.id);
    item->setIcon(entry.image.isNull() ? workbenchIcon("export", "#efa898")
                                      : QIcon(QPixmap::fromImage(entry.image)));
    item->setSizeHint({260, 218});
    if (!entry.success) item->setForeground(QColor("#efa898"));
    entries_.insert_or_assign(entry.id, std::move(entry));
    list_->insertItem(snapshots_ ? list_->count() : 0, item);
    refreshGroups();
    filter();
    if (!item->isHidden()) list_->setCurrentItem(item);
}
void MediaGallery::clear() {
    entries_.clear();
    list_->clear();
    search_->clear();
    if (status_) status_->setCurrentIndex(0);
    refreshGroups();
    filter();
}
void MediaGallery::refreshGroups() {
    QSignalBlocker block(group_);
    const auto previous = group_->currentText();
    group_->clear();
    group_->addItem("全部场景");
    for (const auto& pair : entries_)
        if (group_->findText(pair.second.group) < 0) group_->addItem(pair.second.group);
    const int index = group_->findText(previous);
    group_->setCurrentIndex(index < 0 ? 0 : index);
}
void MediaGallery::filter() {
    int visible = 0;
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        const auto& entry = entries_.at(item->data(Qt::UserRole).toInt());
        const auto text = entry.title + " " + entry.detail + " " + entry.path;
        const bool matches = text.contains(search_->text().trimmed(), Qt::CaseInsensitive) &&
            (group_->currentIndex() == 0 || group_->currentText() == entry.group) &&
            (!status_ || status_->currentIndex() == 0 ||
             (status_->currentIndex() == 1 ? entry.success : !entry.success));
        item->setHidden(!matches);
        visible += matches;
    }
    count_->setText(QString("%1 / %2 个%3").arg(visible).arg(entries_.size())
                        .arg(snapshots_ ? "镜头" : "作品"));
    empty_->setVisible(visible == 0);
    empty_->setText(entries_.empty() ? (snapshots_ ? "还没有镜头快照\n先保存一个喜欢的角度。"
                                                   : "尚未导出作品\n在工作台完成画面后导出 PNG。")
                                     : "没有符合条件的记录，请修改搜索或筛选条件。");
    refreshActions();
}
void MediaGallery::refreshActions() {
    const auto* entry = selected();
    preview_->setEnabled(entry && entry->success && !entry->image.isNull());
    primary_->setEnabled(entry && (snapshots_ || entry->success));
    remove_->setEnabled(entry);
    if (rename_) rename_->setEnabled(entry);
    if (folder_) folder_->setEnabled(entry && entry->success);
}
} // namespace luma

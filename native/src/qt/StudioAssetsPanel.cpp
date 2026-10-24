#include <luma/qt/StudioAssetsPanel.hpp>
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace luma {
StudioAssetsPanel::StudioAssetsPanel(const StudioDocument& document, QWidget* parent)
    : QWidget(parent), document_(document) {
    setObjectName("studioAssetsPanel");
    auto* box = new QVBoxLayout(this);
    box->setContentsMargins(0, 12, 0, 0);
    box->setSpacing(14);
    auto* tools = new QHBoxLayout;
    search_ = new QLineEdit(this);
    search_->setObjectName("assetSearch");
    search_->setPlaceholderText("搜索项目素材…");
    search_->setClearButtonEnabled(true);
    type_ = new QComboBox(this);
    type_->setObjectName("assetType");
    type_->addItems({"全部素材", "图片", "视频"});
    tools->addWidget(search_, 1);
    tools->addWidget(type_);
    box->addLayout(tools);
    count_ = qtui::label("", this, "muted");
    count_->setObjectName("assetCount");
    box->addWidget(count_);
    items_ = new QListWidget(this);
    items_->setObjectName("projectAssets");
    items_->setViewMode(QListView::IconMode);
    items_->setResizeMode(QListView::Adjust);
    items_->setMovement(QListView::Static);
    items_->setIconSize({180, 102});
    items_->setGridSize({210, 155});
    items_->setWordWrap(true);
    box->addWidget(items_, 1);
    locate_ = qtui::button("在时间线中查看", this);
    locate_->setObjectName("locateAsset");
    box->addWidget(locate_, 0, Qt::AlignRight);
    box->addWidget(qtui::label("素材来自场景采集、文件导入和已确认的方案。视频保留本地文件引用。", this, "muted"));
    connect(search_, &QLineEdit::textChanged, this, [this] { filter(); });
    connect(type_, &QComboBox::currentIndexChanged, this, [this] { filter(); });
    connect(items_, &QListWidget::itemSelectionChanged, this, [this] {
        locate_->setEnabled(!items_->selectedItems().isEmpty());
    });
    connect(items_, &QListWidget::itemDoubleClicked, this, [this] { locate(); });
    connect(locate_, &QPushButton::clicked, this, [this] { locate(); });
    refresh();
}
void StudioAssetsPanel::refresh() {
    QString selected;
    if (items_->currentItem()) selected = items_->currentItem()->data(Qt::UserRole).toString();
    {
        QSignalBlocker block(items_);
        items_->clear();
        for (const auto& asset : document_.assets) {
            QPixmap thumbnail(180, 102);
            thumbnail.fill(QColor("#222630"));
            if (asset.image.isNull()) {
                QPainter painter(&thumbnail);
                painter.setPen(QColor("#bac4d9"));
                painter.drawText(thumbnail.rect(), Qt::AlignCenter, asset.video ? "▶  视频" : "图片");
            } else thumbnail = QPixmap::fromImage(asset.image).scaled(180, 102, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            const QString detail = asset.video ? QString("视频 · %1 秒").arg(asset.durationMs / 1000., 0, 'f', 1) : "图片";
            auto* item = new QListWidgetItem(QIcon(thumbnail), asset.title + "\n" + detail, items_);
            item->setData(Qt::UserRole, asset.id);
            item->setData(Qt::UserRole + 1, asset.video);
            item->setToolTip(asset.title + (asset.path.isEmpty() ? "" : "\n" + asset.path));
            if (asset.id == selected) items_->setCurrentItem(item);
        }
    }
    filter();
}
void StudioAssetsPanel::filter() {
    int visible = 0;
    for (int i = 0; i < items_->count(); ++i) {
        auto* item = items_->item(i);
        const bool video = item->data(Qt::UserRole + 1).toBool();
        const bool show = item->text().contains(search_->text(), Qt::CaseInsensitive) &&
            (type_->currentIndex() == 0 || video == (type_->currentIndex() == 2));
        item->setHidden(!show);
        if (!show) item->setSelected(false);
        visible += show;
    }
    count_->setText(QString("%1 项素材%2").arg(visible).arg(items_->count() ? "" : " · 导入素材或采集场景后显示在这里"));
    locate_->setEnabled(!items_->selectedItems().isEmpty());
}
void StudioAssetsPanel::locate() {
    if (!items_->selectedItems().isEmpty() && onLocate)
        onLocate(items_->selectedItems().first()->data(Qt::UserRole).toString());
}
} // namespace luma

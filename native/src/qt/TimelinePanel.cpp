#include <luma/qt/TimelinePanel.hpp>
#include "QtUiHelpers.hpp"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSignalBlocker>
#include <QSlider>
#include <QSet>
#include <QVBoxLayout>
#include <algorithm>
namespace luma {
using namespace qtui;
TimelinePanel::TimelinePanel(StudioDocument& doc, QWidget* parent)
    : QWidget(parent), document_(doc), player_(doc, this) {
    setObjectName("timelinePanel"); setMinimumWidth(350);
    auto* box = new QVBoxLayout(this); box->setContentsMargins(12, 10, 12, 10); box->setSpacing(8);
    auto* tools = new QHBoxLayout; tools->addWidget(label("时间线", this, "sectionTitle"));
    tools->addStretch(); play_ = button("播放", this, "primary"); play_->setObjectName("timelinePlay");
    auto* import = button("导入素材", this); import->setObjectName("importTimelineMedia");
    auto* scene = button("采集场景", this); scene->setObjectName("captureTimelineScene");
    tools->addWidget(play_); tools->addWidget(import); tools->addWidget(scene); box->addLayout(tools);
    clock_ = label("", this, "muted"); clock_->setWordWrap(false); box->addWidget(clock_);
    position_ = new QSlider(Qt::Horizontal, this); position_->setObjectName("timelinePosition"); box->addWidget(position_);
    clips_ = new QListWidget(this); clips_->setObjectName("timelineClips");
    clips_->setViewMode(QListView::IconMode); clips_->setFlow(QListView::LeftToRight);
    clips_->setWrapping(false); clips_->setMovement(QListView::Static); clips_->setIconSize({72, 40});
    clips_->setGridSize({115, 79}); clips_->setFixedHeight(99); box->addWidget(clips_);
    auto* range = new QHBoxLayout;
    in_ = spin(this, 0, 3600, .1); in_->setDecimals(3); in_->setPrefix("入 "); in_->setObjectName("clipIn");
    out_ = spin(this, .1, 3600, .1); out_->setDecimals(3); out_->setPrefix("出 "); out_->setObjectName("clipOut");
    trim_ = button("裁剪", this); trim_->setObjectName("applyTrim");
    split_ = button("拆分", this); split_->setObjectName("splitClip");
    range->addWidget(in_); range->addWidget(out_); range->addWidget(trim_); range->addWidget(split_); box->addLayout(range);
    auto* actions = new QHBoxLayout;
    left_ = button("左移", this); left_->setObjectName("moveClipLeft");
    right_ = button("右移", this); right_->setObjectName("moveClipRight");
    remove_ = button("移除片段", this); remove_->setObjectName("removeClip");
    actions->addWidget(left_); actions->addWidget(right_); actions->addWidget(remove_); actions->addStretch(); box->addLayout(actions);
    player_.onFrame = [this](const QImage& frame) { if (onFrame) onFrame(frame); };
    player_.onPosition = [this](qint64 position) { updatePosition(position); };
    player_.onPlaying = [this](bool value) { play_->setText(value ? "暂停" : "播放"); };
    player_.onError = [this](const QString& text) { if (onError) onError(text); };
    connect(play_, &QPushButton::clicked, this, [this] { if (player_.playing()) player_.pause(); else player_.play(); });
    connect(position_, &QSlider::valueChanged, this, [this](int value) { player_.seek(value); });
    connect(clips_, &QListWidget::itemSelectionChanged, this, [this] { selectionChanged(); });
    connect(import, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, "导入时间线素材", {},
            "视频或图片 (*.mp4 *.mov *.m4v *.webm *.png *.jpg *.jpeg)");
        if (!path.isEmpty()) importPath(path);
    });
    connect(scene, &QPushButton::clicked, this, [this] {
        edit([this] { document_.addImage(captureScene ? captureScene() : QImage{}, "场景静帧"); });
    });
    connect(trim_, &QPushButton::clicked, this, [this] {
        edit([this] {
            const int row = selected(); const auto& clip = document_.timeline.clips().at(row);
            document_.timeline.trim(row, qRound64(in_->value() * 1000), qRound64(out_->value() * 1000),
                document_.assets[QString::fromStdString(clip.assetId)].durationMs);
        });
    });
    connect(split_, &QPushButton::clicked, this, [this] {
        edit([this] { document_.timeline.split(selected(), player_.position() - clipStart(selected()), studioId().toStdString()); });
    });
    connect(left_, &QPushButton::clicked, this, [this] { edit([this] { document_.timeline.move(selected(), selected() - 1); }); });
    connect(right_, &QPushButton::clicked, this, [this] { edit([this] { document_.timeline.move(selected(), selected() + 1); }); });
    connect(remove_, &QPushButton::clicked, this, [this] {
        edit([this] {
            document_.timeline.remove(selected());
            QSet<QString> used;
            for (const auto& clip : document_.timeline.clips()) used.insert(QString::fromStdString(clip.assetId));
            for (auto it = document_.assets.begin(); it != document_.assets.end();)
                if (!used.contains(it.key())) it = document_.assets.erase(it); else ++it;
        });
    });
    refresh();
}
int TimelinePanel::selected() const { return clips_->selectedItems().empty() ? -1 : clips_->currentRow(); }
qint64 TimelinePanel::clipStart(int row) const {
    qint64 start = 0; for (int i = 0; i < row; ++i) start += document_.timeline.clips()[i].duration(); return start;
}
void TimelinePanel::selectionChanged() {
    const int row = selected(); const bool valid = row >= 0;
    for (auto* button : {trim_, remove_}) button->setEnabled(valid);
    in_->setEnabled(valid); out_->setEnabled(valid);
    left_->setEnabled(valid && row > 0); right_->setEnabled(valid && row + 1 < int(document_.timeline.clips().size()));
    if (valid) {
        const auto& clip = document_.timeline.clips()[row];
        in_->setValue(clip.inMs / 1000.); out_->setValue(clip.outMs / 1000.);
        player_.seek(clipStart(row));
    }
    updatePosition(player_.position());
}
void TimelinePanel::updatePosition(qint64 position) {
    const auto duration = document_.timeline.duration();
    QSignalBlocker block(position_); position_->setRange(0, int(duration)); position_->setValue(int(position));
    auto time = [](qint64 ms) { return QString("%1:%2.%3").arg(ms / 60000, 2, 10, QChar('0'))
        .arg(ms / 1000 % 60, 2, 10, QChar('0')).arg(ms % 1000, 3, 10, QChar('0')); };
    clock_->setText(time(position) + " / " + time(duration) + " · " + QString::number(clips_->count()) + " 个片段");
    play_->setEnabled(duration > 0); position_->setEnabled(duration > 0);
    const int row = selected(); const auto local = position - clipStart(std::max(0, row));
    split_->setEnabled(row >= 0 && local >= 100 && local <= document_.timeline.clips()[row].duration() - 100);
}
void TimelinePanel::refresh() {
    const int selectedRow = selected();
    const auto selectedId = selectedRow >= 0 ? clips_->item(selectedRow)->data(Qt::UserRole).toString() : QString{};
    int restoredRow = -1;
    {
        QSignalBlocker block(clips_); clips_->clear();
        for (const auto& clip : document_.timeline.clips()) {
            const auto& asset = document_.assets[QString::fromStdString(clip.assetId)];
            auto* item = new QListWidgetItem(QString::fromStdString(clip.title) + "\n" +
                QString::number(clip.duration() / 1000., 'f', 1) + " 秒", clips_);
            item->setData(Qt::UserRole, QString::fromStdString(clip.id));
            if (item->data(Qt::UserRole).toString() == selectedId) restoredRow = clips_->count() - 1;
            item->setToolTip(QString::fromStdString(clip.title) + (asset.video ? " · 视频" : " · 静帧"));
            if (!asset.image.isNull()) item->setIcon(QIcon(QPixmap::fromImage(asset.image)));
        }
        if (clips_->count()) clips_->setCurrentRow(restoredRow >= 0 ? restoredRow : std::clamp(selectedRow, 0, clips_->count() - 1));
    }
    player_.refresh(); selectionChanged();
}
void TimelinePanel::edit(const std::function<void()>& action) {
    player_.pause();
    try { action(); refresh(); if (onChanged) onChanged(); }
    catch (const std::exception& e) { if (onError) onError(QString::fromUtf8(e.what())); }
}
bool TimelinePanel::locateAsset(const QString& id) {
    for (int i = 0; i < int(document_.timeline.clips().size()); ++i) {
        if (QString::fromStdString(document_.timeline.clips()[i].assetId) != id) continue;
        player_.pause();
        clips_->setCurrentRow(i);
        selectionChanged();
        return true;
    }
    return false;
}
void TimelinePanel::reset() { ++epoch_; player_.pause(); refresh(); }
} // namespace luma

#include <luma/qt/PipelinePanel.hpp>
#include "QtUiHelpers.hpp"
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cmath>
namespace luma {
using namespace qtui;
PipelinePanel::PipelinePanel(StudioDocument& doc, QWidget* parent) : QWidget(parent), document_(doc) {
    setMinimumWidth(230); setObjectName("pipelinePanel");
    auto* box = new QVBoxLayout(this); box->setContentsMargins(14, 16, 14, 14); box->setSpacing(12);
    box->addWidget(label("镜头方案", this, "sectionTitle"));
    box->addWidget(label("从创作助手获得建议，在这里审阅与调整，然后确认加入时间线。", this, "muted"));
    auto* stages = new QHBoxLayout;
    for (auto& node : nodes_) { node = label("", this); stages->addWidget(node, 1); }
    box->addLayout(stages);
    box->addWidget(label("镜头建议 · 标题与时长可编辑", this, "muted"));
    shots_ = new QTableWidget(0, 2, this); shots_->setObjectName("pipelineShots");
    shots_->setHorizontalHeaderLabels({"镜头", "秒"}); shots_->verticalHeader()->hide();
    shots_->verticalHeader()->setDefaultSectionSize(64);
    shots_->setAlternatingRowColors(true);
    shots_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    shots_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); shots_->setColumnWidth(1, 110);
    shots_->setSelectionBehavior(QAbstractItemView::SelectRows); box->addWidget(shots_, 1);
    box->addWidget(label("建议只安排片段；确认后使用引用画面创建静帧，不生成新视频。", this, "muted"));
    approve_ = button("确认并加入时间线", this, "primary"); approve_->setObjectName("approvePipeline");
    discard_ = button("放弃当前方案", this); discard_->setObjectName("discardPipeline");
    box->addWidget(approve_); box->addWidget(discard_);
    connect(approve_, &QPushButton::clicked, this, [this] { if (onApprove) onApprove(); });
    connect(discard_, &QPushButton::clicked, this, [this] {
        document_.pipeline.cancel(); document_.suggestions = {}; document_.reviewImage = {};
        refresh(); if (onChanged) onChanged();
    });
    connect(shots_, &QTableWidget::cellChanged, this, [this](int row, int column) { edit(row, column); });
    refresh();
}
void PipelinePanel::refresh() {
    const QStringList titles{"素材准备", "模型分析", "人工确认", "时间线组装"};
    const QStringList states{"待执行", "执行中", "等待确认", "完成", "失败 / 已中断", "已取消"};
    const QStringList colors{"#9295a0", "#d1b0f2", "#edc477", "#93ccb0", "#efa898", "#9295a0"};
    for (int i = 0; i < 4; ++i) {
        const int state = int(document_.pipeline.nodes()[i]);
        nodes_[i]->setText(QString("%1  %2  ·  %3").arg(i + 1).arg(titles[i], states[state]));
        nodes_[i]->setStyleSheet("padding:9px;background:#24262d;border-radius:7px;color:" + colors[state]);
    }
    QSignalBlocker block(shots_); shots_->setRowCount(document_.suggestions.size());
    for (int i = 0; i < document_.suggestions.size(); ++i) {
        const auto shot = document_.suggestions[i].toObject();
        shots_->setItem(i, 0, new QTableWidgetItem(shot["title"].toString()));
        shots_->setItem(i, 1, new QTableWidgetItem(QString::number(shot["duration_ms"].toInt() / 1000., 'f', 1)));
    }
    const bool waiting = document_.pipeline.nodes()[2] == PipelineNodeState::Waiting;
    shots_->setEditTriggers(waiting ? QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                   : QAbstractItemView::NoEditTriggers);
    approve_->setEnabled(waiting && !document_.suggestions.isEmpty()); discard_->setVisible(waiting);
}
void PipelinePanel::edit(int row, int column) {
    if (row < 0 || row >= document_.suggestions.size()) return;
    auto shot = document_.suggestions[row].toObject();
    const auto text = shots_->item(row, column)->text().trimmed();
    bool valid = true;
    if (column == 0) { valid = !text.isEmpty() && text.toUcs4().size() <= 120; if (valid) shot["title"] = text; }
    else {
        const double seconds = text.toDouble(&valid);
        valid = valid && std::isfinite(seconds) && seconds >= .1 && seconds <= 60;
        if (valid) shot["duration_ms"] = qRound64(seconds * 1000);
    }
    if (!valid) { if (onError) onError("镜头名称须为 1–120 个字符，时长须为 0.1–60 秒。"); refresh(); return; }
    document_.suggestions[row] = shot; if (onChanged) onChanged();
}
} // namespace luma

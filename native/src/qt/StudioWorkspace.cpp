#include <luma/qt/StudioWorkspace.hpp>
#include <luma/qt/ChatPanel.hpp>
#include <luma/qt/PipelinePanel.hpp>
#include <luma/qt/TimelinePanel.hpp>
#include <luma/qt/StudioAssetsPanel.hpp>
#include <QStackedWidget>
#include "QtUiHelpers.hpp"
#include <QEvent>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <stdexcept>
namespace luma {
StudioWorkspace::StudioWorkspace(QWidget* parent) : QWidget(parent) {
    setObjectName("studioWorkspace");
    auto* box = new QVBoxLayout(this);
    box->setContentsMargins(0, 0, 0, 0);
    auto* split = new QSplitter(Qt::Horizontal, this);
    split->setObjectName("projectSplit");
    split->setChildrenCollapsible(false);
    box->addWidget(split);
    chat_ = new ChatPanel(document_, split);
    views_ = new QStackedWidget(split);
    views_->setObjectName("pages");
    split->setSizes({340, 1140});
    split->setStretchFactor(1, 1);
    timelineView_ = new QWidget(this);
    auto* timelineLayout = new QVBoxLayout(timelineView_);
    timelineLayout->setContentsMargins(12, 12, 12, 12);
    auto* center = new QSplitter(Qt::Vertical, timelineView_);
    center->setChildrenCollapsible(false);
    timelineLayout->addWidget(center);
    auto* previewBox = new QWidget(center);
    auto* previewLayout = new QVBoxLayout(previewBox); previewLayout->setContentsMargins(10, 10, 10, 10);
    previewLayout->addWidget(qtui::label("时间线预览", previewBox, "sectionTitle"));
    preview_ = new QLabel("导入视频、采集场景，或确认镜头方案后开始预览。", previewBox);
    preview_->setObjectName("timelinePreview"); preview_->setWordWrap(true); preview_->setAlignment(Qt::AlignCenter);
    preview_->setMinimumSize(280, 150); preview_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    preview_->setStyleSheet("background:#0d0e12;border:1px solid #30323b;border-radius:10px;color:#9b9da8;");
    previewLayout->addWidget(preview_, 1);
    preview_->installEventFilter(this);
    timeline_ = new TimelinePanel(document_, center);
    pipeline_ = new PipelinePanel(document_, this);
    assets_ = new StudioAssetsPanel(document_, this);
    center->setSizes({430, 300});
    assets_->onLocate = [this](const QString& id) {
        if (timeline_->locateAsset(id)) { if (onLocateAsset) onLocateAsset(); }
        else report("该素材当前没有时间线片段。");
    };
    auto capture = [this] { return captureScene ? captureScene() : QImage{}; };
    chat_->captureScene = capture; timeline_->captureScene = capture;
    timeline_->onFrame = [this](const QImage& image) { previewFrame_ = image; refreshPreview(); };
    auto changed = [this] { assets_->refresh(); if (onChanged) onChanged(); };
    auto error = [this](const QString& text) { report(text); };
    timeline_->onChanged = changed; pipeline_->onChanged = changed; chat_->onChanged = changed;
    timeline_->onError = error; pipeline_->onError = error;
    chat_->onError = [this](const QString& text) {
        if (document_.pipeline.nodes()[1] == PipelineNodeState::Running) document_.pipeline.fail();
        pipeline_->refresh(); report(text);
    };
    chat_->onStarted = [this](const QImage& frame) {
        if (document_.pipeline.active()) document_.pipeline.cancel();
        document_.pipeline.start(); document_.pipeline.prepared(); document_.suggestions = {};
        document_.reviewImage = frame; pipeline_->refresh();
    };
    chat_->onRun = [this](const QJsonObject& run) {
        const auto status = run["status"].toString();
        if (status == "succeeded" && document_.pipeline.nodes()[1] == PipelineNodeState::Running) {
            document_.suggestions = run["shots"].toArray(); document_.pipeline.analyzed();
        } else if (status == "cancelled") document_.pipeline.cancel();
        else if (status == "failed") document_.pipeline.fail();
        pipeline_->refresh();
        if (status == "succeeded" && onReviewReady) onReviewReady();
    };
    pipeline_->onApprove = [this] { approve(); };
}
StudioWorkspace::~StudioWorkspace() {
    // All borrowed document references must die before the value member.
    delete chat_;
    delete pipeline_;
    delete timeline_;
    delete assets_;
}
QWidget* StudioWorkspace::reviewView() const { return pipeline_; }
QWidget* StudioWorkspace::assetsView() const { return assets_; }
void StudioWorkspace::setAssistantVisible(bool visible) { chat_->setVisible(visible); }
void StudioWorkspace::openSettings() { chat_->openSettings(); }
bool StudioWorkspace::busy() const { return chat_->busy(); }
bool StudioWorkspace::beginPrompt(const QString& text, const QString& capability, const QImage& image) {
    return chat_->beginPrompt(text, capability, image);
}
void StudioWorkspace::report(const QString& text) { if (onError) onError(text); }
void StudioWorkspace::approve() {
    try {
        auto candidate = document_;
        auto frame = candidate.reviewImage;
        if (frame.isNull() && captureScene) frame = captureScene();
        if (frame.isNull()) throw std::invalid_argument("引用画面尚未就绪，请回工作台检查场景");
        StudioAsset asset{studioId(), "编排引用画面", {}, frame.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation)};
        for (const auto& entry : candidate.suggestions) {
            const auto shot = entry.toObject();
            candidate.timeline.add({studioId().toStdString(), asset.id.toStdString(),
                shot["title"].toString().toStdString(), 0, shot["duration_ms"].toInt()});
        }
        candidate.assets.insert(asset.id, std::move(asset)); candidate.reviewImage = {};
        candidate.pipeline.approve(); candidate.pipeline.assembled(); candidate.toJson();
        document_ = std::move(candidate);
        pipeline_->refresh(); timeline_->refresh(); assets_->refresh();
        chat_->showAssemblyResult();
        if (onChanged) onChanged();
        if (onAssembled) onAssembled();
    } catch (const std::exception& e) { report(QString::fromUtf8(e.what())); }
}
void StudioWorkspace::restoreState(const QJsonObject& value) {
    auto candidate = StudioDocument::fromJson(value);
    document_ = std::move(candidate); chat_->reset(); pipeline_->refresh(); timeline_->reset(); assets_->refresh();
}
void StudioWorkspace::refreshPreview() {
    if (previewFrame_.isNull()) { preview_->clear(); preview_->setText(document_.timeline.clips().empty() ? "时间线为空，导入素材或确认镜头方案。" : "正在加载素材画面…"); }
    else preview_->setPixmap(QPixmap::fromImage(previewFrame_).scaled(preview_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
bool StudioWorkspace::eventFilter(QObject* object, QEvent* event) {
    if (object == preview_ && event->type() == QEvent::Resize)
        QTimer::singleShot(0, this, [this] { refreshPreview(); });
    return QWidget::eventFilter(object, event);
}
} // namespace luma

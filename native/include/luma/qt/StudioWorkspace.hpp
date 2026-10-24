#pragma once
#include <luma/studio/StudioDocument.hpp>
#include <QWidget>
#include <functional>
class QLabel;
class QStackedWidget;
namespace luma {
class ChatPanel;
class PipelinePanel;
class TimelinePanel;
class StudioAssetsPanel;
class StudioWorkspace final : public QWidget {
  public:
    explicit StudioWorkspace(QWidget* parent = nullptr);
    ~StudioWorkspace() override;
    QStackedWidget* viewStack() const { return views_; }
    QWidget* timelineView() const { return timelineView_; }
    QWidget* reviewView() const;
    QWidget* assetsView() const;
    void setAssistantVisible(bool);
    void openSettings();
    bool busy() const;
    bool beginPrompt(const QString&, const QString&, const QImage&);
    const StudioDocument& document() const { return document_; }
    QJsonObject saveState() const { return document_.toJson(); }
    void restoreState(const QJsonObject&);
    std::function<QImage()> captureScene;
    std::function<void()> onChanged;
    std::function<void(const QString&)> onError;
    std::function<void()> onReviewReady, onAssembled, onLocateAsset;
  private:
    StudioDocument document_;
    ChatPanel* chat_;
    PipelinePanel* pipeline_;
    TimelinePanel* timeline_;
    StudioAssetsPanel* assets_;
    QStackedWidget* views_;
    QWidget* timelineView_;
    QLabel* preview_;
    QImage previewFrame_;
    void approve();
    void refreshPreview();
    void report(const QString&);
  protected:
    bool eventFilter(QObject*, QEvent*) override;
};
} // namespace luma

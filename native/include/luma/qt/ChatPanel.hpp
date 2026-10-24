#pragma once
#include <luma/studio/AIService.hpp>
#include <luma/studio/StudioDocument.hpp>
#include <QWidget>
#include <memory>
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QDialog;
namespace luma {
class ChatPanel final : public QWidget {
  public:
    explicit ChatPanel(StudioDocument&, QWidget* parent = nullptr);
    std::function<QImage()> captureScene;
    std::function<void(const QImage&)> onStarted;
    std::function<void(const QJsonObject&)> onRun;
    std::function<void(const QString&)> onError;
    std::function<void()> onChanged;
    void refresh();
    void reset();
    bool busy() const { return busy_; }
    bool beginPrompt(const QString&, const QString&, const QImage&);
    void openSettings();
    void showAssemblyResult();
  private:
    StudioDocument& document_;
    std::unique_ptr<AIService> service_;
    QComboBox *mode_, *capability_, *models_;
    QLineEdit *url_, *token_;
    QPlainTextEdit* input_;
    QTextBrowser* history_;
    QLabel *status_, *attachment_, *settingsStatus_;
    QPushButton *send_, *cancel_, *retry_, *connect_, *clearAttachment_;
    QDialog* settings_;
    QJsonArray availableModels_;
    QImage image_;
    QString lastText_, partial_;
    bool busy_ = false;
    int epoch_ = 0;
    void setStatus(const QString&);
    void configure();
    void buildSettings();
    void filterModels();
    void setBusy(bool);
    void setImage(const QImage&);
    void submit(bool retry = false);
    void failed(const QString&);
};
} // namespace luma

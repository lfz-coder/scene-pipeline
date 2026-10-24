#include <luma/qt/ChatPanel.hpp>
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QVBoxLayout>
namespace luma {
using namespace qtui;
ChatPanel::ChatPanel(StudioDocument& document, QWidget* parent) : QWidget(parent), document_(document) {
    setObjectName("chatPanel"); setMinimumWidth(300);
    auto* box = new QVBoxLayout(this); box->setContentsMargins(14, 16, 14, 14); box->setSpacing(10);
    box->addWidget(label("创作助手", this, "sectionTitle"));
    buildSettings();
    status_ = label("", this, "muted"); status_->setObjectName("chatStatus"); box->addWidget(status_);
    history_ = new QTextBrowser(this); history_->setObjectName("chatHistory");
    history_->setOpenExternalLinks(false); history_->setOpenLinks(false); box->addWidget(history_, 1);
    auto* modelRow = new QHBoxLayout;
    capability_ = new QComboBox(this); capability_->setObjectName("chatCapability");
    capability_->addItem("文本规划", "llm"); capability_->addItem("画面分析", "vlm");
    models_ = new QComboBox(this); models_->setObjectName("chatModel");
    models_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    models_->setMinimumContentsLength(8);
    modelRow->addWidget(capability_); modelRow->addWidget(models_, 1);
    box->addLayout(modelRow);
    attachment_ = label("", this, "muted"); attachment_->setObjectName("chatAttachment"); box->addWidget(attachment_);
    auto* attach = new QHBoxLayout;
    auto* scene = button("引用当前画面", this); scene->setObjectName("attachScene");
    auto* file = button("添加图片", this); file->setObjectName("attachImage");
    clearAttachment_ = button("移除", this); clearAttachment_->setObjectName("clearChatAttachment");
    attach->addWidget(scene); attach->addWidget(file); attach->addWidget(clearAttachment_); box->addLayout(attach);
    input_ = new QPlainTextEdit(this); input_->setObjectName("chatInput");
    input_->setPlaceholderText("描述镜头方案，或选择 VLM 分析附图…"); input_->setMaximumHeight(95); box->addWidget(input_);
    auto* actions = new QHBoxLayout;
    send_ = button("发送并编排", this, "primary"); send_->setObjectName("chatSend");
    cancel_ = button("取消任务", this); cancel_->setObjectName("chatCancel");
    retry_ = button("重试", this); retry_->setObjectName("chatRetry");
    actions->addWidget(send_, 1); actions->addWidget(cancel_); actions->addWidget(retry_); box->addLayout(actions);
    connect(send_, &QPushButton::clicked, this, [this] { submit(); });
    connect(retry_, &QPushButton::clicked, this, [this] { submit(true); });
    connect(cancel_, &QPushButton::clicked, this, [this] {
        if (service_) { setStatus("正在请求取消…"); cancel_->setEnabled(false); service_->cancel(); }
    });
    connect(scene, &QPushButton::clicked, this, [this] { if (captureScene) setImage(captureScene()); });
    connect(file, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, "添加分析图片", {}, "图片 (*.png *.jpg *.jpeg)");
        if (!path.isEmpty()) setImage(QImage(path));
    });
    connect(clearAttachment_, &QPushButton::clicked, this, [this] { image_ = {}; setImage({}); });
    connect(capability_, &QComboBox::currentIndexChanged, this, [this] { filterModels(); });
    setBusy(false); configure(); setImage({}); refresh();
}
void ChatPanel::configure() {
    ++epoch_; service_.reset(); availableModels_ = {}; filterModels();
    const bool remote = mode_->currentIndex() == 1;
    url_->setVisible(remote); token_->setVisible(remote); connect_->setVisible(remote);
    retry_->hide();
    try {
        if (remote && sender() != connect_) { setStatus("填写服务地址后连接，模型列表来自服务。"); return; }
        if (remote) service_ = std::make_unique<HttpAIService>(QUrl(url_->text().trimmed()), token_->text(), this);
        else service_ = std::make_unique<DemoAIService>(this);
        const int epoch = epoch_;
        setStatus(remote ? "正在读取模型列表…" : "本地演示：可体验流程，未接入真实模型。");
        service_->capabilities([this, epoch, remote](const QJsonObject& data) {
            if (epoch != epoch_) return;
            availableModels_ = data["models"].toArray(); filterModels();
            setStatus(remote ? "服务已连接 · 模型列表已通过契约校验" : "本地演示：可体验流程，未接入真实模型。");
        }, [this, epoch](const QString& error) { if (epoch == epoch_) failed(error); });
    } catch (const std::exception& e) { failed(QString::fromUtf8(e.what())); }
}
void ChatPanel::filterModels() {
    models_->clear();
    for (const auto& entry : availableModels_) {
        const auto model = entry.toObject();
        if (model["capability"].toString() == capability_->currentData().toString())
            models_->addItem(model["label"].toString(), model["id"].toString());
    }
    send_->setEnabled(!busy_ && models_->count());
}
void ChatPanel::setBusy(bool busy) {
    busy_ = busy;
    for (auto* field : {mode_, capability_, models_}) field->setEnabled(!busy);
    url_->setEnabled(!busy); token_->setEnabled(!busy); connect_->setEnabled(!busy);
    send_->setEnabled(!busy && models_->count()); cancel_->setEnabled(busy); input_->setEnabled(!busy);
    findChild<QPushButton*>("attachScene")->setEnabled(!busy);
    findChild<QPushButton*>("attachImage")->setEnabled(!busy);
    clearAttachment_->setEnabled(!busy); retry_->setVisible(false);
}
void ChatPanel::setImage(const QImage& value) {
    image_ = value.isNull() ? QImage{} : value.scaled(1280, 1280, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    attachment_->setText(image_.isNull() ? "未附图片" : QString("已附图片 · %1 × %2").arg(image_.width()).arg(image_.height()));
    clearAttachment_->setVisible(!image_.isNull());
}
void ChatPanel::refresh() {
    QString html;
    if (document_.messages.isEmpty())
        html = "<div style='color:#91949e'><h3>一起把想法变成画面</h3>描述主题与风格，我会整理镜头方案。<br><br>你也可以引用右侧 3D 场景，或附上一张参考图。<br><br>所有镜头建议都由你确认后加入时间线。</div>";
    for (const auto& entry : document_.messages) {
        const auto message = entry.toObject();
        html += "<p><b>" + QString(message["role"] == "user" ? "你" : "创作助手") + "</b><br>" +
            message["text"].toString().toHtmlEscaped().replace('\n', "<br>") + "</p>";
    }
    if (!partial_.isEmpty()) html += "<p>" + partial_.toHtmlEscaped() + "</p>";
    history_->setHtml(html); history_->moveCursor(QTextCursor::End);
}
void ChatPanel::reset() {
    if (busy_) {
        // Ignore callbacks from the conversation being replaced.
        ++epoch_;
        if (service_) service_->cancel();
        service_.reset();
    }
    partial_.clear(); lastText_.clear(); image_ = {}; input_->clear();
    setBusy(false);
    if (!service_) configure();
    else setStatus(mode_->currentIndex() == 1 ? "服务已连接 · 可以开始新对话" : "本地演示：可体验流程，未接入真实模型。");
    setImage({}); refresh();
}
} // namespace luma

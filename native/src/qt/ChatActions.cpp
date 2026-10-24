#include <luma/qt/ChatPanel.hpp>
#include <QBuffer>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
namespace luma {
void ChatPanel::showAssemblyResult() {
    setStatus("方案已确认 · 片段已加入时间线");
}
bool ChatPanel::beginPrompt(const QString& text, const QString& capability, const QImage& image) {
    if (busy_) return false;
    const int index = capability_->findData(capability);
    if (index < 0) return false;
    capability_->setCurrentIndex(index);
    setImage(image);
    input_->setPlainText(text);
    if (!service_) { setStatus("请先在模型服务中连接服务，再发送此输入。"); return false; }
    submit();
    return busy_;
}
void ChatPanel::submit(bool retry) {
    if (busy_ || !service_ || !models_->count()) return;
    const auto text = (retry ? lastText_ : input_->toPlainText()).trimmed();
    if (text.isEmpty() || text.toUcs4().size() > 4000) { setStatus("请输入 1–4000 个字符。"); return; }
    if (capability_->currentData() == "vlm" && image_.isNull()) {
        setStatus("VLM 需要附图：引用当前画面或添加图片。"); return;
    }
    QJsonArray images;
    if (!image_.isNull()) {
        QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); image_.save(&buffer, "JPEG", 85);
        images.append(QJsonObject{{"mime_type", "image/jpeg"}, {"data_base64", QString::fromLatin1(bytes.toBase64())}});
    }
    const auto messages = document_.requestMessages(text, retry);
    const QJsonObject body{{"project_id", document_.projectId}, {"conversation_id", document_.conversationId},
        {"capability", capability_->currentData().toString()}, {"model_id", models_->currentData().toString()},
        {"messages", messages}, {"images", images}};
    try { ApiContract{}.validate("ai.RunRequest", body); }
    catch (const std::exception& e) { setStatus(QString::fromUtf8(e.what())); return; }
    if (!retry) document_.messages.append(QJsonObject{{"role", "user"}, {"text", text}});
    while (document_.messages.size() > 98) document_.messages.removeFirst();
    lastText_ = text; input_->clear(); partial_.clear(); setBusy(true); refresh();
    if (onStarted) onStarted(image_.isNull() && captureScene ? captureScene() : image_);
    if (onChanged) onChanged();
    const int epoch = epoch_;
    service_->start(body, [this, epoch](const QJsonObject& run) {
        if (epoch != epoch_) return;
        partial_ = run["message"].toString();
        const auto status = run["status"].toString();
        setStatus(status == "queued" ? "任务已排队" : status == "running" ? "正在分析…" :
                         status == "succeeded" ? "镜头建议已就绪，等待人工确认" :
                         status == "cancelled" ? "任务已取消" : "任务失败：" + run["error"].toString());
        if (status == "succeeded") {
            if (!partial_.isEmpty()) document_.messages.append(QJsonObject{{"role", "assistant"}, {"text", partial_}});
            partial_.clear(); setBusy(false);
        } else if (status == "failed" || status == "cancelled") {
            setBusy(false); retry_->setVisible(status == "failed");
        }
        refresh(); if (onRun) onRun(run); if (onChanged) onChanged();
    }, [this, epoch](const QString& message) { if (epoch == epoch_) failed(message); });
}
void ChatPanel::failed(const QString& message) {
    partial_.clear(); setBusy(false); setStatus(message);
    retry_->setVisible(!lastText_.isEmpty() && models_->count() && service_);
    refresh(); if (onError) onError(message);
}
} // namespace luma

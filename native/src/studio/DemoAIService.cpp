#include <luma/studio/AIService.hpp>
#include <QJsonArray>
#include <QUuid>
namespace luma {
namespace { QString id() { return QUuid::createUuid().toString(QUuid::WithoutBraces); } }
void DemoAIService::capabilities(Update done, Error) {
    done({{"models", QJsonArray{
        QJsonObject{{"id", "demo-llm"}, {"label", "文本流程演示"}, {"capability", "llm"}},
        QJsonObject{{"id", "demo-vlm"}, {"label", "图像流程演示"}, {"capability", "vlm"}}}},
        {"poll_interval_ms", 500}});
}
void DemoAIService::publish(const QString& status, const QString& message) {
    run_["status"] = status; run_["message"] = message;
    run_["revision"] = run_["revision"].toInt() + 1;
    contract_.validate("ai.Run", run_);
    if (update_) update_(run_);
}
void DemoAIService::start(const QJsonObject& request, Update update, Error error) {
    try { contract_.validate("ai.RunRequest", request); }
    catch (const std::exception& e) { error(QString::fromUtf8(e.what())); return; }
    const auto epoch = ++epoch_;
    update_ = std::move(update);
    run_ = {{"id", id()}, {"revision", 0}, {"status", "queued"}, {"message", ""},
            {"shots", QJsonArray{}}, {"error", ""}};
    publish("queued", "本地演示：任务已进入编排，未调用真实模型。");
    QTimer::singleShot(150, this, [this, epoch] {
        if (epoch != epoch_) return;
        publish("running", "本地演示：正在构建镜头建议。");
    });
    QTimer::singleShot(450, this, [this, epoch, request] {
        if (epoch != epoch_) return;
        QJsonArray shots;
        for (const auto* title : {"建立场景", "主体展示", "收束画面"})
            shots.append(QJsonObject{{"id", id()}, {"title", title}, {"duration_ms", 3000}});
        run_["shots"] = shots;
        publish("succeeded", request["capability"] == "vlm"
            ? "本地演示已接收图片，生成 3 个示例镜头。这里未进行视觉模型推理；确认后使用当前画面组装时间线。"
            : "本地演示生成 3 个示例镜头。可调整标题与时长，再确认组装；真实 LLM 由 HTTP 服务提供。");
    });
}
void DemoAIService::cancel() {
    if (run_.isEmpty() || (run_["status"] != "queued" && run_["status"] != "running")) return;
    ++epoch_; publish("cancelled", "本次任务已取消。");
}
} // namespace luma

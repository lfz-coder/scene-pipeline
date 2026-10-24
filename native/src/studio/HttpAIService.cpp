#include <luma/studio/AIService.hpp>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUuid>
#include <stdexcept>
namespace luma {
HttpAIService::HttpAIService(QUrl base, QString token, QObject* parent, int totalTimeoutMs)
    : AIService(parent), base_(std::move(base)), token_(std::move(token)), totalTimeoutMs_(totalTimeoutMs) {
    if (!base_.isValid() || (base_.scheme() != "http" && base_.scheme() != "https") ||
        base_.host().isEmpty() || !base_.userInfo().isEmpty() || base_.hasQuery() ||
        base_.hasFragment() || (!base_.path().isEmpty() && base_.path() != "/"))
        throw std::invalid_argument("服务地址须为 HTTP(S) 根地址，不包含账号、路径、查询或片段");
    poll_.setSingleShot(true);
    connect(&poll_, &QTimer::timeout, this, [this] { fetch(); });
}
void HttpAIService::request(const QString& name, const QJsonObject& body, const QString& schema,
                             Update done, Error error, const QString& key) {
    auto operation = contract_.operation(name);
    operation.path.replace("{run_id}", runId_);
    QNetworkRequest request(base_.resolved(QUrl(operation.path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json, application/problem+json");
    if (!token_.isEmpty()) request.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
    if (!key.isEmpty()) request.setRawHeader("Idempotency-Key", key.toLatin1());
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto* reply = operation.method == "GET" ? network_.get(request)
        : network_.sendCustomRequest(request, operation.method, QJsonDocument(body).toJson(QJsonDocument::Compact));
    if (name != "getCapabilities") pending_ = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, name, schema, done, error] {
        const auto bytes = reply->readAll();
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto networkError = reply->errorString();
        const auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        reply->deleteLater();
        if (pending_ == reply) pending_ = nullptr;
        try {
            if (bytes.size() > 1024 * 1024) throw std::runtime_error("服务响应超过 1 MiB");
            QJsonParseError parse;
            const auto json = QJsonDocument::fromJson(bytes, &parse);
            if (parse.error != QJsonParseError::NoError || !json.isObject())
                throw std::runtime_error(("服务未返回 JSON：" + networkError).toStdString());
            const int expected = name == "createRun" ? 202 : 200;
            if (status != expected) {
                if (contentType.section(';', 0, 0).trimmed().toLower() != "application/problem+json")
                    throw std::runtime_error("服务错误类型不符合 OpenAPI");
                contract_.validate("common.Problem", json.object());
                if (json.object()["status"].toInt() != status)
                    throw std::runtime_error("Problem.status 与 HTTP 状态码不一致");
                error(QString::number(status) + " · " + json.object()["detail"].toString());
                return;
            }
            if (contentType.section(';', 0, 0).trimmed().toLower() != "application/json")
                throw std::runtime_error("服务响应类型不符合 OpenAPI");
            contract_.validate(schema, json.object()); done(json.object());
        } catch (const std::exception& e) { error(QString::fromUtf8(e.what())); }
    });
}
void HttpAIService::capabilities(Update done, Error error) {
    request("getCapabilities", {}, "ai.Capabilities", [this, done](const QJsonObject& value) {
        pollInterval_ = value["poll_interval_ms"].toInt(); done(value);
    }, std::move(error));
}
void HttpAIService::start(const QJsonObject& body, Update update, Error error) {
    if (active_) { error("已有请求正在执行，请先取消"); return; }
    try { contract_.validate("ai.RunRequest", body); }
    catch (const std::exception& e) { error(QString::fromUtf8(e.what())); return; }
    const int epoch = ++epoch_;
    update_ = std::move(update); error_ = std::move(error);
    active_ = true; cancelling_ = false; timedOut_ = false;
    revision_ = -1; conflicts_ = 0; runId_.clear(); snapshot_ = {};
    request("createRun", body, "ai.Run", [this, epoch](const QJsonObject& value) {
        if (epoch == epoch_) accept(value);
    }, [this, epoch](const QString& message) { if (epoch == epoch_) fail(message); },
       QUuid::createUuid().toString(QUuid::WithoutBraces));
    QTimer::singleShot(totalTimeoutMs_, this, [this, epoch] {
        // Keep the run alive until the outstanding request settles and cancellation is sent.
        if (epoch == epoch_ && active_) { timedOut_ = true; cancel(); }
    });
}
void HttpAIService::accept(const QJsonObject& run) {
    if (!active_) return;
    if (!runId_.isEmpty() && run["id"] != runId_) { fail("服务返回了不同的任务 ID"); return; }
    if (run["revision"].toInt() < revision_) { fail("任务版本发生回退"); return; }
    if (run["revision"].toInt() == revision_ && run != snapshot_) {
        fail("同一任务版本返回了不同快照"); return;
    }
    snapshot_ = run;
    runId_ = run["id"].toString(); revision_ = run["revision"].toInt();
    const auto status = run["status"].toString();
    const bool terminal = status == "succeeded" || status == "failed" || status == "cancelled";
    if (status == "cancelled" && timedOut_) { fail("任务等待超时，服务已确认取消"); return; }
    if (terminal) { active_ = false; poll_.stop(); }
    if (update_) update_(run);
    if (!terminal && active_) {
        if (cancelling_) sendCancel();
        else poll_.start(pollInterval_);
    }
}
void HttpAIService::fetch() {
    if (!active_ || pending_) return;
    const int epoch = epoch_;
    request("getRun", {}, "ai.Run", [this, epoch](const QJsonObject& run) { if (epoch == epoch_) accept(run); },
            [this, epoch](const QString& message) {
                if (epoch != epoch_) return;
                // A failed read does not imply the task or cancellation endpoint is unavailable.
                if (active_ && cancelling_ && !runId_.isEmpty()) sendCancel();
                else fail(message);
            });
}
void HttpAIService::cancel() {
    if (!active_) return;
    cancelling_ = true; poll_.stop();
    if (!runId_.isEmpty() && !pending_) sendCancel();
}
void HttpAIService::sendCancel() {
    if (!active_ || pending_) return;
    const int epoch = epoch_;
    const QJsonObject body{{"expected_revision", revision_}};
    contract_.validate("common.CancelRequest", body);
    request("cancelRun", body, "ai.Run", [this, epoch](const QJsonObject& run) { if (epoch == epoch_) accept(run); },
        [this, epoch](const QString& message) {
            if (epoch != epoch_) return;
            if (message.startsWith("409") && ++conflicts_ <= 3) fetch();
            else fail(message);
        });
}
void HttpAIService::fail(const QString& message) {
    if (!active_) return;
    active_ = false; ++epoch_; poll_.stop();
    if (error_) error_(message);
}
} // namespace luma

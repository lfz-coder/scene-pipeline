#include <iostream>
#include <luma/studio/AIService.hpp>
#include <luma/studio/ApiContract.hpp>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUuid>
using namespace luma;
static int failures = 0;
static void check(bool ok, const char* message) {
    std::cout << (ok ? "PASS " : "FAIL ") << message << '\n'; failures += !ok;
}
static bool waitFor(const std::function<bool()>& predicate) {
    QElapsedTimer elapsed; elapsed.start();
    while (!predicate() && elapsed.elapsed() < 5000) QTest::qWait(10);
    return predicate();
}
static QJsonObject request() {
    auto id = [] { return QUuid::createUuid().toString(QUuid::WithoutBraces); };
    return {{"project_id", id()}, {"conversation_id", id()}, {"model_id", "demo-llm"},
        {"capability", "llm"}, {"messages", QJsonArray{QJsonObject{{"role", "user"}, {"text", "三个镜头"}}}},
        {"images", QJsonArray{}}};
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ApiContract contract;
    DemoAIService demo;
    bool done = false;
    demo.start(request(), [&](const QJsonObject& run) {
        contract.validate("ai.Run", run); done = run["status"] == "succeeded";
    }, [&](const QString&) { ++failures; done = true; });
    check(waitFor([&] { return done; }), "local demo completes a contract-valid run");
    bool cancelled = false, lateSuccess = false;
    demo.start(request(), [&](const QJsonObject& run) {
        cancelled |= run["status"] == "cancelled";
        lateSuccess |= run["status"] == "succeeded";
    }, [&](const QString&) { ++failures; });
    demo.cancel(); QTest::qWait(700);
    check(cancelled && !lateSuccess, "cancel prevents delayed demo completion");
    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost), "HTTP fixture listens locally");
    const QString runId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool sawKey = false, sawAuthorization = false, sawCancel = false;
    int polls = 0;
    bool corruptRevision = false;
    bool delayedPoll = false, conflictCancel = false, terminalRace = false, badProblem = false;
    bool pollError = false;
    QObject::connect(&server, &QTcpServer::newConnection, [&] {
        auto* socket = server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket, buffer] {
            buffer->append(socket->readAll());
            const auto separator = buffer->indexOf("\r\n\r\n");
            if (separator < 0) return;
            const auto header = buffer->left(separator).toLower();
            qint64 length = 0;
            for (const auto& line : header.split('\n'))
                if (line.startsWith("content-length:")) length = line.mid(15).trimmed().toLongLong();
            if (buffer->size() < separator + 4 + length) return;
            const auto route = buffer->left(buffer->indexOf("\r\n"));
            QJsonObject body;
            int code = 200;
            bool delay = false;
            QByteArray type = "application/json";
            sawAuthorization |= header.contains("authorization: bearer test-token");
            if (route.startsWith("GET /v1/capabilities ")) {
                body = {{"models", QJsonArray{QJsonObject{{"id", "demo-llm"}, {"label", "测试模型"}, {"capability", "llm"}}}},
                        {"poll_interval_ms", 200}};
            } else {
                body = {{"id", runId}, {"revision", polls + 1}, {"status", "queued"},
                        {"message", ""}, {"shots", QJsonArray{}}, {"error", ""}};
                if (route.startsWith("POST /v1/runs ")) {
                    code = 202; polls = 0; body["revision"] = 1;
                    sawKey = header.contains("idempotency-key:");
                    contract.validate("ai.RunRequest", QJsonDocument::fromJson(buffer->mid(separator + 4)).object());
                    if (badProblem) {
                        code = 503; type = "application/problem+json";
                        body = {{"type", "https://example.org/unavailable"}, {"title", "不可用"},
                            {"status", 502}, {"detail", "故意不一致"}, {"request_id", runId}};
                    }
                } else if (route.startsWith("POST /v1/runs/" + runId.toUtf8() + "/cancel ")) {
                    sawCancel = true; body["status"] = "cancelled"; body["revision"] = polls + 2;
                    contract.validate("common.CancelRequest", QJsonDocument::fromJson(buffer->mid(separator + 4)).object());
                    if (conflictCancel || terminalRace) {
                        conflictCancel = false; code = 409; type = "application/problem+json";
                        body = {{"type", "https://example.org/conflict"}, {"title", "版本冲突"},
                            {"status", 409}, {"detail", "请读取最新版本"}, {"request_id", runId}};
                    }
                } else if (route.startsWith("GET /v1/runs/" + runId.toUtf8() + " ")) {
                    ++polls; body["revision"] = polls + 1;
                    body["status"] = polls >= 2 ? "succeeded" : "running";
                    body["message"] = "真实 HTTP 适配已验证";
                    if (corruptRevision) body["revision"] = 1;
                    if (terminalRace) body["status"] = "succeeded";
                    if (pollError) {
                        code = 503; type = "application/problem+json";
                        body = {{"type", "https://example.org/unavailable"}, {"title", "暂不可用"},
                            {"status", 503}, {"detail", "读取服务临时失败"}, {"request_id", runId}};
                    }
                    delay = delayedPoll;
                } else { ++failures; }
            }
            const auto payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
            const auto response = "HTTP/1.1 " + QByteArray::number(code) + " OK\r\nContent-Type: " + type +
                "\r\nContent-Length: " + QByteArray::number(payload.size()) + "\r\nConnection: close\r\n\r\n" + payload;
            QTimer::singleShot(delay ? 250 : 0, socket, [socket, response] {
                socket->write(response); socket->disconnectFromHost();
            });
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    HttpAIService http(QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort())), "test-token");
    bool capabilities = false;
    http.capabilities([&](const QJsonObject& value) { capabilities = value["models"].toArray().size() == 1; },
                      [&](const QString&) { ++failures; capabilities = true; });
    check(waitFor([&] { return capabilities; }), "HTTP capabilities uses the OpenAPI operation");
    done = false;
    http.start(request(), [&](const QJsonObject& run) { done = run["status"] == "succeeded"; },
               [&](const QString& error) { std::cerr << error.toStdString(); ++failures; done = true; });
    check(waitFor([&] { return done; }) && polls >= 2 && sawKey && sawAuthorization,
          "HTTP create and poll send valid DTO, idempotency and bearer headers");
    cancelled = false;
    http.start(request(), [&](const QJsonObject& run) { cancelled |= run["status"] == "cancelled"; },
               [&](const QString&) { ++failures; cancelled = true; });
    http.cancel();
    check(waitFor([&] { return cancelled; }) && sawCancel, "HTTP cancel waits for the run ID and uses revision");
    corruptRevision = true;
    bool revisionError = false;
    http.start(request(), [](const QJsonObject&) {},
               [&](const QString&) { revisionError = true; });
    check(waitFor([&] { return revisionError; }), "changed snapshot with unchanged revision is rejected");
    corruptRevision = false; delayedPoll = true; sawCancel = false;
    HttpAIService timeoutHttp(QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort())), "", nullptr, 300);
    timeoutHttp.capabilities([](const QJsonObject&) {}, [&](const QString&) { ++failures; });
    QTest::qWait(50);
    bool timeoutReported = false;
    timeoutHttp.start(request(), [](const QJsonObject&) {},
        [&](const QString& error) { timeoutReported = error.contains("超时"); });
    check(waitFor([&] { return timeoutReported; }) && sawCancel && polls >= 1,
          "deadline during outstanding poll still sends backend cancellation");
    pollError = true; timeoutReported = false; sawCancel = false;
    timeoutHttp.start(request(), [](const QJsonObject&) {},
        [&](const QString& error) { timeoutReported = error.contains("超时"); });
    check(waitFor([&] { return timeoutReported; }) && sawCancel,
          "deadline plus failed GET still attempts cancellation on the backend");
    pollError = false; terminalRace = true; done = false;
    timeoutHttp.start(request(), [&](const QJsonObject& run) { done = run["status"] == "succeeded"; },
        [&](const QString&) { ++failures; });
    check(waitFor([&] { return done; }), "successful terminal response racing deadline is preserved");
    terminalRace = false;
    delayedPoll = false; conflictCancel = true; cancelled = false;
    http.start(request(), [&](const QJsonObject& run) { cancelled |= run["status"] == "cancelled"; },
               [&](const QString&) { ++failures; });
    http.cancel();
    check(waitFor([&] { return cancelled; }) && polls == 1, "409 cancellation refetches revision before retry");
    terminalRace = true; done = false;
    http.start(request(), [&](const QJsonObject& run) { done = run["status"] == "succeeded"; },
               [&](const QString&) { ++failures; });
    http.cancel();
    check(waitFor([&] { return done; }), "completion racing cancellation preserves successful terminal state");
    terminalRace = false; badProblem = true; bool mismatchRejected = false;
    http.start(request(), [](const QJsonObject&) {},
               [&](const QString& error) { mismatchRejected = error.contains("状态码"); });
    check(waitFor([&] { return mismatchRejected; }), "Problem status must match HTTP status");
    return failures ? 1 : 0;
}

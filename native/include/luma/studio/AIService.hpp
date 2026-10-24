#pragma once
#include <luma/studio/ApiContract.hpp>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <functional>
class QNetworkReply;
namespace luma {
class AIService : public QObject {
  public:
    using Update = std::function<void(const QJsonObject&)>;
    using Error = std::function<void(const QString&)>;
    using QObject::QObject;
    virtual void capabilities(Update, Error) = 0;
    virtual void start(const QJsonObject&, Update, Error) = 0;
    virtual void cancel() = 0;
};
class DemoAIService final : public AIService {
  public:
    using AIService::AIService;
    void capabilities(Update, Error) override;
    void start(const QJsonObject&, Update, Error) override;
    void cancel() override;
  private:
    ApiContract contract_;
    QJsonObject run_;
    Update update_;
    int epoch_ = 0;
    void publish(const QString& status, const QString& message);
};
class HttpAIService final : public AIService {
  public:
    HttpAIService(QUrl base, QString token, QObject* parent = nullptr, int totalTimeoutMs = 180000);
    void capabilities(Update, Error) override;
    void start(const QJsonObject&, Update, Error) override;
    void cancel() override;
  private:
    ApiContract contract_;
    QNetworkAccessManager network_;
    QUrl base_;
    QString token_, runId_;
    QJsonObject snapshot_;
    QTimer poll_;
    Update update_;
    Error error_;
    int epoch_ = 0, revision_ = -1, conflicts_ = 0, pollInterval_ = 500;
    int totalTimeoutMs_;
    bool active_ = false, cancelling_ = false, timedOut_ = false;
    QPointer<QNetworkReply> pending_;
    void request(const QString& operation, const QJsonObject& body, const QString& schema,
                 Update done, Error error, const QString& key = {});
    void accept(const QJsonObject&);
    void fetch();
    void sendCancel();
    void fail(const QString&);
};
} // namespace luma

#pragma once
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QString>
namespace luma {
struct ApiOperation { QByteArray method; QString path; };
// Validates the controlled schema vocabulary used by this application's contract.
// Unknown keywords are rejected rather than silently treated as supported.
class ApiContract {
  public:
    ApiContract();
    ApiOperation operation(const QString& operationId) const;
    void validate(const QString& name, const QJsonValue& value) const;
  private:
    QJsonObject spec_;
    QMap<QString, QJsonObject> schemas_;
    QJsonObject resolve(const QString& reference) const;
    void check(const QJsonObject& schema, const QJsonValue& value, const QString& path) const;
};
} // namespace luma

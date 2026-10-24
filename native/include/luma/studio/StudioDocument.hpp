#pragma once
#include <luma/studio/TimelineModel.hpp>
#include <luma/studio/PipelineState.hpp>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
namespace luma {
QString studioId();
struct StudioAsset {
    QString id, title, path;
    QImage image;
    qint64 durationMs = 60000;
    bool video = false;
};
struct StudioDocument {
    QString projectId = studioId(), conversationId = studioId();
    QJsonArray messages, suggestions;
    QImage reviewImage;
    QMap<QString, StudioAsset> assets;
    TimelineModel timeline;
    PipelineState pipeline;
    QJsonObject toJson() const;
    QJsonArray requestMessages(const QString& text, bool retry) const;
    static StudioDocument fromJson(const QJsonObject&);
    QJsonObject timelineJson() const;
    QJsonObject pipelineJson() const;
    void addImage(const QImage&, const QString& title, qint64 duration = 3000);
};
} // namespace luma

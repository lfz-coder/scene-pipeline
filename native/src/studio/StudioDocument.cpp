#include <luma/studio/StudioDocument.hpp>
#include <luma/studio/ApiContract.hpp>
#include <QBuffer>
#include <QUuid>
#include <algorithm>
#include <stdexcept>
namespace luma {
QString studioId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
QJsonArray StudioDocument::requestMessages(const QString& text, bool retry) const {
    auto context = messages;
    if (!retry) context.append(QJsonObject{{"role", "user"}, {"text", text}});
    while (context.size() > 40) context.removeFirst();
    for (int i = 0; i < context.size(); ++i) {
        auto message = context[i].toObject();
        const auto points = message["text"].toString().toStdU32String();
        message["text"] = QString::fromUcs4(points.data(), std::min<std::size_t>(points.size(), 4000));
        context[i] = message;
    }
    return context;
}
QJsonObject StudioDocument::timelineJson() const {
    QJsonArray clips;
    for (const auto& clip : timeline.clips()) clips.append(QJsonObject{
        {"id", QString::fromStdString(clip.id)}, {"asset_id", QString::fromStdString(clip.assetId)},
        {"title", QString::fromStdString(clip.title)}, {"in_ms", clip.inMs}, {"out_ms", clip.outMs}});
    return {{"version", 1}, {"clips", clips}};
}
QJsonObject StudioDocument::pipelineJson() const {
    const QStringList ids{"prepare", "analyze", "review", "assemble"};
    const QStringList states{"pending", "running", "waiting", "done", "failed", "cancelled"};
    QJsonArray nodes;
    for (int i = 0; i < 4; ++i) nodes.append(QJsonObject{{"id", ids[i]},
        {"status", states[int(pipeline.nodes()[i])]},
        {"depends_on", i ? QJsonArray{ids[i - 1]} : QJsonArray{}}});
    return {{"version", 1}, {"nodes", nodes}, {"shots", suggestions}};
}
QJsonObject StudioDocument::toJson() const {
    QByteArray reviewPng;
    if (!reviewImage.isNull()) {
        QBuffer buffer(&reviewPng); buffer.open(QIODevice::WriteOnly); reviewImage.save(&buffer, "PNG");
    }
    QJsonArray media;
    for (const auto& asset : assets) {
        QByteArray png;
        if (!asset.image.isNull() && asset.path.isEmpty()) {
            QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); asset.image.save(&buffer, "PNG");
        }
        media.append(QJsonObject{{"id", asset.id}, {"title", asset.title},
            {"kind", asset.video ? "video" : "image"}, {"path", asset.path},
            {"image_base64", QString::fromLatin1(png.toBase64())}, {"duration_ms", asset.durationMs}});
    }
    QJsonObject value{{"version", 1}, {"project_id", projectId}, {"conversation_id", conversationId},
        {"messages", messages}, {"assets", media}, {"timeline", timelineJson()}, {"pipeline", pipelineJson()},
        {"review_image_base64", QString::fromLatin1(reviewPng.toBase64())}};
    ApiContract{}.validate("studio.Document", value);
    return value;
}
StudioDocument StudioDocument::fromJson(const QJsonObject& value) {
    if (value.isEmpty()) return {};
    ApiContract{}.validate("studio.Document", value);
    StudioDocument doc;
    doc.projectId = value["project_id"].toString(); doc.conversationId = value["conversation_id"].toString();
    doc.messages = value["messages"].toArray();
    const auto review = value["review_image_base64"].toString().toLatin1();
    if (!review.isEmpty()) {
        doc.reviewImage = QImage::fromData(QByteArray::fromBase64(review));
        if (doc.reviewImage.isNull() || doc.reviewImage.width() > 4096 || doc.reviewImage.height() > 4096)
            throw std::invalid_argument("编排引用画面无法解码或尺寸超限");
    }
    for (const auto& entry : value["assets"].toArray()) {
        const auto data = entry.toObject();
        StudioAsset asset;
        asset.id = data["id"].toString(); asset.title = data["title"].toString();
        asset.path = data["path"].toString(); asset.video = data["kind"] == "video";
        asset.durationMs = data["duration_ms"].toInteger();
        if (doc.assets.contains(asset.id)) throw std::invalid_argument("素材 ID 重复");
        const auto encoded = data["image_base64"].toString().toLatin1();
        if (!encoded.isEmpty()) {
            asset.image = QImage::fromData(QByteArray::fromBase64(encoded));
            if (asset.image.isNull() || asset.image.width() > 4096 || asset.image.height() > 4096)
                throw std::invalid_argument("工程快照无法解码或尺寸超限");
        } else if (asset.path.isEmpty()) throw std::invalid_argument("素材缺少路径或图像");
        if (!asset.video && !asset.path.isEmpty()) asset.image.load(asset.path);
        doc.assets.insert(asset.id, asset);
    }
    for (const auto& entry : value["timeline"].toObject()["clips"].toArray()) {
        const auto clip = entry.toObject();
        const auto assetId = clip["asset_id"].toString();
        if (!doc.assets.contains(assetId) || clip["out_ms"].toInteger() > doc.assets[assetId].durationMs)
            throw std::invalid_argument("片段引用了缺失素材或超出素材时长");
        doc.timeline.add({clip["id"].toString().toStdString(), assetId.toStdString(),
            clip["title"].toString().toStdString(), clip["in_ms"].toInteger(), clip["out_ms"].toInteger()});
    }
    const auto plan = value["pipeline"].toObject(); doc.suggestions = plan["shots"].toArray();
    const auto nodes = plan["nodes"].toArray();
    const QStringList ids{"prepare", "analyze", "review", "assemble"};
    for (int i = 0; i < 4; ++i)
        if (nodes[i].toObject()["id"] != ids[i] || nodes[i].toObject()["depends_on"].toArray() !=
            (i ? QJsonArray{ids[i - 1]} : QJsonArray{})) throw std::invalid_argument("编排依赖不正确");
    const QStringList states{"pending", "running", "waiting", "done", "failed", "cancelled"};
    std::array<PipelineNodeState, 4> restored;
    for (int i = 0; i < 4; ++i)
        restored[i] = PipelineNodeState(states.indexOf(nodes[i].toObject()["status"].toString()));
    doc.pipeline.restore(restored);
    return doc;
}
void StudioDocument::addImage(const QImage& image, const QString& title, qint64 duration) {
    if (image.isNull()) throw std::invalid_argument("当前画面尚未就绪");
    StudioAsset asset{studioId(), title, {}, image.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation)};
    timeline.add({studioId().toStdString(), asset.id.toStdString(), title.toStdString(), 0, duration});
    assets.insert(asset.id, std::move(asset));
}
} // namespace luma

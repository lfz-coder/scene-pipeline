#include <luma/qt/TimelinePanel.hpp>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <memory>
namespace luma {
void TimelinePanel::appendAsset(StudioAsset asset) {
    const auto duration = asset.video ? std::min<qint64>(asset.durationMs, 10000) : 3000;
    document_.timeline.add({studioId().toStdString(), asset.id.toStdString(), asset.title.toStdString(), 0, duration});
    document_.assets.insert(asset.id, std::move(asset));
}
void TimelinePanel::importPath(const QString& path) {
    const QFileInfo file(path);
    if (!file.exists() || !file.isFile()) { if (onError) onError("素材文件不存在：" + path); return; }
    StudioAsset asset; asset.id = studioId(); asset.title = file.completeBaseName().left(120);
    if (asset.title.isEmpty()) asset.title = "未命名素材";
    asset.path = file.absoluteFilePath();
    const auto suffix = file.suffix().toLower();
    if (QStringList{"png", "jpg", "jpeg"}.contains(suffix)) {
        asset.image.load(path);
        if (asset.image.isNull()) { if (onError) onError("无法解码图片：" + path); return; }
        asset.image = asset.image.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        edit([&] { appendAsset(asset); }); return;
    }
    asset.video = true;
    auto* probe = new QMediaPlayer(this);
    auto completed = std::make_shared<bool>(false);
    const int epoch = epoch_;
    auto accept = [this, probe, completed, asset, epoch]() mutable {
        if (*completed || epoch != epoch_ || probe->duration() <= 0 || !probe->hasVideo()) return;
        *completed = true; asset.durationMs = probe->duration();
        if (asset.durationMs < 100 || asset.durationMs > 3600000) {
            if (onError) onError("视频时长须为 0.1 秒到一小时");
        } else edit([&] { appendAsset(asset); });
        probe->deleteLater();
    };
    connect(probe, &QMediaPlayer::durationChanged, this, [accept](qint64) mutable { accept(); });
    connect(probe, &QMediaPlayer::hasVideoChanged, this, [accept](bool) mutable { accept(); });
    connect(probe, &QMediaPlayer::mediaStatusChanged, this, [accept](QMediaPlayer::MediaStatus) mutable { accept(); });
    connect(probe, &QMediaPlayer::errorOccurred, this, [this, probe, completed, epoch](QMediaPlayer::Error, const QString& text) {
        if (!*completed && epoch == epoch_ && onError) onError("视频导入失败：" + text);
        *completed = true; probe->deleteLater();
    });
    QTimer::singleShot(10000, probe, [this, probe, completed, epoch] {
        if (!*completed && epoch == epoch_ && onError) onError("视频导入超时，文件可能没有可解码的视频轨道");
        *completed = true; probe->deleteLater();
    });
    probe->setSource(QUrl::fromLocalFile(asset.path));
}
} // namespace luma

#include <luma/qt/TimelinePlayer.hpp>
#include <QFileInfo>
#include <QVideoFrame>
#include <QUrl>
#include <algorithm>
#include <cmath>
namespace luma {
TimelinePlayer::TimelinePlayer(StudioDocument& doc, QObject* parent) : QObject(parent), document_(doc) {
    video_.setVideoSink(&sink_); video_.setAudioOutput(&audio_); audio_.setVolume(.6);
    tick_.setInterval(33);
    connect(&tick_, &QTimer::timeout, this, [this] {
        position_ = std::min<qint64>(origin_ + clock_.elapsed(), document_.timeline.duration());
        if (position_ >= document_.timeline.duration()) pause();
        showAt(false); if (onPosition) onPosition(position_);
    });
    connect(&sink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
        if (!document_.assets.contains(assetId_) || !document_.assets[assetId_].video) return;
        const auto image = frame.toImage(); if (!image.isNull() && onFrame) onFrame(image);
    });
    connect(&video_, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia && !document_.timeline.clips().empty()) {
            const auto position = document_.timeline.locate(position_); video_.setPosition(position.sourceMs);
            if (playing_) video_.play();
            else video_.pause(); // Activate the video output for a paused first-frame preview.
        }
    });
    connect(&video_, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString& message) {
        failed("视频播放失败：" + message);
    });
}
void TimelinePlayer::failed(const QString& text) { pause(); if (onError) onError(text); }
void TimelinePlayer::play() {
    if (document_.timeline.clips().empty()) return;
    if (position_ >= document_.timeline.duration()) position_ = 0;
    origin_ = position_; clock_.restart(); playing_ = true; tick_.start(); showAt(true);
    if (onPlaying) onPlaying(playing_);
}
void TimelinePlayer::pause() {
    playing_ = false; tick_.stop(); video_.pause(); if (onPlaying) onPlaying(false);
}
void TimelinePlayer::seek(qint64 position) {
    position_ = std::clamp<qint64>(position, 0, document_.timeline.duration());
    origin_ = position_; clock_.restart(); showAt(true); if (onPosition) onPosition(position_);
}
void TimelinePlayer::refresh() { pause(); assetId_.clear(); clipId_.clear(); video_.setSource({}); seek(position_); }
void TimelinePlayer::showAt(bool seekVideo) {
    if (document_.timeline.clips().empty()) { if (onFrame) onFrame({}); return; }
    const auto position = document_.timeline.locate(position_);
    const auto& clip = document_.timeline.clips()[position.index];
    const auto id = QString::fromStdString(clip.assetId);
    if (!document_.assets.contains(id)) { failed("片段引用的素材不存在"); return; }
    const auto& asset = document_.assets[id];
    const bool changed = assetId_ != id;
    const auto clipId = QString::fromStdString(clip.id);
    const bool cut = clipId_ != clipId;
    clipId_ = clipId;
    if (changed) { video_.pause(); assetId_ = id; }
    if (asset.video) {
        if (!QFileInfo::exists(asset.path)) { failed("视频文件已移动或删除：" + asset.path); return; }
        if (changed) video_.setSource(QUrl::fromLocalFile(asset.path));
        if (seekVideo || changed || cut || std::abs(video_.position() - position.sourceMs) > 250)
            video_.setPosition(position.sourceMs);
        if (playing_) video_.play();
    } else {
        if (asset.image.isNull()) { failed("图片文件无法读取：" + asset.path); return; }
        if ((changed || seekVideo) && onFrame) onFrame(asset.image);
    }
}
} // namespace luma

#pragma once
#include <luma/studio/StudioDocument.hpp>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QTimer>
#include <functional>
namespace luma {
class TimelinePlayer final : public QObject {
  public:
    TimelinePlayer(StudioDocument&, QObject* parent = nullptr);
    std::function<void(qint64)> onPosition;
    std::function<void(const QImage&)> onFrame;
    std::function<void(const QString&)> onError;
    std::function<void(bool)> onPlaying;
    void play();
    void pause();
    void seek(qint64 position);
    void refresh();
    bool playing() const { return playing_; }
    qint64 position() const { return position_; }
    qint64 videoSourcePosition() const { return video_.position(); }
  private:
    StudioDocument& document_;
    QMediaPlayer video_;
    QAudioOutput audio_;
    QVideoSink sink_;
    QTimer tick_;
    QElapsedTimer clock_;
    qint64 position_ = 0, origin_ = 0;
    QString assetId_, clipId_;
    bool playing_ = false;
    void showAt(bool seekVideo);
    void failed(const QString&);
};
} // namespace luma

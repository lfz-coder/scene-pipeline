#pragma once
#include <luma/qt/TimelinePlayer.hpp>
#include <QWidget>
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QPushButton;
class QSlider;
namespace luma {
class TimelinePanel final : public QWidget {
  public:
    TimelinePanel(StudioDocument&, QWidget* parent = nullptr);
    std::function<QImage()> captureScene;
    std::function<void(const QImage&)> onFrame;
    std::function<void()> onChanged;
    std::function<void(const QString&)> onError;
    void importPath(const QString& path);
    bool locateAsset(const QString& id);
    void refresh();
    void reset();
  private:
    StudioDocument& document_;
    TimelinePlayer player_;
    QListWidget* clips_;
    QSlider* position_;
    QDoubleSpinBox *in_, *out_;
    QLabel* clock_;
    QPushButton *play_, *trim_, *split_, *left_, *right_, *remove_;
    int epoch_ = 0;
    int selected() const;
    qint64 clipStart(int row) const;
    void selectionChanged();
    void updatePosition(qint64);
    void edit(const std::function<void()>&);
    void appendAsset(StudioAsset);
};
} // namespace luma

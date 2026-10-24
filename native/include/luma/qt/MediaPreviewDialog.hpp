#pragma once
#include <QDialog>
#include <QImage>
#include <vector>
class QLabel;
class QPushButton;
namespace luma {
struct MediaEntry {
    int id = 0;
    QString title, group, detail, path;
    QImage image;
    bool success = true;
};
class MediaPreviewDialog final : public QDialog {
  public:
    MediaPreviewDialog(std::vector<MediaEntry> entries, int index, QWidget* parent);
  protected:
    void resizeEvent(QResizeEvent*) override;
  private:
    std::vector<MediaEntry> entries_;
    int index_;
    QLabel *image_, *detail_, *counter_, *title_;
    QPushButton *previous_, *next_;
    void present();
};
} // namespace luma

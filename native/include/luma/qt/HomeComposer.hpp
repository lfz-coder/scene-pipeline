#pragma once
#include <QImage>
#include <QWidget>
#include <functional>
class QComboBox;
class QLabel;
class QPlainTextEdit;
namespace luma {
class HomeComposer final : public QWidget {
  public:
    explicit HomeComposer(QWidget* parent = nullptr);
    QString presetKey() const;
    void clear();
    std::function<void(const QString&, const QString&, const QImage&)> onSubmit;
  private:
    QPlainTextEdit* prompt_;
    QComboBox *capability_, *preset_;
    QLabel* attachment_;
    QImage image_;
};
} // namespace luma

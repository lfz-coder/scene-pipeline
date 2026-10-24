#pragma once
#include <luma/studio/StudioDocument.hpp>
#include <QWidget>
#include <functional>
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
namespace luma {
class StudioAssetsPanel final : public QWidget {
  public:
    StudioAssetsPanel(const StudioDocument&, QWidget* parent = nullptr);
    void refresh();
    std::function<void(const QString&)> onLocate;
  private:
    const StudioDocument& document_;
    QLineEdit* search_;
    QComboBox* type_;
    QListWidget* items_;
    QLabel* count_;
    QPushButton* locate_;
    void filter();
    void locate();
};
} // namespace luma

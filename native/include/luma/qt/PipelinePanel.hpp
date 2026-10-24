#pragma once
#include <luma/studio/StudioDocument.hpp>
#include <QWidget>
#include <functional>
class QLabel;
class QPushButton;
class QTableWidget;
namespace luma {
class PipelinePanel final : public QWidget {
  public:
    PipelinePanel(StudioDocument&, QWidget* parent = nullptr);
    std::function<void()> onApprove, onChanged;
    std::function<void(const QString&)> onError;
    void refresh();
  private:
    StudioDocument& document_;
    std::array<QLabel*, 4> nodes_;
    QTableWidget* shots_;
    QPushButton *approve_, *discard_;
    void edit(int row, int column);
};
} // namespace luma

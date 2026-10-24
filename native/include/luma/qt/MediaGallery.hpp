#pragma once
#include <QWidget>
#include <functional>
#include <map>
#include <luma/qt/MediaPreviewDialog.hpp>
class QListWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
namespace luma {
// Qt-only media collection. It has no knowledge of cameras, GPU resources or projects.
class MediaGallery final : public QWidget {
  public:
    MediaGallery(const QString& prefix, bool snapshots, QWidget* parent = nullptr);
    void addEntry(MediaEntry entry);
    void clear();
    QListWidget* list() const { return list_; }
    const MediaEntry* selected() const;
    std::function<void(int)> onRestore;
    std::function<void(int, const QString&)> onRename;
    std::function<void(int)> onRemove;
    std::function<void(const QString&, bool)> onMessage;
  private:
    QString prefix_;
    bool snapshots_;
    std::map<int, MediaEntry> entries_;
    QListWidget* list_;
    QLineEdit* search_;
    QComboBox *group_, *status_ = nullptr;
    QLabel *count_, *empty_;
    QPushButton *preview_, *primary_, *rename_ = nullptr, *remove_, *folder_ = nullptr;
    void filter();
    void refreshGroups();
    void refreshActions();
    void preview();
    void rename();
    void remove();
    void open(bool directory);
    void contextMenu(const QPoint& position);
};
} // namespace luma

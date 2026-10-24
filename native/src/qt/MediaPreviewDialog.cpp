#include <luma/qt/MediaPreviewDialog.hpp>
#include "QtUiHelpers.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
namespace luma {
MediaPreviewDialog::MediaPreviewDialog(std::vector<MediaEntry> entries, int index, QWidget* parent)
    : QDialog(parent), entries_(std::move(entries)), index_(index) {
    setObjectName("mediaPreview");
    setWindowTitle("画面预览");
    resize(1040, 720);
    setMinimumSize(720, 480);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 18, 20, 18);
    auto* top = new QHBoxLayout;
    title_ = qtui::label("", this, "sectionTitle");
    title_->setTextFormat(Qt::PlainText);
    top->addWidget(title_, 1);
    auto* info = qtui::button("隐藏信息", this);
    info->setObjectName("previewInfo");
    info->setCheckable(true);
    info->setChecked(true);
    auto* close = qtui::button("关闭  Esc", this);
    close->setObjectName("previewClose");
    top->addWidget(info);
    top->addWidget(close);
    layout->addLayout(top);
    auto* body = new QHBoxLayout;
    image_ = new QLabel(this);
    image_->setObjectName("previewImage");
    image_->setAlignment(Qt::AlignCenter);
    image_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    image_->setMinimumSize(320, 240);
    image_->setStyleSheet("background:#0c0d0f;border-radius:10px;");
    body->addWidget(image_, 1);
    detail_ = qtui::label("", this, "muted");
    detail_->setObjectName("previewDetails");
    detail_->setTextFormat(Qt::PlainText);
    detail_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detail_->setAlignment(Qt::AlignTop);
    detail_->setFixedWidth(220);
    body->addWidget(detail_);
    layout->addLayout(body, 1);
    auto* navigation = new QHBoxLayout;
    previous_ = qtui::button("← 上一张", this);
    previous_->setObjectName("previewPrevious");
    next_ = qtui::button("下一张 →", this);
    next_->setObjectName("previewNext");
    counter_ = qtui::label("", this, "muted");
    counter_->setObjectName("previewCounter");
    navigation->addWidget(previous_);
    navigation->addStretch();
    navigation->addWidget(counter_);
    navigation->addStretch();
    navigation->addWidget(next_);
    layout->addLayout(navigation);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(info, &QPushButton::toggled, this, [this, info](bool shown) {
        info->setText(shown ? "隐藏信息" : "显示信息");
        info->setAccessibleName(info->text());
        detail_->setVisible(shown);
        QTimer::singleShot(0, this, [this] { present(); });
    });
    auto move = [this](int delta) {
        const int candidate = index_ + delta;
        if (candidate >= 0 && candidate < int(entries_.size())) { index_ = candidate; present(); }
    };
    connect(previous_, &QPushButton::clicked, this, [move] { move(-1); });
    connect(next_, &QPushButton::clicked, this, [move] { move(1); });
    auto* left = new QShortcut(QKeySequence(Qt::Key_Left), this);
    auto* right = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(left, &QShortcut::activated, this, [move] { move(-1); });
    connect(right, &QShortcut::activated, this, [move] { move(1); });
    present();
}
void MediaPreviewDialog::present() {
    if (entries_.empty()) return;
    const auto& entry = entries_.at(index_);
    title_->setText(entry.title);
    image_->setPixmap(QPixmap::fromImage(entry.image).scaled(image_->size(), Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation));
    detail_->setText(QString("%1\n\n%2\n\n像素尺寸\n%3 × %4\n\n%5")
                         .arg(entry.group, entry.detail)
                         .arg(entry.image.width()).arg(entry.image.height()).arg(entry.path));
    counter_->setText(QString("%1 / %2").arg(index_ + 1).arg(entries_.size()));
    previous_->setEnabled(index_ > 0);
    next_->setEnabled(index_ + 1 < int(entries_.size()));
}
void MediaPreviewDialog::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    present();
}
} // namespace luma

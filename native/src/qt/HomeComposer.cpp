#include <luma/qt/HomeComposer.hpp>
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QVBoxLayout>
namespace luma {
HomeComposer::HomeComposer(QWidget* parent) : QWidget(parent) {
    setObjectName("homeComposer");
    setMinimumWidth(680);
    setMaximumWidth(820);
    auto* box = new QVBoxLayout(this);
    box->setContentsMargins(18, 18, 18, 16);
    box->setSpacing(12);
    prompt_ = new QPlainTextEdit(this);
    prompt_->setObjectName("homePrompt");
    prompt_->setPlaceholderText("描述你想创作的画面…\n例如：为机器人设计一组二次元风格的展示镜头");
    prompt_->setFixedHeight(100);
    box->addWidget(prompt_);
    attachment_ = qtui::label("", this, "muted");
    attachment_->setObjectName("homeAttachment");
    attachment_->hide();
    box->addWidget(attachment_);
    auto* bar = new QHBoxLayout;
    capability_ = new QComboBox(this);
    capability_->setObjectName("homeCapability");
    capability_->addItem("文本规划", "llm");
    capability_->addItem("画面分析", "vlm");
    bar->addWidget(capability_);
    auto* attach = qtui::button("＋ 参考图", this);
    attach->setObjectName("homeAttach");
    bar->addWidget(attach);
    auto* remove = qtui::button("移除", this, "link");
    remove->setObjectName("homeRemoveAttachment");
    remove->hide();
    bar->addWidget(remove);
    bar->addStretch();
    preset_ = new QComboBox(this);
    preset_->setObjectName("homePreset");
    preset_->addItem("光照实验室", "lighting");
    preset_->addItem("机器人展台", "robot");
    preset_->addItem("纹理工作室", "texture");
    preset_->setToolTip("新工程的起始场景");
    bar->addWidget(preset_);
    auto* send = qtui::button("开始创作  ↑", this, "primary");
    send->setObjectName("homeSubmit");
    bar->addWidget(send);
    box->addLayout(bar);
    connect(attach, &QPushButton::clicked, this, [this, remove] {
        const auto path = QFileDialog::getOpenFileName(this, "添加参考图", {}, "图片 (*.png *.jpg *.jpeg)");
        if (path.isEmpty()) return;
        const auto image = QImage(path);
        if (image.isNull()) { attachment_->setText("无法读取图片，请选择 PNG 或 JPEG。"); attachment_->show(); return; }
        image_ = image.scaled(1280, 1280, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        attachment_->setText(QString("已附参考图 · %1 × %2").arg(image_.width()).arg(image_.height()));
        attachment_->show(); remove->show(); capability_->setCurrentIndex(1);
    });
    connect(remove, &QPushButton::clicked, this, [this, remove] {
        image_ = {}; attachment_->hide(); remove->hide();
    });
    connect(send, &QPushButton::clicked, this, [this] {
        const auto text = prompt_->toPlainText().trimmed();
        if (text.isEmpty() || text.toUcs4().size() > 4000) {
            attachment_->setText("请输入 1–4000 个字符。"); attachment_->show(); return;
        }
        if (capability_->currentData() == "vlm" && image_.isNull()) {
            attachment_->setText("画面分析需要添加一张参考图。"); attachment_->show(); return;
        }
        if (onSubmit) onSubmit(text, capability_->currentData().toString(), image_);
    });
}
QString HomeComposer::presetKey() const { return preset_->currentData().toString(); }
void HomeComposer::clear() {
    prompt_->clear(); image_ = {}; attachment_->clear(); attachment_->hide();
    findChild<QPushButton*>("homeRemoveAttachment")->hide();
}
} // namespace luma

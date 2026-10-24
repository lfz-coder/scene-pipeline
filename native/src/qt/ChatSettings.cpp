#include <luma/qt/ChatPanel.hpp>
#include "QtUiHelpers.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QVBoxLayout>
namespace luma {
void ChatPanel::buildSettings() {
    settings_ = new QDialog(this);
    settings_->setObjectName("modelSettingsDialog");
    settings_->setWindowTitle("模型服务");
    settings_->setMinimumWidth(440);
    auto* box = new QVBoxLayout(settings_);
    box->setContentsMargins(24, 24, 24, 24);
    box->setSpacing(14);
    box->addWidget(qtui::label("模型服务", settings_, "sectionTitle"));
    box->addWidget(qtui::label("本地演示用于体验对话与镜头流程。连接 HTTP 服务后，读取服务提供的模型。", settings_, "muted"));
    mode_ = new QComboBox(settings_);
    mode_->setObjectName("chatServiceMode");
    mode_->addItems({"本地演示 · 不调用模型", "HTTP 服务 · 按 OpenAPI"});
    box->addWidget(mode_);
    url_ = new QLineEdit("http://127.0.0.1:8787", settings_);
    url_->setObjectName("aiServiceUrl");
    url_->setPlaceholderText("服务根地址");
    box->addWidget(url_);
    token_ = new QLineEdit(settings_);
    token_->setObjectName("aiServiceToken");
    token_->setEchoMode(QLineEdit::Password);
    token_->setPlaceholderText("Bearer token · 仅本次会话");
    box->addWidget(token_);
    connect_ = qtui::button("连接并读取模型", settings_);
    connect_->setObjectName("connectAiService");
    box->addWidget(connect_);
    settingsStatus_ = qtui::label("", settings_, "muted");
    settingsStatus_->setObjectName("modelSettingsStatus");
    box->addWidget(settingsStatus_);
    auto* hint = qtui::label("请求运行时不能切换服务。关闭此窗口不会取消任务；令牌不会写入工程。", settings_, "muted");
    box->addWidget(hint);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, settings_);
    close->button(QDialogButtonBox::Close)->setText("关闭");
    box->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, settings_, &QDialog::reject);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this] { configure(); });
    connect(connect_, &QPushButton::clicked, this, [this] { configure(); });
}
void ChatPanel::setStatus(const QString& text) {
    status_->setText(text);
    settingsStatus_->setText(text);
}
void ChatPanel::openSettings() {
    settings_->show();
    settings_->raise();
    settings_->activateWindow();
}
} // namespace luma

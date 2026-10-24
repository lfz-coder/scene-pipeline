#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/HomeComposer.hpp>
#include <luma/qt/StudioWorkspace.hpp>
#include "QtUiHelpers.hpp"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QVBoxLayout>
namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildHome() {
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setSpacing(22);
    layout->addWidget(label("创作空间 / 欢迎回来", content, "eyebrow"));
    layout->setContentsMargins(48, 42, 48, 36);
    auto* title = label("让想法，成为下一幅画面。", content, "pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    auto* subtitle = label("AI 整理镜头 · 自由编辑 3D · 确认后组装时间线", content, "muted");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);
    auto* composer = new HomeComposer(content);
    layout->addWidget(composer, 0, Qt::AlignHCenter);
    composer->onSubmit = [this, composer](const QString& text, const QString& capability, const QImage& image) {
        if (studio_->busy()) { notify("当前任务仍在运行，请进入项目完成或取消任务。", true); return; }
        if (!createProject(composer->presetKey().toStdString())) return;
        findChild<QPushButton*>("toggleAssistant")->setChecked(true);
        if (studio_->beginPrompt(text, capability, image)) composer->clear();
        else notify("输入已放入创作助手，请检查模型服务连接后发送。", true);
    };
    auto* note = label("默认使用本地演示；可在右上角「模型服务」连接自己的服务。", content, "muted");
    note->setAlignment(Qt::AlignCenter);
    layout->addWidget(note);
    layout->addSpacing(12);
    auto* current = card(content);
    auto* currentBox = new QHBoxLayout(current);
    currentBox->setContentsMargins(16, 12, 16, 12);
    homePreview_ = label("当前场景", current, "muted");
    homePreview_->setAlignment(Qt::AlignCenter);
    homePreview_->setFixedSize(160, 90);
    currentBox->addWidget(homePreview_);
    auto* info = new QVBoxLayout;
    info->addWidget(label("当前工程", current, "sectionTitle"));
    auto* name = label(projectName_->text(), current, "muted");
    name->setObjectName("homeProjectTitle");
    info->addWidget(name);
    connect(projectName_, &QLineEdit::textChanged, name, &QLabel::setText);
    currentBox->addLayout(info, 1);
    auto* resume = button("继续当前工程  →", current);
    resume->setObjectName("resumeProject");
    currentBox->addWidget(resume);
    connect(resume, &QPushButton::clicked, this, [this] { showPage(1); });
    layout->addWidget(current);
    auto* heading = new QHBoxLayout;
    heading->addWidget(label("选择一个起点", content, "sectionTitle"));
    heading->addStretch();
    auto* templateCount = label("7 个可编辑场景 · 全部本地运行", content, "muted");
    templateCount->setWordWrap(false);
    heading->addWidget(templateCount);
    layout->addLayout(heading);
    auto* grid = new QGridLayout;
    grid->setSpacing(14);
    struct Template {
        const char* name;
        const char* key;
        const char* detail;
        const char* mark;
    };
    const Template cards[] = {{"光照实验室", "lighting", "材质高光、三种灯光与 FBO 后处理", "✦"},
                              {"机器人展台", "robot", "几何组合、独立变换与大胆配色", "◈"},
                              {"纹理工作室", "texture", "棋盘、双纹理混合与透明裁切", "▦"},
                              {"透明叠色", "state", "深度关系、透明层与轮廓实验", "▱"},
                              {"彩色三角形", "triangle", "从顶点色开始，认识第一幅画面", "△"},
                              {"纹理画布", "quad", "一张棋盘纹理，练习材质与构图", "▧"},
                              {"自由漫游", "camera", "旋转物体、环绕与第一人称观察", "⌘"}};
    const QStringList accents{"#efc77f", "#c7a2ed", "#84b9cd", "#d5a49a", "#a4c8a1", "#c9b78a", "#97a9d3"};
    for (int i = 0; i < 7; ++i) {
        auto* tile = card(content);
        auto* box = new QVBoxLayout(tile);
        box->setContentsMargins(18, 16, 18, 16);
        box->setSpacing(9);
        auto* symbol = label(cards[i].mark, tile);
        symbol->setStyleSheet(QString("font-size:35px;color:%1;").arg(accents[i]));
        box->addWidget(symbol);
        box->addWidget(label(cards[i].name, tile, "sectionTitle"));
        box->addWidget(label(cards[i].detail, tile, "muted"));
        auto* use = button("使用这个场景  ↗", tile, "link");
        use->setObjectName(QString("template_%1").arg(cards[i].key));
        box->addWidget(use, 0, Qt::AlignLeft);
        auto key = std::string(cards[i].key);
        connect(use, &QPushButton::clicked, this, [this, key] {
            createProject(key);
        });
        grid->addWidget(tile, i / 3, i % 3);
    }
    layout->addLayout(grid);
    layout->addStretch();
    return makeScroll(content, this);
}
} // namespace luma

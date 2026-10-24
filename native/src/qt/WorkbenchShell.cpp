#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/StudioWorkspace.hpp>
#include <luma/qt/WorkbenchTheme.hpp>
#include "QtUiHelpers.hpp"
#include <QButtonGroup>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
namespace luma {
using namespace qtui;
void WorkbenchWindow::buildShell() {
    auto* shell = new QWidget(this);
    shell->setObjectName("shell");
    setCentralWidget(shell);
    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* top = new QWidget(shell);
    top->setObjectName("topbar");
    auto* bar = new QHBoxLayout(top);
    bar->setContentsMargins(16, 10, 16, 10);
    bar->setSpacing(8);
    auto* home = button("光场工坊", top, "link");
    home->setObjectName("navHome");
    home->setIcon(workbenchIcon("home"));
    home->setCheckable(true);
    home->setToolTip("返回创作首页");
    nav_[0] = home;
    bar->addWidget(home);
    connect(home, &QPushButton::clicked, this, [this] { showPage(0); });
    projectName_ = new QLineEdit("我的机器人展台", top);
    projectName_->setObjectName("projectName");
    projectName_->setMaxLength(120);
    projectName_->setMaximumWidth(160);
    projectName_->setMinimumWidth(90);
    projectName_->setToolTip("点击修改作品名称");
    bar->addWidget(projectName_);
    bar->addStretch();
    projectTabs_ = new QWidget(top);
    projectTabs_->setObjectName("projectTabs");
    auto* tabs = new QHBoxLayout(projectTabs_);
    tabs->setContentsMargins(3, 3, 3, 3);
    tabs->setSpacing(2);
    auto* group = new QButtonGroup(this);
    group->addButton(home, 0);
    const QStringList names{"3D 场景", "镜头", "媒体", "时间线", "方案"};
    const char* objects[]{"navEditor", "navShots", "navExports", "navTimeline", "navReview"};
    for (int i : {5, 1, 2, 3, 4}) {
        auto* tab = button(names[i - 1], projectTabs_, "projectTab");
        tab->setObjectName(objects[i - 1]);
        tab->setCheckable(true);
        group->addButton(tab, i);
        nav_[i] = tab;
        tabs->addWidget(tab);
        connect(tab, &QPushButton::clicked, this, [this, i] { showPage(i); });
    }
    bar->addWidget(projectTabs_);
    bar->addStretch();
    saveState_ = label("本地创作", top, "muted");
    saveState_->setObjectName("saveState");
    saveState_->setWordWrap(false);
    bar->addWidget(saveState_);
    for (auto item : {std::pair{"打开", "openProject"}, {"保存", "saveProject"},
                      {"模型服务", "modelSettings"}}) {
        auto* action = button(item.first, top);
        action->setObjectName(item.second);
        bar->addWidget(action);
        const QString name = item.second;
        connect(action, &QPushButton::clicked, this, [this, name] {
            if (name == "openProject") chooseLoadProject();
            else if (name == "saveProject") chooseSaveProject();
            else studio_->openSettings();
        });
    }
    auto* menu = button("···", top);
    menu->setObjectName("projectMenu");
    menu->setAccessibleName("项目菜单");
    buildProjectMenu(menu);
    bar->addWidget(menu);
    layout->addWidget(top);

    viewport_ = new QtViewport(this);
    buildStudio();
    pages_ = studio_->viewStack();
    pages_->addWidget(buildEditor());
    pages_->addWidget(buildShots());
    pages_->addWidget(buildExports());
    pages_->addWidget(studio_->timelineView());
    pages_->addWidget(studio_->reviewView());
    shellPages_ = new QStackedWidget(shell);
    shellPages_->setObjectName("shellPages");
    shellPages_->addWidget(buildHome());
    shellPages_->addWidget(studio_);
    layout->addWidget(shellPages_, 1);

    taskDock_ = new QDockWidget("本次操作记录", this);
    taskDock_->setObjectName("taskDock");
    taskDock_->setAllowedAreas(Qt::BottomDockWidgetArea);
    tasks_ = new QListWidget(taskDock_);
    tasks_->setObjectName("taskRecords");
    tasks_->setMinimumHeight(130);
    taskDock_->setWidget(tasks_);
    addDockWidget(Qt::BottomDockWidgetArea, taskDock_);
    taskDock_->hide();
    // Secondary utilities are kept together, outside the project view navigation.
    auto* utilities = new QWidget(shell);
    auto* tools = new QHBoxLayout(utilities);
    tools->setContentsMargins(16, 3, 16, 3);
    auto* assistant = button("AI 助手", utilities, "utility");
    assistant->setObjectName("toggleAssistant");
    assistant->setCheckable(true);
    assistant->setChecked(true);
    tools->addWidget(assistant);
    connect(assistant, &QPushButton::toggled, studio_, &StudioWorkspace::setAssistantVisible);
    tools->addStretch();
    auto* task = button("操作记录", utilities, "utility");
    task->setObjectName("toggleTasks");
    task->setCheckable(true);
    tools->addWidget(task);
    connect(task, &QPushButton::toggled, taskDock_, &QDockWidget::setVisible);
    connect(taskDock_, &QDockWidget::visibilityChanged, task, &QPushButton::setChecked);
    auto* help = button("使用帮助", utilities, "utility");
    help->setObjectName("help");
    tools->addWidget(help);
    connect(help, &QPushButton::clicked, this, [this] { showHelp(); });
    layout->addWidget(utilities);
    connect(projectName_, &QLineEdit::textEdited, this, [this] { changed(WorkflowStage::Export); });
}
} // namespace luma

#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include "QtUiHelpers.hpp"
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <luma/qt/WorkbenchTheme.hpp>
#include <luma/qt/WorkbenchWindow.hpp>
#include "WorkbenchWorkflowText.hpp"
namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildEditor() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(12);
    auto* flow = new QWidget(page);
    flow->setObjectName("workflowBar");
    auto* flowBox = new QHBoxLayout(flow);
    flowBox->setContentsMargins(12, 6, 12, 6);
    flowBox->setSpacing(5);
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    for (int i = 0; i < 7; ++i) {
        auto* step = button(QString("%1   %2").arg(i + 1).arg(titles[i]), flow, "step");
        step->setCheckable(true);
        step->setObjectName(QString("workflowStep%1").arg(i));
        step->setStyleSheet("QPushButton{border:0;background:transparent;color:#9595a4;padding:9px "
                            "5px;} QPushButton:checked{background:#383a41;color:#f5f5f6;} QPushButton:focus{border:1px solid #c4c7d0;}");
        steps_[i] = step;
        group->addButton(step, i);
        flowBox->addWidget(step, 1);
        connect(step, &QPushButton::clicked, this, [this, i] { selectStage(i); });
        if (i < 6)
            flowBox->addWidget(label("›", flow, "muted"));
    }
    auto* flowScroll = new QScrollArea(page);
    flowScroll->setObjectName("workflowScroll");
    flowScroll->setWidget(flow);
    flowScroll->setWidgetResizable(true);
    flowScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    flowScroll->setFixedHeight(64);
    layout->addWidget(flowScroll);
    auto* splitter = new QSplitter(Qt::Horizontal, page);
    splitter->setChildrenCollapsible(false);
    auto* left = new QWidget;
    left->setObjectName("sidePanel");
    left->setMinimumWidth(260);
    auto* leftBox = new QVBoxLayout(left);
    leftBox->setContentsMargins(12, 16, 12, 12);
    leftBox->setSpacing(12);
    leftBox->addWidget(label("场景资源", left, "sectionTitle"));
    presets_ = new QComboBox(left);
    presets_->setObjectName("presets");
    for (auto p : {std::pair{"机器人展台", "robot"},
                   {"纯色窗口", "window"},
                   {"彩色三角形", "triangle"},
                   {"纹理正方形", "quad"},
                   {"纹理工作室", "texture"},
                   {"自由漫游", "camera"},
                   {"透明叠色实验", "state"},
                   {"光照实验室", "lighting"}})
        presets_->addItem(p.first, p.second);
    leftBox->addWidget(presets_);
    connect(presets_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!refreshing_)
            switchPreset(presets_->currentData().toString().toStdString());
    });
    search_ = new QLineEdit(left);
    search_->setObjectName("objectSearch");
    search_->setPlaceholderText("搜索场景对象…");
    search_->setClearButtonEnabled(true);
    leftBox->addWidget(search_);
    sceneInfo_ = label("场景对象", left, "muted");
    leftBox->addWidget(sceneInfo_);
    objects_ = new QListWidget(left);
    objects_->setObjectName("objects");
    objects_->setSpacing(1);
    leftBox->addWidget(objects_, 1);
    connect(search_, &QLineEdit::textChanged, this, [this] { filterObjects(); });
    connect(objects_, &QListWidget::itemSelectionChanged, this, [this] {
        if (refreshing_) return;
        const auto selection = objects_->selectedItems();
        if (selection.empty()) invoke([](auto& c) { c.clearSelection(); });
        else {
            EntityId id{selection.front()->data(Qt::UserRole).toUInt()};
            invoke([id](auto& c) { c.select(id); });
        }
        refreshInspector();
    });
    leftBox->addWidget(label("几何体 / 材质 / 纹理", left, "eyebrow"));
    leftBox->addWidget(
        label("选择对象后切换到属性编辑。\n切换场景时保留本次会话的草稿。", left, "muted"));
    auto* center = new QWidget(splitter);
    center->setMinimumWidth(320);
    auto* centerBox = new QVBoxLayout(center);
    centerBox->setContentsMargins(0, 0, 0, 0);
    centerBox->setSpacing(8);
    auto* tools = new QWidget(center);
    tools->setObjectName("viewportTools");
    auto* toolBox = new QHBoxLayout(tools);
    toolBox->setContentsMargins(10, 8, 10, 8);
    toolBox->setSpacing(7);
    toolBox->addStretch();
    modes_ = new QComboBox(tools);
    modes_->setObjectName("cameraMode");
    modes_->addItems({"环绕观察", "第一人称"});
    toolBox->addWidget(modes_);
    connect(modes_, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (refreshing_)
            return;
        if (invoke([i](auto& c) { c.setCameraMode(i ? CameraMode::Fly : CameraMode::Orbit); }))
            changed(WorkflowStage::Camera);
    });
    style_ = new QComboBox(tools);
    style_->setObjectName("renderStyle");
    style_->addItems({"实体预览", "线框观察"});
    toolBox->addWidget(style_);
    connect(style_, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (refreshing_)
            return;
        if (invoke([i](auto& c) { c.setStyle(i ? StyleKind::Wireframe : StyleKind::Basic); }))
            changed(WorkflowStage::Review);
    });
    auto* reset = button("复位", tools);
    reset->setObjectName("resetCamera");
    reset->setToolTip("恢复当前场景的默认机位");
    toolBox->addWidget(reset);
    connect(reset, &QPushButton::clicked, this, [this] {
        viewport_->resetCamera();
        updateCameraInfo();
    });
    centerBox->addWidget(tools);
    centerBox->addWidget(viewport_, 1);
    auto* foot = new QHBoxLayout;
    foot->addWidget(label("左键环绕 · 滚轮缩放", center, "muted"));
    foot->addStretch();
    foot->addWidget(label("右键 + WASD 漫游", center, "muted"));
    centerBox->addLayout(foot);
    auto* right = new QWidget;
    right->setObjectName("properties");
    right->setMinimumWidth(285);
    auto* rightBox = new QVBoxLayout(right);
    rightBox->setContentsMargins(16, 18, 16, 14);
    rightBox->setSpacing(12);
    stageTitle_ = label("", right, "stepTitle");
    stageHelp_ = label("", right, "muted");
    rightBox->addWidget(stageTitle_);
    rightBox->addWidget(stageHelp_);
    stagePages_ = new QStackedWidget(right);
    stagePages_->setObjectName("stagePages");
    for (int i = 0; i < 7; ++i)
        stagePages_->addWidget(buildStagePage(i));
    rightBox->addWidget(stagePages_, 1);
    auto* bottom = new QHBoxLayout;
    previous_ = button("← 上一步", right);
    next_ = button("确认并继续  →", right, "primary");
    next_->setObjectName("completeStep");
    bottom->addWidget(previous_);
    bottom->addWidget(next_, 1);
    rightBox->addLayout(bottom);
    connect(previous_, &QPushButton::clicked, this,
            [this] { selectStage(std::max(0, int(workflow_.current()) - 1)); });
    connect(next_, &QPushButton::clicked, this, [this] { completeStage(); });
    auto* details = new QTabWidget(splitter);
    details->setObjectName("sceneDetails");
    details->setMinimumWidth(300);
    details->addTab(right, "属性");
    details->addTab(left, "对象");
    splitter->addWidget(center);
    splitter->addWidget(details);
    splitter->setSizes({700, 310});
    splitter->setStretchFactor(0, 1);
    connect(objects_, &QListWidget::itemDoubleClicked, details, [details] { details->setCurrentIndex(0); });
    layout->addWidget(splitter, 1);
    return page;
}
} // namespace luma

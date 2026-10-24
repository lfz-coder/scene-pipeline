#include <luma/qt/WorkbenchWindow.hpp>
#include "QtUiHelpers.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildStagePage(int index) {
    if (index == 2) return buildLightingPage();
    if (index == 5) return buildPostProcessPage();
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 4, 6);
    layout->setSpacing(16);
    if (index == 0) {
        selectionInfo_ = label("选择一个对象", content, "badge");
        layout->addWidget(selectionInfo_);
        inspector_ = new QWidget(content);
        auto* box = new QVBoxLayout(inspector_);
        box->setContentsMargins(0, 0, 0, 0);
        box->setSpacing(14);
        auto row = [&](const QString& title, std::array<QDoubleSpinBox*, 3>& fields, double lo,
                       double hi, double step, int field) {
            box->addWidget(label(title, inspector_, "muted"));
            auto* line = new QHBoxLayout;
            line->setSpacing(5);
            for (int i = 0; i < 3; ++i) {
                fields[i] = spin(inspector_, lo, hi, step);
                fields[i]->setPrefix(QString("%1 ").arg("XYZ"[i]));
                line->addWidget(fields[i]);
                connect(fields[i], &QDoubleSpinBox::valueChanged, this,
                        [this, field, i] { editTransform(field, i); });
            }
            box->addLayout(line);
        };
        row("位置", position_, -1000, 1000, .1, 0);
        position_[0]->setObjectName("positionX");
        row("旋转 · 度", rotation_, -3600, 3600, 5, 1);
        row("缩放", scale_, .001, 1000, .1, 2);
        for (auto* scale : scale_) { scale->setDecimals(3); scale->setMinimum(.001); }
        scale_[0]->setObjectName("scaleX");
        auto* reset = button("重置当前对象变换", inspector_);
        reset->setObjectName("resetTransform");
        box->addWidget(reset);
        connect(reset, &QPushButton::clicked, this, [this] {
            invoke([](auto& c) { c.setTransform({0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}); });
            changed(WorkflowStage::Scene);
            refreshInspector();
        });
        layout->addWidget(inspector_);
        layout->addWidget(label("小提示", content, "eyebrow"));
        layout->addWidget(
            label("先选左侧对象，再调整位置与大小。\n边框标出当前选中的物体。", content, "muted"));
    } else if (index == 1) {
        auto* swatchRow = new QHBoxLayout;
        colorSwatch_ = button("选择颜色", content);
        colorSwatch_->setObjectName("chooseMaterialColor");
        colorSwatch_->setMinimumHeight(46);
        swatchRow->addWidget(colorSwatch_, 1);
        layout->addLayout(swatchRow);
        connect(colorSwatch_, &QPushButton::clicked, this, [this] { chooseColor(); });
        auto* channels = new QGridLayout;
        channels->setHorizontalSpacing(8);
        channels->setVerticalSpacing(7);
        for (int i = 0; i < 4; ++i) {
            channels->addWidget(label(QString("%1").arg("RGBA"[i]), content, "muted"), i / 2,
                                (i % 2) * 2);
            color_[i] = spin(content, 0, 1, .05);
            channels->addWidget(color_[i], i / 2, (i % 2) * 2 + 1);
            color_[i]->setObjectName(QString("baseColor%1").arg(i));
            connect(color_[i], &QDoubleSpinBox::valueChanged, this, [this, i] { editMaterial(0, i); });
        }
        layout->addLayout(channels);
        layout->addWidget(label("RGB 为线性数值；颜色选择器使用屏幕色。", content, "muted"));
        auto* specular = new QGridLayout;
        for (int i = 0; i < 3; ++i) {
            specular->addWidget(label(QString("高光 %1").arg("RGB"[i]), content, "muted"), 0, i);
            specularColor_[i] = spin(content, 0, 1, .05);
            specular->addWidget(specularColor_[i], 1, i);
            connect(specularColor_[i], &QDoubleSpinBox::valueChanged, this,
                    [this, i] { editMaterial(1, i); });
        }
        layout->addLayout(specular);
        shininess_ = spin(content, 1, 256, 1);
        shininess_->setObjectName("shininess");
        shininess_->setDecimals(0);
        shininess_->setPrefix("光泽度  ");
        layout->addWidget(shininess_);
        connect(shininess_, &QDoubleSpinBox::valueChanged, this, [this] { editMaterial(2); });
        materialInfo_ = label("", content, "muted");
        layout->addWidget(materialInfo_);
        auto* form = new QFormLayout;
        form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        form->setVerticalSpacing(10);
        mix_ = spin(content, 0, 1, .05);
        form->addRow("双纹理混合比例", mix_);
        connect(mix_, &QDoubleSpinBox::valueChanged, this, [this] { editMaterial(3); });
        alpha_ = new QComboBox(content);
        alpha_->addItems({"不透明", "Alpha 裁切", "Alpha 混合"});
        form->addRow("透明模式", alpha_);
        alpha_->setObjectName("alphaMode");
        connect(alpha_, &QComboBox::currentIndexChanged, this, [this] { editMaterial(4); });
        layout->addLayout(form);
        doubleSided_ = new QCheckBox("双面显示", content);
        layout->addWidget(doubleSided_);
        doubleSided_->setObjectName("doubleSided");
        connect(doubleSided_, &QCheckBox::toggled, this, [this] { editMaterial(5); });
        auto* clone = button("复制材质，独立编辑", content);
        clone->setObjectName("cloneMaterial");
        layout->addWidget(clone);
        connect(clone, &QPushButton::clicked, this, [this] {
            if (invoke([](auto& c) { c.cloneSelectedMaterial(); }))
                changed(WorkflowStage::Material);
            refreshInspector();
        });
    } else if (index == 3) {
        layout->addWidget(label("常用机位", content, "eyebrow"));
        auto* grid = new QGridLayout;
        const QStringList poses{"正面", "侧面", "俯视", "三分之四"};
        for (int i = 0; i < 4; ++i) {
            auto* b = button(poses[i], content);
            b->setObjectName(QString("cameraPose%1").arg(i));
            grid->addWidget(b, i / 2, i % 2);
            connect(b, &QPushButton::clicked, this, [this, i] {
                invoke([i](auto& c) {
                    if (i == 0)
                        c.setCameraPose({0, .7f, 7}, {0, .7f, 0});
                    if (i == 1)
                        c.setCameraPose({7, .7f, 0}, {0, .7f, 0});
                    if (i == 2)
                        c.setCameraPose({0, 8, 0}, {0, 0, 0}, {0, 0, -1});
                    if (i == 3)
                        c.setCameraPose({5, 3.3f, 7}, {0, .7f, 0});
                });
                changed(WorkflowStage::Camera);
            });
        }
        layout->addLayout(grid);
        cameraInfo_ = label("", content, "muted");
        layout->addWidget(cameraInfo_);
        auto* capture = button("＋ 保存镜头快照", content, "primary");
        capture->setObjectName("captureShot");
        layout->addWidget(capture);
        connect(capture, &QPushButton::clicked, this, [this] { captureShot(); });
        layout->addWidget(label("环绕模式", content, "eyebrow"));
        layout->addWidget(label("左键拖动画面，滚轮改变距离。", content, "muted"));
        layout->addWidget(label("第一人称", content, "eyebrow"));
        layout->addWidget(
            label("右键拖动转向，WASD 移动，Q/E 升降，Shift 加速。", content, "muted"));
    } else if (index == 4) {
        const QStringList stateNames{"深度测试", "背面剔除", "透明混合", "选中轮廓"};
        for (int i = 0; i < 4; ++i) {
            states_[i] = new QCheckBox(stateNames[i], content);
            states_[i]->setObjectName(QString("renderState%1").arg(i));
            layout->addWidget(states_[i]);
            connect(states_[i], &QCheckBox::toggled, this, [this, i](bool value) {
                if (refreshing_ || !viewport_->model())
                    return;
                auto o = viewport_->model()->options();
                if (i == 0)
                    o.depth = value;
                if (i == 1)
                    o.cull = value;
                if (i == 2)
                    o.blend = value;
                if (i == 3)
                    o.outline = value;
                if (invoke([o](auto& c) { c.setRenderOptions(o); }))
                    changed(WorkflowStage::Review);
            });
        }
        layout->addWidget(label("画布背景", content, "eyebrow"));
        auto* backgrounds = new QHBoxLayout;
        const glm::vec3 values[] = {
            {.025f, .035f, .06f}, {.1f, .075f, .14f}, {.22f, .23f, .25f}, {.78f, .8f, .84f}};
        const QStringList colors{"#2c3445", "#594d68", "#81848a", "#e4e7ec"};
        for (int i = 0; i < 4; ++i) {
            auto* b = button("", content);
            b->setObjectName(QString("background%1").arg(i));
            b->setCheckable(true);
            b->setAccessibleName(QStringList{"深蓝背景", "暮紫背景", "灰色背景", "浅色背景"}[i]);
            b->setProperty("backgroundR", values[i].r);
            b->setProperty("backgroundG", values[i].g);
            b->setProperty("backgroundB", values[i].b);
            b->setFixedSize(42, 28);
            b->setStyleSheet("QPushButton{background:" + colors[i] +
                             ";border:1px solid #777b85;border-radius:6px;}"
                             "QPushButton:checked{border:3px solid #ffffff;}"
                             "QPushButton:focus{border:2px solid #d5baff;}"
                             "QPushButton:hover{border:2px solid #eeeeef;}");
            b->setToolTip(QStringList{"深蓝", "暮紫", "灰色", "浅色"}[i]);
            backgrounds->addWidget(b);
            auto value = values[i];
            connect(b, &QPushButton::clicked, this, [this, value] {
                if (!viewport_->model())
                    return;
                auto o = viewport_->model()->options();
                o.background = {value, 1};
                invoke([o](auto& c) { c.setRenderOptions(o); });
                changed(WorkflowStage::Review);
                refreshInspector();
            });
        }
        backgrounds->addStretch();
        layout->addLayout(backgrounds);
        reviewInfo_ = label("", content, "muted");
        layout->addWidget(reviewInfo_);
        layout->addWidget(label("确认前检查物体遮挡、透明叠色和构图。\n导出前建议关闭选中轮廓。",
                                content, "muted"));
    } else {
        auto* summary = card(content);
        auto* box = new QVBoxLayout(summary);
        box->setContentsMargins(16, 18, 16, 18);
        box->setSpacing(10);
        box->addWidget(label("保存这一刻", summary, "sectionTitle"));
        box->addWidget(
            label("PNG 图片 · 当前视口尺寸\n包含当前可见的画面与轮廓设置。", summary, "muted"));
        layout->addWidget(summary);
        layout->addWidget(label("继续打磨", content, "eyebrow"));
        layout->addWidget(label(
            "工程保存场景、材质、灯光、后处理与相机，下次可继续调整。\n镜头快照与操作记录属于本次会话。",
            content, "muted"));
    }
    layout->addStretch();
    return makeScroll(content, this);
}
}

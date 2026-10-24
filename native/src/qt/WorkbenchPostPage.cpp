#include <luma/qt/WorkbenchWindow.hpp>
#include "QtUiHelpers.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildPostProcessPage() {
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0,0,4,6);
    layout->setSpacing(14);
        postEnabled_ = new QCheckBox("启用后处理", content);
        postEnabled_->setObjectName("postEnabled");
        postEffect_ = new QComboBox(content);
        postEffect_->setObjectName("postEffect");
        postEffect_->addItems({"无", "反相", "灰度", "锐化", "模糊"});
        postIntensity_ = spin(content, 0, 1, .05);
        postIntensity_->setPrefix("效果强度  ");
        postRadius_ = spin(content, 1, 15, 1);
        postRadius_->setDecimals(0);
        postRadius_->setPrefix("模糊半径  ");
        layout->addWidget(postEnabled_);
        layout->addWidget(label("效果", content, "eyebrow"));
        layout->addWidget(postEffect_);
        layout->addWidget(postIntensity_);
        layout->addWidget(postRadius_);
        postIntensity_->setObjectName("postIntensity");
        postRadius_->setObjectName("postRadius");
        connect(postEnabled_, &QCheckBox::toggled, this, [this] { editPostProcess(0); });
        connect(postEffect_, &QComboBox::currentIndexChanged, this,
                [this] { editPostProcess(1); });
        connect(postIntensity_, &QDoubleSpinBox::valueChanged, this,
                [this] { editPostProcess(2); });
        connect(postRadius_, &QDoubleSpinBox::valueChanged, this,
                [this] { editPostProcess(3); });
        postInfo_ = label("", content, "muted");
        layout->addWidget(postInfo_);
        layout->addWidget(label("后处理发生在场景已经完成光照之后，适合做最终风格化输出。",
                                content, "muted"));
    layout->addStretch();
    return makeScroll(content, this);
}
void WorkbenchWindow::editPostProcess(int field) {
    if (refreshing_ || !viewport_->model())
        return;
    auto settings = viewport_->model()->postProcess();
    if (field == 0) settings.enabled = postEnabled_->isChecked();
    if (field == 1) settings.effect = static_cast<PostProcessEffect>(postEffect_->currentIndex());
    if (field == 2) settings.intensity = postIntensity_->value();
    if (field == 3) settings.radius = postRadius_->value();
    postRadius_->setEnabled(settings.enabled && settings.effect == PostProcessEffect::Blur);
    postIntensity_->setEnabled(settings.enabled && settings.effect != PostProcessEffect::None);
    if (invoke([settings](auto& c) { c.setPostProcessSettings(settings); })) {
        changed(WorkflowStage::PostProcess);
        postInfo_->setText(settings.enabled ? "效果已启用 · 将在最终输出前应用"
                                            : "效果关闭 · 直接输出光照结果");
    }
}
} // namespace luma

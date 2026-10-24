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
#include <QUrl>
#include <QVBoxLayout>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <luma/qt/WorkbenchTheme.hpp>
#include <luma/qt/WorkbenchWindow.hpp>
namespace luma {
using namespace qtui;
namespace {
QString objectLabel(std::string_view text) {
    const std::map<std::string, QString> names{
        {"Ground", "地面"},
        {"Ruby", "红玉"},
        {"Gold", "格纹黄铜"},
        {"Azure", "蔚蓝"},
        {"Robot / head", "机器人 · 头部"},
        {"Robot / body", "机器人 · 身体"},
        {"Arm L", "左臂"},
        {"Arm R", "右臂"},
        {"Foot L", "左脚"},
        {"Foot R", "右脚"},
        {"Eye L", "左眼"},
        {"Eye R", "右眼"},
        {"Pedestal", "展台"},
        {"Vertex colors", "顶点色三角形"},
        {"Textured square", "纹理正方形"},
        {"sRGB checker", "棋盘纹理"},
        {"Two textures / linear mix", "双纹理混合"},
        {"Alpha cutout", "透明裁切纹理"},
        {"Opaque back box", "不透明背景方块"},
        {"Back-facing triangle (toggle cull)", "背向三角形"}};
    auto found = names.find(std::string(text));
    if (found != names.end())
        return found->second;
    if (text.substr(0, 12) == "Alpha layer ")
        return "透明层 " + QString::fromStdString(std::string(text.substr(12)));
    return QString::fromUtf8(text.data(), int(text.size()));
}
}
void WorkbenchWindow::rebuildObjects() {
    refreshing_ = true;
    objects_->clear();
    auto* m = viewport_->model();
    if (m)
        for (const auto& item : m->scene().renderItems()) {
            auto* row = new QListWidgetItem(workbenchIcon("cube"),
                                            objectLabel(m->scene().name(item.entity)), objects_);
            row->setData(Qt::UserRole, item.entity.value);
            row->setHidden(!row->text().contains(search_->text(), Qt::CaseInsensitive));
            if (item.entity.value == m->selected().value)
                objects_->setCurrentItem(row);
        }
    filterObjects();
    refreshing_ = false;
    refreshInspector();
}
void WorkbenchWindow::filterObjects() {
    int visible = 0;
    for (int i = 0; i < objects_->count(); ++i) {
        auto* item = objects_->item(i);
        const bool match = item->text().contains(search_->text(), Qt::CaseInsensitive);
        item->setHidden(!match);
        visible += match;
    }
    sceneInfo_->setText(QString("场景对象 · %1 / %2%3").arg(visible).arg(objects_->count())
                            .arg(visible ? "" : " · 无匹配"));
}
void WorkbenchWindow::refreshInspector() {
    const auto* m = viewport_->model();
    bool valid = m && m->scene().contains(m->selected());
    inspector_->setEnabled(valid);
    findChild<QPushButton*>("cloneMaterial")->setEnabled(valid);
    if (m) {
        refreshing_ = true;
        modes_->setCurrentIndex(m->controls().mode() == CameraMode::Fly ? 1 : 0);
        style_->setCurrentIndex(m->style() == StyleKind::Wireframe ? 1 : 0);
        const auto& options = m->options();
        const bool flags[]{options.depth, options.cull, options.blend, options.outline};
        for (int i = 0; i < 4; ++i)
            states_[i]->setChecked(flags[i]);
        for (int i = 0; i < 4; ++i) {
            auto* b = findChild<QPushButton*>(QString("background%1").arg(i));
            const glm::vec3 value{b->property("backgroundR").toFloat(),
                                  b->property("backgroundG").toFloat(),
                                  b->property("backgroundB").toFloat()};
            b->setChecked(glm::distance(glm::vec3(options.background), value) < .0001f);
        }
        reviewInfo_->setText(QString("%1 个对象 · %2\n当前预览直接来自渲染视口。")
                                 .arg(objects_->count())
                                 .arg(m->style() == StyleKind::Basic ? "基础着色" : "线框观察"));
        const auto& post = m->postProcess();
        postEnabled_->setChecked(post.enabled);
        postEffect_->setCurrentIndex(static_cast<int>(post.effect));
        postIntensity_->setValue(post.intensity);
        postRadius_->setValue(post.radius);
        postRadius_->setEnabled(post.enabled && post.effect == PostProcessEffect::Blur);
        postIntensity_->setEnabled(post.enabled && post.effect != PostProcessEffect::None);
        postInfo_->setText(post.enabled ? "效果已启用 · 将在最终输出前应用"
                                        : "效果关闭 · 直接输出光照结果");
        refreshing_ = false;
    }
    refreshLighting();
    colorSwatch_->setEnabled(valid);
    alpha_->setEnabled(valid);
    doubleSided_->setEnabled(valid);
    for (auto* w : color_)
        w->setEnabled(valid);
    for (auto* w : specularColor_)
        w->setEnabled(valid);
    shininess_->setEnabled(valid);
    if (!valid) {
        selectionInfo_->setText("当前场景没有选中的对象");
        materialInfo_->setText("选择对象后编辑材质");
        mix_->setEnabled(false);
        return;
    }
    refreshing_ = true;
    const auto& t = m->scene().transform(m->selected());
    auto e = glm::degrees(glm::eulerAngles(t.rotation()));
    for (int i = 0; i < 3; ++i) {
        position_[i]->setValue(t.position()[i]);
        rotation_[i]->setValue(e[i]);
        scale_[i]->setValue(t.scale()[i]);
    }
    selectionInfo_->setText(objectLabel(m->scene().name(m->selected())));
    auto id = m->selectedMaterial();
    const auto& mat = m->materials().material(id);
    for (int i = 0; i < 4; ++i)
        color_[i]->setValue(mat.baseColor[i]);
    for (int i = 0; i < 3; ++i)
        specularColor_[i]->setValue(mat.specularColor[i]);
    shininess_->setValue(mat.shininess);
    mix_->setValue(mat.textureMix);
    mix_->setEnabled(bool(mat.secondaryColorTexture));
    alpha_->setCurrentIndex(int(mat.alphaMode));
    doubleSided_->setChecked(mat.doubleSided);
    int users = 0;
    for (const auto& item : m->scene().renderItems())
        if (item.material.value == id.value)
            ++users;
    materialInfo_->setText(QString("材质 #%1  ·  %2 个对象共享\n%3")
                               .arg(id.value)
                               .arg(users)
                               .arg(mat.secondaryColorTexture ? "双纹理 · 混合比例可调"
                                    : mat.baseColorTexture    ? "单纹理 · 颜色与纹理相乘"
                                                              : "常量底色 · 高光由光照模型控制"));
    auto toSrgb = [](float v) {
        return v <= .0031308f ? 12.92f * v : 1.055f * std::pow(v, 1 / 2.4f) - .055f;
    };
    auto color =
        QColor::fromRgbF(toSrgb(mat.baseColor.r), toSrgb(mat.baseColor.g), toSrgb(mat.baseColor.b));
    colorSwatch_->setText("选择颜色   " + color.name().toUpper());
    colorSwatch_->setStyleSheet(
        QString("background:%1;color:%2;border:1px solid #9b8ba9;border-radius:8px;padding:12px;")
            .arg(color.name(), color.lightnessF() > .6 ? "#18151e" : "#ffffff"));
    refreshing_ = false;
}
void WorkbenchWindow::editTransform(int field, int component) {
    const auto* model = viewport_->model();
    if (refreshing_ || !model || !model->scene().contains(model->selected()))
        return;
    const auto& original = model->scene().transform(model->selected());
    auto p = original.position(), s = original.scale();
    auto q = original.rotation();
    if (field == 0) p[component] = position_[component]->value();
    if (field == 1) {
        auto r = glm::degrees(glm::eulerAngles(q));
        r[component] = rotation_[component]->value();
        q = glm::quat(glm::radians(r));
    }
    if (field == 2) s[component] = scale_[component]->value();
    if (invoke([=](auto& c) { c.setTransform(p, q, s); }))
        changed(WorkflowStage::Scene);
}
void WorkbenchWindow::editMaterial(int field, int component) {
    if (refreshing_ || !viewport_->model())
        return;
    auto* m = viewport_->model();
    if (!m->scene().contains(m->selected()))
        return;
    auto mat = m->materials().material(m->selectedMaterial());
    if (field == 0) mat.baseColor[component] = color_[component]->value();
    if (field == 1) mat.specularColor[component] = specularColor_[component]->value();
    if (field == 2) mat.shininess = shininess_->value();
    if (field == 3) mat.textureMix = mix_->value();
    if (field == 4) mat.alphaMode = AlphaMode(alpha_->currentIndex());
    if (field == 5) mat.doubleSided = doubleSided_->isChecked();
    if (invoke([mat](auto& c) { c.setMaterial(mat); }))
        changed(WorkflowStage::Material);
    refreshInspector();
}
void WorkbenchWindow::updateCameraInfo() {
    if (!viewport_->model())
        return;
    auto e = viewport_->model()->camera().eye();
    cameraInfo_->setText(QString("相机位置\nX %1   Y %2   Z %3\n\n镜头快照  %4 张")
                             .arg(e.x, 0, 'f', 2)
                             .arg(e.y, 0, 'f', 2)
                             .arg(e.z, 0, 'f', 2)
                             .arg(shotData_.size()));
}
void WorkbenchWindow::updateHomePreview() {
    if (!viewport_->isValid())
        return;
    auto image = viewport_->grabFramebuffer();
    image.setDevicePixelRatio(1);
    if (!image.isNull())
        homePreview_->setPixmap(QPixmap::fromImage(image).scaled(320, 180, Qt::KeepAspectRatio,
                                                                 Qt::SmoothTransformation));
}
} // namespace luma

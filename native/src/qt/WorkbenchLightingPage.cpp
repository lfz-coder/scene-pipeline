#include <luma/qt/WorkbenchWindow.hpp>
#include "QtUiHelpers.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QVBoxLayout>
#include <type_traits>

namespace luma {
using namespace qtui;
QWidget* WorkbenchWindow::buildLightingPage() {
    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 4, 6);
    layout->setSpacing(12);
    lightingModel_ = new QComboBox(content);
    lightingModel_->setObjectName("lightingModel");
    lightingModel_->addItems({"基础配色 · Unlit", "Blinn-Phong 光照"});
    layout->addWidget(lightingModel_);
    ambient_ = spin(content, 0, 1, .01);
    ambient_->setObjectName("ambientStrength");
    ambient_->setPrefix("环境光  ");
    layout->addWidget(ambient_);
    normalViz_ = new QCheckBox("显示世界空间法线颜色", content);
    layout->addWidget(normalViz_);
    connect(lightingModel_, &QComboBox::currentIndexChanged, this, [this] { editLighting(0); });
    connect(ambient_, &QDoubleSpinBox::valueChanged, this, [this] { editLighting(1); });
    connect(normalViz_, &QCheckBox::toggled, this, [this] { editLighting(2); });
    layout->addWidget(label("场景灯光", content, "eyebrow"));
    lightList_ = new QListWidget(content);
    lightList_->setObjectName("lightList");
    lightList_->setFixedHeight(100);
    layout->addWidget(lightList_);
    auto* actions = new QHBoxLayout;
    lightKind_ = new QComboBox(content);
    lightKind_->setObjectName("lightKind");
    lightKind_->addItems({"方向光", "点光源", "聚光灯"});
    actions->addWidget(lightKind_, 1);
    auto* add = button("＋", content);
    add->setObjectName("addLight");
    add->setToolTip("添加所选类型的灯光");
    auto* remove = button("删除", content);
    remove->setObjectName("removeLight");
    actions->addWidget(add);
    actions->addWidget(remove);
    layout->addLayout(actions);
    connect(add, &QPushButton::clicked, this, [this] { addLight(); });
    connect(remove, &QPushButton::clicked, this, [this] { removeLight(); });
    connect(lightKind_, &QComboBox::currentIndexChanged, this, [this] { refreshLighting(); });
    connect(lightList_, &QListWidget::itemSelectionChanged, this, [this] {
        if (!refreshing_) refreshLighting(false);
    });
    lightFields_ = new QWidget(content);
    auto* fields = new QVBoxLayout(lightFields_);
    fields->setContentsMargins(0, 0, 0, 0);
    fields->setSpacing(10);
    lightEnabled_ = new QCheckBox("启用当前灯光", lightFields_);
    fields->addWidget(lightEnabled_);
    connect(lightEnabled_, &QCheckBox::toggled, this, [this] { editLight(LightField::Enabled); });
    lightIntensity_ = spin(lightFields_, 0, 100, .1);
    lightIntensity_->setObjectName("lightIntensity");
    lightIntensity_->setPrefix("光照强度  ");
    fields->addWidget(lightIntensity_);
    connect(lightIntensity_, &QDoubleSpinBox::valueChanged, this, [this] { editLight(LightField::Intensity); });
    auto vectorRow = [&](QString title, auto& values, QStringList names, double lo, double hi,
                         double step, LightField field) {
        auto* row = new QWidget(lightFields_);
        auto* box = new QVBoxLayout(row);
        box->setContentsMargins(0, 0, 0, 0);
        box->setSpacing(5);
        box->addWidget(label(title, row, "muted"));
        auto* line = new QHBoxLayout;
        line->setSpacing(4);
        for (std::size_t i = 0; i < values.size(); ++i) {
            values[i] = spin(row, lo, hi, step);
            if (field == LightField::Attenuation) values[i]->setDecimals(4);
            values[i]->setPrefix(names[int(i)] + " ");
            line->addWidget(values[i]);
            connect(values[i], &QDoubleSpinBox::valueChanged, this, [this, field, i] { editLight(field, int(i)); });
        }
        box->addLayout(line);
        fields->addWidget(row);
        return row;
    };
    vectorRow("光色 · 线性 RGB", lightColor_, {"R", "G", "B"}, 0, 1, .05, LightField::Color);
    lightPositionRow_ = vectorRow("位置", lightPosition_, {"X", "Y", "Z"}, -1000, 1000, .1, LightField::Position);
    lightDirectionRow_ = vectorRow("射线方向 · 非零向量", lightDirection_, {"X", "Y", "Z"}, -10, 10, .1, LightField::Direction);
    lightAttenuationRow_ = vectorRow("衰减 · 常量 / 一次 / 二次", lightAttenuation_,
                                     {"C", "L", "Q"}, 0, 10, .001, LightField::Attenuation);
    lightConeRow_ = vectorRow("聚光锥 · 内角 < 外角", lightCone_, {"内", "外"}, .1, 90, 1, LightField::Cone);
    layout->addWidget(lightFields_);
    lightingInfo_ = label("", content, "muted");
    layout->addWidget(lightingInfo_);
    layout->addStretch();
    return makeScroll(content, this);
}

void WorkbenchWindow::refreshLighting(bool rebuildList) {
    const auto* model = viewport_->model();
    if (!model || !lightList_) return;
    const bool previous = refreshing_;
    refreshing_ = true;
    lightingModel_->setCurrentIndex(model->lighting().model == ShadingModel::BlinnPhong ? 1 : 0);
    ambient_->setValue(model->lighting().ambientStrength);
    normalViz_->setChecked(model->lighting().visualizeNormals);
    unsigned selected = lightList_->currentItem() && lightList_->currentItem()->isSelected()
                            ? lightList_->currentItem()->data(Qt::UserRole).toUInt() : 0;
    const QStringList names{"方向光", "点光源", "聚光灯"};
    if (rebuildList) {
        lightList_->clear();
        for (const auto& entry : model->lights().entries()) {
            auto* row = new QListWidgetItem(QString("%1  #%2").arg(names[int(entry.kind)]).arg(entry.id.value),
                                            lightList_);
            row->setData(Qt::UserRole, entry.id.value);
            if (entry.id.value == selected) lightList_->setCurrentItem(row);
        }
        if (!lightList_->currentItem() && lightList_->count()) lightList_->setCurrentRow(0);
    }
    const bool valid = lightList_->currentItem() && lightList_->currentItem()->isSelected();
    lightFields_->setEnabled(valid);
    findChild<QPushButton*>("removeLight")->setEnabled(valid);
    int counts[3]{};
    for (const auto& entry : model->lights().entries()) ++counts[int(entry.kind)];
    const int limits[]{1, 8, 4};
    auto* add = findChild<QPushButton*>("addLight");
    const bool capacity = counts[lightKind_->currentIndex()] < limits[lightKind_->currentIndex()];
    add->setEnabled(capacity);
    add->setToolTip(capacity ? "添加所选类型的灯光" : "此类型已达到数量上限");
    if (valid) {
        const auto id = LightId{lightList_->currentItem()->data(Qt::UserRole).toUInt()};
        const auto& entry = model->lights().entry(id);
        lightPositionRow_->setVisible(entry.kind != LightKind::Directional);
        lightDirectionRow_->setVisible(entry.kind != LightKind::Point);
        lightAttenuationRow_->setVisible(entry.kind != LightKind::Directional);
        lightConeRow_->setVisible(entry.kind == LightKind::Spot);
        auto setVector = [](auto& fields, auto value) {
            for (std::size_t i = 0; i < fields.size(); ++i) fields[i]->setValue(value[int(i)]);
        };
        std::visit([&](const auto& light) {
            using T = std::decay_t<decltype(light)>;
            lightEnabled_->setChecked(light.enabled);
            lightIntensity_->setValue(light.intensity);
            setVector(lightColor_, light.color);
            if constexpr (!std::is_same_v<T, DirectionalLight>) {
                setVector(lightPosition_, light.position);
                setVector(lightAttenuation_, glm::vec3(light.constant, light.linear, light.quadratic));
            }
            if constexpr (!std::is_same_v<T, PointLight>) setVector(lightDirection_, light.direction);
            if constexpr (std::is_same_v<T, SpotLight>) {
                lightCone_[0]->setValue(light.innerCutoffDegrees);
                lightCone_[1]->setValue(light.outerCutoffDegrees);
            }
        }, entry.value);
    }
    lightingInfo_->setText(QString("%1 个灯光 · 最多 1 个方向光、8 个点光、4 个聚光灯。")
                               .arg(model->lights().entries().size()));
    refreshing_ = previous;
}
} // namespace luma

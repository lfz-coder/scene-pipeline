#include <glad/gl.h>
#include <luma/qt/WorkbenchWindow.hpp>
#include "qt_interaction_checks.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <iostream>
using namespace luma;
static int failures = 0;
static int checks = 0;
static void check(bool ok, const char* message) {
    ++checks;
    std::cout << (ok ? "PASS " : "FAIL ") << message << '\n';
    if (!ok) ++failures;
}
static void settle() { QTest::qWait(180); QApplication::processEvents(); }
void interactionCheck(bool ok, const char* message) { check(ok, message); }
void interactionSettle() { settle(); }
static QPushButton* byText(WorkbenchWindow& window, const QString& text) {
    for (auto* button : window.findChildren<QPushButton*>())
        if (button->text() == text) return button;
    return nullptr;
}
int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    configureQtSurface();
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    WorkbenchWindow window;
    window.show();
    settle();
    auto* view = window.viewport();
    auto* presets = window.findChild<QComboBox*>("presets");
    presets->setCurrentIndex(presets->findData("lighting"));
    settle();
    const auto eye = view->model()->camera().eye();
    const auto target = view->model()->camera().target();
    view->controller()->setCameraPose({1, 2, 4}, {0, 0, 0});
    byText(window, "复位")->click();
    check(glm::distance(view->model()->camera().eye(), eye) < .00001f &&
              glm::distance(view->model()->camera().target(), target) < .00001f,
          "camera reset returns the actual preset pose");
    auto material = view->model()->materials().material(view->model()->selectedMaterial());
    material.baseColor = {.075f, .1234f, .5678f, 1};
    material.specularColor = {.2345f, .3456f, .4567f};
    view->controller()->setMaterial(material);
    presets->setCurrentIndex(presets->findData("robot"));
    presets->setCurrentIndex(presets->findData("lighting"));
    window.findChild<QDoubleSpinBox*>("shininess")->setValue(100);
    const auto edited = view->model()->materials().material(view->model()->selectedMaterial());
    check(edited.baseColor == material.baseColor && edited.specularColor == material.specularColor,
          "editing shininess preserves unrelated material precision");
    view->controller()->setTransform({.1234f, .5678f, .9012f}, {1, 0, 0, 0}, {1.2345f, 1, 1});
    presets->setCurrentIndex(presets->findData("robot"));
    presets->setCurrentIndex(presets->findData("lighting"));
    window.findChild<QDoubleSpinBox*>("positionX")->setValue(2);
    const auto& transform = view->model()->scene().transform(view->model()->selected());
    check(std::abs(transform.position().y - .5678f) < .000001f &&
              std::abs(transform.scale().x - 1.2345f) < .000001f,
          "editing position X preserves Y and scale precision");
    auto post = view->model()->postProcess();
    post.intensity = .1234f;
    view->controller()->setPostProcessSettings(post);
    presets->setCurrentIndex(presets->findData("robot"));
    presets->setCurrentIndex(presets->findData("lighting"));
    window.findChild<QCheckBox*>("postEnabled")->setChecked(true);
    check(std::abs(view->model()->postProcess().intensity - .1234f) < .000001f,
          "post enable preserves effect intensity precision");
    QTemporaryDir temp;
    check(window.findChild<QLineEdit*>("shotsSearch"), "shots provide searchable media cards");
    check(window.findChild<QPushButton*>("shotsPreview"), "shots provide image preview action");
    check(window.findChild<QComboBox*>("exportsStatus"), "exports provide success/failure filter");
    check(window.findChild<QPushButton*>("projectMenu"), "project actions are available in a menu");
    auto* kind = window.findChild<QComboBox*>("lightKind");
    kind->setCurrentIndex(0);
    check(!window.findChild<QPushButton*>("addLight")->isEnabled(),
          "directional light capacity disables add instead of a failing action");
    if (window.findChild<QLineEdit*>("shotsSearch")) checkMediaInteractions(window);
    if (window.findChild<QPushButton*>("projectMenu")) checkProjectInteractions(window);
    checkControlInteractions(window);
    window.saveProject(temp.filePath("audit.luma"));
    window.close();
    std::cout << checks << " checks, " << failures << " interaction failures\n";
    return failures ? 1 : 0;
}

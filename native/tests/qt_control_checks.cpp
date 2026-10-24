#include <glad/gl.h>
#include <luma/qt/WorkbenchWindow.hpp>
#include "qt_interaction_checks.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QTest>
#include <cmath>
using namespace luma;
void checkControlInteractions(WorkbenchWindow& window) {
    auto* view = window.viewport();
    auto button = [&](const QString& name) { return window.findChild<QPushButton*>(name); };
    auto* pages = window.findChild<QStackedWidget*>("pages");
    for (auto pair : {std::pair{"navHome", 0}, {"navEditor", 1}, {"navShots", 2}, {"navExports", 3}}) {
        button(pair.first)->click();
        interactionCheck((pair.second == 0 ? window.findChild<QStackedWidget*>("shellPages")->currentIndex() == 0
                                          : pages->currentIndex() == pair.second - 1) && button(pair.first)->isChecked(),
                         "navigation changes page and synchronizes active state");
    }
    button("navEditor")->click();
    QTimer::singleShot(30, qApp, [&] {
        auto* menu = window.findChild<QMenu*>("projectActions");
        interactionCheck(menu->isVisible(), "project menu opens from its visible button");
        menu->close();
    });
    QTest::mouseClick(button("projectMenu"), Qt::LeftButton);
    QTest::qWait(70); // A popup is non-blocking; inspect it before opening the next dialog.
    QTimer::singleShot(30, qApp, [] {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        interactionCheck(dialog && dialog->text().contains("第二阶段"), "help opens current Chinese instructions");
        if (dialog) dialog->accept();
    });
    button("help")->click();
    auto* presets = window.findChild<QComboBox*>("presets");
    auto* objects = window.findChild<QListWidget*>("objects");
    auto* search = window.findChild<QLineEdit*>("objectSearch");
    for (int i = 0; i < presets->count(); ++i) {
        presets->setCurrentIndex(i);
        interactionCheck(view->lastError().empty() && view->currentPreset() == presets->itemData(i).toString().toStdString(),
                         "each scene selector entry resolves to a real preset");
        if (presets->currentData().toString() == "window")
            interactionCheck(!button("cloneMaterial")->isEnabled() && !button("resetTransform")->isEnabled(),
                             "empty scene disables object and material actions");
    }
    presets->setCurrentIndex(presets->findData("robot"));
    search->setText("不存在");
    presets->setCurrentIndex(presets->findData("lighting"));
    bool zero = true;
    for (int i = 0; i < objects->count(); ++i) zero &= objects->item(i)->isHidden();
    interactionCheck(zero, "object filtering remains applied after scene rebuild");
    search->clear();
    objects->clearSelection();
    interactionCheck(!view->model()->selected().value && !button("cloneMaterial")->isEnabled(),
                     "object deselection clears MVC selection and disables editing");
    objects->setCurrentRow(1, QItemSelectionModel::ClearAndSelect);
    auto* x = window.findChild<QDoubleSpinBox*>("positionX");
    x->setValue(2);
    button("resetTransform")->click();
    const auto& transform = view->model()->scene().transform(view->model()->selected());
    interactionCheck(transform.position() == glm::vec3(0) && transform.scale() == glm::vec3(1),
                     "reset transform restores identity through controller");
    const auto previousMaterial = view->model()->selectedMaterial();
    button("cloneMaterial")->click();
    interactionCheck(view->model()->selectedMaterial().value != previousMaterial.value,
                     "clone material creates an independent editable resource");
    const auto before = view->model()->materials().material(view->model()->selectedMaterial());
    QTimer::singleShot(30, qApp, [] {
        auto* dialog = qobject_cast<QColorDialog*>(QApplication::activeModalWidget());
        interactionCheck(dialog, "material color chooser opens");
        if (dialog) dialog->reject();
    });
    button("chooseMaterialColor")->click();
    interactionCheck(view->model()->materials().material(view->model()->selectedMaterial()).baseColor == before.baseColor,
                     "canceling color choice preserves the material");
    QColor chosenColor;
    QTimer::singleShot(30, qApp, [&chosenColor] {
        auto* dialog = qobject_cast<QColorDialog*>(QApplication::activeModalWidget());
        if (dialog) {
            dialog->setCurrentColor(QColor::fromRgbF(.5, .25, .75, .8));
            chosenColor = dialog->currentColor();
            dialog->accept();
        }
    });
    button("chooseMaterialColor")->click();
    const auto color = view->model()->materials().material(view->model()->selectedMaterial()).baseColor;
    const auto screenRed = chosenColor.redF();
    const auto expectedRed = screenRed <= .04045 ? screenRed / 12.92 : std::pow((screenRed + .055) / 1.055, 2.4);
    interactionCheck(chosenColor.isValid() && std::abs(color.r - expectedRed) < .00001 &&
                     std::abs(color.a - chosenColor.alphaF()) < .00001,
                     "accepted screen color converts to linear RGB and preserves alpha");
    auto* alpha = window.findChild<QComboBox*>("alphaMode");
    alpha->setCurrentIndex(2);
    interactionCheck(view->model()->materials().material(view->model()->selectedMaterial()).alphaMode == AlphaMode::Blend,
                     "alpha mode updates actual material state");
    auto* doubleSided = window.findChild<QCheckBox*>("doubleSided");
    doubleSided->setChecked(true);
    interactionCheck(view->model()->materials().material(view->model()->selectedMaterial()).doubleSided,
                     "double-sided option updates actual material state");
    for (int i = 0; i < 4; ++i) {
        button(QString("cameraPose%1").arg(i))->click();
        interactionCheck(glm::distance(view->model()->camera().eye(), view->model()->camera().target()) > .1f,
                         "each camera pose produces a valid view");
        button(QString("background%1").arg(i))->click();
        interactionCheck(button(QString("background%1").arg(i))->isChecked(),
                         "background swatch marks the applied canvas color");
        auto* state = window.findChild<QCheckBox*>(QString("renderState%1").arg(i));
        const bool initial = state->isChecked();
        state->click();
        const auto options = view->model()->options();
        const bool values[]{options.depth, options.cull, options.blend, options.outline};
        interactionCheck(values[i] != initial, "each render-state checkbox changes renderer options");
        state->click();
    }
    auto* lights = window.findChild<QListWidget*>("lightList");
    lights->clearSelection();
    interactionCheck(!button("removeLight")->isEnabled() &&
                     !window.findChild<QDoubleSpinBox*>("lightIntensity")->isEnabled(),
                     "deselected light cannot be edited or removed");
    auto* kind = window.findChild<QComboBox*>("lightKind");
    kind->setCurrentIndex(1);
    const auto count = view->model()->lights().entries().size();
    button("addLight")->click();
    interactionCheck(view->model()->lights().entries().size() == count + 1, "add light creates the selected type");
    button("removeLight")->click();
    interactionCheck(view->model()->lights().entries().size() == count, "delete light removes the selected light");
    for (int i = 0; i < 7; ++i) {
        button(QString("workflowStep%1").arg(i))->click();
        interactionCheck(button(QString("workflowStep%1").arg(i))->isChecked(),
                         "each workflow tab exposes its own inspector page");
    }
    window.resize(1100, 720);
    button("workflowStep2")->click();
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-editor-narrow.png");
    for (const auto* key : {"lighting", "robot", "texture", "state", "triangle", "quad", "camera"}) {
        button("navHome")->click();
        QTimer::singleShot(30, qApp, [] {
            auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            interactionCheck(dialog && dialog->objectName() == "unsavedChanges", "template creation protects unsaved work");
            if (!dialog) return;
            for (auto* b : dialog->findChildren<QPushButton*>())
                if (b->text() == "放弃修改") { b->click(); return; }
            dialog->reject();
        });
        button(QString("template_%1").arg(key))->click();
        interactionCheck(view->currentPreset() == key && pages->currentIndex() == 0 &&
                         window.findChild<QListWidget*>("shots")->count() == 0,
                         "each home template creates a fresh matching project");
    }
    presets->setCurrentIndex(presets->findData("lighting"));
    window.resize(1440, 900);
    button("workflowStep2")->click();
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-editor.png");
    window.resize(1100, 720);
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-editor-narrow.png");
    window.resize(1440, 900);
    button("navHome")->click();
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-home.png");
    button("navEditor")->click();
    button("workflowStep6")->click();
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-export-stage.png");
}

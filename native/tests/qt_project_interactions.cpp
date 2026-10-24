#include <glad/gl.h>
#include <luma/qt/WorkbenchWindow.hpp>
#include "qt_interaction_checks.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDateTime>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace luma;
static void modal(const std::function<void(QWidget*)>& action) {
    QTimer::singleShot(30, qApp, [action] {
        auto* dialog = QApplication::activeModalWidget();
        interactionCheck(dialog, "project dialog is visible");
        if (dialog) {
            std::cout << "DIALOG " << dialog->metaObject()->className() << " " << dialog->windowTitle().toStdString() << '\n';
            action(dialog);
        }
    });
}
static void answer(QWidget* dialog, const QString& text) {
    for (auto* button : dialog->findChildren<QPushButton*>())
        if (button->text() == text) { button->click(); return; }
    interactionCheck(false, "expected project dialog action exists");
    static_cast<QDialog*>(dialog)->reject();
}
static void expectDirectSave(QPushButton* button) {
    bool prompted = false;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&] {
        if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
            prompted = true;
            dialog->reject();
        }
    });
    timer.start(50);
    button->click();
    QTest::qWait(65);
    timer.stop();
    interactionCheck(!prompted, "save reuses the existing project path without prompting");
}
void checkProjectInteractions(WorkbenchWindow& window) {
    QTemporaryDir temp;
    const auto path = temp.filePath("current.luma");
    auto* x = window.findChild<QDoubleSpinBox*>("positionX");
    auto* save = window.findChild<QPushButton*>("saveProject");
    auto* presets = window.findChild<QComboBox*>("presets");
    interactionCheck(window.saveProject(path), "save fixture project");
    x->setValue(4);
    expectDirectSave(save);
    auto document = ProjectDocument::read(path);
    interactionCheck(!document.name.isEmpty(), "saved file remains readable");
    const auto originalPreset = window.viewport()->currentPreset();
    presets->setCurrentIndex(presets->findData(originalPreset == "robot" ? "lighting" : "robot"));
    presets->setCurrentIndex(presets->findData(QString::fromStdString(originalPreset)));
    x->setValue(5);
    expectDirectSave(save);
    auto* menu = window.findChild<QMenu*>("projectActions");
    auto* saveAs = window.findChild<QAction*>("saveProjectAs");
    interactionCheck(menu && saveAs, "project menu exposes save-as independently");
    modal([](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        interactionCheck(dialog && dialog->defaultSuffix() == "luma",
                         "save dialog resolves extension before overwrite checks");
        if (dialog) dialog->reject();
    });
    saveAs->trigger();
    auto* task = window.findChild<QPushButton*>("toggleTasks");
    auto* dock = window.findChild<QDockWidget*>("taskDock");
    task->click();
    interactionCheck(dock->isVisible() && task->isChecked(), "task button opens the operation drawer");
    dock->close();
    interactionCheck(!task->isChecked(), "closing task drawer synchronizes its toggle");
    auto* step = window.findChild<QPushButton*>("workflowStep6");
    step->click();
    auto* exportButton = window.findChild<QPushButton*>("completeStep");
    modal([&](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        interactionCheck(dialog && dialog->defaultSuffix() == "png",
                         "export dialog resolves PNG suffix before accepting");
        if (!dialog) return;
        dialog->setDirectory(temp.path());
        dialog->findChild<QLineEdit*>("fileNameEdit")->setText("from-dialog");
        std::cout << "EXPORT PATH " << dialog->selectedFiles().join("|").toStdString() << '\n';
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    exportButton->click();
    const auto png = temp.filePath("from-dialog.png");
    std::cout << "EXPORT STATUS " << window.statusBar()->currentMessage().toStdString() << '\n';
    interactionCheck(QFileInfo::exists(png), "export button writes an extensionless input as PNG");
    const auto pngTime = QFileInfo(png).lastModified();
    modal([&](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        if (!dialog) return;
        dialog->setDirectory(temp.path());
        dialog->findChild<QLineEdit*>("fileNameEdit")->setText("from-dialog");
        modal([dialog](QWidget* overwrite) {
            auto* message = qobject_cast<QMessageBox*>(overwrite);
            interactionCheck(message, "existing suffixed file prompts before overwriting");
            if (message) message->button(QMessageBox::No)->click();
            QTimer::singleShot(20, dialog, &QFileDialog::reject);
        });
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    exportButton->click();
    interactionCheck(QFileInfo(png).lastModified() == pngTime,
                     "rejecting overwrite preserves the original PNG");
    // Open remains a reversible choice: cancelling the unsaved guard preserves live data.
    x->setValue(6);
    auto* open = window.findChild<QPushButton*>("openProject");
    modal([&](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        if (!dialog) return;
        dialog->findChild<QLineEdit*>("fileNameEdit")->setText(path);
        modal([](QWidget* guard) { answer(guard, "取消"); });
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    open->click();
    interactionCheck(x->value() == 6, "cancel opening preserves unsaved edits");
    modal([&](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        if (!dialog) return;
        dialog->findChild<QLineEdit*>("fileNameEdit")->setText(path);
        modal([](QWidget* guard) { answer(guard, "放弃修改"); });
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    open->click();
    interactionCheck(x->value() == 5, "discard then open restores saved project values");
    x->setValue(8);
    modal([&](QWidget* widget) {
        auto* dialog = qobject_cast<QFileDialog*>(widget);
        if (!dialog) return;
        dialog->findChild<QLineEdit*>("fileNameEdit")->setText(path);
        modal([](QWidget* guard) { answer(guard, "保存并继续"); });
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    open->click();
    interactionCheck(x->value() == 8, "save-and-continue commits edits before opening the saved project");
    x->setValue(7);
    modal([](QWidget* guard) { answer(guard, "取消"); });
    window.close();
    interactionCheck(window.isVisible(), "cancel close keeps the current project open");
    window.saveProject(path);
    window.showPage(1);
    window.findChild<QPushButton*>("workflowStep2")->click();
    window.resize(1440, 900);
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-editor.png");
}

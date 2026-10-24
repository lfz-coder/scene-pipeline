#include <glad/gl.h>
#include <luma/qt/WorkbenchWindow.hpp>
#include "qt_interaction_checks.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QUrl>
class MediaUrlProbe final : public QObject {
    Q_OBJECT
  public:
    std::vector<QUrl> urls;
  public slots:
    void openUrl(const QUrl& url) { urls.push_back(url); }
};
static void modal(const std::function<void(QWidget*)>& action) {
    QTimer::singleShot(30, qApp, [action] {
        auto* dialog = QApplication::activeModalWidget();
        interactionCheck(dialog, "expected modal dialog is visible");
        if (dialog) action(dialog);
    });
}
static void answer(QWidget* dialog, const QString& text) {
    for (auto* button : dialog->findChildren<QPushButton*>())
        if (button->text() == text) { button->click(); return; }
    interactionCheck(false, "expected Chinese dialog action exists");
    static_cast<QDialog*>(dialog)->reject();
}
void checkMediaInteractions(luma::WorkbenchWindow& window) {
    auto findButton = [&](const char* name) { return window.findChild<QPushButton*>(name); };
    auto* shots = window.findChild<QListWidget*>("shots");
    auto* search = window.findChild<QLineEdit*>("shotsSearch");
    auto* group = window.findChild<QComboBox*>("shotsGroup");
    interactionCheck(!findButton("shotsPreview")->isEnabled() &&
                     !findButton("restoreShot")->isEnabled() && !findButton("shotsRemove")->isEnabled(),
                     "empty snapshot collection disables selection actions");
    findButton("captureShot")->click();
    const auto firstEye = window.viewport()->model()->camera().eye();
    window.viewport()->controller()->setCameraPose({3, 4, 6}, {0, 0, 0});
    findButton("captureShot")->click();
    window.showPage(2);
    interactionSettle();
    interactionCheck(shots->count() == 2, "two full-resolution snapshots are captured");
    shots->setCurrentRow(0);
    shots->currentItem()->setSelected(false);
    interactionCheck(!findButton("restoreShot")->isEnabled(), "deselected current card cannot restore");
    shots->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    search->setText("不存在的镜头");
    interactionCheck(shots->item(0)->isHidden() && !findButton("shotsPreview")->isEnabled() &&
                     window.findChild<QLabel*>("shotsEmpty")->text().contains("没有符合"),
                     "search has explicit zero-result state and disables hidden selection");
    search->clear();
    interactionCheck(group->count() == 2, "gallery groups snapshots by actual scene");
    group->setCurrentIndex(1);
    interactionCheck(!shots->item(0)->isHidden(), "scene group includes matching snapshots");
    group->setCurrentIndex(0);
    shots->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
    QTimer::singleShot(30, qApp, [] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        interactionCheck(menu && menu->actions().size() >= 4, "snapshot context menu exposes real selection actions");
        if (menu) menu->close();
    });
    const auto cardCenter = shots->visualItemRect(shots->item(0)).center();
    QContextMenuEvent context(QContextMenuEvent::Mouse, cardCenter, shots->viewport()->mapToGlobal(cardCenter));
    QApplication::sendEvent(shots->viewport(), &context);
    QTest::qWait(60);
    QTest::mouseClick(shots->viewport(), Qt::LeftButton, Qt::NoModifier,
                     shots->visualItemRect(shots->item(0)).center());
    modal([&](QWidget* dialog) {
        interactionCheck(dialog->objectName() == "mediaPreview", "double-click opens the media preview");
        if (dialog->objectName() != "mediaPreview") { static_cast<QDialog*>(dialog)->reject(); return; }
        interactionCheck(dialog->findChild<QLabel*>("previewCounter")->text() == "1 / 2",
                         "preview starts at the selected image");
        interactionCheck(!dialog->findChild<QPushButton*>("previewPrevious")->isEnabled(),
                         "preview previous is disabled at first image");
        dialog->findChild<QPushButton*>("previewNext")->click();
        interactionCheck(dialog->findChild<QLabel*>("previewCounter")->text() == "2 / 2" &&
                         !dialog->findChild<QPushButton*>("previewNext")->isEnabled(),
                         "preview next reaches last image without wrapping");
        dialog->findChild<QPushButton*>("previewInfo")->click();
        interactionCheck(dialog->findChild<QLabel*>("previewDetails")->isHidden(),
                         "preview information panel can collapse");
        dialog->resize(1000, 680);
        dialog->grab().save("/private/tmp/luma-interaction-preview.png");
        QTest::keyClick(dialog, Qt::Key_Escape);
    });
    QTest::mouseDClick(shots->viewport(), Qt::LeftButton, Qt::NoModifier,
                      shots->visualItemRect(shots->item(0)).center());
    modal([](QWidget* dialog) {
        static_cast<QInputDialog*>(dialog)->setTextValue("新镜头名称");
        dialog->findChild<QPushButton*>()->setFocus();
        static_cast<QInputDialog*>(dialog)->accept();
    });
    findButton("shotsRename")->click();
    interactionCheck(shots->item(0)->text().startsWith("新镜头名称"), "rename commits to selected snapshot");
    modal([](QWidget* dialog) { static_cast<QDialog*>(dialog)->reject(); });
    findButton("shotsRename")->click();
    interactionCheck(shots->item(0)->text().startsWith("新镜头名称"), "cancel rename preserves snapshot name");
    modal([](QWidget* dialog) {
        static_cast<QInputDialog*>(dialog)->setTextValue("   ");
        static_cast<QInputDialog*>(dialog)->accept();
    });
    findButton("shotsRename")->click();
    interactionCheck(shots->item(0)->text().startsWith("新镜头名称"), "blank snapshot names are rejected without changing data");
    modal([](QWidget* dialog) { answer(dialog, "取消"); });
    findButton("shotsRemove")->click();
    interactionCheck(shots->count() == 2, "cancel removal preserves collection");
    findButton("restoreShot")->click();
    interactionCheck(glm::distance(firstEye, window.viewport()->model()->camera().eye()) < .00001f,
                     "restore after rename recovers the same saved camera");
    window.showPage(2);
    modal([](QWidget* dialog) { answer(dialog, "移除记录"); });
    findButton("shotsRemove")->click();
    interactionCheck(shots->count() == 1, "confirmed removal deletes only the selected snapshot");
    shots->setCurrentRow(0);
    findButton("restoreShot")->click();
    interactionCheck(glm::distance(glm::vec3(3, 4, 6), window.viewport()->model()->camera().eye()) < .00001f,
                     "snapshot IDs remain correct after removing an earlier record");
    auto* presets = window.findChild<QComboBox*>("presets");
    presets->setCurrentIndex(presets->findData("robot"));
    findButton("captureShot")->click();
    window.showPage(2);
    interactionCheck(group->count() == 3, "multiple scenes create distinct gallery groups");
    group->setCurrentIndex(group->findText("光照实验室"));
    interactionCheck(!shots->item(0)->isHidden() && shots->item(1)->isHidden(),
                     "scene filter hides records from other presets");
    shots->setCurrentRow(0);
    findButton("restoreShot")->click();
    interactionCheck(window.viewport()->currentPreset() == "lighting" &&
                     glm::distance(glm::vec3(3, 4, 6), window.viewport()->model()->camera().eye()) < .00001f,
                     "restoring a snapshot switches to its own scene draft and camera");
    group->setCurrentIndex(0);
    window.showPage(2);
    window.resize(1440, 900);
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-shots.png");
    window.resize(1100, 720);
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-shots-narrow.png");
    QTemporaryDir temp;
    const auto path = temp.filePath("作品.png");
    interactionCheck(window.exportImage(path), "export succeeds while viewport page is hidden");
    interactionCheck(!window.exportImage(temp.filePath("missing/fail.png")), "export failure becomes a media record");
    window.showPage(3);
    auto* exports = window.findChild<QListWidget*>("exports");
    auto* status = window.findChild<QComboBox*>("exportsStatus");
    status->setCurrentIndex(2);
    exports->setCurrentRow(0);
    interactionCheck(!exports->item(0)->isHidden() && exports->item(1)->isHidden() &&
                     !findButton("exportsOpen")->isEnabled() && !findButton("exportsPreview")->isEnabled(),
                     "failure filter shows failed exports without invalid open/preview actions");
    status->setCurrentIndex(1);
    exports->setCurrentRow(1);
    interactionCheck(findButton("exportsOpen")->isEnabled(), "successful export enables opening the original file");
    MediaUrlProbe probe;
    QDesktopServices::setUrlHandler("file", &probe, "openUrl");
    findButton("exportsOpen")->click();
    findButton("exportsFolder")->click();
    QDesktopServices::unsetUrlHandler("file");
    interactionCheck(probe.urls.size() == 2 && probe.urls[0].toLocalFile() == path &&
                     probe.urls[1].toLocalFile() == temp.path(),
                     "open file and folder dispatch the correct local URLs");
    QFile::remove(path);
    findButton("exportsOpen")->click();
    interactionCheck(window.statusBar()->currentMessage().contains("原文件已不存在"),
                     "missing exported file produces a clear error");
    interactionCheck(window.exportImage(path), "successful export recreates the file");
    exports->setCurrentRow(0);
    modal([](QWidget* dialog) { answer(dialog, "移除记录"); });
    findButton("exportsRemove")->click();
    interactionCheck(QFile::exists(path), "removing an export record preserves the disk file");
    status->setCurrentIndex(0);
    window.resize(1440, 900);
    interactionSettle();
    window.grab().save("/private/tmp/luma-interaction-exports.png");
    window.showPage(1);
}
#include "qt_media_checks.moc"

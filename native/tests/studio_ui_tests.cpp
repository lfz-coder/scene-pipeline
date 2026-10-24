#include <iostream>
#include <cstdlib>
#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/StudioWorkspace.hpp>
#include <luma/qt/TimelinePanel.hpp>
#include <luma/qt/TimelinePlayer.hpp>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QDialog>
#include <QFileDialog>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include <QTemporaryDir>
#include <QTest>
using namespace luma;
static int failures = 0;
static void check(bool value, const char* message) {
    std::cout << (value ? "PASS " : "FAIL ") << message << '\n'; failures += !value;
}
// Qt 6.12 AVFoundation uses NSRunLoop/CADisplayLink: polling processEvents alone starves frames.
static void nativeWait(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
static bool waitFor(const std::function<bool()>& test) {
    QElapsedTimer timer; timer.start();
    while (!test() && timer.elapsed() < 8000) nativeWait(20);
    return test();
}
int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    configureQtSurface(); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv); WorkbenchWindow window; window.show(); window.raise(); window.activateWindow(); (void)QTest::qWaitForWindowActive(&window); nativeWait(250);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &app, [] {
        std::cerr << "FAIL studio UI regression exceeded 45 seconds\n";
        std::exit(2);
    });
    watchdog.start(45000);
    auto button = [&](const char* name) { return window.findChild<QPushButton*>(name); };
    check(window.findChild<QPlainTextEdit*>("homePrompt"), "home has a creation prompt");
    window.showPage(1);
    auto* assistant = window.findChild<QWidget*>("chatPanel");
    check(assistant && assistant->isVisible(), "assistant remains visible in scene view");
    if (failures) return 1;
    check(!button("navStudio") && button("navTimeline") && button("navReview"), "project views replace the standalone studio entry");
    for (int view : {1, 2, 3, 4, 5}) {
        window.showPage(view);
        check(assistant->isVisible() && window.findChild<QWidget*>("chatPanel") == assistant,
              "project view switches retain the same assistant instance");
    }
    button("toggleAssistant")->click();
    check(!assistant->isVisible(), "assistant can collapse without changing the project view");
    button("toggleAssistant")->click();
    check(assistant->isVisible(), "assistant restores from its single toggle");
    button("modelSettings")->click();
    auto* settings = window.findChild<QDialog*>("modelSettingsDialog");
    check(settings && settings->isVisible(), "model service opens from the common settings entry");
    if (settings) settings->close();
    window.raise(); window.activateWindow(); (void)QTest::qWaitForWindowActive(&window);
    button("navEditor")->click(); nativeWait(150);
    auto* studio = static_cast<StudioWorkspace*>(window.findChild<QWidget*>("studioWorkspace"));
    auto* input = window.findChild<QPlainTextEdit*>("chatInput");
    input->setPlainText("为当前场景准备三个镜头"); button("chatSend")->click();
    const bool reviewed = waitFor([&] { return button("approvePipeline")->isEnabled(); });
    check(reviewed, "chat reaches human review");
    check(button("navReview")->isChecked() && assistant->isVisible(), "ready suggestions reveal the review view beside the conversation");
    if (!reviewed) {
        std::cout << "CHAT STATUS " << window.findChild<QLabel*>("chatStatus")->text().toStdString() << '\n';
        window.grab().save("/private/tmp/luma-studio-failure.png"); return 1;
    }
    check(studio->document().timeline.clips().empty(), "AI suggestions wait before modifying the timeline");
    button("approvePipeline")->click();
    check(button("navTimeline")->isChecked(), "approval opens the timeline view");
    check(!window.findChild<QLabel*>("chatStatus")->text().contains("等待人工确认"),
          "approved task no longer displays a pending review status");
    check(studio->document().timeline.clips().size() == 3, "approved pipeline appends real captured images");
    input->setPlainText("取消这个任务"); button("chatSend")->click(); button("chatCancel")->click();
    nativeWait(650);
    check(studio->document().timeline.clips().size() == 3 && !button("approvePipeline")->isEnabled(),
          "cancelled task cannot apply delayed suggestions");
    window.findChild<QComboBox*>("chatCapability")->setCurrentIndex(1);
    button("attachScene")->click(); input->setPlainText("分析这张画面"); button("chatSend")->click();
    check(waitFor([&] { return button("approvePipeline")->isEnabled(); }), "VLM path accepts a scene image");
    button("approvePipeline")->click();
    check(studio->document().timeline.clips().size() == 6, "image pipeline uses the same timeline boundary");
    auto* panel = static_cast<TimelinePanel*>(window.findChild<QWidget*>("timelinePanel"));
    if (argc < 2) { check(false, "video fixture path is required"); return 1; }
    panel->importPath(QString::fromLocal8Bit(argv[1]));
    check(waitFor([&] { return studio->document().timeline.clips().size() == 7; }), "local video imports with decoded duration");
    button("navExports")->click();
    auto* assets = window.findChild<QListWidget*>("projectAssets");
    check(assets && assets->count() == 3, "project media shows the same imported and approved assets");
    window.findChild<QComboBox*>("assetType")->setCurrentIndex(2);
    int visibleAssets = 0;
    for (int i = 0; i < assets->count(); ++i) {
        if (assets->item(i)->isHidden()) continue;
        ++visibleAssets; assets->setCurrentItem(assets->item(i));
    }
    check(visibleAssets == 1, "video filter only shows the imported video");
    button("locateAsset")->click();
    check(button("navTimeline")->isChecked(), "media selection locates its existing timeline clip");
    auto* clips = window.findChild<QListWidget*>("timelineClips"); clips->setCurrentRow(6);
    nativeWait(350);
    auto* preview = window.findChild<QLabel*>("timelinePreview");
    const auto firstFrame = preview->pixmap().toImage();
    check(!firstFrame.isNull() && std::abs(double(firstFrame.width()) / firstFrame.height() - 16. / 9) < .02,
          "selected video previews its first frame before playback");
    const auto before = window.findChild<QSlider*>("timelinePosition")->value();
    button("timelinePlay")->click(); nativeWait(600); button("timelinePlay")->click();
    check(window.findChild<QSlider*>("timelinePosition")->value() > before,
          "real video playback advances global timeline time");
    check(!firstFrame.isNull() && preview->pixmap().toImage() != firstFrame,
          "decoded video preview frames arrive and change");
    auto* in = window.findChild<QDoubleSpinBox*>("clipIn");
    auto* out = window.findChild<QDoubleSpinBox*>("clipOut");
    in->setValue(.2); out->setValue(2.5); button("applyTrim")->click();
    check(studio->document().timeline.clips().back().inMs == 200, "trim updates the source interval");
    window.findChild<QSlider*>("timelinePosition")->setValue(19000); button("splitClip")->click();
    check(studio->document().timeline.clips().size() == 8, "split uses the actual global playhead");
    const auto movingId = clips->currentItem()->data(Qt::UserRole).toString();
    button("moveClipLeft")->click();
    check(clips->currentItem()->data(Qt::UserRole).toString() == movingId,
          "reorder preserves selected clip identity");
    button("removeClip")->click();
    check(studio->document().timeline.clips().size() == 7, "reorder and remove modify the clip model");
    bool removed = true;
    for (const auto& clip : studio->document().timeline.clips()) removed &= QString::fromStdString(clip.id) != movingId;
    check(removed, "remove after reorder deletes the intended clip");
    {
        StudioDocument cuts;
        const auto assetId = studioId();
        cuts.assets.insert(assetId, {assetId, "测试视频", QString::fromLocal8Bit(argv[1]), {}, 3000, true});
        cuts.timeline.add({studioId().toStdString(), assetId.toStdString(), "A", 0, 400});
        cuts.timeline.add({studioId().toStdString(), assetId.toStdString(), "B", 500, 1300});
        TimelinePlayer player(cuts);
        player.seek(0); nativeWait(300);
        bool mapped = false;
        player.onPosition = [&](qint64 ms) {
            if (ms >= 410 && ms <= 550)
                mapped |= std::abs(player.videoSourcePosition() - (ms + 100)) < 60;
        };
        player.play(); nativeWait(580); player.pause();
        check(mapped, "same-video clip boundary seeks across a 100 ms excluded interval");
    }
    QTemporaryDir temp;
    check(window.saveProject(temp.filePath("studio.luma")), "studio saves through the existing project action");
    const auto expected = studio->saveState();
    studio->restoreState({});
    check(window.loadProject(temp.filePath("studio.luma")) && studio->saveState() == expected,
          "project roundtrip restores conversation, assets, pipeline and timeline");
    button("navTimeline")->click(); window.resize(1440, 900); nativeWait(150);
    window.grab().save("/private/tmp/luma-studio-wide.png");
    window.resize(1100, 720); nativeWait(150); window.grab().save("/private/tmp/luma-studio-narrow.png");
    check(window.width() == 1100, "timeline layout respects the minimum window width");
    button("navEditor")->click(); nativeWait(100);
    check(window.width() == 1100 && assistant->isVisible() && window.viewport()->width() >= 320,
          "scene view keeps assistant, viewport and inspector usable at 1100 pixels");
    window.grab().save("/private/tmp/luma-project-scene-narrow.png");
    window.saveProject(temp.filePath("studio.luma"));
    button("navHome")->click();
    check(!assistant->isVisible(), "home uses the full canvas for the creation prompt");
    window.findChild<QPlainTextEdit*>("homePrompt")->setPlainText("为机器人创建三个展示镜头");
    button("homeSubmit")->click();
    check(waitFor([&] { return button("approvePipeline")->isEnabled(); }), "home prompt creates a project and starts the shared pipeline");
    check(studio->document().messages.size() == 2 && assistant->isVisible(), "home prompt appears in the persistent conversation");
    window.resize(1440, 900); nativeWait(100);
    window.grab().save("/private/tmp/luma-project-review.png");
    button("approvePipeline")->click();
    nativeWait(100);
    window.grab().save("/private/tmp/luma-project-timeline.png");
    button("navHome")->click(); nativeWait(100); window.grab().save("/private/tmp/luma-project-home.png");
    window.saveProject(temp.filePath("studio.luma"));
    const auto referencePath = temp.filePath("reference.png");
    window.viewport()->grabFramebuffer().save(referencePath);
    QTimer picker;
    picker.setInterval(80);
    QElapsedTimer pickerElapsed;
    bool pickerPrepared = false;
    QObject::connect(&picker, &QTimer::timeout, &window, [&] {
        auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        if (pickerElapsed.elapsed() > 4000) {
            check(false, "reference image selection finishes within four seconds");
            dialog->reject(); picker.stop(); return;
        }
        if (!pickerPrepared) {
            check(true, "home reference image opens a real file chooser");
            dialog->setDirectory(temp.path());
            dialog->selectFile(referencePath);
            // Populate the visible filename after QFileSystemModel starts loading.
            if (auto* name = dialog->findChild<QLineEdit*>("fileNameEdit")) name->setText(referencePath);
            pickerPrepared = true;
        } else QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
    });
    pickerElapsed.start(); picker.start();
    button("homeAttach")->click();
    picker.stop();
    check(window.findChild<QComboBox*>("homeCapability")->currentData() == "vlm",
          "adding a home reference image selects visual analysis");
    window.findChild<QPlainTextEdit*>("homePrompt")->setPlainText("按参考图规划镜头");
    button("homeSubmit")->click();
    check(waitFor([&] { return button("approvePipeline")->isEnabled(); }) && !studio->document().reviewImage.isNull(),
          "home VLM submission carries its image through the shared pipeline");
    button("approvePipeline")->click();
    window.saveProject(temp.filePath("studio.luma"));
    button("navHome")->click();
    QTcpServer server;
    check(server.listen(QHostAddress::LocalHost), "layout HTTP fixture listens");
    int createdRuns = 0;
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&] {
        auto* socket = server.nextPendingConnection();
        auto buffer = std::make_shared<QByteArray>();
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [&, socket, buffer] {
            buffer->append(socket->readAll());
            const auto separator = buffer->indexOf("\r\n\r\n");
            if (separator < 0) return;
            int length = 0;
            for (const auto& line : buffer->left(separator).toLower().split('\n'))
                if (line.startsWith("content-length:")) length = line.mid(15).trimmed().toInt();
            if (buffer->size() < separator + 4 + length) return;
            const bool capabilities = buffer->startsWith("GET /v1/capabilities ");
            QJsonObject body;
            if (capabilities) body = {{"models", QJsonArray{QJsonObject{{"id", "fixture-llm"},
                {"label", "测试文本模型"}, {"capability", "llm"}}}}, {"poll_interval_ms", 200}};
            else {
                ++createdRuns;
                body = {{"id", studioId()}, {"revision", 1}, {"status", "succeeded"},
                    {"message", "测试服务返回镜头方案"}, {"error", ""},
                    {"shots", QJsonArray{QJsonObject{{"id", studioId()}, {"title", "服务镜头"}, {"duration_ms", 1000}}}}};
            }
            const auto json = QJsonDocument(body).toJson(QJsonDocument::Compact);
            socket->write(QByteArray("HTTP/1.1 ") + (capabilities ? "200 OK" : "202 Accepted") +
                "\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                QByteArray::number(json.size()) + "\r\n\r\n" + json);
            socket->disconnectFromHost();
        });
    });
    window.findChild<QComboBox*>("homeCapability")->setCurrentIndex(0);
    window.findChild<QComboBox*>("chatCapability")->setCurrentIndex(0);
    window.findChild<QComboBox*>("chatServiceMode")->setCurrentIndex(1);
    window.findChild<QLineEdit*>("aiServiceUrl")->setText(QString("http://127.0.0.1:%1").arg(server.serverPort()));
    button("connectAiService")->click();
    check(waitFor([&] { return window.findChild<QComboBox*>("chatModel")->count() == 1; }), "HTTP model connects before home submission");
    window.findChild<QPlainTextEdit*>("homePrompt")->setPlainText("通过已连接的服务规划镜头");
    button("homeSubmit")->click();
    check(waitFor([&] { return createdRuns == 1 && button("approvePipeline")->isEnabled(); }),
          "home submission preserves the connected HTTP service across project creation");
    check(window.findChild<QTextBrowser*>("chatHistory")->toPlainText().contains("通过已连接的服务规划镜头"),
          "home HTTP prompt is retained in the conversation");
    window.saveProject(temp.filePath("studio.luma")); window.close();
    return failures ? 1 : 0;
}

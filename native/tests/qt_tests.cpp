#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
#include <luma/qt/QtViewport.hpp>
#include <luma/qt/WorkbenchWindow.hpp>
#include <stdexcept>
using namespace luma;
namespace luma {
void checkTextureLoadFailure(WorkbenchWindow&);
void checkSharedTextureProject(WorkbenchWindow&);
}
static void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
static void settle() {
    QTest::qWait(160);
    QApplication::processEvents();
}
int main(int argc, char** argv) {
    configureQtSurface();
    QApplication app(argc, argv);
    try {
        WorkbenchWindow window;
        window.show();
        settle();
        auto* view = window.viewport();
        checkTextureLoadFailure(window);
        checkSharedTextureProject(window);
        check(view->lastError().empty(), view->lastError().c_str());
        check(view->model() && view->model()->scene().renderItems().size() > 1,
              "Qt must create a real scene through our resources");
        QTemporaryDir lightingTemp;
        auto lightingProject = lightingTemp.filePath("lighting-v2.luma");
        auto lighting = view->model()->lighting();
        lighting.model = ShadingModel::BlinnPhong;
        lighting.ambientStrength = .22f;
        view->controller()->setLightingSettings(lighting);
        auto pointId = view->controller()->addPointLight({});
        auto point = std::get<PointLight>(view->model()->lights().entry(pointId).value);
        point.intensity = 2.25f;
        view->controller()->updateLight(pointId, point);
        auto post = view->model()->postProcess();
        post.enabled = true;
        post.effect = PostProcessEffect::Grayscale;
        view->controller()->setPostProcessSettings(post);
        check(window.saveProject(lightingProject), "v2 lighting project saves");
        QFile v2File(lightingProject);
        check(v2File.open(QIODevice::ReadOnly), "read v2 project");
        auto v2Root = QJsonDocument::fromJson(v2File.readAll()).object();
        check(v2Root["version"].toInt() == 2 && v2Root["lights"].isArray() &&
                  v2Root["postProcess"].isObject(),
              "v2 project stores lighting and postprocess");
        check(v2Root["objects"].toArray()[0].toObject()["baseTexture"].isNull(),
              "project texture references must be portable asset descriptions, not runtime GPU IDs");
        auto invalidTextureRoot = v2Root;
        auto invalidTextureObjects = invalidTextureRoot["objects"].toArray();
        auto invalidTextureObject = invalidTextureObjects[0].toObject();
        invalidTextureObject["baseTexture"] = QJsonValue(QJsonValue::Null);
        invalidTextureObject["secondaryTexture"] = QJsonObject{
            {"asset", "textures/checker.png"}, {"colorSpace", 1}, {"flipY", true},
            {"wrap", 0}, {"minFilter", 2}, {"magFilter", 1}, {"mipmaps", true}};
        invalidTextureObjects[0] = invalidTextureObject;
        invalidTextureRoot["objects"] = invalidTextureObjects;
        QFile invalidTextureFile(lightingTemp.filePath("invalid-texture.luma"));
        check(invalidTextureFile.open(QIODevice::WriteOnly), "write invalid texture fixture");
        invalidTextureFile.write(QJsonDocument(invalidTextureRoot).toJson());
        invalidTextureFile.close();
        const auto liveEntity = view->model()->scene().renderItems()[0].entity;
        check(!window.loadProject(invalidTextureFile.fileName()) &&
                  view->model()->scene().contains(liveEntity),
              "secondary texture without base must be rejected before replacing the scene");
        auto badLightRoot = v2Root;
        auto badLights = badLightRoot["lights"].toArray();
        auto badLight = badLights[0].toObject();
        badLight["constant"] = 0;
        badLights[0] = badLight;
        badLightRoot["lights"] = badLights;
        QFile badLightFile(lightingTemp.filePath("bad-light.luma"));
        check(badLightFile.open(QIODevice::WriteOnly), "write invalid light fixture");
        badLightFile.write(QJsonDocument(badLightRoot).toJson());
        badLightFile.close();
        check(!window.loadProject(badLightFile.fileName()) &&
                  view->model()->lights().entries().size() == 1,
              "invalid light must be rejected before changing live state");
        auto legacyRoot = v2Root;
        legacyRoot["version"] = 1;
        legacyRoot.remove("lighting");
        legacyRoot.remove("lights");
        legacyRoot.remove("postProcess");
        auto legacyPath = lightingTemp.filePath("legacy-v1.luma");
        QFile legacyFile(legacyPath);
        check(legacyFile.open(QIODevice::WriteOnly), "write v1 project");
        legacyFile.write(QJsonDocument(legacyRoot).toJson());
        legacyFile.close();
        check(window.loadProject(legacyPath), "v1 project loads");
        check(view->model()->lighting().model == ShadingModel::Unlit &&
                  view->model()->postProcess().effect == PostProcessEffect::None,
              "v1 project receives stable stage-two defaults");
        auto* objects = window.findChild<QListWidget*>("objects");
        check(objects && objects->count() > 1, "Qt object list must reflect scene");
        auto* home = window.findChild<QPushButton*>("navHome");
        auto* editor = window.findChild<QPushButton*>("navEditor");
        auto* pages = window.findChild<QStackedWidget*>("pages");
        check(home && editor && pages, "new navigation shell must exist");
        home->click();
        settle();
        check(window.findChild<QStackedWidget*>("shellPages")->currentIndex() == 0, "home navigation opens dashboard");
        editor->click();
        settle();
        check(pages->currentIndex() == 0, "editor navigation returns to live scene");
        auto* search = window.findChild<QLineEdit*>("objectSearch");
        check(search, "object search exists");
        search->setText("头部");
        settle();
        int visible = 0;
        for (int i = 0; i < objects->count(); ++i)
            visible += !objects->item(i)->isHidden();
        check(visible == 1, "search filters the actual scene objects");
        search->clear();
        objects->setCurrentRow(1);
        settle();
        auto selected = view->model()->selected();
        auto* x = window.findChild<QDoubleSpinBox*>("positionX");
        check(x != nullptr, "transform editor exists");
        x->setValue(1.25);
        settle();
        check(std::abs(view->model()->scene().transform(selected).position().x - 1.25f) < 0.001f,
              "Qt control must update CPU scene through controller");
        auto* scenes = window.findChild<QComboBox*>("presets");
        scenes->setCurrentIndex(scenes->findData("triangle"));
        settle();
        check(!view->model()->scene().contains(selected),
              "preset change must invalidate old entity IDs");
        auto image = view->grabFramebuffer();
        check(!image.isNull(), "Qt must produce framebuffer pixels");
        auto center = image.pixelColor(image.width() / 2, image.height() / 2);
        check(center.red() + center.green() + center.blue() > 200,
              "triangle center must contain rendered vertex colors");
        check(view->defaultFramebufferObject() != 0, "Qt host must render to borrowed nonzero FBO");
        auto original = image.size();
        window.resize(1100, 720);
        settle();
        image = view->grabFramebuffer();
        check(image.size() != original, "Qt resize must recreate output dimensions");
        scenes->setCurrentIndex(scenes->findData("camera"));
        settle();
        auto* modes = window.findChild<QComboBox*>("cameraMode");
        modes->setCurrentIndex(1);
        settle();
        scenes->setCurrentIndex(scenes->findData("lighting"));
        // Clear the shared-material draft exercised by the project regression.
        view->loadPreset("lighting");
        settle();
        check(view->model()->lighting().model == ShadingModel::BlinnPhong &&
                  view->model()->lights().entries().size() == 3,
              "lighting preset must expose the reusable three-light laboratory");
        auto* lightList = window.findChild<QListWidget*>("lightList");
        auto* addLight = window.findChild<QPushButton*>("addLight");
        auto* removeLight = window.findChild<QPushButton*>("removeLight");
        auto* lightKind = window.findChild<QComboBox*>("lightKind");
        auto* lightIntensity = window.findChild<QDoubleSpinBox*>("lightIntensity");
        check(lightList && addLight && removeLight && lightKind && lightIntensity,
              "light editor must support add/select/edit/remove");
        lightKind->setCurrentIndex(1);
        addLight->click();
        settle();
        check(view->model()->lights().entries().size() == 4, "add point light through controller");
        lightIntensity->setValue(2.8);
        settle();
        auto lightId = LightId{lightList->currentItem()->data(Qt::UserRole).toUInt()};
        check(std::abs(std::get<PointLight>(view->model()->lights().entry(lightId).value).quadratic -
                       .032f) < .000001f, "intensity edit must preserve exact untouched attenuation");
        check(std::abs(std::get<PointLight>(view->model()->lights().entry(lightId).value).intensity -
                       2.8f) < .001f, "selected light edit reaches the model");
        removeLight->click();
        settle();
        check(view->model()->lights().entries().size() == 3, "remove selected light");
        auto* lightingModel = window.findChild<QComboBox*>("lightingModel");
        auto* ambient = window.findChild<QDoubleSpinBox*>("ambientStrength");
        check(lightingModel && ambient, "lighting controls must be part of the MVC view");
        lightingModel->setCurrentIndex(0);
        ambient->setValue(.3);
        settle();
        check(view->model()->lighting().model == ShadingModel::Unlit,
              "lighting model control must update controller state");
        lightingModel->setCurrentIndex(1);
        auto* postEnabled = window.findChild<QCheckBox*>("postEnabled");
        auto* postEffect = window.findChild<QComboBox*>("postEffect");
        check(postEnabled && postEffect, "post-process controls must be part of the MVC view");
        postEnabled->setChecked(true);
        postEffect->setCurrentIndex(static_cast<int>(PostProcessEffect::Grayscale));
        settle();
        check(view->model()->postProcess().enabled &&
                  view->model()->postProcess().effect == PostProcessEffect::Grayscale,
              "post-process controls must update controller state");
        ambient->setValue(.08);
        postEnabled->setChecked(false);
        window.resize(1440, 900);
        window.findChild<QPushButton*>("workflowStep2")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-stage-two-lighting.png"), "lighting screenshot");
        postEnabled->setChecked(true);
        window.findChild<QPushButton*>("workflowStep5")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-stage-two-postprocess.png"), "postprocess screenshot");
        postEnabled->setChecked(false);
        window.resize(1100, 720);
        window.findChild<QPushButton*>("workflowStep2")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-stage-two-narrow.png"), "narrow lighting screenshot");
        window.resize(1440, 900);
        window.findChild<QPushButton*>("workflowStep1")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-stage-two-material.png"), "specular material screenshot");
        auto* shininess = window.findChild<QDoubleSpinBox*>("shininess");
        check(shininess, "specular material control exists");
        shininess->setValue(128);
        check(view->model()->materials().material(view->model()->selectedMaterial()).shininess == 128,
              "shininess control edits the material through MVC");
        settle();
        modes->setCurrentIndex(1);
        for (int i = 0; i < 6; ++i) {
            window.findChild<QPushButton*>(QString("workflowStep%1").arg(i))->click();
            window.findChild<QPushButton*>("completeStep")->click();
        }
        auto* reviewStep = window.findChild<QPushButton*>("workflowStep4");
        check(reviewStep->text().startsWith("✓"), "review can be approved");
        window.raise();
        window.activateWindow();
        check(QTest::qWaitForWindowActive(&window), "test window must be active before keyboard input");
        view->setFocus();
        settle();
        check(view->hasFocus(), "viewport must hold keyboard focus before WASD input");
        auto before = view->model()->camera().eye();
        QTest::keyPress(view, Qt::Key_W);
        QTest::qWait(100);
        QTest::keyRelease(view, Qt::Key_W);
        check(glm::distance(before, view->model()->camera().eye()) > 0.01f,
              "Qt WASD events must drive the reusable camera controls");
        check(!reviewStep->text().startsWith("✓"),
              "camera movement must invalidate approved rendering review");
        view->loadPreset("missing-preset");
        check(!view->lastError().empty(), "invalid preset must surface error without unwinding Qt");
        view->loadPreset("quad");
        settle();
        check(view->lastError().empty(), "valid preset must recover from failure");
        check(scenes->currentData().toString() == "quad",
              "programmatic scene switch must synchronize toolbar");
        check(view->savePng("/private/tmp/luma-qt-viewport-test.png"), "PNG capture must succeed");
        auto* captureShot = window.findChild<QPushButton*>("captureShot");
        auto* shots = window.findChild<QListWidget*>("shots");
        check(captureShot && shots, "camera snapshots UI exists");
        captureShot->click();
        settle();
        check(shots->count() == 1, "snapshot captures a real camera view");
        auto shotEye = view->model()->camera().eye();
        view->controller()->setCameraPose({3, 2, 5}, {0, 0, 0});
        auto* restore = window.findChild<QPushButton*>("restoreShot");
        check(restore, "snapshot restore action exists");
        restore->click();
        settle();
        check(glm::distance(shotEye, view->model()->camera().eye()) < 0.001,
              "snapshot restore recovers camera pose");
        QTemporaryDir temp;
        auto projectPath = temp.filePath("work.luma");
        check(window.saveProject(projectPath), "project must save atomically");
        QFile quadFile(projectPath);
        check(quadFile.open(QIODevice::ReadOnly), "read quad fixture");
        auto oldQuad = QJsonDocument::fromJson(quadFile.readAll()).object();
        quadFile.close();
        oldQuad["version"] = 1;
        QFile oldQuadFile(temp.filePath("quad-v1.luma"));
        check(oldQuadFile.open(QIODevice::WriteOnly), "write old quad fixture");
        oldQuadFile.write(QJsonDocument(oldQuad).toJson());
        oldQuadFile.close();
        check(window.loadProject(oldQuadFile.fileName()), "v1 textured project loads");
        check(view->model()->materials().material(view->model()->selectedMaterial()).baseColorTexture.has_value(),
              "v1 migration must preserve the texture rebuilt by the preset");
        auto originalX = view->model()->scene().transform(view->model()->selected()).position().x;
        x->setValue(7);
        settle();
        check(window.loadProject(projectPath), "project must load");
        settle();
        check(std::abs(view->model()->scene().transform(view->model()->selected()).position().x -
                       originalX) < 0.001,
              "project restore must recover object transforms");
        QFile corrupt(temp.filePath("bad.luma"));
        check(corrupt.open(QIODevice::WriteOnly), "create corrupt fixture");
        corrupt.write("{\"version\":99}");
        corrupt.close();
        auto presetBefore = view->currentPreset();
        check(!window.loadProject(corrupt.fileName()), "invalid project must be rejected");
        check(view->currentPreset() == presetBefore, "bad document leaves current scene intact");
        // A malformed document with a valid version must also leave live data alone.
        QFile valid(projectPath);
        check(valid.open(QIODevice::ReadOnly), "open saved fixture");
        auto badRoot = QJsonDocument::fromJson(valid.readAll()).object();
        auto badItems = badRoot["objects"].toArray();
        auto badObject = badItems[0].toObject();
        badObject["scale"] = QJsonArray{0, 1, 1};
        badItems[0] = badObject;
        badRoot["objects"] = badItems;
        check(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate), "rewrite corrupt fixture");
        corrupt.write(QJsonDocument(badRoot).toJson());
        corrupt.close();
        check(!window.loadProject(corrupt.fileName()),
              "invalid transform document rejected before scene load");
        check(view->currentPreset() == presetBefore, "invalid transform preserves live preset");
        scenes->setCurrentIndex(scenes->findData("robot"));
        settle();
        const auto& items = view->model()->scene().renderItems();
        // Robot arms initially share a material. Break one link, save and restore.
        auto armL = items[3].entity;
        view->controller()->select(armL);
        view->controller()->cloneSelectedMaterial();
        auto armMaterial = view->model()->materials().material(view->model()->selectedMaterial());
        armMaterial.baseColor = {.2f, .3f, .4f, 1};
        view->controller()->setMaterial(armMaterial);
        view->controller()->setCameraPose({2, 3, 6}, {0, .2f, 0});
        auto settings = view->model()->options();
        settings.depth = false;
        settings.outline = false;
        view->controller()->setRenderOptions(settings);
        check(window.saveProject(projectPath), "save independent material project");
        check(window.loadProject(projectPath), "load independent material project");
        settle();
        const auto& loaded = view->model()->scene().renderItems();
        check(loaded[3].material.value != loaded[6].material.value,
              "independent arm material survives project roundtrip");
        check(loaded[4].material.value == loaded[7].material.value,
              "shared feet material survives project roundtrip");
        check(view->model()->materials().material(loaded[3].material).baseColor ==
                  armMaterial.baseColor,
              "material values survive roundtrip");
        check(glm::distance(view->model()->camera().eye(), glm::vec3(2, 3, 6)) < .001,
              "camera pose survives project roundtrip");
        check(!window.findChild<QCheckBox*>("renderState0")->isChecked(),
              "state controls reflect loaded options");
        auto* toggle = window.findChild<QCheckBox*>("renderState0");
        toggle->click();
        toggle->click();
        check(window.saveProject(projectPath), "save final test edits");
        check(window.exportImage(temp.filePath("output.png")), "real PNG export must succeed");
        check(!window.exportImage(temp.filePath("missing/output.png")),
              "failed export reports failure");
        check(window.grab().save("/private/tmp/luma-qt-window-test.png"),
              "window screenshot must save");
        // Keep representative full-size and narrow captures for visual inspection.
        window.resize(1440, 900);
        settle();
        view->loadPreset("robot");
        view->controller()->setRenderOptions(RenderOptions{});
        settle();
        objects->setCurrentRow(1);
        window.findChild<QPushButton*>("workflowStep1")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-workflow-material.png"),
              "save material page screenshot");
        captureShot->click();
        settle();
        window.findChild<QPushButton*>("navShots")->click();
        settle();
        check(window.grab().save("/private/tmp/luma-workflow-shots.png"),
              "save snapshots screenshot");
        home->click();
        settle();
        check(window.grab().save("/private/tmp/luma-workflow-home-clean.png"),
              "save dashboard screenshot");
        editor->click();
        window.resize(1100, 720);
        settle();
        check(view->width() >= 320, "narrow layout preserves usable viewport");
        check(window.grab().save("/private/tmp/luma-workflow-narrow.png"),
              "save narrow screenshot");
        home->click();
        settle();
        check(window.grab().save("/private/tmp/luma-workflow-home-narrow.png"),
              "save narrow dashboard screenshot");
        check(window.saveProject(projectPath), "save before closing test window");
        window.close();
        settle();
        std::cout << "PASS Qt: navigation, search, MVC edits, FBO pixels, resize, WASD "
                     "invalidation, snapshots, project roundtrip, shared materials, bad documents, "
                     "PNG success/failure, close\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL Qt: " << e.what() << '\n';
        return 1;
    }
}

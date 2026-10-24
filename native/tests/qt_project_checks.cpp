#include <glad/gl.h>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/app/Application.hpp>
#include <stdexcept>

namespace luma {
void checkTextureLoadFailure(WorkbenchWindow& window) {
    auto* view = window.viewport();
    view->loadPreset("triangle");
    auto document = view->captureProject("纹理故障回归");
    document.objects[0].baseTexture = ProjectTexture{"textures/cutout.png", {}};
    QTemporaryDir directory;
    const auto project = directory.filePath("missing-asset.luma");
    document.write(project);
    view->loadPreset("robot");
    const auto live = view->model()->scene().renderItems()[0].entity;
    const auto source = QString::fromStdString((assetRoot() / "textures/cutout.png").string());
    const auto backup = directory.filePath("cutout.png");
    struct Restore {
        QString source, backup;
        ~Restore() { QFile::rename(backup, source); }
    } restore{source, backup};
    if (!QFile::rename(source, backup))
        throw std::runtime_error("cannot stage texture I/O failure");
    if (window.loadProject(project) || !view->model()->scene().contains(live) ||
        !view->lastError().empty())
        throw std::runtime_error("texture I/O failure must preserve the live workspace");
}
void checkSharedTextureProject(WorkbenchWindow& window) {
    auto* view = window.viewport();
    view->loadPreset("lighting");
    QTest::qWait(160);
    const auto items = view->model()->scene().renderItems();
    const auto gold = items[2].material;
    const auto original = view->model()->materials().material(gold);
    if (!original.baseColorTexture || !original.specularTexture)
        throw std::runtime_error("gold fixture must have both maps");
    view->controller()->select(items[0].entity);
    view->controller()->assignMaterial(gold);
    QTemporaryDir directory;
    const auto path = directory.filePath("shared-textures.luma");
    if (!window.saveProject(path) || !window.loadProject(path))
        throw std::runtime_error("shared textured material must save and load");
    const auto& restored = view->model()->scene().renderItems();
    const auto material = view->model()->materials().material(restored[0].material);
    if (restored[0].material.value != restored[2].material.value ||
        !material.baseColorTexture || material.baseColorTexture->value != original.baseColorTexture->value ||
        !material.specularTexture || material.specularTexture->value != original.specularTexture->value)
        throw std::runtime_error("sharing gold with ground must preserve both texture assets");
    view->loadPreset("robot");
    QTest::qWait(160);
}
} // namespace luma

#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QStatusBar>
#include <cmath>
#include <luma/qt/WorkbenchTheme.hpp>
#include <luma/qt/WorkbenchWindow.hpp>
#include <luma/qt/MediaGallery.hpp>
#include <luma/qt/StudioWorkspace.hpp>
namespace luma {
void WorkbenchWindow::notify(const QString& text, bool error) {
    statusBar()->showMessage(text, error ? 14000 : 7000);
    if (error)
        addTask(text, false);
}
void WorkbenchWindow::addTask(const QString& message, bool success) {
    auto* item = new QListWidgetItem(
        QString("%1  %2   %3")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), success ? "✓" : "!", message));
    if (!success)
        item->setForeground(QColor("#efa898"));
    tasks_->insertItem(0, item);
    while (tasks_->count() > 100)
        delete tasks_->takeItem(tasks_->count() - 1);
}
void WorkbenchWindow::switchPreset(const std::string& name) {
    if (!viewport_->model()) {
        viewport_->loadPreset(name);
        return;
    }
    drafts_.insert_or_assign(viewport_->currentPreset(),
                             Draft{viewport_->captureProject(projectName_->text()), projectPath_, workflow_, dirty_, studio_->saveState()});
    restoring_ = true;
    try {
        auto found = drafts_.find(name);
        if (found != drafts_.end()) {
            viewport_->applyProject(found->second.document);
            projectName_->setText(found->second.document.name);
            projectPath_ = found->second.path;
            workflow_ = found->second.workflow;
            dirty_ = found->second.dirty;
            studio_->restoreState(found->second.studio);
        } else {
            viewport_->loadPreset(name);
            if (!viewport_->lastError().empty()) throw std::runtime_error(viewport_->lastError());
            projectName_->setText(presets_->currentText() + " · 新作品");
            viewport_->controller()->setStyle(StyleKind::Basic);
            viewport_->controller()->setRenderOptions(RenderOptions{});
            projectPath_.clear();
            workflow_.reset();
            dirty_ = true;
            studio_->restoreState({});
        }
        saveState_->setText(dirty_ ? "● 会话草稿"
                                   : projectPath_.isEmpty() ? "本地创作" : "✓ 已保存");
        rebuildObjects();
        refreshInspector();
        selectStage(int(workflow_.current()));
    } catch (const std::exception& e) {
        notify(QString("场景恢复失败：") + e.what(), true);
    }
    restoring_ = false;
    refreshWorkflow();
}
bool WorkbenchWindow::saveProject(const QString& path) {
    if (!viewport_->model())
        return false;
    try {
        auto title = projectName_->text().trimmed();
        if (title.isEmpty())
            throw std::runtime_error("请先填写作品名称");
        auto document =
            viewport_->captureProject(title);
        document.studio = studio_->saveState();
        document.write(path);
        projectPath_ = path;
        dirty_ = false;
        saveState_->setText("✓ 已保存");
        addTask("工程已保存 · " + QFileInfo(path).fileName(), true);
        notify("已保存工程：" + path);
        return true;
    } catch (const std::exception& e) {
        notify(QString("保存失败：") + e.what(), true);
        return false;
    }
}
bool WorkbenchWindow::loadProject(const QString& path) {
    if (!viewport_->controller())
        return false;
    try {
        // Validate all CPU values before replacing a running scene.
        auto document = ProjectDocument::read(path);
        drafts_.insert_or_assign(viewport_->currentPreset(),
                                 Draft{viewport_->captureProject(projectName_->text()), projectPath_, workflow_, dirty_, studio_->saveState()});
        restoring_ = true;
        viewport_->applyProject(document);
        studio_->restoreState(document.studio);
        projectName_->setText(document.name);
        projectPath_ = path;
        dirty_ = false;
        restoring_ = false;
        workflow_.reset();
        shotData_.clear();
        shotGallery_->clear();
        rebuildObjects();
        selectStage(0);
        saveState_->setText("✓ 已载入");
        showPage(1);
        addTask("打开工程 · " + document.name, true);
        notify("工程已载入，可以继续编辑");
        return true;
    } catch (const std::exception& e) {
        restoring_ = false;
        notify(QString("打开失败：") + e.what(), true);
        return false;
    }
}
void WorkbenchWindow::chooseColor() {
    if (!viewport_->model() ||
        !viewport_->model()->scene().contains(viewport_->model()->selected()))
        return;
    auto mat = viewport_->model()->materials().material(viewport_->model()->selectedMaterial());
    auto srgb = [](float v) {
        return v <= .0031308f ? 12.92f * v : 1.055f * std::pow(v, 1 / 2.4f) - .055f;
    };
    auto initial = QColor::fromRgbF(srgb(mat.baseColor.r), srgb(mat.baseColor.g),
                                    srgb(mat.baseColor.b), mat.baseColor.a);
    auto chosen =
        QColorDialog::getColor(initial, this, "选择材质颜色", QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid())
        return;
    auto linear = [](float v) {
        return v <= .04045f ? v / 12.92f : std::pow((v + .055f) / 1.055f, 2.4f);
    };
    mat.baseColor = {linear(chosen.redF()), linear(chosen.greenF()), linear(chosen.blueF()),
                     chosen.alphaF()};
    if (invoke([mat](auto& c) { c.setMaterial(mat); }))
        changed(WorkflowStage::Material);
    refreshInspector();
}
void WorkbenchWindow::showHelp() {
    QMessageBox dialog(QMessageBox::Information, "光场工坊 · 使用说明",
        "1. 首页输入想法、添加参考图，或选择模板与打开 .luma 工程。\n2. 场景：在对象标签选择对象，在属性标签调整。\n"
        "3. 材质：底色、纹理、高光颜色与光泽度。\n4. 光照：选择着色模型，添加和编辑灯光。\n"
        "5. 镜头：环绕、漫游、保存快照。\n6. 检查：深度、透明、剔除与轮廓。\n"
        "7. 后处理：反相、灰度、锐化、模糊。\n8. 导出：PNG 或可编辑工程。\n"
        "9. 项目左侧助手常驻，顶部切换方案、3D 场景、镜头、媒体和时间线。\n"
        "10. 方案审核后入轨；时间线可导入视频、播放、裁剪、拆分与重排。\n"
        "右上角模型服务统一配置：默认本地演示不调用模型，HTTP 连接自己的服务；暂不编码导出视频。\n\n"
        "第一人称：右键拖动 + WASD，Q/E 升降。\n⌘S 保存，⌘O 打开。\n\n"
        "v2 工程保存场景、灯光、机位与编排数据，兼容 v1；token 不写入工程。\n"
        "镜头快照、草稿和操作记录仅在本次会话保留。\n"
        "当前为第二阶段 Blinn-Phong；Toon、PBR、阴影随后续课程加入。",
        QMessageBox::NoButton, this);
    dialog.addButton("知道了", QMessageBox::AcceptRole);
    dialog.exec();
}
} // namespace luma

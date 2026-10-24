#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QWheelEvent>
#include <algorithm>
#include <iostream>
#include <luma/app/Application.hpp>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/qt/QtViewport.hpp>
#include <luma/qt/ProjectDocument.hpp>
#include <luma/render/Renderer.hpp>
namespace luma {
void configureQtSurface() {
    QSurfaceFormat f;
    f.setRenderableType(QSurfaceFormat::OpenGL);
#ifdef __APPLE__
    f.setVersion(4, 1);
#else
    f.setVersion(3, 3);
#endif
    f.setProfile(QSurfaceFormat::CoreProfile);
    f.setDepthBufferSize(24);
    f.setStencilBufferSize(8);
    f.setSamples(0);
    QSurfaceFormat::setDefaultFormat(f);
}
struct QtViewport::Session {
    NativeResourceFactory resources;
    AssetStore assets{resources};
    Scene scene;
    Camera camera;
    Camera presetCamera;
    CameraControls controls;
    EditorModel model{scene, assets.materials(), camera, controls};
    EditorController controller{model};
    Renderer renderer{resources};
    std::unique_ptr<SceneLayer> layer;
    bool created = false;
    SceneContext context() {
        return {assets, scene, camera, controls, model.lights(), model.lighting(),
                model.postProcess()};
    }
    ~Session() {
        if (created && layer) {
            auto ctx = context();
            try {
                layer->onDestroy(ctx);
            } catch (...) {
                std::cerr << "Qt scene cleanup failed\n";
            }
        }
    }
};
QtViewport::QtViewport(QWidget* parent) : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(320, 280);
    setTextureFormat(GL_RGBA8);
    timer_.setInterval(16);
    connect(&timer_, &QTimer::timeout, this, [this] { update(); });
    timer_.start();
    clock_.start();
}
QtViewport::~QtViewport() {
    timer_.stop();
    disconnect(contextConnection_);
    cleanup();
}
const EditorModel* QtViewport::model() const {
    return session_ ? &session_->model : nullptr;
}
EditorController* QtViewport::controller() {
    return session_ ? &session_->controller : nullptr;
}
void QtViewport::resetCamera() {
    if (!session_) return;
    const auto& pose = session_->presetCamera;
    session_->controller.setCameraPose(pose.eye(), pose.target(), pose.up());
    if (onCameraChanged) onCameraChanged();
    update();
}
ProjectDocument QtViewport::captureProject(const QString& title) const {
    if (!session_) throw std::logic_error("视口尚未初始化");
    return ProjectDocument::capture(session_->model, session_->assets, preset_, title);
}
void QtViewport::applyProject(const ProjectDocument& document) {
    if (!session_) throw std::logic_error("视口尚未初始化");
    const auto previousError = error_;
    replaceScene(document.preset, &document);
    if (!error_.empty()) {
        const auto failure = error_;
        error_ = previousError;
        throw std::runtime_error(failure);
    }
}
void QtViewport::fail(const std::string& message) {
    error_ = message;
    if (onError)
        onError(QString::fromStdString(message));
    std::cerr << "QtViewport: " << message << '\n';
}
void QtViewport::cleanup() {
    if (!session_)
        return;
    makeCurrent();
    session_.reset();
    doneCurrent();
}
void QtViewport::initializeGL() {
    try {
        if (!gladLoadGL([](const char* name) -> GLADapiproc {
                return QOpenGLContext::currentContext()->getProcAddress(name);
            }))
            throw std::runtime_error("GLAD 初始化失败");
        if (!GLAD_GL_VERSION_3_3)
            throw std::runtime_error("需要 OpenGL 3.3 Core");
        std::cout << "Qt OpenGL: " << glGetString(GL_VERSION) << " / " << glGetString(GL_RENDERER)
                  << '\n';
        disconnect(contextConnection_);
        contextConnection_ = connect(
            context(), &QOpenGLContext::aboutToBeDestroyed, this, [this] { cleanup(); },
            Qt::DirectConnection);
        session_ = std::make_unique<Session>();
        session_->renderer.initialize(assetRoot() / "shaders");
        loadPreset(preset_);
    } catch (const std::exception& e) {
        fail(e.what());
    } catch (...) {
        fail("未知初始化错误");
    }
}
void QtViewport::loadPreset(const std::string& name) {
    replaceScene(name, nullptr);
}
void QtViewport::replaceScene(const std::string& name, const ProjectDocument* document) {
    if (!session_) {
        preset_ = name;
        return;
    }
    const bool acquired = QOpenGLContext::currentContext() != context();
    if (acquired)
        makeCurrent();
    try {
        auto next = makePreset(name);
        // Build a replacement first; invalid presets leave the current scene usable.
        auto& s = *session_;
        Scene candidate = s.scene;
        candidate.clear();
        Camera camera;
        CameraControls controls;
        EditorModel candidateModel{candidate, s.assets.materials(), camera, controls};
        EditorController candidateController{candidateModel};
        SceneContext ctx{s.assets, candidate, camera, controls, candidateModel.lights(),
                         candidateModel.lighting(), candidateModel.postProcess()};
        next->onCreate(ctx);
        const auto presetCamera = camera;
        // Resolve assets and apply every edit while the live scene is untouched.
        try {
            if (document) document->apply(candidateController, candidateModel, s.assets);
        } catch (...) {
            try { next->onDestroy(ctx); }
            catch (...) { std::cerr << "Qt candidate cleanup failed\n"; }
            throw;
        }
        if (s.created) {
            auto previous = s.context();
            s.layer->onDestroy(previous);
            s.created = false;
        }
        s.scene = std::move(candidate);
        s.camera = camera;
        s.presetCamera = presetCamera;
        s.controls = controls;
        s.model.lights() = candidateModel.lights();
        s.model.lighting() = candidateModel.lighting();
        s.model.postProcess() = candidateModel.postProcess();
        s.layer = std::move(next);
        s.created = true;
        s.controller.clearSelection();
        if (document) {
            s.controller.setStyle(candidateModel.style());
            s.controller.setRenderOptions(candidateModel.options());
        }
        if (!s.scene.renderItems().empty())
            s.controller.select(s.scene.renderItems().front().entity);
        preset_ = name;
        error_.clear();
        input_.reset();
        clock_.restart();
        elapsed_ = 0;
        timer_.start();
        if (onSceneChanged)
            onSceneChanged();
    } catch (const std::exception& e) {
        fail(e.what());
    } catch (...) {
        fail("未知场景错误");
    }
    if (acquired)
        doneCurrent();
    update();
}
void QtViewport::paintGL() {
    if (!session_)
        return;
    try {
        auto& s = *session_;
        float dt = std::clamp(clock_.restart() / 1000.0f, 0.0f, 0.05f);
        elapsed_ += dt;
        auto in = input_.take();
        in.focused = hasFocus();
        const auto oldEye = s.camera.eye(), oldTarget = s.camera.target(), oldUp = s.camera.up();
        s.controls.update(s.camera, in, dt);
        if (onCameraChanged &&
            (s.camera.eye() != oldEye || s.camera.target() != oldTarget || s.camera.up() != oldUp))
            onCameraChanged();
        auto ctx = s.context();
        if (s.created)
            s.layer->onUpdate(ctx, {dt, elapsed_, in});
        s.controller.reconcileSelection();
        int w = qRound(width() * devicePixelRatioF()), h = qRound(height() * devicePixelRatioF());
        if (w <= 0 || h <= 0)
            return;
        s.camera.setAspect(float(w) / h);
        s.renderer.options() = s.model.options();
        s.renderer.setStyle(s.model.style());
        s.renderer.setLighting(s.model.lighting(), s.model.lights());
        s.renderer.setPostProcess(s.model.postProcess());
        s.renderer.render(s.scene, s.assets, s.camera, {defaultFramebufferObject(), w, h, false});
    } catch (const std::exception& e) {
        timer_.stop();
        fail(e.what());
    } catch (...) {
        timer_.stop();
        fail("未知渲染错误");
    }
}
bool QtViewport::savePng(const QString& path) {
    if (!isValid())
        return false;
    const auto image = grabFramebuffer();
    return !image.isNull() && image.save(path, "PNG");
}
void QtViewport::keyPressEvent(QKeyEvent* e) {
    if (!e->isAutoRepeat())
        input_.key(e->key(), true);
    e->accept();
}
void QtViewport::keyReleaseEvent(QKeyEvent* e) {
    if (!e->isAutoRepeat())
        input_.key(e->key(), false);
    e->accept();
}
void QtViewport::mouseMoveEvent(QMouseEvent* e) {
    input_.pointer(e->position(), e->buttons() & Qt::LeftButton, e->buttons() & Qt::RightButton);
    e->accept();
}
void QtViewport::mousePressEvent(QMouseEvent* e) {
    setFocus();
    input_.reset();
    mouseMoveEvent(e);
}
void QtViewport::mouseReleaseEvent(QMouseEvent* e) {
    mouseMoveEvent(e);
}
void QtViewport::wheelEvent(QWheelEvent* e) {
    input_.wheel(e->angleDelta().y() / 120.0);
    e->accept();
}
void QtViewport::focusOutEvent(QFocusEvent* e) {
    input_.reset();
    QOpenGLWidget::focusOutEvent(e);
}
} // namespace luma

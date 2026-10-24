#pragma once
// GLAD must precede Qt's platform OpenGL headers.
#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTimer>
#include <functional>
#include <luma/editor/EditorController.hpp>
#include <luma/qt/QtInput.hpp>
#include <memory>
namespace luma {
void configureQtSurface();
struct ProjectDocument;
class QtViewport final : public QOpenGLWidget {
  public:
    explicit QtViewport(QWidget* parent = nullptr);
    ~QtViewport() override;
    const EditorModel* model() const;
    EditorController* controller();
    void loadPreset(const std::string& name);
    void resetCamera();
    ProjectDocument captureProject(const QString& title) const;
    void applyProject(const ProjectDocument&);
    bool savePng(const QString& path);
    const std::string& lastError() const {
        return error_;
    }
    const std::string& currentPreset() const {
        return preset_;
    }
    std::function<void()> onSceneChanged;
    std::function<void()> onCameraChanged;
    std::function<void(const QString&)> onError;

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int, int) override {}
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void focusOutEvent(QFocusEvent*) override;

  private:
    struct Session;
    std::unique_ptr<Session> session_;
    QtInput input_;
    QTimer timer_;
    QElapsedTimer clock_;
    double elapsed_ = 0;
    std::string preset_ = "robot", error_;
    QMetaObject::Connection contextConnection_;
    void replaceScene(const std::string&, const ProjectDocument*);
    void cleanup();
    void fail(const std::string& message);
};
} // namespace luma

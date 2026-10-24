#pragma once
#include <glad/gl.h> // Must precede Qt platform OpenGL headers.
#include <QImage>
#include <QMainWindow>
#include <array>
#include <luma/editor/WorkflowState.hpp>
#include <luma/qt/ProjectDocument.hpp>
#include <luma/qt/QtViewport.hpp>
#include <map>
#include <vector>
class QListWidget;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QStackedWidget;
class QDockWidget;
namespace luma {
class MediaGallery;
class StudioWorkspace;
class WorkbenchWindow final : public QMainWindow {
  public:
    WorkbenchWindow();
    QtViewport* viewport() const {
        return viewport_;
    }
    bool saveProject(const QString& path);
    bool loadProject(const QString& path);
    bool exportImage(const QString& path);
    void showPage(int index);

  private:
    struct Shot {
        int id;
        QString name;
        glm::vec3 eye, target, up;
        CameraMode mode;
        QImage image;
        std::string preset;
    };
    QtViewport* viewport_ = nullptr;
    QListWidget *objects_ = nullptr, *tasks_ = nullptr;
    QWidget* inspector_ = nullptr;
    QStackedWidget *pages_ = nullptr, *shellPages_ = nullptr, *stagePages_ = nullptr;
    QWidget* projectTabs_ = nullptr;
    QLineEdit *projectName_ = nullptr, *search_ = nullptr;
    QComboBox *presets_ = nullptr, *modes_ = nullptr, *style_ = nullptr;
    std::array<QDoubleSpinBox*, 3> position_{}, rotation_{}, scale_{};
    std::array<QDoubleSpinBox*, 4> color_{};
    std::array<QDoubleSpinBox*, 3> specularColor_{};
    std::array<QCheckBox*, 4> states_{};
    std::array<QPushButton*, 7> steps_{};
    std::array<QPushButton*, 6> nav_{};
    QDoubleSpinBox* mix_ = nullptr;
    QDoubleSpinBox* shininess_ = nullptr;
    QComboBox* alpha_ = nullptr;
    QCheckBox* doubleSided_ = nullptr;
    QComboBox *lightingModel_ = nullptr, *postEffect_ = nullptr;
    QDoubleSpinBox *ambient_ = nullptr, *postIntensity_ = nullptr, *postRadius_ = nullptr;
    QCheckBox *normalViz_ = nullptr, *postEnabled_ = nullptr;
    QListWidget* lightList_ = nullptr;
    QComboBox* lightKind_ = nullptr;
    QWidget *lightFields_ = nullptr, *lightPositionRow_ = nullptr,
            *lightDirectionRow_ = nullptr, *lightAttenuationRow_ = nullptr,
            *lightConeRow_ = nullptr;
    QCheckBox* lightEnabled_ = nullptr;
    QDoubleSpinBox* lightIntensity_ = nullptr;
    std::array<QDoubleSpinBox*, 3> lightPosition_{}, lightDirection_{}, lightColor_{},
                                 lightAttenuation_{};
    std::array<QDoubleSpinBox*, 2> lightCone_{};
    QLabel *materialInfo_ = nullptr, *selectionInfo_ = nullptr, *sceneInfo_ = nullptr,
           *stageTitle_ = nullptr, *stageHelp_ = nullptr, *saveState_ = nullptr,
           *cameraInfo_ = nullptr,
           *reviewInfo_ = nullptr, *homePreview_ = nullptr, *lightingInfo_ = nullptr,
           *postInfo_ = nullptr;
    QPushButton *colorSwatch_ = nullptr, *next_ = nullptr, *previous_ = nullptr;
    QDockWidget* taskDock_ = nullptr;
    WorkflowState workflow_;
    std::vector<Shot> shotData_;
    MediaGallery *shotGallery_ = nullptr, *exportGallery_ = nullptr;
    StudioWorkspace* studio_ = nullptr;
    int nextMediaId_ = 1;
    struct Draft {
        ProjectDocument document;
        QString path;
        WorkflowState workflow;
        bool dirty;
        QJsonObject studio;
    };
    std::map<std::string, Draft> drafts_;
    QString projectPath_;
    bool refreshing_ = false, restoring_ = false, dirty_ = false;
    QWidget* buildHome();
    void buildShell();
    QWidget* buildEditor();
    QWidget* buildShots();
    QWidget* buildExports();
    QWidget* buildStudio();
    QWidget* buildStagePage(int index);
    QWidget* buildLightingPage();
    QWidget* buildPostProcessPage();
    void refreshLighting(bool rebuildList = true);
    void addLight();
    void removeLight();
    enum class LightField { Enabled, Intensity, Color, Position, Direction, Attenuation, Cone };
    void editLight(LightField, int component = 0);
    void rebuildObjects();
    void filterObjects();
    void refreshInspector();
    void refreshWorkflow();
    void selectStage(int index);
    void completeStage();
    void editTransform(int field, int component);
    void editMaterial(int field, int component = 0);
    void editLighting(int field);
    void editPostProcess(int field);
    bool invoke(const std::function<void(EditorController&)>& action);
    void changed(WorkflowStage stage);
    void switchPreset(const std::string& name);
    void captureShot();
    void restoreShot();
    bool chooseSaveProject(bool saveAs = false);
    void chooseLoadProject();
    bool confirmLeave();
    bool createProject(const std::string& preset);
    void buildProjectMenu(QPushButton* button);
    void chooseExport();
    void chooseColor();
    void notify(const QString& text, bool error = false);
    void addTask(const QString& message, bool success);
    void updateHomePreview();
    void updateCameraInfo();
    void showHelp();

  protected:
    void closeEvent(QCloseEvent*) override;
};
} // namespace luma

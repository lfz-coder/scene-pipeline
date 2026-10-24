#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>
#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <luma/editor/EditorUI.hpp>
#include <stdexcept>
namespace luma {
namespace {
float toSrgb(float c) {
    return c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1 / 2.4f) - 0.055f;
}
float toLinear(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
} // namespace
EditorUI::EditorUI(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 9;
    style.FrameRounding = 4;
    style.Colors[ImGuiCol_WindowBg] = {0.045f, 0.055f, 0.085f, 0.96f};
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        throw std::runtime_error("ImGui GLFW initialization failed");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("ImGui GL initialization failed");
    }
}
EditorUI::~EditorUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void EditorUI::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
bool EditorUI::captureMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}
bool EditorUI::captureKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}
std::string EditorUI::inspect(const EditorModel& model, EditorController& controller, double dt) {
    std::string preset;
    ImGui::SetNextWindowPos({14, 14}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({340, 748}, ImGuiCond_FirstUseEver);
    ImGui::Begin("LUMA / Stage One");
    ImGui::TextUnformatted("UNLIT SCENE WORKBENCH");
    ImGui::TextDisabled("OpenGL 3.3 Core | %.1f ms", dt * 1000);
    ImGui::SeparatorText("Scene");
    if (ImGui::Button("Robot"))
        preset = "robot";
    ImGui::SameLine();
    if (ImGui::Button("Texture studio"))
        preset = "texture";
    ImGui::SameLine();
    if (ImGui::Button("State lab"))
        preset = "state";
    if (ImGui::Button("Textured square"))
        preset = "quad";
    int style = static_cast<int>(model.style());
    if (ImGui::Combo("Style", &style, "Basic (unlit)\0Wireframe\0"))
        controller.setStyle(static_cast<StyleKind>(style));
    int mode = static_cast<int>(model.controls().mode());
    if (ImGui::Combo("Camera", &mode, "Orbit\0Fly\0"))
        controller.setCameraMode(static_cast<CameraMode>(mode));
    ImGui::TextWrapped(
        "Orbit: drag LMB + wheel. Fly: drag RMB, WASD, Q/E, Shift. UI captures its own input.");
    controller.reconcileSelection();
    if (!model.selected().value) {
        for (const auto& item : model.scene().renderItems())
            if (model.scene().name(item.entity) != "Ground") {
                controller.select(item.entity);
                break;
            }
    }
    ImGui::SeparatorText("Objects");
    if (ImGui::BeginListBox("##objects", {-1, 135})) {
        for (const auto& item : model.scene().renderItems()) {
            ImGui::PushID(static_cast<int>(item.entity.value));
            if (ImGui::Selectable(std::string(model.scene().name(item.entity)).c_str(),
                                  model.selected().value == item.entity.value))
                controller.select(item.entity);
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }
    if (model.scene().contains(model.selected())) {
        const auto& transform = model.scene().transform(model.selected());
        ImGui::SeparatorText("Transform / per object");
        auto p = transform.position(), s = transform.scale();
        auto rotation = glm::degrees(glm::eulerAngles(transform.rotation()));
        if (ImGui::DragFloat3("Position", glm::value_ptr(p), 0.03f))
            controller.setTransform(p, glm::quat(glm::radians(rotation)), s);
        if (ImGui::DragFloat3("Rotation (deg)", glm::value_ptr(rotation), 0.5f))
            controller.setTransform(p, glm::quat(glm::radians(rotation)), s);
        if (ImGui::DragFloat3("Scale", glm::value_ptr(s), 0.02f, 0.02f, 20, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp))
            controller.setTransform(p, glm::quat(glm::radians(rotation)), s);
        MaterialId material;
        for (const auto& item : model.scene().renderItems())
            if (item.entity.value == model.selected().value)
                material = item.material;
        auto m = model.materials().material(material);
        int users = 0;
        for (const auto& item : model.scene().renderItems())
            if (item.material.value == material.value)
                ++users;
        ImGui::SeparatorText("Material / unlit");
        ImGui::Text("Material #%u (%d object%s)", material.value, users, users == 1 ? "" : "s");
        if (ImGui::Button("Create independent material")) {
            material = controller.cloneSelectedMaterial();
            m = model.materials().material(material);
        }
        glm::vec4 display = m.baseColor;
        for (int i = 0; i < 3; ++i)
            display[i] = toSrgb(display[i]);
        if (ImGui::ColorEdit4("sRGB color", glm::value_ptr(display))) {
            for (int i = 0; i < 3; ++i)
                display[i] = toLinear(display[i]);
            m.baseColor = display;
            controller.setMaterial(m);
        }
        ImGui::TextDisabled("Material/shader RGB stays linear.");
        int alpha = static_cast<int>(m.alphaMode);
        if (ImGui::Combo("Alpha mode", &alpha, "Opaque\0Mask\0Blend\0")) {
            m.alphaMode = static_cast<AlphaMode>(alpha);
            controller.setMaterial(m);
        }
        if (m.alphaMode == AlphaMode::Mask && ImGui::SliderFloat("Cutoff", &m.alphaCutoff, 0, 1))
            controller.setMaterial(m);
        if (ImGui::Checkbox("Double sided", &m.doubleSided))
            controller.setMaterial(m);
        if (m.baseColorTexture)
            ImGui::Text("Texture #%u (sRGB decoded)", m.baseColorTexture->value);
        if (m.secondaryColorTexture) {
            ImGui::Text("Second texture #%u (sRGB decoded)", m.secondaryColorTexture->value);
            if (ImGui::SliderFloat("Texture mix", &m.textureMix, 0, 1))
                controller.setMaterial(m);
        }
    }
    ImGui::SeparatorText("State experiments");
    auto options = model.options();
    ImGui::Checkbox("Depth test", &options.depth);
    ImGui::SameLine();
    ImGui::Checkbox("Back-face cull", &options.cull);
    ImGui::Checkbox("Alpha blending", &options.blend);
    ImGui::SameLine();
    ImGui::Checkbox("Stencil selection", &options.outline);
    controller.setRenderOptions(options);
    ImGui::TextWrapped("Depth: nearer fragments win. Cull: CCW fronts survive. Blend: sorted "
                       "centers, depth writes off; intersecting alpha surfaces remain limited. "
                       "Stencil: selected silhouette only, no lighting or toon shading.");
    ImGui::End();
    return preset;
}
void EditorUI::endWithoutDraw() {
    ImGui::EndFrame();
}
void EditorUI::draw() {
    ImGui::Render();
    glDisable(GL_FRAMEBUFFER_SRGB);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace luma

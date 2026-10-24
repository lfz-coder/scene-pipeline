# 光场工坊第二阶段实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 在现有第一阶段 C++/OpenGL/Qt 工作台中加入可复用的 Blinn-Phong 光照、多光源、FBO 后期和可保存的第二阶段工作流，并交付中文文章与真实验证。

**Architecture:** CPU 层保存并验证 LightRig、LightingSettings、PostProcessSettings 和扩展后的 MaterialDesc；EditorController 是唯一编辑入口。Renderer 通过 LightingPipeline 选择 Unlit/BlinnPhong，并通过 PostProcessPipeline 把离屏目标连接到全屏后期 pass。Qt 只组合视图和控件，不直接改 GL 资源。每个职责独立文件，所有手写代码遵守文件行数上限。

**Tech Stack:** C++17、OpenGL Core/GLAD、GLM、Qt6 Widgets/OpenGLWidgets/Test、GLSL 330、QJsonDocument/QSaveFile、CMake/CTest。

**Spec:** `opengl_visual_workbench/docs/superpowers/specs/2026-10-24-stage-two-lighting-design.md`

## Global Constraints

- C++ `.cpp` 文件最多 360 行，C++ 头文件最多 260 行。
- Qt 页面/动作文件最多 320 行，GLSL 单文件最多 180 行，测试文件最多 420 行。
- 第一阶段 Unlit、纹理、深度/模板/混合/剔除、Qt 工作台和版本 1 `.luma` 必须保持可用。
- 版本 1 工程加载时光照默认为 Unlit、后期默认为 None；版本 2 保存新增数据。
- 光照计算在线性空间执行，输出继续经过现有 DisplayTransfer 做 sRGB 编码。
- 不加入 PBR、HDR/Bloom、阴影、模型导入、AI SDK、网络服务或假进度。
- GPU 对象只在创建它的 current OpenGL context 中创建、使用和销毁。
- 每个新行为先写失败测试并实际观察失败，再实现最小代码。

## 文件地图

### 新建

- `native/include/luma/lighting/LightTypes.hpp` / `src/lighting/LightTypes.cpp`：光源值对象和边界验证。
- `native/include/luma/lighting/LightRig.hpp` / `src/lighting/LightRig.cpp`：光源集合、ID 和操作。
- `native/include/luma/lighting/LightingSettings.hpp`：着色模型和环境光配置。
- `native/include/luma/render/PostProcessSettings.hpp`：后期效果值对象和验证。
- `native/include/luma/render/PostProcessTarget.hpp` / `src/render/PostProcessTarget.cpp`：FBO、颜色纹理、深度/模板附件。
- `native/include/luma/render/PostProcessPipeline.hpp` / `src/render/PostProcessPipeline.cpp`：全屏 pass 和效果分派。
- `native/include/luma/render/LightingPipeline.hpp` / `src/render/LightingPipeline.cpp`：光源和材质 uniform 上传。
- `native/assets/shaders/lighting.vert`、`lighting.frag`、`postprocess.vert`、`postprocess.frag`：阶段二 shader。
- `native/tests/lighting_tests.cpp`：CPU 光照/材质/工作流行为。
- `native/tests/postprocess_tests.cpp`：后期值对象行为。
- `native/examples/lighting_example.cpp`：无窗口复用示例。
- `articles/05-stage-two-lighting/index.zh.md`：中文第二阶段文章。
- `articles/assets/stage-two-lighting.svg`、`stage-two-fbo.svg`、`stage-two-lighting.png`：文章图示与真实截图。

### 修改

- `native/include/luma/scene/MaterialDesc.hpp` / `src/scene/MaterialDesc.cpp`：镜面属性和高光贴图。
- `native/include/luma/editor/EditorModel.hpp` / `src/editor/EditorController.*`：光照、后期状态和编辑命令。
- `native/include/luma/editor/WorkflowState.hpp`：增加 Lighting/PostProcess 阶段。
- `native/include/luma/render/Renderer.hpp` / `src/render/Renderer.cpp`：双 shading path 和离屏后期。
- `native/include/luma/qt/ProjectDocument.hpp` / `src/qt/ProjectDocument.cpp`：版本 2 JSON 与版本 1 迁移。
- `native/include/luma/qt/WorkbenchWindow.hpp`、`src/qt/WorkbenchWindow.cpp`、`WorkbenchStages.cpp`、`WorkbenchPages.cpp`、`WorkbenchActions.cpp`：第二阶段 Qt 页面和交互。
- `native/scenes/Presets.cpp`：光照实验室预设。
- `native/CMakeLists.txt`：新库、shader 资产、测试和示例目标。
- `native/assets/shaders/unlit.*`：保持旧 path，并为选中/后期输出补齐接口。
- `native/tests/qt_tests.cpp`、`native/tests/gl_tests.cpp`：增加第二阶段集成检查。
- `tools/build-articles.py`、`articles/README.md`、项目 README：加入第五篇中文文章。

## Task 1: CPU 光源值对象和材质镜面属性

**Files:**
- Create: `native/include/luma/lighting/LightTypes.hpp`, `native/src/lighting/LightTypes.cpp`, `native/include/luma/lighting/LightingSettings.hpp`.
- Modify: `native/include/luma/scene/MaterialDesc.hpp`, `native/src/scene/MaterialDesc.cpp`.
- Test: `native/tests/lighting_tests.cpp`, `native/tests/controller_tests.cpp`.

**Interfaces:**
- `DirectionalLight::validate() const`, `PointLight::validate() const`, `SpotLight::validate() const`.
- `LightingSettings::validate() const`, fields `model`, `ambientColor`, `ambientStrength`, `visualizeNormals`, `maxLights`.
- `MaterialDesc::specularColor`, `shininess`, `specularTexture`.

- [x] **Step 1: Write failing tests** for zero directions, negative intensity, invalid attenuation, invalid cone angles, shininess outside `[1,256]`, and valid defaults.
- [x] **Step 2: Run `cmake --build ... --target luma_lighting_tests`** and verify failure is caused by missing types/fields.
- [x] **Step 3: Implement the value types and validation only; preserve old material defaults.** Use finite checks and throw `std::invalid_argument` before mutation.
- [x] **Step 4: Run `luma_lighting_tests` and existing `luma_controller_tests`; verify green.**
- [x] **Step 5: Check line counts** for new files and split validation helpers if any file exceeds its limit.

## Task 2: LightRig collection and CPU Controller integration

**Files:**
- Create: `native/include/luma/lighting/LightRig.hpp`, `native/src/lighting/LightRig.cpp`.
- Modify: `native/include/luma/editor/EditorModel.hpp`, `native/src/editor/EditorController.cpp`, `native/include/luma/editor/EditorController.hpp`, `native/include/luma/editor/WorkflowState.hpp`.
- Test: `native/tests/lighting_tests.cpp`, `native/tests/workflow_tests.cpp`, `native/tests/controller_tests.cpp`.

**Interfaces:**
- `struct LightId { std::uint32_t value = 0; };` with equality helpers and value `0` reserved as invalid.
- `LightRig::addDirectional`, `addPoint`, `addSpot`, `remove`, `entry`, `entries`, `validate()`.
- `EditorModel::lights()`, `lighting()`, `postProcess()` read-only accessors.
- `EditorController::setLightingSettings`, `addDirectionalLight`, `addPointLight`, `addSpotLight`, `updateLight`, `removeLight`, `setPostProcessSettings`.

- [x] **Step 1: Add failing tests** for deterministic IDs, max-count rejection, remove-and-read rejection, controller atomic update, and workflow invalidation from Lighting/PostProcess.
- [x] **Step 2: Run the focused CPU tests** and verify the expected missing API failures.
- [x] **Step 3: Add `LightRig` with a bounded variant collection** (one directional, up to eight point, up to four spot), no GPU handles, and copyable value data.
- [x] **Step 4: Add the model/controller references and extend workflow stages** while keeping stage navigation free and completion sequential.
- [x] **Step 5: Run focused tests, then `ctest --test-dir native/build --output-on-failure`.**

## Task 3: Version 2 project document

**Files:**
- Modify: `native/include/luma/qt/ProjectDocument.hpp`, `native/src/qt/ProjectDocument.cpp`.
- Test: `native/tests/qt_tests.cpp`, new helper in `native/tests/project_document_tests.cpp` if the Qt test file would exceed 420 lines.

**Interfaces:**
- `ProjectDocument.version = 2` on write.
- `ProjectDocument::read()` accepts version 1 and 2.
- New serialized fields: `lighting`, `lights`, `postProcess`, material `specularColor`, `shininess`, `specularTexture`.
- `ProjectDocument::apply()` restores light IDs by document order and never publishes a partially validated document.

- [x] **Step 1: Write failing Qt tests** for v1 defaults, v2 round-trip, shared material specular fields, light collection, post-process settings, and invalid light rejection (cone boundaries also covered by CPU validation).
- [x] **Step 2: Run `luma_qt_tests`** and verify failures occur before implementation.
- [x] **Step 3: Implement JSON helpers split by concern** (`ProjectDocumentJson.cpp` if the current file approaches 320 lines), using QSaveFile and full-read validation.
- [x] **Step 4: Implement apply ordering**: validate entire document, rebuild scene preset, restore object transforms/material groups, then light rig/settings/camera/postprocess.
- [x] **Step 5: Run v1 and v2 tests and confirm a failed read leaves the live viewport unchanged.**

## Task 4: Lighting shader and LightingPipeline

**Files:**
- Create: `native/include/luma/render/LightingPipeline.hpp`, `native/src/render/LightingPipeline.cpp`.
- Create: `native/assets/shaders/lighting.vert`, `native/assets/shaders/lighting.frag`.
- Modify: `native/include/luma/gfx/IResourceFactory.hpp`, `native/src/gfx/ShaderProgram.cpp` only if typed vec3/mat3 setters are required.
- Test: `native/tests/gl_tests.cpp`, `native/tests/lighting_tests.cpp`.

**Interfaces:**
- `LightingPipeline::initialize(IResourceFactory&, shaderDirectory)`.
- `LightingPipeline::draw(const RenderItem&, const Scene&, const AssetStore&, const Camera&, const LightRig&, const LightingSettings&, bool manualEncoding)`.
- Shader uniforms use fixed arrays `uPointLights[8]` and `uSpotLights[4]`, with count uniforms and no dynamic GLSL source generation.

- [x] **Step 1: Add a GL test** that requests the lighting shader and expects a brighter pixel with one valid directional light; add CPU validation for the light budget and type limits.
- [x] **Step 2: Build/run the GL test and observe shader/API failure.**
- [x] **Step 3: Add typed `vec3`/`mat3` uniform support only where needed, then implement GLSL with world position, normal matrix, ambient/diffuse/specular, attenuation and smooth spotlight edges.**
- [x] **Step 4: Implement `LightingPipeline` to upload material and light arrays, bind diffuse/specular textures, and call the existing mesh contract.**
- [x] **Step 5: Run `luma_gl_tests --hidden` and compare a directional, point and spot fixture using readback pixels.**
- [x] **Step 6: Verify all new shader files are under 180 lines.**

## Task 5: Renderer shading selection and light laboratory scene

**Files:**
- Modify: `native/include/luma/render/Renderer.hpp`, `native/src/render/Renderer.cpp`, `native/scenes/Presets.cpp`, `native/include/luma/render/RenderSettings.hpp`.
- Test: `native/tests/gl_tests.cpp`, `native/tests/qt_tests.cpp`.

**Interfaces:**
- `Renderer::setLighting(const LightingSettings&, const LightRig&)`.
- `Renderer::setPostProcess(const PostProcessSettings&)` (initially stores validated settings; execution is Task 6).
- `StyleKind` remains independent from `ShadingModel`; Wireframe can combine with either model.

- [x] **Step 1: Add failing integration checks** for Unlit compatibility, Phong mode and the new `lighting` preset object count; complete the non-uniform-scale slanted-face pixel regression during final verification.
- [x] **Step 2: Run existing GL/Qt integration binaries and capture expected failures.**
- [x] **Step 3: Add Renderer dispatch**: current Unlit path remains unchanged; Phong path delegates per-item to LightingPipeline; shared state and stencil outline remain explicit.
- [x] **Step 4: Add the “光照实验室” preset** with ground, three differently colored/specular objects and default three light types.
- [x] **Step 5: Run GL/Qt checks, then take a clean screenshot of the laboratory preset.**

## Task 6: PostProcessTarget and post-processing passes

**Files:**
- Create: `native/include/luma/render/PostProcessSettings.hpp`, `PostProcessTarget.hpp`, `PostProcessPipeline.hpp` and their `.cpp` files.
- Create: `native/assets/shaders/postprocess.vert`, `postprocess.frag`.
- Modify: `native/include/luma/gfx/IResourceFactory.hpp` only if a fullscreen mesh contract is needed; otherwise use gl_VertexID.
- Test: `native/tests/postprocess_tests.cpp`, `native/tests/gl_tests.cpp`.

**Interfaces:**
- `enum class PostProcessEffect { None, Invert, Grayscale, Sharpen, Blur }`.
- `PostProcessSettings { enabled, effect, intensity, radius }` with `validate()`.
- `PostProcessTarget::resize(width,height,depthStencilFormat)`, `bindForScene()`, `texture()`, `complete()`; two instances are used for the separable blur.
- `PostProcessPipeline::initialize(shaderDirectory)`, `apply(PostProcessTarget& source, PostProcessTarget& scratch, DisplayTarget destination, const PostProcessSettings&)`.

- [x] **Step 1: Write failing CPU tests** for effect enums, intensity/radius bounds, default None, and failed settings atomicity.
- [x] **Step 2: Write failing GL tests** for FBO completeness, resize, and output pixel difference for each effect.
- [x] **Step 3: Run focused tests and confirm failures.**
- [x] **Step 4: Implement the target with RGBA8 color texture and packed D24/S8 renderbuffer, checking `glCheckFramebufferStatus`.**
- [x] **Step 5: Implement one fullscreen shader with effect branch and a two-pass blur using two targets; preserve the existing DisplayTransfer contract.**
- [x] **Step 6: Run GL tests and ensure target destruction occurs while context is current.**

## Task 7: Connect Renderer to the FBO pipeline

**Files:**
- Modify: `native/src/render/Renderer.cpp`, `native/include/luma/render/Renderer.hpp`, `native/src/qt/QtViewport.cpp`, and GLFW host draw code only where the display target is assembled.
- Test: `native/tests/gl_tests.cpp`, `native/tests/qt_tests.cpp`, `native/tests/lifecycle_tests.cpp`.

**Interfaces:**
- `Renderer::render()` keeps accepting a borrowed `DisplayTarget`; it internally renders directly for `None` and to `PostProcessTarget` for enabled effects.
- `QtViewport` calls `renderer.setLighting(model.lighting(), model.lights())` and `renderer.setPostProcess(model.postProcess())` before render.

- [x] **Step 1: Add failing test** that enables grayscale and proves Qt/OpenGL output differs from the source while existing Unlit/None pixel regressions remain green.
- [x] **Step 2: Run the focused integration test and observe the missing pass.**
- [x] **Step 3: Wire target allocation to actual pixel size and apply the effect before DisplayTransfer; reset GL state after each pass.**
- [x] **Step 4: Verify resize and minimized-window paths do not allocate zero-sized targets.**
- [x] **Step 5: Run all lifecycle and GL tests.**

## Task 8: Qt lighting/material/postprocess workflow

**Files:**
- Modify: `native/include/luma/qt/WorkbenchWindow.hpp`, `src/qt/WorkbenchWindow.cpp`, `WorkbenchStages.cpp`, `WorkbenchActions.cpp`, `WorkbenchPages.cpp`, and `WorkbenchTheme.cpp`.
- Create focused helper files if needed: `src/qt/WorkbenchLightingPage.cpp`, `src/qt/WorkbenchPostPage.cpp`, each under 320 lines.
- Test: `native/tests/qt_tests.cpp` or split into `qt_lighting_tests.cpp` before the 420-line limit.

**Interfaces:**
- Workflow stages become seven entries with indices `Scene=0, Material=1, Lighting=2, Camera=3, Review=4, PostProcess=5, Export=6`.
- Light list selection uses `QListWidget` item data containing `LightId.value` and type.
- New controls call only `EditorController` and trigger `changed(WorkflowStage::Lighting/PostProcess)`.

- [x] **Step 1: Add failing Qt tests** for stage labels, add/select/edit/remove light, Phong toggle, material specular fields, effect selection, ; exercise the resulting task records as part of the live workflow.
- [x] **Step 2: Run `luma_qt_tests` and verify expected missing control failures.**
- [x] **Step 3: Split pages before editing if any current source file is over its line budget; add reusable light-row and numeric-form helpers.**
- [x] **Step 4: Implement lighting panel with one selected light editor and explicit add/remove actions.**
- [x] **Step 5: Implement material specular controls, review normal visualization, and postprocess panel.**
- [x] **Step 6: Update navigation text, close/save state, screenshots, and task log.**
- [x] **Step 7: Run Qt tests at both 1440×900 and 1100×720.**

## Task 9: Reusable example and documentation

**Files:**
- Create: `native/examples/lighting_example.cpp`, `articles/05-stage-two-lighting/index.zh.md`, `articles/assets/stage-two-lighting.svg`, `articles/assets/stage-two-fbo.svg`.
- Modify: `native/CMakeLists.txt`, `articles/README.md`, `README.md`, `native/README.md`, `tools/build-articles.py` only for dynamic chapter discovery.
- Test: `articles/tests/test_reader_build.py`, `native/tests/verification.md`.

- [x] **Step 1: Add article build assertions** for five chapters, the new article ID, embedded screenshots and SVG validity; observe failure.
- [x] **Step 2: Add the example target and article with real class/function snippets, equations, call chain, boundaries, FAQ and links to source files.**
- [x] **Step 3: Build the offline reader and run its tests.**
- [x] **Step 4: Update run instructions with the Qt lighting laboratory and example command.**

## Task 10: Full verification and visual review

**Files:**
- Modify: `docs/implementation/2026-10-24-stage-two-verification.md`, `native/tests/verification.md`.
- Create: `articles/assets/stage-two-lighting.png`, `stage-two-narrow.png`, `stage-two-postprocess.png`.

- [x] **Step 1: Run a clean CMake configure/build** with Qt enabled, then rebuild affected targets after review fixes.
- [x] **Step 2: Run `ctest --test-dir native/build --output-on-failure`.**
- [x] **Step 3: Run `luma_gl_tests --hidden`, `luma_qt_tests`, and the new example; record actual output.**
- [x] **Step 4: Capture and inspect laboratory, material, lighting, postprocess, home and narrow screenshots; use `view_image` for each.**
- [x] **Step 5: Run line-count checks for all handwritten source and shader files.**
- [x] **Step 6: Rebuild `articles/index.html`, run article tests, and verify links/assets.**
- [x] **Step 7: Write the verification report with passed checks, known macOS diagnostics, explicit current limitations and exact launch command.**

## Execution Notes

After each task, keep the smallest focused test green before moving to the next task. If a file reaches its limit, split it immediately by responsibility and update this plan's file map. Do not hide an unfinished feature behind a disabled control; if a shader or FBO cannot run on the current context, surface the error in the task log and keep the previous valid state.


## 实施收尾（2026-10-24）

- 新增 `ProjectTexture.hpp/.cpp` 与 `ProjectTextureJson.hpp`，保存内置路径和完整采样选项；AssetStore 保存 TextureSource。
- QtViewport 在候选场景中应用文档，再发布；共享材质贴图、非法槽位和资源读取失败都有真实回归。
- 灯光控件按字段/分量提交，保留 0.032 等原始精度；默认灯光预算为 13。
- 拆出窗口、页面、Inspector、工作流、灯光动作以及 GL/Qt 测试辅助文件。119 个原生文件通过硬上限检查。
- 斜面非均匀缩放测试通过故意注入错误 shader 验证断言有效，最终恢复正确源码。
- 独立只读审查发现的三个原始问题及后续资源 I/O 原子性问题均修复；最终复核无剩余 Critical/Important。
- 最终命令、输出、截图和 HTML 浏览器检查限制见 `docs/implementation/2026-10-24-stage-two-verification.md`。

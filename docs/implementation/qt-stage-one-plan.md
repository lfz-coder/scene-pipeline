# 第一阶段收尾与 Qt 宿主 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** 完成第一阶段缺失的双纹理示例，提供可运行的基础 Qt 软件。

**Architecture:** Qt View → CPU EditorController/EditorModel → 自有 Renderer/资源工厂。GLFW 与 Qt 共用场景、材质、相机与 GPU 包装。

**Tech Stack:** C++17、OpenGL 3.3 Core、GLAD、GLM、stb_image、Qt6 Widgets/OpenGLWidgets；macOS 实际使用 OpenGL 4.1。

**Spec:** `docs/implementation/qt-stage-one-design.md`

## Global Constraints

- 只修改 opengl_visual_workbench；不修改 opengl_image_lab。
- 中文界面、中文博客；不实现 AI pipeline 或第二至第五阶段渲染功能。
- Qt 只做宿主与视图；继续使用我们的 NativeResourceFactory、Renderer 和 CPU MVC。
- 所有 GPU 资源在所属上下文 current 时释放；Qt FBO 每帧获取，不假定为 0。
- 保留 GLFW 示例与原有 CPU/GL/生命周期测试；Qt 依赖可以按 CMake 选项关闭。
- 本目录没有 Git 仓库；用 /private/tmp 的基线快照与 diff 审查，不创建提交。

### Task 1: 完成双纹理与正方形示例

**Files:** MaterialDesc.hpp/.cpp、Renderer.cpp、unlit.frag、Presets.cpp、EditorUI.cpp、CPU/GL 测试。
**Interfaces:** `std::optional<TextureId> secondaryColorTexture; float textureMix = 0.5f;`，`makePreset("quad")`。

- [ ] 先写材质边界测试与双纹理 GPU 像素测试；记录功能缺失时的失败结果。
- [ ] 验证 `textureMix` 有限且在 0..1，第二纹理要求第一纹理存在；Renderer 验证资源，绑定 unit0/unit1，shader 用 `mix(texture(uTexture,vUv),texture(uTexture2,vUv),uTextureMix)`。
- [ ] 增加 `quad` 场景：面向相机的纹理正方形；纹理场景包含真正的双纹理示例；ImGui 可编辑混合系数。
- [ ] 跑 CPU/GL 覆盖测试，生成报告与任务 diff，完成独立审查。

### Task 2: 真实 Qt 桌面宿主

**Files:** 新增 include/luma/qt 与 src/qt 下 QtInput、QtViewport、WorkbenchWindow、main；tests/qt_tests.cpp；共享 Assets 路径模块；CMakeLists.txt。
**Interfaces:** `QtViewport::model() const`、`controller()`、`loadPreset(const std::string&)`、`lastError() const`；暴露只读状态供 Qt 控件刷新与测试使用，GPU 资源放在私有会话对象。

- [ ] 先写 Qt 输入、控件修改 CPU 状态、真实渲染输出和视口 resize 的行为测试，观察缺失实现的失败。
- [ ] 抽离共享 assetRoot 与 presets 目标，增加可关闭的 Qt CMake 目标；Qt 可执行文件不能依赖 GLFW/ImGui。
- [ ] 完成中文主窗口、对象树、变换/材质属性、渲染状态、场景/相机/基础线框切换、保存视口 PNG。控件只调用控制器。
- [ ] 完成 Qt 输入事件转 InputSnapshot、GLAD 加载、DPI/FBO、异常边界、上下文清理和重建；不引入 Qt GPU 资源包装。
- [ ] 运行真实 Qt 测试和软件，截图并检查画面，执行既有回归；报告与独立审查。

### Task 3: 中文交付与第一阶段核对

**Files:** README、native/README、docs/architecture、api-contract、milestones、新增 stage-one-checklist、三篇中文文章及构建输出。

- [ ] 逐项映射第一阶段知识点和四个小项目到实际源码、演示和测试，明确未来课程扩展点。
- [ ] 更新文章的 Qt 现状、架构图、运行入口与真实截图，保留之后可替换资源工厂的说明。
- [ ] 重建文章 HTML，验证构建测试与页面资源；检查最终软件截图。
- [ ] 全范围审查并处理问题，记录最终运行命令与限制。

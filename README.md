# 光场工坊 · LUMA Visual Workbench

多风格 3D 创作软件的可交互产品原型。用于先体验未来软件的主要工作流程：选择场景 → 选择对象 → 调整材质和灯光 → 切换风格 → 导出画面。

## 仓库与开始使用

项目在 [scene-pipeline（场景生产线）](https://github.com/lfz-coder/scene-pipeline) 独立管理，保留光场工坊的产品界面与模块结构。仓库根目录就是本项目，不包含其他 OpenGL 学习工程。

```sh
git clone https://github.com/lfz-coder/scene-pipeline.git
cd scene-pipeline
./启动Qt工作台.command
```

启动脚本适用于已安装 CMake、GLFW、Qt6 Widgets / OpenGLWidgets / Network / Multimedia 的 macOS Homebrew 环境；完整构建和验证方式见 [原生工程说明](native/README.md)。查看产品原型可直接打开根目录 `index.html`，阅读中文版博客可打开 `articles/index.html`。

后续提交标题和正文统一使用中文，规则保存在 [AGENTS.md](AGENTS.md) 与 [开发边界](docs/development-boundaries.md#仓库与提交)。本机仍可保留 `opengl_visual_workbench` 目录名；以下命令均从本仓库根目录执行。

## 新版 Qt 中文软件

双击 [启动Qt工作台.command](启动Qt工作台.command)，会增量编译最新代码并打开首页。新版首页包含创作输入框与七个模板；项目内采用常驻 AI 助手和五个内容视图，保留七步人工流水线、颜色选择器、镜头快照、`.luma` 工程保存/恢复、真实 PNG 导出与记录。

流程是 **场景 → 材质 → 光照 → 镜头 → 检查 → 后处理 → 导出**。修改上游会使后续确认失效；Qt 界面仍调用自有 OpenGL 封装。项目整体接入对话、人工确认与视频时间线，切换右侧视图时保留同一个助手。

[新版首页](articles/assets/qt-project-home.png) · [新版项目界面](articles/assets/qt-studio-wide.png) · [本轮验证记录](docs/implementation/2026-10-20-manual-workflow-verification.md) · [第四篇：Qt 人工流水线](articles/04-qt-manual-workflow/index.zh.md)

最新交互修正：共享媒体库、搜索/分组/状态筛选、图像预览与信息侧栏、镜头重命名/移除、项目菜单、直接保存/另存为、未保存保护、按钮禁用与焦点反馈。具体实现及与 Pixo 的适配差异见 [2026-10-24 交互审计](docs/implementation/2026-10-24-interaction-audit.md)。

入口去重：移除与首页相同的加号，以及重复的页面返回、打开/保存和导出按钮。首页从左上角品牌进入，项目视图统一从顶栏切换，打开/保存使用顶栏，快照在镜头构图创建，PNG 在导出阶段底部执行；清理规则写入[开发边界](docs/development-boundaries.md)。

## 第二阶段：光照与后处理

双击 [启动光照实验室.command](启动光照实验室.command)，直接打开新场景。新增 Blinn-Phong、法线矩阵、方向/点/聚光灯增删改、材质高光颜色/光泽度、底色/高光贴图，以及 FBO 反相、灰度、锐化和两遍模糊。`.luma` v2 保存这些参数，兼容 v1。

[第五篇中文文章](articles/05-stage-two-lighting/index.zh.md) · [实际运行截图](articles/assets/stage-two-lighting.png) · [本阶段验收](docs/implementation/2026-10-24-stage-two-verification.md) · [模块和文件行数边界](docs/development-boundaries.md)。原生软件的 PBR、Toon、阴影与模型导入留给后续学习阶段。

## 对话、Pipeline 与视频时间线

首页输入想法或参考图后创建工程；项目左侧常驻 LLM/VLM 助手，右侧切换方案、3D 场景、镜头、媒体、时间线。四节点流程经人工确认后入轨，模型服务在顶栏统一配置。支持本地图片和真实视频播放、定位、裁剪、拆分、重排与工程保存。

默认本地演示不调用模型；HTTP 适配器按 [OpenAPI 3.1.1](api/openapi.json) 工作，Go 本轮只交 [后端约束](docs/backend-go-contract.md)。当前不包含视频生成、多轨合成或视频编码导出。

[第六篇中文教程](articles/06-studio-pipeline/index.zh.md) · [真实界面](articles/assets/qt-studio-wide.png) · [整体布局验收](docs/implementation/2026-10-24-ai-project-layout.md)。

## C++ / OpenGL 第一、二阶段工程与中文博客

原生学习工程已实现，并加入 MVC 编辑与可替换资源边界：C++17 / OpenGL Core、可复用资源模块、场景与材质、Orbit / Fly 相机、编辑面板，以及深度 / 模板 / 混合 / 剔除实验。新增场景只编写物体、材质和更新逻辑，公共层负责窗口、GPU 资源与渲染流程。

- **[打开六篇中文博客 HTML 阅读版](articles/index.html)**：架构与生命周期、GPU 资源、场景与相机、Qt 人工创作流水线、光照与 FBO 后处理、对话与视频时间线。正文、配图、目录和代码均已内嵌，可离线阅读。
- **[构建并运行 C++ 原生工作台](native/README.md)**：含五个知识点入口、独立场景和无窗口控制器两个复用例子。
- [原生程序实际截图](articles/assets/native-robot.png) · [实际验证记录](native/tests/verification.md)

- [模块架构与调用示意](docs/architecture.md)
- [类、成员函数和生命周期契约](docs/api-contract.md)
- [实现顺序与验收标准](docs/milestones.md)
- [中文文章目录及后续连载](articles/README.md)

现在复用代码有两条入口：SceneLayer 负责创建与更新业务场景；EditorController 负责选中、变换、材质、相机与渲染偏好，面板和无窗口程序调用同一接口。资源通过 IResourceFactory 注入，当前使用自写 OpenGL 包装的 NativeResourceFactory。Qt6 中文宿主已经接入 UI / 窗口，并继续使用我们的 OpenGL 封装；QtResourceFactory 与 Go 服务留给后续阶段；客户端编排和 HTTP 适配已接入。

原生版当前对应已学完的第一、二阶段。下面的 HTML 产品原型包含写实、二次元和霓虹等未来视觉目标；这些高级效果会随后续课程逐步进入原生软件。

## 直接打开

双击 **index.html**，用 Chrome、Safari 或 Edge 打开。渲染代码、着色器和程序生成的场景都包含在这个 HTML 文件里，不需要安装依赖或联网。也可以只复制这一个文件给其他人体验。

## 可以体验的内容

| 场景 | 可编辑对象 |
| --- | --- |
| 星际信使 | 机器人角色、轨道伙伴、信号灯塔 |
| 材质展台 | 香槟金属、樱粉陶瓷、月光铬银、薄荷磨砂、紫晶涂层 |
| 天空庭院 | 庭院小屋、春色树冠、庭院陈设 |

同一场景可以切换写实、二次元、霓虹和线框四种风格。左侧对象列表和视口点击都可以选择对象；右侧能调节主题色、金属度、粗糙度、旋转、缩放、光源方向、曝光、阴影、辉光和网格。

金属度和粗糙度主要在写实模式中比较；二次元模式使用分段光照和描边，霓虹模式使用轮廓光及 FBO 多 Pass 辉光。

| 操作 | 效果 |
| --- | --- |
| 1 / 2 / 3 / 4 | 写实 / 二次元 / 霓虹 / 线框 |
| 左键拖动 | 围绕场景旋转相机 |
| 右键或 Shift + 左键拖动 | 平移相机 |
| 滚轮 | 缩放视图 |
| WASD / Q、E | 水平平移 / 上下移动 |
| Space | 开关自动转台 |
| R | 重置当前场景、参数和视角，恢复二次元风格 |
| F1 | 隐藏或显示视口标题和操作提示 |
| 导出画面 | 保存当前 3D 画面的 PNG 图片 |

## 开发文件

```text
index.html                  完整离线入口；CSS、界面与生成后的脚本
workbench.js                便于继续开发的源代码
tools/build-demo.cjs        把源代码嵌入 index.html
tests/geometry.test.cjs     检查三角形绕序与法线是否一致
tests/browser-smoke.md      浏览器验收记录
previews/                   浏览器实测截图
```

修改 `workbench.js` 后运行：

```bash
node tools/build-demo.cjs
node --check workbench.js
node tests/geometry.test.cjs
```

## 可继续扩展的模块

`Mat4` 管理矩阵；`MeshFactory` 生成网格；`Mesh` 封装 VAO/VBO/EBO；`Camera` 管理视图与输入；`ShaderProgram` 封装 GLSL 编译、链接和 uniform；`Entity` 和 `Scene` 管理对象、材质与变换；`StylePipeline` 选择渲染风格；`RenderTarget` 管理 FBO；`Renderer` 执行阴影、场景、描边、辉光和合成，并通过颜色 ID Pass 完成拾取。

当前版本是用于确认软件方向和交互的 WebGL2 原型。写实模式使用简化的金属/粗糙度着色和程序化环境反射，不包含完整 IBL、外部模型导入、角色骨骼动画或工程存档。Bloom 使用 RGBA8 颜色缓冲；完整 HDR 管线可以后续接入。之后的 C++/OpenGL 软件可沿用这些模块的职责、材质参数和风格设计。

# 光场工坊：原生 C++ 第一、二阶段

这是可运行的 C++17 / OpenGL Core 桌面工作台。当前能力是 **Unlit 纯色、顶点色、单/双纹理、MVP、相机与深度 / 模板 / 混合 / 剔除 / 线框**。现在有两种宿主：原有 GLFW + ImGui 学习入口，以及真实 Qt6 中文软件界面；二者共享我们自己的 Scene、Material、Renderer 与资源封装。第二阶段已加入 Blinn-Phong、多光源、材质高光与 FBO 滤镜；PBR、Bloom 和模型导入尚未实现。

## Qt 软件入口

本节中的 `build/bin/…` 命令在 `native/` 目录执行；标明“仓库根目录”的命令在包含 `native/` 的项目根目录执行。

Qt 版本的完整界面是 `luma_qt_workbench`：首页创作输入框与七个场景模板、常驻 AI 助手与顶栏项目视图、七步人工创作流水线、镜头快照、工程存档和导出记录。左侧助手保留对话；右侧 3D 场景由 QOpenGLWidget 视口与对象/属性标签组成，支持变换 / 材质 / 光照 / 镜头 / 渲染检查 / 后处理 / 导出。Qt 负责 View、输入和上下文；视口仍调用 `NativeResourceFactory`、`Renderer` 和 `EditorController`。

流程为 **场景 → 材质 → 光照 → 镜头 → 检查 → 后处理 → 导出**。可以自由返回编辑；改动从对应阶段开始清除后续确认标记，鼠标或 WASD 移动相机也会使检查失效。整个项目共享对话、人工确认、媒体与视频时间线；不依赖真实模型即可运行本地演示。

`⌘S / ⌘O` 保存 / 打开 `.luma` 工程。版本 2 保存内置预设、物体变换、材质及共享关系、三种灯光、光照/后处理设置、相机和渲染状态，兼容版本 1，使用 QSaveFile 原子写入；非法文件先校验再拒绝。studio 可选扩展另存对话、方案、静帧、媒体路径和时间线，文件上限 16 MiB，不保存 token。镜头页快照、场景草稿与导出记录仍仅本次会话保留。PNG 为当前视口尺寸，包含当前选中轮廓设置。可在渲染检查中关闭轮廓后导出。

已有路径时“保存工程”直接保存；项目菜单提供“另存为”。打开工程、新建及关闭前会提示处理未保存修改。切换场景草稿保留自己的保存路径与流程确认状态。

界面入口已精简：项目视图切换只走顶栏，打开/保存只走顶栏，首页卡片创建模板工程；项目菜单仅保留“新建当前场景工程”和“另存为”。快照在“镜头构图”创建，PNG 使用“导出作品”阶段底部的单一按钮。原来的加号、返回/继续导航和重复导出控件已删除，快捷键仍可用。

镜头与作品共用 `MediaGallery`：搜索、场景/成功失败筛选、计数、双击预览、前后切换和信息侧栏。镜头可恢复机位、重命名、移除；作品可打开文件/所在目录、移除记录。移除记录不删除磁盘文件。详见 [逐项交互审计](../docs/implementation/2026-10-24-interaction-audit.md)。

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew -DLUMA_WITH_QT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
build/bin/luma_qt_workbench
build/bin/luma_qt_workbench --page 0
# 无界面截图验证
build/bin/luma_qt_workbench --preset quad --screenshot /tmp/luma-qt.png
```

也可以双击上级目录的 `启动Qt工作台.command`。Qt 视口每帧把 `defaultFramebufferObject()` 和实际像素尺寸传给 Renderer；资源在 current context 中清理。详见 [第一阶段验收清单](../docs/stage-one-checklist.md) 和 [Qt 验证报告](../docs/implementation/qt-stage-one-qt-report.md)。

## 编排、对话与视频时间线

在首页发起创作，或在项目顶栏选择“时间线”。直接进入时间线可执行 `build/bin/luma_qt_workbench --preset lighting --page 4`。Qt 构建需要匹配版本的 Network 和 Multimedia；Homebrew 环境缺少时安装 `qtmultimedia`。

默认“本地演示”明确不调用真实模型，可以发送/取消、选择 LLM/VLM、附图、审核并修改镜头建议，确认后采集引用画面加入静帧时间线。导入视频后支持真实解码播放、定位、裁剪、拆分、重排和移除。当前是单轨，不包含视频生成、多轨/转场或视频编码导出。

点击右上角“模型服务”，切换“HTTP 服务”，输入服务根地址与本应用 bearer token，点击连接读取模型能力，再发送请求。[OpenAPI 3.1.1](../api/openapi.json) 是唯一 HTTP 契约；[Go 约束](../docs/backend-go-contract.md) 规定后续 provider、幂等和取消行为，Go 服务本轮未实现。媒体路径只在本机保存，token 不入盘。

[第六篇完整中文教程](../articles/06-studio-pipeline/index.zh.md) · [本轮验收](../docs/implementation/2026-10-24-studio-pipeline.md)。在仓库根目录执行：

```sh
python3 -m venv /private/tmp/luma-contract-venv
/private/tmp/luma-contract-venv/bin/pip install -r api/requirements-validation.txt
native/build/bin/luma_studio_contract_tests /private/tmp/luma-studio-client-dtos.json
LUMA_CLIENT_FIXTURES=/private/tmp/luma-studio-client-dtos.json /private/tmp/luma-contract-venv/bin/python tests/test_api_contract.py
swift -module-cache-path /private/tmp/luma-swift-cache tools/make-video-fixture.swift /private/tmp/luma-studio-fixture.mov
native/build/bin/luma_studio_ui_tests /private/tmp/luma-studio-fixture.mov
```

## 第二阶段快速体验

```sh
build/bin/luma_qt_workbench --preset lighting
build/bin/luma_workbench --preset lighting
build/bin/luma_lighting_example
```

在 Qt 的“光照实验”页添加、选择、修改或删除方向光、点光源、聚光灯。可编辑灯色、强度、位置、方向、衰减与锥角。在材质页调整镜面颜色和光泽度；中间黄铜方块包含底色与线性高光贴图。后处理页提供 None、反相、灰度、锐化、可分离模糊。

[完整中文文章](../articles/05-stage-two-lighting/index.zh.md) · [开发边界](../docs/development-boundaries.md) · [本次实际验证](../docs/implementation/2026-10-24-stage-two-verification.md)。GPU 上限为 8 个点光、4 个聚光与 1 个方向光；默认上传预算是 13 项。后处理为 RGBA8 LDR，当前无阴影、PBR、HDR/Bloom、Toon 或外部 3D 模型导入。

工程纹理保存内置资源相对路径与颜色空间、过滤、环绕等选项，空槽为 null；AssetStore 在加载时解析并缓存，磁盘不保存 TextureId。共享材质的贴图也能恢复。外部资产导入与打包机制留给后续阶段。

新增验证命令：

```sh
build/bin/luma_lighting_gl_tests --hidden
build/bin/luma_qt_tests
build/bin/luma_qt_interaction_tests
python3 ../tools/check-native-boundaries.py
```

文件硬上限：C++ 实现 360 行、头文件 260 行、Qt 页面/动作 320 行、GLSL 180 行、测试 420 行。第三方与生成 HTML 不计入；检查已纳入 CTest。

## 构建与运行

```sh
cmake -S native -B native/build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew -DCMAKE_BUILD_TYPE=Debug
cmake --build native/build -j4
native/build/bin/luma_workbench
```

命令从仓库工作目录执行。也可以在 `native/` 下使用 `cmake -S . -B build`。GLFW 优先使用本机 CMake 包；缺少时才下载固定版本 3.4。其余依赖已放入 `third_party/`：GLM 1.0.1、Dear ImGui 1.91.9b、GLAD 2、stb_image 固定提交 `f0569113c93ad095470c54bf34a17b36646bbbb5`。有本地 GLFW 和这些头文件时，构建不需要联网。

```sh
native/build/bin/luma_workbench --help
native/build/bin/luma_workbench --preset texture
native/build/bin/luma_workbench --preset state
native/build/bin/luma_reuse_example
native/build/bin/luma_controller_example
```

`luma_reuse_example` 来自完整独立调用示例 [examples/reusable_scene.cpp](examples/reusable_scene.cpp)。场景只调用 `SceneContext` 与 `SceneLayer`，不初始化窗口、不编译 shader、不发 OpenGL 调用。

`luma_controller_example` 来自 [examples/controller_example.cpp](examples/controller_example.cpp)，只链接 `luma_cpu`，不创建窗口。它使用真实 Scene / MaterialLibrary / EditorModel / EditorController：选择对象、提交变换和共享颜色、clone 材质后独立编辑，并检查另一个对象保持原值。它是当前 ImGui 界面之外的第二个控制器调用方。

## MVC 与资源替换入口

[EditorModel](include/luma/editor/EditorModel.hpp) 借用 Scene、MaterialLibrary、Camera 和 CameraControls，向视图提供 const 读取；[EditorController](include/luma/editor/EditorController.hpp) 提供 `select`、`clearSelection`、`reconcileSelection`、`setTransform`、`setMaterial`、`cloneSelectedMaterial`、`setCameraMode`、`setCameraPose`、`setStyle`、`setRenderOptions`。这些类型属于 `luma_cpu`，不包含 OpenGL、GLFW、ImGui 或 Qt 头文件。输入验证失败抛出异常；多字段变换和相机先验证副本再提交，保留原值。删除对象或切换场景后调用 `reconcileSelection`；它清除失效选择，默认选择策略由视图决定。`setRenderOptions` 更新渲染标志和背景，选择仍由专用选择操作管理。

[MaterialLibrary](include/luma/scene/MaterialLibrary.hpp) 只存 CPU 材质；相同 MaterialId 分享编辑，`cloneSelectedMaterial` 只替换所选实体的引用。AssetStore 的 `createMaterial` / `cloneMaterial` / `material` 保留并转发，现有 SceneLayer 无需重写。Model 借用的 CPU 对象必须比 Model 活得更久；操作为同步、单线程调用。

[资源契约](include/luma/gfx/IResourceFactory.hpp) 提供 IShader 的 typed `set` / `use`、IMesh 的 `draw`、ITexture 的 `bind`；工厂返回独占资源，不公开原生资源 handle、GLFW 或 Qt 类型。[NativeResourceFactory](include/luma/gfx/NativeResourceFactory.hpp) 委托现有 move-only ShaderProgram / Mesh / Texture2D。应用组合点显式注入：

```cpp
NativeResourceFactory resources;
AssetStore assets(resources);
Renderer renderer(resources);
renderer.initialize(shaderDirectory);
```

单独使用 DisplayTransfer 时同样以 `DisplayTransfer(resources)` 构造。工厂必须比消费者活得更久；其返回的资源在所属有效 current context 中使用和销毁。nullptr 返回会被拒绝，失败的网格与纹理不进入缓存。`luma_gfx` 不链接 GLFW；宿主负责使上下文 current 后加载 GLAD，GPU 操作用宿主无关的 OpenGL 函数加载 / version 查询拒绝缺上下文。

Application 连接 CPU Model、Controller、GLFW 输入、ImGui View 和 Renderer；Qt `QtViewport` 连接同一 CPU MVC 和 Renderer。两种宿主每帧将 Model 的 [RenderSettings](include/luma/render/RenderSettings.hpp) 应用于 Renderer。Qt 当前仍使用 NativeResourceFactory；QtResourceFactory 留给后续。项目工作区通过画面采集回调连接现有视口，模型服务不接触 GL。RenderState 和 DisplayTransfer 保留 OpenGL 渲染与输出编码算法，资源工厂的范围是替换资源包装。

## 操作

- 左侧 `Robot / Texture studio / State lab` 切换三个完整场景，基础资源缓存继续复用。
- `Objects` 选择对象。`Position / Rotation (deg) / Scale` 修改独立变换；缩放保持正数。默认工作台场景不自动旋转，编辑值会保留。
- `sRGB color` 是 UI 的显示颜色输入，写入材质前转换为线性 RGB。`Alpha mode` 支持 Opaque / Mask / Blend；Blend 使用颜色中的 alpha，Mask 使用纹理 alpha 和 Cutoff。
- 材质编号与引用对象数明确显示。相同编号表示共享；`Create independent material` 复制 CPU 材质并只替换当前对象的引用。网格继续共享。
- `Camera: Orbit`：在画面内按住鼠标左键旋转，滚轮拉近 / 拉远。
- `Camera: Fly`：按住右键转向，WASD 移动，Q / E 沿相机提供的 up 参考方向升降，Shift 加速。偏航／俯仰同样围绕该 up 参考建立控制基底，支持俯视或带滚转的相机；切换模式保留位置、朝向、up 与原有观察距离。Orbit 的距离限制只在实际滚轮缩放时应用。
- 操作 UI / 输入文本时相机不会消费对应输入；窗口失焦后清除键盘与鼠标历史状态。
- Esc 退出。窗口最小化时暂停绘制，继续处理事件。

面板较小时可以滚动。当前帧会完整恢复 pass 状态，下一帧不依赖 ImGui 或前一个 pass 遗留的状态。

## 状态实验

`State lab` 放置不透明物体、三个透明薄盒和一个背向三角形。关闭 `Depth test` 可观察提交顺序覆盖；切换 `Back-face cull` 可观察 CCW 正面规则；关闭 `Alpha blending` 可观察透明对象如何覆盖后方颜色。透明队列放在不透明队列后，按中心距离从远到近排列，开启深度测试并关闭深度写入。相交透明表面仍有中心排序的固有限制。

`Stencil selection` 用模板附件记录所选物体的可见片元，再放大绘制边框；它是选择反馈，不是光照或 Toon 风格。`Style: Wireframe` 使用桌面 OpenGL `glPolygonMode`，下一 pass 显式恢复填充。

`Texture studio` 包含真实 PNG 棋盘纹理、颜色相乘与 alpha-cutout 纹理。纹理由项目自行生成，没有外部素材许可依赖。sRGB 纹理由纹理采样器解码；材质和 shader 内部保持线性。宿主查询默认附件实际编码，支持 sRGB 时硬件编码一次；否则在内部线性浮点附件中完成混合，最后显示 pass 编码一次。DisplayTransfer 的内部附件用于正确的输出编码；第二阶段滤镜由独立的 PostProcessTarget / PostProcessPipeline 实现。手动编码路径对用户 FBO 支持单采样、共享同一图像的 D24/S8 或 D32F/S8 深度／模板附件（renderbuffer 或非 layered 纹理）；会查询目标格式并使用匹配的内部格式，因此最终输出仍保留绘制后的深度／模板值。系统默认 framebuffer 不公开 sized internal format；本机 Metal 将默认深度报告为 32 位 normalized，但实际 D24/S8 与 D32F/S8 传输均已用深度／模板读回验证，因此 normalized 默认目标沿用 D24/S8 兼容路径。可用 `luma_gl_tests --hidden --probe-display` 查看当前驱动的真实传输结果。其他布局在绘制前给出 DisplayTarget 错误，不等待不兼容 blit 产生驱动错误。ImGui 的 UI 颜色按其显示颜色约定输出。

## 五个可运行学习例子

```sh
native/build/bin/luma_workbench --example window
native/build/bin/luma_workbench --example triangle
native/build/bin/luma_workbench --example texture
native/build/bin/luma_workbench --example camera
native/build/bin/luma_workbench --example state
```

分别展示空窗口清屏、顶点色三角形、纹理 / cutout、相机加旋转物体、真实渲染状态。加 `--no-ui` 隔离渲染例子。`camera` 例子有自动旋转；其他工作台预设保持静止，便于编辑。

## 自动验证与画面导出

```sh
ctest --test-dir native/build --output-on-failure
native/build/bin/luma_gl_tests --hidden
native/build/bin/luma_lifecycle_tests
native/build/bin/luma_workbench \
  --preset robot --frames 3 --hidden --screenshot /tmp/luma-robot.ppm
```

`ctest` 运行 CPU 行为、相机边界、控制器、无窗口资源工厂、OpenAPI 与本地 HTTP 回归，不创建窗口；HTTP 测试需要允许监听 localhost。控制器覆盖输入原子性、共享 / clone、失效选择、camera / settings；资源替身覆盖缓存键、异常 / nullptr 后重试、释放和 Renderer / DisplayTransfer 初始化失败清理。两个集成程序需要可用图形会话；`--hidden` 仍然创建真实上下文。GL 测试检查实际 GPU 对象所有权、shader 编译失败、缺图错误、缓存、材质克隆以及深度 / 模板 / 混合的像素结果。生命周期测试为 create / update / destroy 分别注入真实回调异常，检查退出状态和回调次数。

`--frames N` 使用固定 1/60 秒模拟步长；`--deterministic` 可单独开启固定步长。`--screenshot` 导出最后一帧交换缓冲前的 **P6 PPM**，包含 UI；配合 `--no-ui` 只导出场景。文件扩展名请使用 `.ppm`，不会因扩展名改成 PNG。只提供截图路径时自动运行一帧。尺寸使用 framebuffer 像素，Retina 上会大于逻辑窗口尺寸。

实际执行记录见 [tests/verification.md](tests/verification.md)，没有图形会话时不能把 GPU 测试当作已通过。

## 源码入口与边界

| 模块 | 阅读入口 | 责任 |
| --- | --- | --- |
| app | `include/luma/app/Application.hpp`, `src/app/Application.cpp` | CPU 配置、运行一次、循环与失败清理 |
| core | `GlfwWindow.hpp`, `Input.hpp` | 单 GLFW runtime、上下文、像素尺寸与输入快照 |
| gfx | `IResourceFactory`, `NativeResourceFactory`, `ShaderProgram`, `Mesh`, `Texture2D`, `RenderState` | typed 资源契约、move-only GPU 所有权、完整 pass 状态 |
| scene | `Transform`, `Camera`, `CameraControls`, `Scene`, `MaterialDesc`, `MaterialLibrary`, `AssetStore` | CPU 数据、强类型 ID、CPU 材质共享、注入的 GPU 缓存 |
| render | `Renderer`, `DisplayTransfer` | 绘制队列、线框、模板选择与最后显示编码 |
| editor | `EditorModel`, `EditorController`, `EditorUI.cpp` | CPU 读取与校验操作、Dear ImGui View 适配 |
| scenes | `scenes/Presets.cpp` | 三个预设与五个学习入口 |

`Application` 构造函数不创建上下文；`run()` 才初始化窗口与 GPU。GPU 对象必须在创建它的有效 current context 中使用和销毁，应用按逆构造顺序保证它们先于窗口销毁。正常创建完成后，`onDestroy()` 恰好调用一次；创建中途失败不调用此完成回调。销毁回调失败仍继续释放资源并返回非零。

查询错误包含资源 ID；shader 错误包含文件路径、阶段与驱动完整编译日志；图片解码错误包含资产路径。实体 clear / erase 后旧 ID 失效，新 ID 不复用。资源缓存随应用存活，首版不提供运行中卸载。材质在创建和提交绘制队列时都验证；不要保存跨容器结构修改的 Transform 引用或对象名 string_view。

资产根路径由 CMake 固定到源码 `assets/`，因此从任何工作目录运行都可找到 shader / PNG。分发二进制时应连同源码资产目录移动后重新配置；尚未实现安装包资源定位。

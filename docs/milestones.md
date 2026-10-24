# 实现里程碑与可复现证据

状态：M1–M4 的原生模块、预设、学习入口、复用示例与 Qt6 中文宿主已实现。构建、CPU / GL 行为和生命周期结果见 [实际验证记录](../native/tests/verification.md)。第二阶段光照与 FBO 后期也已实现，见[本阶段验收](implementation/2026-10-24-stage-two-verification.md)。以下保留各里程碑的验收重点。

## 第一批：用第一阶段知识完成基础工作台

### M1（已实现）：窗口与生命周期

交付 `native/` 的独立 CMake 工程、GLFW runtime、GlfwWindow、InputSnapshot 和 window 示例，先验证可独立复用的宿主模块。Application / SceneLayer 在 M3 连接真实资源与场景，底层窗口模块仍可单独使用。使用 C++17、OpenGL 3.3 Core、GLFW、GLAD 2、GLM；stb_image 在 M2 接入。基础阶段不用高于 3.3 的 API，macOS 请求 Core 并保留 forward-compatible 设置；实际版本和功能在启动时查询。

验收：窗口正常创建、清屏、缩放、最小化恢复和关闭；输出 renderer / version / profile；验证 framebuffer 像素尺寸与 aspect；失败能报阶段原因并清理；无 current context 时不创建 GPU 资源。

文章 01 的实现部分以此为基础。记录依赖版本、构建命令、运行命令、真实输出、截图，以及目前只验证过的平台。

### M2（已实现）：GPU 资源与最小绘制链路

交付 ShaderProgram、MeshData / PrimitiveFactory、Mesh、Texture2D、AssetStore，以及 triangle 和 texture 示例。先让底层独立画出图，再由相同封装支撑工作台。

需要保留的行为证据：

- CPU 几何测试验证三角形绕序与法线方向、索引范围；复用现有 WebGL 原型曾暴露的“平面 / 球体绕序错误”问题作为检查动机，不复制其 GL API。
- 真实上下文中执行编译错误和缺失纹理负例，报告包含文件路径，退出不遗留自己的 GL 对象。
- 画出彩色三角形、索引矩形和带纹理的方块；移动资源对象后仅新对象拥有该 GL 名称。
- 同一 primitive 请求命中缓存，两个纹理加载选项不同的请求不会错误共用一个缓存结果。

文章 02 引用真实资源包装实现、调用链和失败记录。截图证明画面，错误日志证明异常路径，两者不能互相替代。

### M3（已实现）：基础场景工作台

交付 app 层 Application / SceneLayer，连接 Scene、Transform、MaterialDesc、Camera、Orbit / Fly 控制、Renderer、Basic / Wireframe 管线与 CubeStudio。提供几个共享网格、独立变换的物体及纹理地面；不需要外部模型。

先使用快捷键完成切换 / 参数操作，再通过隔离在 editor 目录的 Dear ImGui 面板提供对象列表、变换和材质编辑。第一版对象选择采用列表或快捷键，FBO 颜色 ID 拾取仍在后续计划内，本阶段使用对象列表选择。

验收：修改一个对象不改变其他对象的 Transform；共享 / 独立材质行为符合契约；旋转立方体、透视、滚轮、Orbit、WASD、失焦释放输入正常；修改 UI 参数不会误触发相机或重置场景；窗口零尺寸不会除零。

文章 03 给出完整 SceneLayer 调用链，以及如何复用公共模块添加第二个场景。几何、矩阵等纯 CPU 测试无窗口运行；相机输入和最终画面使用真实应用检查。

### M4（已实现）：第一阶段状态专题

交付可独立进入的 state 示例：深度遮挡、模板选中轮廓、Alpha Mask 与 Alpha Blend、面剔除和线框。每项展示开启 / 关闭的差异，避免只用一个复杂场景同时开启所有效果。

验收重点是 pass 状态泄漏：先关深度写入再切场景、先画线框再画实体、先用模板再画普通对象，结果都正确；透明混合展示排序成功案例及相交几何限制。完成第一版工作台的阶段验收。

当前文章 03 已说明状态实验的操作和调用链；后续独立专题再深入各个像素规则。

## 第二批：第二阶段已实现

可编辑 Blinn-Phong、三类灯光、镜面材质/高光贴图、法线矩阵、FBO 反相/灰度/锐化/两遍模糊已经接入 Qt。工程 v2 保存这些字段且读取 v1；真实 PNG 导出、七阶段人工流程、候选场景恢复和文件预算检查同时可用。完整调用链与验证见[文章 05](../articles/05-stage-two-lighting/index.zh.md)。

## 后续批次

| 版本 | 可见成果 | 新模块 | 对应文章 |
| --- | --- | --- | --- |
| v0.3 编辑增强 | 视口拾取、撤销重做、资源选择 | PickPass、EditorCommand、AssetBrowser | 后续专题 |
| v0.4 模型与环境 | 模型导入、天空盒、阴影、实例化 | ModelImporter、Cubemap、ShadowPass、InstanceBatch | 07–08 |
| v0.5 多风格高级渲染 | Toon、PBR、HDR / Bloom、Neon | ToonPipeline、PbrPipeline、BloomPass、线性 HDR 目标 | 09 |
| v0.6 动态内容 | 粒子与骨骼动画，按能力启用高级功能 | ParticleSystem、Animator、Capabilities | 10 |
| v1.0 软件交付 | 外部资源打包、缺失处理、安装包与后端工程库 | AssetPackage、Repository、发布流程 | 11 |

后续表格用于确定扩展方向，各版本开始时再细化接口、例子和验证方法。

## 每次交付同时留下什么

1. 可运行版本：入口、构建 / 运行命令和实际依赖。
2. 可阅读源码：公共头、实现、shader 和最小使用例子。
3. 证据：有意义的行为测试、实际运行日志和最终截图。
4. 文章：使用方法、完整调用链、关键实现、排错与边界。

文章的命令应能在对应版本上重现。工程尚未成为 Git 仓库；实际开始版本化时再固定 tag 或 commit，当前不能给出不存在的提交号。

关联：[接口契约](api-contract.md) · [文章安排](../articles/README.md)

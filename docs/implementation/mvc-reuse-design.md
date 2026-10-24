# MVC 与渲染模块复用：已确认设计

用户已确认：Qt 可负责界面；OpenGL 使用项目自己的封装，后续可适配 Qt 的资源包装；采用 MVC 分工，为未来业务服务与 AI 调用提供稳定入口。本次明确不加入 AI Pipeline、模型推理、远程服务或空壳占位模块。

## 当前交付边界

1. Model 继续使用 Scene、Camera、Material 等 CPU 数据；从 AssetStore 提取纯 CPU MaterialLibrary，GPU 资源与材质参数分开持有。
2. 新建纯 C++ EditorModel / EditorController。面板读取只读数据，通过控制器选择、设置变换、修改材质和调整视图；脚本示例调用同一入口。控制器不引用 GLFW、Qt、Dear ImGui、OpenGL，也不上传资源。
3. 渲染器与资源缓存依赖 IResourceFactory / IShader / IMesh / ITexture 的稳定契约；自写 OpenGL 包装保留现有实现，通过 NativeResourceFactory 适配。工厂显式注入，应用组合点选择实现。
4. gfx 不检查 glfwGetCurrentContext、不链接 GLFW；GLAD 在宿主上下文 current 后加载。Renderer 借用 framebuffer，宿主提供像素尺寸与编码。Qt UI / QtViewport 已在第一阶段收尾中接入并验收；QtResourceFactory 仍在后续按需实现，本次不把资源包装替换算作完成。
5. 当前 Dear ImGui 界面先接同一控制器，维持三个预设、五个学习入口、MVP、纹理、相机与状态实验行为。Qt 界面以后读同一模型、调用同一控制器。现有 SceneLayer 客户端继续编译。

## 为什么这些边界足够

CPU 模型和操作入口可以无窗口调用；未来 AI / 后端返回的参数先验证，再通过同一控制器落地。Shader / Mesh / Texture 实现可通过工厂替换，场景不依赖具体 GPU 类。RenderState 与显示编码仍是 OpenGL 渲染算法；资源工厂没有宣称可以切换 Vulkan / Metal 等图形 API。

首版不引入事件总线、通用任务调度器、插件注册或节点图。按需要增加边界，以实际第二个调用方（无 UI 示例）和注入测试证明复用。

## 验收

- CPU 控制器测试覆盖非法输入不修改原值、独立变换、材质共享和克隆、选择失效、相机模式与视图设置。
- 无窗口的资源工厂替身验证 AssetStore 缓存、失败不入缓存和资源释放；原生 GL 测试继续验证真实对象及像素。
- 新增无窗口控制器示例，使用真实 Model / Controller，不创建窗口或模拟 AI。
- 完整构建、CPU CTest、GL 像素回归、生命周期及三个实际画面复验。
- 更新三篇中文正文、接口与依赖图；显式标记 Qt 适配和未来服务尚未实现。

工程没有 Git 仓库。第一方基线保存在 `/private/tmp/luma-mvc-before-20261020`，可用于差异审查；不修改第三方依赖、旧 image lab 或其他学习目录。

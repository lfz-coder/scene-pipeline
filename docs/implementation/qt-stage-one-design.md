# 第一阶段收尾与 Qt 宿主设计

目标是把已经确认的 MVC 与自有 OpenGL 封装放进真正的 Qt 桌面软件。后续课程继续扩展同一工程；当前不加入光照、模型加载、PBR 或 AI 的空实现。

Qt 仅负责主窗口、停靠面板、属性控件、输入事件和 OpenGL 上下文。`EditorModel` 保存 CPU 状态，`EditorController` 执行编辑，`Renderer`、`AssetStore` 与 `NativeResourceFactory` 继续使用我们的 GLAD/OpenGL 封装。Qt 视图不得绕过控制器修改编辑状态。GLFW 示例保留用于单独学习，两个宿主共享预设与资源路径代码。

新增 `QtViewport`（QOpenGLWidget 宿主）、`QtInput`（事件转 InputSnapshot）、`WorkbenchWindow`（中文 QMainWindow）。视口每次绘制获取 Qt 当前默认 FBO 和实际像素尺寸；不假定 FBO 为 0。上下文释放或重建时先使其 current，再释放我们的 GPU 资源。异常在 Qt 回调边界转成可见错误，避免穿过 Qt 事件循环。

第一阶段逐项核对发现双纹理混合缺口。为 `MaterialDesc` 增加可选第二纹理和 0..1 混合系数，片元着色器在线性空间混合，保留单纹理默认行为。增加面对相机的纹理正方形学习示例。无效系数与孤立第二纹理必须拒绝，双纹理材质可克隆、可编辑并验证真实 GPU 像素。

验收：CPU 测试、真实 GL 测试、Qt 控件驱动控制器、Qt FBO 渲染、缩放/场景切换/截图/关闭清理。提供运行入口、第一阶段源码映射、实际截图和中文文章更新。未来扩展点以文档和现有模块边界表达，不放无效按钮。

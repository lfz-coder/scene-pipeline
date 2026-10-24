> 历史提纲：三篇完整中文版已交付，阅读 [正文目录](../README.md)。以下保留实现前的安排。

# 文章 01 提纲：从 OpenGL 练习到一款软件

状态：详细提纲。依赖 M1 原生窗口版本，正文与实际运行证据在 M1 完成后补齐。

## 读者任务与判断

读者要把窗口、GLAD 加载和渲染循环从每个例子里提取出来，得到只负责自身内容的场景入口。推荐 `Application + SceneLayer`，因为它把固定的生命周期集中管理，同时让底层 OpenGL 仍可阅读。

适用范围是单窗口、单线程渲染的学习与桌面工作台。多窗口共享、后台 GPU 上传和完整跨 API 引擎不在首篇展开。

## 正文推进

### 从产品目标回到第一版

放现有光场工坊真实截图，说明最终三场景、多画风的方向。明确这张是 WebGL 原型截图，随后展示 C++ 第一版窗口截图，两者标明来源。用产品工作流确定第一版只需要哪些基础能力。

### 把重复的启动顺序收进 Application

对照原始 main：GLFW runtime → 窗口 / context → makeCurrent → GLAD → 基础状态 → 渲染循环。展示 GlfwWindow 的职责与错误传播；`app.run(scene)` 作为最终使用目标单独标注“接口设计，M3 实现”，不用尚不存在的 Scene / AssetStore 拼装伪代码教程。

正文将区分 M1 的实际调用链：`window example main → GlfwWindow → makeCurrent → GLAD → clear / swap`，以及最终架构图中 Application 怎样组织场景。编译 / 运行的完整命令根据 M1 实际工程记录，不预写不存在的构建成功输出。

### 生命周期是封装的核心

解释为何 CPU 相机可以先创建，而 shader / VAO 不能随意在全局构造。画正常退出和初始化中途失败两条清理路径，证明 GPU 对象在窗口上下文之前释放。

展示 framebuffer 像素尺寸与逻辑尺寸的区别，实际检查缩放、最小化恢复和零尺寸保护。Qt 的生命周期仅用来说明借鉴的组织方式，不宣称实现了 QOpenGLWidget。

### 第一版模块如何为下一篇服务

展示 public headers、src、examples 和 assets 的对应关系；通过清屏颜色不同的 window 示例验证宿主初始化链路不必复制。列出 GPU 资源模块将接入的位置，SceneLayer 的落地留到 M3。

## 必须收集的三组证据

1. 实际构建 / 启动输出，包含依赖版本和 GL 版本。
2. 正常缩放、最小化恢复和退出的操作记录。
3. 上下文 / 初始化失败路径的可读错误与清理记录。

## 需要讲清的两个坑

- 在成为 current 之前创建 GPU 资源；以及析构函数在 context 销毁后调用 glDelete。
- 用窗口逻辑尺寸设置像素 viewport，或者高度为零时直接计算 aspect。

## 读者带走的产物

一个可运行的统一窗口入口、一张生命周期图，以及 SceneLayer 的目标接口设计。文章最后给出进入 M2 的操作，不把后续的 SceneLayer / Toon / PBR 列成当前已经实现的成果。

## FAQ 方向

- 为什么用 GLFW，而不是立即引入 Qt？
- 把初始化包进类之后，怎样继续学习真实 OpenGL 调用？
- 资源释放写在析构函数里，为什么还需要规定应用退出顺序？

依据：[项目架构](../../docs/architecture.md) · [接口契约](../../docs/api-contract.md) · [GLFW context guide](https://www.glfw.org/docs/latest/context_guide.html) · [Qt 生命周期](https://doc.qt.io/qt-6/qopenglwidget.html)

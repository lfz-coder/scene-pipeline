# Qt 第一阶段宿主验证报告

日期：2026-10-20。Qt 6.12.0（Homebrew `/opt/homebrew`），macOS Apple M2 Pro，OpenGL 4.1 Core / Metal。

## 调用链

`QMainWindow`（中文 View）→ `QtViewport`（QOpenGLWidget + QtInput）→ `EditorController` / `EditorModel`（CPU MVC）→ `Scene` / `AssetStore` → `Renderer` → `NativeResourceFactory` → 自有 `ShaderProgram` / `Mesh` / `Texture2D`。Qt 没有替换我们的 GPU 资源封装，也没有把 GLFW 循环嵌进 widget。

每帧在 `paintGL()` 中读取 `defaultFramebufferObject()` 和 `devicePixelRatio` 后的像素尺寸，Renderer 从宿主接收目标；不绑定 FBO 0。初始化加载 GLAD，`aboutToBeDestroyed` 和析构时在 current context 清理 GPU 对象。场景切换先创建并验证新 CPU 场景，失败时保留旧场景。

## RED → GREEN

先写 `tests/qt_tests.cpp`，接口未存在时得到 include 缺失；Qt View 实现后，第一次真实运行覆盖了错误 preset、控件提交、FBO、resize、WASD、PNG 和关闭路径。

## 实际命令与结果

```sh
cmake -S opengl_visual_workbench/native -B opengl_visual_workbench/native/build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew -DLUMA_WITH_QT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build opengl_visual_workbench/native/build -j4
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
```

结果：CPU / 资源 / 相机 4 项通过；Qt 行为测试在真实 macOS 窗口会话直接运行通过。Qt GUI 测试默认不注册到 CTest（无屏幕 CI 会在 macOS 启动阶段失败），需要真实屏幕时显式加入 `-DLUMA_REGISTER_QT_TESTS=ON`；本机直接运行退出码 0。Qt 运行时的 macOS IME / visual effect warning 不影响退出码和像素结果。

```text
PASS Qt: controls->MVC, real nonzero FBO pixels, resize, WASD, failure recovery, PNG, close
```

Qt 测试实际验证：对象列表反映真实 Scene；位置 spin box 经 Controller 改变 Transform；场景切换清除旧 EntityId；彩色三角形中心有像素；Qt FBO 非 0；窗口 resize 改变抓取尺寸；WASD 改变 Camera；无效 preset 显示错误且可恢复；`grabFramebuffer()` 与窗口 PNG 导出成功。

真实产品截图：[`qt-workbench.png`](../../articles/assets/qt-workbench.png)。可执行截图命令：

```sh
native/build/bin/luma_qt_workbench --preset quad --screenshot /tmp/luma-qt.png
```

当前 Qt 宿主覆盖第一阶段软件形态；第二阶段的光照、FBO 后处理、模型导入、PBR 和 AI pipeline 没有伪装成已完成按钮。以后增加能力时，沿 `EditorController`、`Renderer` 和 `IResourceFactory` 的边界补充真实模块与测试。

最终产品入口截图已更新为 `articles/assets/qt-workbench.png`，对应真实执行：

```text
Qt OpenGL: 4.1 Metal - 89.4 / Apple M2 Pro
Saved Qt window: /private/tmp/luma-qt-final.png
```

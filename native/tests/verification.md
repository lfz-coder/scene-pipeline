# 实际验证记录

日期：2026-10-19～20。以下按实际执行过程保留记录；最终状态为：完整构建成功、CPU CTest 2/2、真实 GL 像素测试及生命周期集成测试通过。三个预设已导出并目测，原生窗口已实际打开。详细结果在末尾，早期“待执行”只描述当时状态。

## CPU RED → GREEN

先写 `tests/transform_red.cpp`，手工推导输入：位置 `(3,2,1)`、绕 Z 轴旋转 90°、缩放 `(2,3,4)`，点 `(1,0,0)` 应变为 `(3,4,1)`。初始 `Transform::matrix()` 占位实现返回单位矩阵。

```sh
c++ -std=c++17 \
  -Iopengl_visual_workbench/native/include \
  -Iopengl_visual_workbench/native/third_party/glm \
  opengl_visual_workbench/native/tests/transform_red.cpp \
  opengl_visual_workbench/native/src/scene/Transform.cpp \
  -o /tmp/luma_transform_red
/tmp/luma_transform_red
```

真实 RED，退出码 1：

```text
FAIL: TRS expected (3,4,1), got (1,0,0)
```

实现 GLM `translate * mat4_cast * scale` 后，编译并运行完整 `tests/cpu_tests.cpp`：

```text
PASS CPU: TRS, validation, camera inputs, projection, geometry winding, IDs/sharing, materials
```

完整 CPU 用例在实现前已写入；其中 TRS 的 RED 是单独观察到的运行失败。没有把缺失文件 / 链接错误算作功能 RED，也不宣称每个分支都分别观察过 RED。

## CMake build 与 CPU CTest

```sh
cmake -S opengl_visual_workbench/native -B opengl_visual_workbench/native/build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew -DCMAKE_BUILD_TYPE=Debug
cmake --build opengl_visual_workbench/native/build -j4
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
```

AppleClang 17，C++17，GLFW 使用本机 `/opt/homebrew` 包；本次配置未下载依赖。编译得到 `luma_workbench / luma_reuse_example / luma_cpu_tests / luma_gl_tests / luma_lifecycle_tests`，运行文件现位于 `build/bin/`。

首次全量构建发现 `EditorUI.cpp` 缺 GLAD include（`GL_FRAMEBUFFER_SRGB` 未声明）；补上头文件后所有目标成功链接。之后的 CPU CTest 实际输出：

```text
1/1 Test #1: cpu_behaviors .................... Passed
100% tests passed, 0 tests failed out of 1
```

CPU 用例覆盖：TRS 次序、非有限 / 零 / 负缩放与无效四元数、投影参数与 aspect、Fly 输入 / 失焦 / Orbit 滚轮、立方体 / 平面 / 三角形的逐三角形绕序与法线一致性、索引越界、实体清空不复用 ID、实体共享网格但独立变换、材质参数验证。

`build/bin/luma_workbench --help` 已运行成功，列出五个学习入口、有限帧、截图、隐藏窗口与确定时间步长参数。

## GPU / GUI 验证状态

实现者没有把尚未运行的图形测试记为通过。可执行命令为：

```sh
opengl_visual_workbench/native/build/bin/luma_gl_tests --hidden
opengl_visual_workbench/native/build/bin/luma_lifecycle_tests
opengl_visual_workbench/native/build/bin/luma_workbench \
  --preset robot --hidden --frames 3 --screenshot /tmp/luma-robot.ppm
```

`luma_gl_tests` 使用真实上下文 / GPU 对象与 framebuffer 读回，检查 shader / mesh / texture 移动与替换释放、shader 编译负例的路径错误、缺图、纹理选项、规范路径缓存、共享与克隆材质、污染状态后的完整恢复、真实深度与模板像素、透明混合以及手动显示编码约 188 的线性混合结果。它不在 CTest 自动列表中。

`luma_lifecycle_tests` 在真实隐藏窗口下分别执行 onCreate / onUpdate / onDestroy 抛异常的场景，以及正常场景，检查返回值和回调次数。尚未执行的部分包括实际窗口缩放 / 最小化恢复、失焦 UI 手动操作、场景切换与截图目测；由宿主后续验证并追加实际记录。

### 沙箱图形启动尝试

实现者实际尝试 `build/bin/luma_gl_tests --hidden`。编译成功，但沙箱内启动在 AppKit 初始化期间停住，输出：

```text
Connection Invalid error for service com.apple.hiservices-xpcservice.
Failure ... _LSModifyNotification ...
```

这不算 GL 测试通过或算法失败；尚未到像素断言。已通知宿主以图形会话权限运行。此时补充了“完全被遮挡的选中对象不能透过遮挡物变成实心轮廓”真实像素回归；沙箱无法到达断言，已修复 outline 的深度测试与线框状态；此 GL 回归尚未实际运行，不宣称观察到 GL RED。

### 数值边界回归 RED → GREEN

另行先加入以下真实 CPU 回归并逐次运行 `cmake --build ... --target luma_cpu_tests -j4`、`build/bin/luma_cpu_tests`：

1. 有限非零但很小的四元数 `{1e-20,0,0,0}` 应归一化为单位旋转。实际 RED：`FAIL CPU: rotation must be a finite nonzero quaternion`（退出 1）。改用 double 累计长度后 GREEN。
2. `lookAt({FLT_MAX,0,0},{-FLT_MAX,0,0})` 的方向减法溢出，应拒绝。实际 RED：`FAIL CPU: invalid value must throw`（退出 1）。检查方向和长度的有限性，并先归一化再计算叉积后 GREEN。
3. 极小正 FOV / aspect 不能让投影矩阵生成无穷。实际 RED：`FAIL CPU: invalid value must throw`（退出 1）。设置参数前验证生成矩阵全部有限后 GREEN。

每次修复后的实际输出均为完整 CPU `PASS CPU: TRS, validation, camera inputs, projection, geometry winding, IDs/sharing, materials`。

### 最终格式与 CPU 复验

使用本机已有 VSCode C++ 扩展中的 `clang-format` 处理了全部 43 个第一方 `.hpp / .cpp` 文件，未触碰 `third_party/`。配置为 `native/.clang-format`。随后重新编译全部目标，成功且无第一方编译警告；再次执行 CTest：

```text
1/1 Test #1: cpu_behaviors .................... Passed 0.39 sec
100% tests passed, 0 tests failed out of 1
```

最终 `build/bin/luma_workbench --help` 退出 0。GPU、生命周期和实际 GUI 结果仍须由宿主在真实图形会话下执行并追加。

## 2026-10-20 独立评审回归

宿主在真实图形会话中已运行前一版 `build/bin/luma_gl_tests --hidden` 与 `build/bin/luma_lifecycle_tests`，两者退出 0；OpenGL 为 `4.1 Metal - 89.4`，默认目标报告 depth=32、stencil=8、manual sRGB。宿主还实际导出并查看了 2560×1600 机器人截图。此前“GPU 待执行”是实现者早期沙箱状态的历史记录。

### CameraControls：真实 CPU RED → GREEN

新增独立 CPU 目标和 CTest `luma_camera_controls_tests / camera_control_edges`。先运行原实现，真实退出 1，五个场景各自执行并报告：

```text
FAIL idle custom up: Camera lookAt up must not parallel the view direction
FAIL idle roll: mode switch must preserve camera roll
FAIL mode switch distance: idle Orbit must not clamp existing distance
FAIL custom up movement: Camera lookAt up must not parallel the view direction
FAIL custom up rotation: yaw must rotate around custom up rather than world Y
```

修复为 Camera 保留的 up 参考基底；有实际输入才更新 pose，滚轮距离限制只在 dolly 时应用。相同命令 `build/bin/luma_camera_controls_tests` 实际退出 0：

```text
PASS idle custom up
PASS idle roll
PASS mode switch distance
PASS custom up movement
PASS custom up rotation
```

回归包含俯视相机 up=(0,0,-1)、带滚转相机、Fly→Orbit 在距离 100 与 0.1 时的空输入、真实 dolly 限制，以及自定义 up 下的侧移 / 升降 / 偏航 / 俯仰。完整构建后 CTest 2/2 通过。

### DisplayTarget：真实 GL RED 与驱动诊断

评审构建 `/tmp/luma-review-display`，宿主在真实图形会话执行，创建完整 D32F/S8 目标后捕获实际错误：

```text
complete=1
FAIL borrowed depth32f/stencil8 target: final display encoding: OpenGL error 0x502
```

探针程序捕获异常后本身退出 0，不能把此退出码当作渲染通过；上面是实际 GL RED。

修复后按用户 FBO 附件的类型 / 精度匹配内部 packed D24/S8 或 D32F/S8，并把格式加入附件缓存条件；缺失、不共享、layered 或多采样布局在绘制前报告 DisplayTarget 契约错误。系统默认 framebuffer 不提供 sized internal format，不能仅按报告的位数推断其私有存储。第一次严格分类在本机默认 depth32 normalized 上被拒绝，因此专门增加真实诊断：

```sh
opengl_visual_workbench/native/build/bin/luma_gl_tests --hidden --probe-display
```

宿主执行的实际输出：

```text
DEFAULT depthBits=32 depthType=0x8c17 stencilBits=8 samples=0
PROBE sourceFormat=0x88f0 blitError=0x0 outputDepth=0.25 outputStencil=7
PROBE sourceFormat=0x8cad blitError=0x0 outputDepth=0.25 outputStencil=7
```

`0x8c17` 为 unsigned normalized；D24/S8 (`0x88f0`) 和 D32F/S8 (`0x8cad`) 向此系统默认目标传输都无 GL 错误，并真正读回指定深度和模板。系统 normalized 默认目标因此保留已验证的 D24/S8 路径；用户创建的 FBO 仍按具体格式匹配。

正式 `luma_gl_tests` 已加入同尺寸 D24/S8 renderbuffer → D32F/S8 renderbuffer → D32F/S8 texture → D24/S8 的切换回归，逐次检查编码后的混合颜色、深度与模板，并验证缺失深度／模板被清晰拒绝且不产生 GL 错误。最终版本的宿主复验结果待追加。

### 最终真实 GL GREEN（宿主执行）

宿主对最终修复版本实际执行：

```sh
opengl_visual_workbench/native/build/bin/luma_gl_tests --hidden
```

真实退出码 0，输出：

```text
OpenGL 4.1 Metal - 89.4; depth=32 stencil=8; output=manual sRGB
PASS GL: resource moves, negative shader/image, cache, material clone, poisoned-state reset, real depth/blend/stencil pixels; no GL errors
```

这一版正式测试包含 D24/S8 renderbuffer、D32F/S8 renderbuffer、D32F/S8 texture 的同尺寸切换，以及最终混合颜色 / 深度 / 模板读回和缺失附件的无 GL 错误拒绝；这些断言均实际执行并通过。D32F/S8 的原始 GL RED 至此获得真实 GREEN，系统默认目标兼容路径也通过完整场景链路。

宿主另实际运行 texture / state 两个预设各 3 帧，导出 2560×1600 图像；两个运行均退出 0，报告无 OpenGL 错误。最终 CPU 本地复验为 CTest 2/2 通过，完整构建成功。窗口缩放、焦点与面板交互等手动 GUI 检查由宿主另行记录，不将截图导出等同于这些交互全部验证。


## 宿主最终复核与交互边界

重新执行完整 `cmake --build ... -j4` 成功，CTest 2/2 通过。独立复审确认自定义 up、空输入距离与借用 FBO 三项问题均已解决。

机器人、纹理和状态实验的 PPM 读回已转为 PNG 并逐张目测，保存在 `articles/assets/native-robot.png`、`native-texture.png`、`native-state.png`。原生窗口通过临时测试 app bundle 实际打开并观察，画面正确；现有自动化无法稳定向该窗口发送面板操作，因此没有把场景按钮、滑杆、窗口最小化和失焦的手动交互全数记为验收通过。输入路由与相机行为有 CPU 回归覆盖，复杂真实交互仍建议在本机继续体验。临时 bundle 属于 build 目录测试产物，不是软件安装包。


宿主最终逐个执行 `--example window / triangle / texture / camera / state --no-ui --hidden --frames 3`，五个进程均退出 0，输出 `Completed 3 frames; no OpenGL errors`。最终重跑生命周期测试退出 0，报告 `PASS lifecycle: create/update/destroy failures, cleanup, run-once`。验证启动的临时窗口与早期挂起测试在结束时清理。


## 2026-10-20 MVC / 资源工厂改造最终复验

本轮已实现 MaterialLibrary、EditorModel / EditorController、IResourceFactory 与 NativeResourceFactory，并接入现有面板。完整实施和 RED / GREEN 记录见 [原生报告](../../docs/implementation/mvc-native-report.md)，宿主运行详情见 [本轮验证记录](../../docs/implementation/mvc-host-validation.md)。

- 完整构建成功；另使用全新 Debug 目录构建成功，无编译警告。
- CTest 4/4 通过：controller_behaviors、resource_factory_behaviors、cpu_behaviors、camera_control_edges。
- luma_controller_example 实际退出 0；otool 只列 C++ 与系统库，不链接 GLFW / OpenGL。
- 最终 luma_gl_tests --hidden（含 GLAD 加载后 detach current 的新断言）退出 0，报告 PASS GL；luma_lifecycle_tests 退出 0，报告 PASS lifecycle。
- robot / texture / state 带面板各运行 3 帧并导出实际 2560×1600 画面，逐张目测正常；window / triangle / texture / camera / state 五入口各运行 3 帧，全部无 GL 错误。

测试平台仍为 OpenGL 4.1 Metal - 89.4，depth=32、stencil=8、manual sRGB。截图位于 build/captures/mvc-*.ppm，博客内 PNG 已更新。本轮未扩大手动鼠标、缩放、最小化和失焦的完整交互验收声明。没有 Qt 运行适配器、后端或 AI Pipeline。


## 2026-10-24 第二阶段最终验收

本轮完整构建成功，CTest 8/8、真实 GL 基础与光照/后期测试、Qt 交互测试、生命周期测试和光照复用示例均通过。新增文件预算检查覆盖 119 个手写原生文件；中文阅读页为五篇文章，3 项构建测试通过。

完整命令、像素范围、真实失败回归、截图、驱动诊断和限制以[第二阶段验收记录](../../docs/implementation/2026-10-24-stage-two-verification.md)为准。本文更早的“待执行”和“没有 Qt”属于历史版本记录。

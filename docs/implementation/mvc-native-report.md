# MVC 原生实现报告

状态：DONE。任务 1 的原生实现、完整构建、无窗口测试与第二调用方完成。没有 Git 仓库，未创建 worktree、未提交。原生实现者没有启动 GUI；真实图形验证由宿主执行并记录。

## 已实现 API

- `scene/MaterialLibrary.hpp`：`createMaterial(MaterialDesc)`、`cloneMaterial(MaterialId)`、`material(MaterialId)` 的 const / mutable 重载。CPU 材质和 GPU 缓存分离；AssetStore 同名便利方法转发到 `materials()`，现有 SceneLayer 调用端保留。
- `editor/EditorModel.hpp`：`EditorModel(Scene&, MaterialLibrary&, Camera&, CameraControls&)`；只读 `scene()`、`materials()`、`camera()`、`controls()`、`options()`、`selected()`、`selectedMaterial()`、`style()`。借用依赖必须比模型活得更久。
- `editor/EditorController.hpp`：`select(EntityId)`、`clearSelection()`、`reconcileSelection()`；`setTransform(glm::vec3, glm::quat, glm::vec3)`；`setMaterial(MaterialDesc)`；`cloneSelectedMaterial() -> MaterialId`；`setCameraMode(CameraMode)`；`setCameraPose(glm::vec3 eye, glm::vec3 target, glm::vec3 up = {0,1,0})`；`setStyle(StyleKind)`；`setRenderOptions(RenderOptions)`。
- `render/RenderSettings.hpp`：独立于 Renderer 的纯 CPU `StyleKind`、`RenderOptions`。`setRenderOptions` 保留当前已验证的 selection；选择通过专用操作更新。
- `gfx/IResourceFactory.hpp`：IShader 的 `use() const` 与 int / float / vec4 / mat4 typed `set`；IMesh 的 `draw() const`；ITexture 的 `bind(unsigned) const`。虚析构，均不公开原生 handle 或窗口类型。
- `IResourceFactory::createShader(vertex, fragment)` / `createMesh(const MeshData&)` / `createTexture(path, const TextureOptions&)` 返回对应 `unique_ptr`。NativeResourceFactory 委托原 ShaderProgram / Mesh / Texture2D，保留其 move-only 所有权。
- `AssetStore(IResourceFactory&)`、`Renderer(IResourceFactory&)`、`DisplayTransfer(IResourceFactory&)` 显式注入。工厂必须比消费者活得更久。nullptr 返回被拒绝；失败的网格 / 纹理不进入成功缓存。
- `EditorUI::inspect(const EditorModel&, EditorController&, double)` 不再拿到可变 SceneContext 或 Renderer。Application 是组合点，每帧将 model settings 赋给 Renderer。
- `examples/controller_example.cpp`：真实 CPU 第二调用方，选择对象 → 修改变换 / 共享颜色 → clone → 独立颜色，并断言另一对象保持原值；仅链接 luma_cpu。

控制器不包含 OpenGL、GLFW、ImGui 或 Qt 头文件。多字段编辑先验证副本，再提交；非法 enum、非有限 / 非法范围数据被拒绝。失效选择有 reconcile / clear 路径。共享 MaterialId 共享编辑，clone 仅替换当前实体的材质引用。

## RED / GREEN 记录

先添加 `tests/controller_tests.cpp` 与 `tests/resource_factory_tests.cpp`，再实现缺失 API。初始 API RED：

```sh
c++ -std=c++17 -Iopengl_visual_workbench/native/include \
  -Iopengl_visual_workbench/native/third_party/glm -fsyntax-only \
  opengl_visual_workbench/native/tests/controller_tests.cpp
# exit 1
fatal error: 'luma/editor/EditorController.hpp' file not found

c++ -std=c++17 -Iopengl_visual_workbench/native/include \
  -Iopengl_visual_workbench/native/third_party/glm -fsyntax-only \
  opengl_visual_workbench/native/tests/resource_factory_tests.cpp
# exit 1; 12 errors
error: unknown class name 'IMesh'
error: no matching constructor for initialization of 'AssetStore'
error: no member named 'materials' in 'luma::AssetStore'
```

以上只证明 API 尚未实现。运行时行为 RED 另外实际执行：先加入非法 AlphaMode 和非法 Wrap enum 的拒绝断言，旧校验接受这些非法值。

```sh
cmake --build opengl_visual_workbench/native/build \
  --target luma_controller_tests luma_resource_factory_tests -j2
ctest --test-dir opengl_visual_workbench/native/build \
  -R 'controller_behaviors|resource_factory_behaviors' --output-on-failure
# exit 8
1/2 Test #1: controller_behaviors .............Subprocess aborted
libc++abi: terminating due to uncaught exception of type std::runtime_error: invalid input accepted
2/2 Test #2: resource_factory_behaviors .......Subprocess aborted
libc++abi: terminating due to uncaught exception of type std::runtime_error: expected resource failure
0% tests passed, 2 tests failed out of 2
```

补齐 MaterialDesc / TextureOptions 校验后的同命令 GREEN：

```text
1/2 Test #1: controller_behaviors ............. Passed
2/2 Test #2: resource_factory_behaviors ....... Passed
100% tests passed, 0 tests failed out of 2
```

MaterialLibrary 提取后，新增保留资源 ID 诊断的回归断言；`.at()` 默认消息不含 MaterialId，先观察 RED，再恢复原有有用诊断：

```sh
cmake --build opengl_visual_workbench/native/build --target luma_resource_factory_tests -j2
ctest --test-dir opengl_visual_workbench/native/build -R resource_factory_behaviors --output-on-failure
# RED exit 8: std::runtime_error: wrong resource failure
# GREEN exit 0 after reporting "invalid MaterialId 999"
1/1 Test #2: resource_factory_behaviors ....... Passed
100% tests passed, 0 tests failed out of 1
```

## 最终验证

现有工作目录完整 configure / 全 target build / CTest / 示例：

```sh
cmake -S opengl_visual_workbench/native -B opengl_visual_workbench/native/build -DBUILD_TESTING=ON
cmake --build opengl_visual_workbench/native/build -j2
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
opengl_visual_workbench/native/build/bin/luma_controller_example
```

```text
[100%] Built target luma_gl_tests
1/4 Test #1: controller_behaviors ............. Passed
2/4 Test #2: resource_factory_behaviors ....... Passed
3/4 Test #3: cpu_behaviors .................... Passed
4/4 Test #4: camera_control_edges ............. Passed
100% tests passed, 0 tests failed out of 4
CPU controller: selection, transform, shared color and independent clone passed
```

另外使用新建构建目录排除旧缓存：

```sh
cmake -S opengl_visual_workbench/native -B /private/tmp/luma-mvc-clean-build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build /private/tmp/luma-mvc-clean-build -j2
ctest --test-dir /private/tmp/luma-mvc-clean-build --output-on-failure
/private/tmp/luma-mvc-clean-build/bin/luma_controller_example
```

```text
-- The CXX compiler identification is AppleClang 17.0.0.17000013
-- Configuring done
-- Generating done
[100%] Built target luma_gl_tests
100% tests passed, 0 tests failed out of 4
Total Test time (real) = 1.30 sec
CPU controller: selection, transform, shared color and independent clone passed
```

无编译警告。最后新增的真实上下文 detach / restore 断言已单独重建 `luma_gl_tests`；宿主将重跑其真实 GL 验证。

`otool -L native/build/bin/luma_controller_example` 仅列 libc++ 与 libSystem。资源工厂替身测试仅额外链接 OpenGL.framework，没有 GLFW。`nm -u libluma_gfx.a` 无 GLFW 符号。`luma_gfx` CMake PUBLIC 依赖为 luma_cpu / luma_glad / OpenGL::GL，GLFW 由 luma_core 负责。

测试覆盖：

- TRS 多字段提交失败时 position / rotation / scale 保持原值；两个实体的变换独立。
- 材质共享编辑、clone 后独立编辑、非法 alpha / cutoff 不提交。
- 有效 / 无效选择，erase / clear 后 reconcile，主动 clear，失效选择拒绝编辑 / clone。
- camera mode 不改变已有 pose；无效 pose 不提交；style / 全部 render flags / 背景校验。
- AssetStore 网格缓存、纹理路径规范化与 TextureOptions 缓存身份、异常 / nullptr 后重试、非法资源 ID、作用域释放。
- Renderer 首 shader nullptr 拒绝、DisplayTransfer shader nullptr 拒绝、第二 shader 失败释放第一个临时 shader，错误来源有消息断言避免误由缺上下文通过。
- 未加载 GLAD 时 requireContext 拒绝；真实 GL 测试另覆盖 GLAD 已加载但当前上下文被宿主 detach 时拒绝。
- 原 GPU ShaderProgram / Mesh / Texture2D 移动语义和真实像素测试仍在，未改为替身。

## 自审与范围

对照第一方基线 `/private/tmp/luma-mvc-before-20261020` 逐段审查 Application、Renderer、DisplayTransfer、RenderState 差异，生成 `/private/tmp/luma-mvc-native.diff`。没有改第三方、预设、SceneLayer、reusable_scene 或其他学习目录。

- Application 的 runtime → window → factory → assets → CPU state → renderer 顺序保证反向释放时资源早于上下文；原 onCreate / onUpdate / onDestroy 异常协议保持。
- RenderState 仅替换 current-context 检查：先验证 glGetString 函数已加载，再检查 `glGetString(GL_VERSION)`，不查询 GLFW。
- Renderer draw ordering、depth / stencil / blend / cull / wireframe 状态逻辑保持；多增加一次明确 IShader::use，并通过接口写 typed uniforms。
- DisplayTransfer 的默认 FBO macOS depth32 normalized 兼容路径、D24/S8 / D32F/S8 精度查询、线性中间附件、深度模板传输和 final sRGB 编码逻辑保持。
- 同帧 clone 后 UI 使用新材质副本继续编辑，避免后续 widget 改到旧的共享材质。
- NativeResourceFactory 内部持有并委托原 move-only 包装，接口不泄漏原生 GPU handle。

限制：本轮实现当前 GLFW / ImGui 和无窗口 CPU 调用方，没有 Qt UI / QtResourceFactory / AI / 网络。DisplayTransfer 内部 framebuffer、附件、全屏 VAO 仍是 OpenGL 算法自身所有权；资源工厂不构成全图形 API abstraction。单线程同步编辑；材质库允许 SceneLayer 使用原 mutable 便利接口，View 只拿到 const Model。根 CMake 仍配置图形依赖，CPU 示例运行时不需要窗口或图形会话。真实 GL / 生命周期 / 截图由宿主负责最终记录，不把替身测试等同于真实像素验证。

## 修改文件

- `native/CMakeLists.txt`
- `native/README.md`
- `native/examples/controller_example.cpp`
- `native/include/luma/editor/EditorController.hpp`
- `native/include/luma/editor/EditorModel.hpp`
- `native/include/luma/editor/EditorUI.hpp`
- `native/include/luma/gfx/IResourceFactory.hpp`
- `native/include/luma/gfx/NativeResourceFactory.hpp`
- `native/include/luma/gfx/Texture2D.hpp`
- `native/include/luma/gfx/TextureOptions.hpp`
- `native/include/luma/render/DisplayTransfer.hpp`
- `native/include/luma/render/RenderSettings.hpp`
- `native/include/luma/render/Renderer.hpp`
- `native/include/luma/scene/AssetStore.hpp`
- `native/include/luma/scene/MaterialDesc.hpp`
- `native/include/luma/scene/MaterialLibrary.hpp`
- `native/src/app/Application.cpp`
- `native/src/editor/EditorController.cpp`
- `native/src/editor/EditorUI.cpp`
- `native/src/gfx/NativeResourceFactory.cpp`
- `native/src/gfx/RenderState.cpp`
- `native/src/gfx/Texture2D.cpp`
- `native/src/gfx/TextureOptions.cpp`
- `native/src/render/DisplayTransfer.cpp`
- `native/src/render/Renderer.cpp`
- `native/src/scene/AssetStore.cpp`
- `native/src/scene/MaterialDesc.cpp`
- `native/src/scene/MaterialLibrary.cpp`
- `native/tests/controller_tests.cpp`
- `native/tests/gl_tests.cpp`
- `native/tests/resource_factory_tests.cpp`

## 最终修复

按最终审查唯一 Minor，修正 `docs/architecture.md:202` 的“已完成的第一阶段”表格：将不存在的 `BasicPipeline` 替换为 `Renderer（Basic / Wireframe）`。未新增类，原生源码未变动。

修复命令：Python `Path.read_text()` 后对唯一表格单元执行 `replace('| 核心资源、BasicPipeline、Scene、CameraControls |', '| 核心资源、Renderer（Basic / Wireframe）、Scene、CameraControls |')`；写回前断言原单元仅出现一次，写回后断言全文不存在旧类型名。

检查命令与实际输出：

```text
Python assertion over docs/architecture.md:
architecture.md: BasicPipeline occurrences = 0
architecture.md: exactly one table-cell replacement applied

rg -n 'class Renderer|void setStyle|StyleKind style_' native/include/luma/render/Renderer.hpp
18:class Renderer {
22:    void setStyle(StyleKind style) {
41:    StyleKind style_ = StyleKind::Basic;

rg -n 'enum class StyleKind' native/include/luma/render/RenderSettings.hpp
5:enum class StyleKind { Basic, Wireframe };

rg -n '已完成的第一阶段' docs/architecture.md
202:| 已完成的第一阶段 | 纯色 / 纹理场景、MVP、相机、深度 / 模板 / 混合 / 剔除、线框 | 核心资源、Renderer（Basic / Wireframe）、Scene、CameraControls | 创建物体和交互 |
```

文档文本修改不影响运行，按宿主指示未重跑原生 / GPU 全套测试。此修复批次仅修改该表格单元并追加本报告，现已冻结。

# Scene Pipeline · 光场工坊

**C++17 / Qt6 / OpenGL Core 桌面 3D 创作软件的源码工程。** 主程序是 `luma_qt_workbench`，软件界面、OpenGL 资源封装、渲染器、场景编辑、AI 对话和时间线的实现均在 `native/`。

Qt 负责界面、输入和 OpenGL 上下文；渲染继续使用项目自己的 Shader、Mesh、Texture、Camera 与 Renderer 封装。MVC 模型与控制器可由 Qt、GLFW 或无窗口程序复用。

## 项目源码入口

| 内容 | 目录或文件 |
| --- | --- |
| **完整 C++ 工程与 CMake 构建入口** | **[native/](native/) · [CMakeLists.txt](native/CMakeLists.txt)** |
| Qt 主程序入口 | [native/src/qt/main.cpp](native/src/qt/main.cpp) |
| Qt 窗口、首页、常驻助手与项目页面 | [native/src/qt/](native/src/qt/) |
| 自有 OpenGL 资源封装 | [native/src/gfx/](native/src/gfx/) |
| 光照、FBO 与后处理渲染 | [native/src/render/](native/src/render/) |
| 场景、材质、相机与 MVC 编辑 | [native/src/scene/](native/src/scene/) · [native/src/editor/](native/src/editor/) |
| 对话适配、项目文档与 Pipeline / Timeline 模型 | [native/src/studio/](native/src/studio/) |
| 公开头文件与复用接口 | [native/include/luma/](native/include/luma/) |
| GLSL 着色器与必需图片资源 | [native/assets/](native/assets/) |
| 场景配方与独立调用示例 | [native/scenes/](native/scenes/) · [native/examples/](native/examples/) |
| C++ / Qt 测试 | [native/tests/](native/tests/) |
| HTTP 契约与 Schema | [api/openapi.json](api/openapi.json) · [api/schemas/](api/schemas/) |

在 Qt Creator、CLion 等 IDE 中，打开 **`native/CMakeLists.txt`** 导入源码工程。仓库未提交编译后的程序，克隆后从源码构建。

## 从源码构建与运行

需要 CMake 3.20+、C++17 编译器、OpenGL、GLFW，以及 Qt6 Widgets / OpenGLWidgets / Test / Network / Multimedia。GLM、GLAD、stb_image 与 Dear ImGui 的源码和许可证已在 `native/third_party/` 中；缺少本机 GLFW 时，CMake 会下载固定版本。

以下命令适用于 Apple Silicon Mac 的 Homebrew 环境，从仓库根目录执行：

```sh
git clone https://github.com/lfz-coder/scene-pipeline.git
cd scene-pipeline
cmake -S native -B native/build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DLUMA_WITH_QT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build native/build -j4
native/build/bin/luma_qt_workbench
```

也可以双击 [启动Qt工作台.command](启动Qt工作台.command)，脚本会增量编译再运行。Qt 安装在其他目录时，将 `CMAKE_PREFIX_PATH` 改为实际路径；更多启动参数、依赖版本和 GLFW 学习入口见 [原生工程说明](native/README.md)。本机目录仍可叫 `opengl_visual_workbench`，构建不依赖克隆目录名。

## 当前实现

- **整体创作界面**：首页需求输入、参考图与七个模板；项目内常驻助手，顶部切换方案、3D 场景、镜头、媒体与时间线。
- **第一阶段 OpenGL**：窗口与输入、VAO / VBO / EBO、GLSL、图片纹理、MVP、场景与相机，以及深度、模板、混合、剔除和线框。
- **第二阶段光照**：Blinn-Phong、法线矩阵、方向光 / 点光源 / 聚光灯、共享材质、底色与高光贴图，以及 FBO 反相、灰度、锐化和两遍模糊。
- **人工创作流程**：场景 → 材质 → 光照 → 镜头 → 检查 → 后处理 → 导出；上游修改使下游确认失效，支持工程保存 / 恢复、机位快照和真实 PNG 导出。
- **对话与时间线**：LLM / VLM 接口、附图、取消、受限镜头建议、人工确认后入轨；本地图片和真实视频预览、定位、裁剪、拆分、重排与 `.luma` 工程保存。

默认模型服务是明确标注的本地演示。HTTP 适配器遵循 [OpenAPI 3.1.1](api/openapi.json)，Go 后端目前交付 [约束文档](docs/backend-go-contract.md)，尚未实现服务。原生软件的外部模型、Toon、PBR、阴影、HDR / Bloom 等随第三、四阶段继续扩展。时间线当前为单轨，尚无视频生成、多轨合成或视频编码导出。

[Qt 项目实际界面](articles/assets/qt-studio-wide.png) · [第二阶段验收](docs/implementation/2026-10-24-stage-two-verification.md) · [整体布局验收](docs/implementation/2026-10-24-ai-project-layout.md)

## 源码验证

完成上述构建后，从仓库根目录执行：

```sh
ctest --test-dir native/build --output-on-failure
python3 tools/check-native-boundaries.py
python3 articles/tests/test_reader_build.py
```

CTest 默认执行 11 项 CPU、工作流、资源边界、契约和服务检查。真实 OpenGL / Qt 交互测试为独立可执行程序，需要桌面与图形上下文；运行方法见 [native/README.md](native/README.md)。

## 模块约束与中文文章

- [开发边界](docs/development-boundaries.md)：模块职责、依赖、文件行数、GPU 生命周期与界面去重。
- [架构与调用链](docs/architecture.md) · [类与接口契约](docs/api-contract.md) · [后续阶段](docs/milestones.md)。
- [六篇中文博客原稿](articles/README.md)：架构、GPU 资源、场景与相机、Qt 工作流、光照 / FBO、对话与时间线。
- [中文版离线阅读文件](articles/index.html)：下载后用浏览器打开；源码实现仍以 `native/` 为准。

所有后续提交的标题和正文使用中文，约束保存在 [AGENTS.md](AGENTS.md)。构建目录、缓存、个人 IDE 配置和服务令牌不入库。

## 辅助 WebGL 原型

根目录 [index.html](index.html) 和 [workbench.js](workbench.js) 是早期产品视觉原型，演示未来多风格目标。操作与生成方式见 [WebGL 原型说明](docs/web-prototype.md)。其中高级视觉效果的实现范围与当前 C++ 阶段不同，桌面软件的已实现功能以上文及原生源码为准。

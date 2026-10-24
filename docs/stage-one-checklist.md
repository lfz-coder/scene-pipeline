# 第一阶段验收与后续扩展

第一阶段基础能力和 Qt 软件宿主已实现。当前画面是无光照纯色、顶点色和纹理；二次元光照、写实材质和模型等随以后课程加入。产品目标仍是同一个多风格 3D 工作台。

从工程目录双击 `启动Qt工作台.command`，或执行 `native/build/bin/luma_qt_workbench`。顶部场景菜单提供所有学习入口。中文 [博客阅读版](../articles/index.html) 和 [运行说明](../native/README.md) 都在项目内。

| 第一阶段内容 | 实际源码 | 观察 / 验证 |
|---|---|---|
| GLFW 窗口、输入、循环、资源释放 | native/src/core/GlfwWindow.cpp、Input.cpp；src/app/Application.cpp | GLFW window 示例；luma_lifecycle_tests |
| GLAD、GLM、stb_image | third_party；src/scene/Transform.cpp；src/gfx/Texture2D.cpp | 真实 GL 4.1；CPU TRS；图片上传 |
| Core 状态机、顶点到片元管线 | src/gfx/RenderState.cpp；assets/shaders/unlit.vert/.frag | triangle、state；状态污染回归 |
| VAO / VBO / EBO 和属性 | src/gfx/Mesh.cpp、MeshData.cpp | 彩色三角形、索引正方形；绕序测试 |
| GLSL 编译、链接、uniform、in/out | src/gfx/ShaderProgram.cpp；unlit shaders | 顶点色插值；编译失败携带路径 |
| 图片、纹理单元、环绕、过滤、mipmap | Texture2D.cpp、TextureOptions.cpp | quad；TextureOptions 可复用配置；缓存区分采样选项 |
| 多纹理 | MaterialDesc、Renderer.cpp、unlit.frag | texture 中间方块；双纹理滑杆；GPU 0 / 0.5 / 1 像素 |
| TRS、MVP、各坐标空间 | Transform.cpp、Camera.cpp、unlit.vert | 属性面板位置 / 旋转 / 缩放；CPU 矩阵结果 |
| 第一人称、欧拉视角、鼠标和 WASD | CameraControls.cpp、QtInput.cpp | camera 场景 + 第一人称；Qt 实际按键测试；相机保留任意 up，内部用四元数稳定更新 |
| 视锥与近远裁剪 | Camera::setPerspective、setAspect、projection | FOV/near/far API 校验；窗口缩放与投影测试 |
| 深度、模板、Alpha、剔除 | Renderer.cpp、RenderState.cpp | state 场景；右侧状态控件；真实 depth / stencil / blend 像素 |
| 可复用封装与 MVC | editor、gfx、scene、render；examples | UI → Controller → Model → Renderer；独立场景及无窗口调用示例 |
| Qt 软件界面与自有 OpenGL | src/qt、include/luma/qt | 对象列表、属性栏、视口、相机、PNG；luma_qt_tests |

四个小项目已具备入口：纯色窗口 `window`；彩色三角形 `triangle` 与纹理正方形 `quad`；`camera` 场景包含持续旋转的立方体头部与多个立方体；同一场景切换第一人称即可漫游。GLFW 命令可用 `--example` 或 `--preset`，Qt 用顶部菜单。

“预留”通过现有模块边界和新增功能位置表达。下表是后续学习任务，还没有实现；界面中不放无效按钮。

| 后续学习 | 在现有工程增加什么 | 可复用什么 |
|---|---|---|
| 第二阶段光照 / FBO | Light、光照 shader、材质贴图、后期 pass、灯光面板 | Scene / Mesh / Texture / Camera / Qt 布局 |
| 第三阶段模型 / 阴影 | Assimp 导入、Cubemap、ShadowPass、实例数据 | 资源工厂、MeshData、材质库、Renderer |
| 第四阶段高级效果 | HDR / Bloom、PBR、粒子、骨骼；按 GPU 能力决定高版本算法 | 场景数据、资源所有权、编辑控制器 |
| 第五阶段软件化 | 工程保存 / 打开、撤销、资源浏览、稳定发布 | 现有 MVC 与独立渲染服务 |
| 后端或 AI（以后按需要） | 宿主接收业务结果并调用已支持的控制器操作 | CPU Model / Controller；当前无 AI pipeline |

QtResourceFactory 也尚未实现。现在 Qt UI 确实使用自己的 NativeResourceFactory；以后若改用 Qt 的 Shader/Buffer 包装，再实现同一 IResourceFactory 契约。它不是简单更换 include，仍要适配矩阵、纹理与释放语义。

证据见 [Qt 验证报告](implementation/qt-stage-one-qt-report.md) 与 [双纹理报告](implementation/stage-one-textures-report.md)。已实测 macOS / Apple M2 Pro；其他平台尚未验收。Qt 运行依赖本机 Qt，暂未打包成可分发安装包。

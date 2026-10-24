# 第二阶段验收记录

日期：2026-10-24。平台：macOS、Apple M2 Pro、AppleClang 17、OpenGL 4.1 Metal - 89.4、Qt6。以下结果来自本机执行与 framebuffer 读回，不把 HTML 产品原型当成原生效果证明。

## 交付范围

Blinn-Phong 环境/漫反射/镜面光、逆转置法线矩阵、镜面材质与高光贴图、方向/点/聚光及多光源、FBO 反相/灰度/锐化/水平垂直两遍模糊，均已接入真实 Qt 工作台。原有 Unlit、第一阶段状态实验与 GLFW 宿主保留。

人工流程为“场景 → 材质 → 光照 → 镜头 → 检查 → 后处理 → 导出”。`.luma` v2 保存新增参数，兼容 v1；资源描述使用内置相对路径与完整 TextureOptions，磁盘不保存运行期 TextureId。候选 Scene 完成所有资源解析与文档应用后才替换旧场景。

## 构建与实际结果

```sh
cmake -S opengl_visual_workbench/native -B opengl_visual_workbench/native/build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew -DLUMA_WITH_QT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build opengl_visual_workbench/native/build -j6
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
```

本轮执行过全量干净构建；修复审查问题后再次完整构建成功。最终编译日志未出现第一方 warning/error。Qt 配置提示可选的 WrapVulkanHeaders 缺失，OpenGL 目标正常生成，无需为该项目安装 Vulkan。

| 检查 | 实际结果 | 主要证据 |
| --- | --- | --- |
| CTest | 8/8，0 失败 | 文件边界、CPU、Controller、工作流、光照/后期值、资源替身、相机 |
| `luma_gl_tests --hidden` | 退出 0，PASS GL | 真实资源、缓存、旧状态像素、显示编码、深度与模板 |
| `luma_lighting_gl_tests --hidden` | 退出 0，PASS GL lighting | 斜面非均匀缩放法线、方向/点/聚光、距离衰减、多光源、滤镜 |
| `luma_lifecycle_tests` | 退出 0，PASS lifecycle | 光照/后期场景的创建、更新、销毁异常及资源清理 |
| `luma_qt_tests` | 退出 0，PASS Qt | 真实 QOpenGLWidget、界面编辑、WASD、缩放、工程恢复、PNG |
| `luma_lighting_example` | 退出 0，PASS lighting example | 三盏灯、shininess 96、灰度强度 0.35 的无窗口 CPU 配方 |
| `check-native-boundaries.py` | 119 个手写原生文件通过 | cpp 360、hpp 260、Qt cpp 320、GLSL 180、测试 420 |
| 文章阅读页测试 | 3/3 | 五章、离线图像、SVG 解析、链接变换与 script 安全序列化 |
| 本地文档链接检查 | 通过 | 第五篇文章、接口契约、开发边界、里程碑的文件引用存在 |

图形测试需真实桌面上下文，未注册为默认 CTest；不能用 Qt offscreen 插件代替 QOpenGLWidget。运行 GPU/Qt 命令时使用本机图形会话权限。

## 有意义的回归

- 删除方向光后重绘，必须清空前一帧 uniform，不能留有“幽灵灯”。点光源单独照亮物体，移远后实际像素变暗；聚光锥内亮、锥外暗。
- 法线使用斜面网格与 `(4,1,1)` 非均匀缩放。临时注入错误模型矩阵变换后，实际失败为 `nonuniform scale must use inverse-transpose normal matrix`；恢复逆转置后通过。轴对齐立方体无法有效揭露此错误，因此专门使用斜面。
- FBO 测试验证上下方向、灰度通道、反相、锐化边缘、模糊扩散与只编码一次；resize 后完整，D32F/S8 目标读回 depth≈0.33、stencil=5。
- 非法“第二张颜色贴图但无第一张”工程曾实际失败，修复后在替换场景前拒绝，旧 EntityId 保留。
- 把带漫反射/高光贴图的黄铜材质共享给地面，保存并重新加载，两个贴图与共享材质关系都保留。
- 修改点光强度曾把未动过的二次衰减 0.032 写成 0.03；真实 Qt 测试观察失败后改为字段级提交，最终保持精确旧值。
- 资源故障测试暂时移开内置 cutout.png（RAII 自动恢复）。旧实现实际失败为 `texture I/O failure must preserve the live workspace`。新实现构建候选场景，失败后保留旧实体及可用状态。
- 默认上传预算改为 13，覆盖 1 个方向光、8 个点光、4 个聚光灯的总容量；CPU 测试先观察到旧预算失败，再修改并通过。

没有宣称每个校验分支都独立观察过 RED；上述列出的失败均实际运行，常规边界行为则以完整测试通过为证据。

## 截图与视觉检查

真实 Qt 测试在 1440×900、1100×720 逻辑窗口尺寸生成截图。已用图片查看工具检查材质、灯光、后处理、首页和窄窗口：主要操作可见，长表单可滚动，视口保留可用空间；高光 RGB 标签在输入框上方。

- [光照实验室](../../articles/assets/stage-two-lighting.png)
- [材质高光](../../articles/assets/stage-two-material.png)
- [灰度后处理](../../articles/assets/stage-two-postprocess.png)
- [窄窗口](../../articles/assets/stage-two-narrow.png)

五篇中文文章已生成到 `articles/index.html`，正文、代码和配图内嵌，可离线打开。文章源码块由构建脚本同步自实际 C++/GLSL 文件。浏览器工具因本地 file URL 安全策略拒绝打开阅读页，未绕过策略；本轮 HTML 通过构建、离线资源和本地链接检查，未声称完成浏览器视觉验收。原生 Qt 界面的视觉检查已实际完成。

## 诊断与边界

macOS 本机出现 `Can not manage visual effect areas...` 与一次性纹理 sampler 的 `UNSUPPORTED...using zero texture` 驱动诊断；像素断言和最终 GL 错误检查通过。测试中 unknown preset、缺失图片、deliberate create/update/destroy failure 是明确注入的负例，均以预期方式处理。

当前是 LDR Blinn-Phong 与基础滤镜。PBR、HDR/Bloom、阴影、Toon、模型导入、撤销重做、视口拾取、外部资源打包、后端与 AI 尚未实现。QtResourceFactory 可按接口新增，但目前仍使用自己的 NativeResourceFactory；FBO/RenderState/算法也属于自己的 OpenGL 实现。图形验证仅覆盖本机，未进行跨显卡或跨平台认证。

## 启动

双击项目根目录的[启动光照实验室.command](../../启动光照实验室.command)，或从工作区执行：

```sh
opengl_visual_workbench/native/build/bin/luma_qt_workbench --preset lighting --page 1
```

左侧选择对象，右侧编辑当前阶段；顶部“光照实验”可增删灯光，“后处理”可切换滤镜。左键环绕、滚轮缩放；切换漫游后使用右键与 WASD。保存工程便于继续创作，导出 PNG 保存当前渲染结果。

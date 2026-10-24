# 光场工坊第二阶段：光照与后期渲染设计

## 目标

把已完成的第一阶段 C++/OpenGL/Qt 工作台扩展为一个可以继续承载后续课程的光照实验产品。第二阶段必须能在真实 Qt 窗口中运行，并且每个知识点都有可复用的 C++ 类型、明确的调用链、对应的测试和中文文章。

本阶段交付：颜色空间说明、Phong/Blinn-Phong 光照、法线矩阵、材质镜面属性、方向光、点光源、聚光灯、多光源、离屏 FBO，以及反相、灰度、锐化、模糊四个后期效果。Unlit、纹理、深度/模板/混合/剔除和第一阶段工程文件保持可用。

本阶段不交付：PBR、HDR/Bloom、阴影贴图、模型导入、骨骼动画、Compute Shader、视频时间线、网络服务和 AI Pipeline。未来自动执行器直接复用本阶段的 Controller、LightRig、PostProcessPipeline 和项目文档格式；当前执行者仍然是人。

## 产品行为

Qt 工作台的人工流程扩展为：

```text
场景 → 材质 → 光照 → 镜头 → 检查 → 后期 → 导出
```

用户可以自由返回任意阶段。修改场景会清除材质、光照、镜头、检查、后期和导出的确认状态；修改材质会清除光照之后的状态；移动相机只清除镜头之后的状态；修改后期只清除导出状态。导出按钮仍然可以直接执行，确认状态用于表达结果是否需要重新检查。

首页新增“光照实验室”预设。该预设包含地面、三个材质不同的几何体和方向光、点光源、聚光灯，能在一个画面中观察漫反射、镜面高光、衰减、聚光边缘和多光源叠加。原有机器人、纹理、状态和基础几何预设继续存在；每个预设进入光照模式后使用相同的 Renderer 与光源编辑器。

## 架构

### 文件边界

为保持模块可读、可替换和适合后续 AI 执行器接入，所有手写代码遵守以下硬上限：C++ `.cpp` 文件最多 360 行，C++ 头文件最多 260 行，Qt 页面/动作文件最多 320 行，单个 GLSL 文件最多 180 行，测试文件最多 420 行。文档和生成后的离线 HTML 不受此限制。任何文件接近上限时，按职责拆出新文件；不得通过压缩格式或大段内联代码规避上限。

### CPU 数据层

新增 `luma::lighting` 命名空间：

- `LightColor`：线性 RGB、强度与启用标记。
- `DirectionalLight`：方向、颜色、强度。
- `PointLight`：位置、颜色、强度、常数/一次/二次衰减。
- `SpotLight`：位置、方向、颜色、强度、衰减、内外锥角。
- `LightRig`：固定上限的方向光、点光源、聚光灯集合；负责 ID、增删、查找和验证。
- `LightingSettings`：环境光、`ShadingModel::Unlit/BlinnPhong`、法线可视化开关和最大光源数量。

所有编辑操作先在副本上验证，再一次性提交。方向、光源颜色、强度、衰减、锥角必须是有限值；方向不能为零；颜色与强度不能为负；外锥角必须大于内锥角且位于 `(0, 90]`；集合数量受固定上限约束。错误通过异常返回，并保证旧值不变。

`MaterialDesc` 扩展为：

- `specularColor`：线性镜面颜色；
- `shininess`：Blinn-Phong 高光指数，范围 `[1, 256]`；
- `specularTexture`：可选高光贴图，沿用现有 `TextureId` 和线性采样约定。

漫反射仍使用 `baseColor` 与现有双纹理混合。材质验证同时检查镜面颜色、高光指数和贴图 ID。

### 渲染层

`Renderer` 保持现有资源工厂和 `DisplayTarget` 契约，新增：

- `setLighting(LightingSettings, const LightRig*)`：设置当前 CPU 光照快照；Renderer 不拥有场景编辑数据。
- `setPostProcess(PostProcessSettings)`：设置离屏与后期参数。
- `ShadingModel` 分派：`Unlit` 调用现有 shader，`BlinnPhong` 调用新的 lighting shader。
- `LightingPipeline`：把相机、模型、材质和光源转换为 shader uniform；限制每个 draw call 的光源数组长度。
- `PostProcessTarget`：创建/复用颜色纹理、深度/模板附件和 FBO；尺寸变化时安全重建，析构前要求创建它的 OpenGL context current。
- `PostProcessPipeline`：使用全屏三角形执行 source → effect → destination，支持 `None/Invert/Grayscale/Sharpen/Blur`。

Phong 顶点着色器传递世界空间位置和法线。法线使用 `transpose(inverse(mat3(model)))`，而不是直接乘模型矩阵。片元着色器计算：

```text
ambient  = ambientColor × material.diffuse
diffuse  = lightColor × max(dot(N, L), 0) × material.diffuse
specular = lightColor × pow(max(dot(N, H), 0), shininess) × material.specular
result   = ambient + Σ(diffuse + specular)
```

方向光使用固定方向；点光源和聚光灯使用距离衰减；聚光灯使用内外角之间的平滑插值。所有光照计算保持线性，最后交给现有 DisplayTransfer 做 sRGB 编码。透明排序、深度写入、剔除和轮廓 pass 沿用第一阶段状态约束。

后期效果不修改场景数据。每个 effect 是无状态的 `PostProcessEffect`，输入/输出通过 `PostProcessTarget` 连接。灰度和反相为单 pass，锐化使用 3×3 kernel，模糊使用可配置半径的 separable 两 pass。默认效果为 `None`，无法创建 FBO 时抛出带尺寸和附件信息的错误，不退回到隐藏的错误画面。

### Qt/MVC 层

`EditorModel` 暴露只读 `LightRig`、`LightingSettings` 和 `PostProcessSettings`。`EditorController` 新增：

- `setLightingSettings`；
- `addDirectionalLight`、`addPointLight`、`addSpotLight`；
- `updateLight`、`removeLight`；
- `setPostProcessSettings`。

Qt 面板只调用 Controller，不直接改 Renderer 或 GL 对象。`QtViewport` 每帧把 Model 的光照快照和后期设置交给 Renderer。渲染错误在 Qt 回调边界转换为状态栏和任务记录，不让异常穿过 Qt 事件循环。

工作台右侧按阶段显示：

- 材质：漫反射颜色、镜面颜色、高光指数、高光贴图状态；
- 光照：光源类型列表、启用、颜色、强度、方向/位置、衰减、锥角和环境光；
- 后期：效果选择、强度/半径、启用预览；
- 检查：光照开关、法线可视化、深度/剔除/透明/轮廓；
- 导出：PNG 与当前后期设置一起保存。

面板在窗口窄于 1100px 时使用滚动容器，不添加与真实能力无关的生成按钮、进度假象或网络入口。

## 工程文件兼容

`.luma` 升级到版本 2。版本 1 文件仍可读取：缺失的光照字段使用稳定默认值，`ShadingModel` 默认为 `Unlit`，后期默认为 `None`。版本 2 保存材质镜面字段、光源集合、光照设置、后期设置和对象纹理引用。读取顺序为：文件大小限制 → JSON 结构 → 版本 → 场景对象数量/顺序 → 数值与枚举 → 材质共享关系 → 光源约束 → 后期约束；全部通过后才应用到当前 Scene，失败不得替换当前工作区。

GPU handle、Qt 控件状态、窗口尺寸、任务记录和镜头缩略图不进入文件。保存使用 `QSaveFile` 原子提交。

## 测试策略

### CPU

- 光源和材质边界、零方向、衰减、锥角、最大数量；
- Controller 原子性：无效更新不改变旧值；
- 工作流失效：从材质、光照、相机、后期修改分别清除正确的下游阶段；
- 版本 1 → 2 工程默认迁移，以及损坏版本拒绝。

### OpenGL

- lighting shader 编译与 uniform 数组边界；
- 法线矩阵在非均匀缩放下仍得到稳定像素；
- 方向光、点光源衰减、聚光灯内外角和多光源叠加的读回像素；
- FBO 完整性、尺寸重建、四种后期效果输出差异；
- 资源释放顺序与无上下文拒绝。

### Qt

- 光源列表新增/选择/编辑/删除；
- 材质镜面控件和光照模式切换；
- 后期选择与导出记录；
- 版本 1 工程加载、版本 2 往返保存；
- 1440×900 和 1100×720 截图检查，确认无主要控件遮挡；
- 真实 PNG 导出和失败路径记录。

## 文档与示例

新增中文文章 `articles/05-stage-two-lighting/index.zh.md`，内容包括颜色空间、Phong 公式、法线矩阵、LightRig/Material/Renderer 类的调用链、Qt 编辑方式、FBO 后期数据流、代码片段、截图、边界和 FAQ。`tools/build-articles.py` 将其嵌入离线阅读页，文章目录显示为第二阶段入口。

新增 `examples/lighting_example.cpp`，不创建 Qt 窗口，展示如何创建 LightRig、配置材质、注入 Renderer，并说明 GPU 调用必须在宿主 current context 内完成。Qt 工作台新增“光照实验室”入口作为真实可玩的集成示例。

## 明确的后续接口

后续阴影可在 `LightingPipeline` 前增加 shadow-map 输入，PBR 可新增 `ShadingModel` 分支，HDR/Bloom 可扩展 `PostProcessEffect` 与浮点附件。AI 执行器应把 Controller 命令封装成可取消、带版本号的任务，并在主线程提交 GPU 变更；本设计不预先制造这些尚无调用方的接口。


## 实施后的接口对应（2026-10-24）

值对象统一放在 `luma` 命名空间，三类灯光直接携带颜色/强度/启用字段，没有新增无实际用途的 LightColor 类。LightRig 使用带 LightKind 的 variant 集合与单调 LightId，`entry()/entries()` 查询；固定类型上限不变。Renderer 的 setLighting 接收 const 引用并复制快照，默认 13 项预算。PostProcessTarget 通过 texture() 提供采样句柄，无额外 bindForSampling 接口。显示编码与后期分开，中间保持线性，最终一步编码一次。

工程持久化新增 ProjectTexture，保存内置路径与采样选项；QtViewport 在候选 Scene 上完成文档应用和资源加载，再发布新场景。实现签名以公共头文件和 api-contract.md 为准。

# 光场工坊开发边界

适用：当前第一、第二阶段及后续课程。新增功能先确定所属模块，再增加类或函数。禁止把业务、控件和 GPU 生命周期塞进一个文件。

## 仓库与提交

本项目独立托管在 https://github.com/lfz-coder/scene-pipeline ，仓库根目录直接包含 `native/`、`api/`、`articles/`、`docs/`、`tests/` 与 `tools/`，不混入相邻的学习工程。现有本机目录名不需要更改。

仓库主入口是 `native/` 下的 Qt / C++ / OpenGL 源码工程，README 首先提供源码目录和 CMake 构建方式。根目录 `index.html` 是早期 WebGL 产品原型，`articles/index.html` 是文章生成的离线阅读版，作为辅助资料说明。`.gitattributes` 标明生成物、文档和第三方依赖，避免这些内容掩盖业务源码的语言统计。

所有新提交的标题和正文使用中文，代码标识符和技术名词允许保留原文。例如 `完善项目首页与常驻 AI 助手布局`、`修复时间线暂停时的首帧预览`。保留远程原有历史，不为了中文规则重写旧提交。

仅提交可复用源码、必需资源、第三方源码及许可证、规范、测试和中文版文档。构建目录、缓存、IDE 私人配置、服务令牌与私钥不入库。提交前审查暂存内容并执行对应验证，推送后核对远程分支。

## 文件预算

| 手写文件 | 硬上限 | 拆分时机 |
| --- | ---: | --- |
| C++ 实现 `.cpp` | 360 行 | 到 280 行时检查职责，下一项功能会超限则先拆 |
| C++ 头文件 `.hpp` | 260 行 | 优先前置声明，接口与具体实现分开 |
| Qt 页面、窗口与动作 `.cpp` | 320 行 | 按页面构建、状态同步、动作分开 |
| GLSL `.vert` / `.frag` | 180 行 | 新渲染算法成为独立 pass / shader |
| 原生测试 `.cpp` | 420 行 | 按 CPU、像素读回、宿主交互拆分 |

行数包含注释与空行。第三方依赖、构建产物、生成的离线 HTML 和文章不计入；不得通过压缩排版规避限制。行数是兜底，职责清晰是首要条件。单个函数建议控制在 60 行附近；页面组合函数可稍长，但不混入业务计算。

执行 `python3 tools/check-native-boundaries.py` 检查。该脚本已加入 CTest，超限会使检查失败。当前窗口拆为 WorkbenchWindow、WorkbenchEditorPage、WorkbenchInspector、WorkbenchWorkflow、WorkbenchLightingPage、WorkbenchLightingActions、WorkbenchPostPage。

## 模块只能做什么

| 模块 / 构建目标 | 拥有的职责 | 允许依赖 | 不得出现 |
| --- | --- | --- | --- |
| `luma_cpu`：scene、lighting、editor、studio 模型 | 值对象、校验、场景、材质、灯光、相机、编辑命令、TimelineModel、PipelineState | GLM、标准库 | QWidget、GLFW、GL 调用、文件对话框 |
| `luma_gfx` | Shader / Mesh / Texture 资源实现与显式绘制状态 | CPU、GLAD、OpenGL | 业务场景、控件、流程确认规则 |
| `luma_scene`：AssetStore | GPU 资源缓存与强类型 ID 查询 | gfx、CPU 材质库 | 当前选中对象、界面表单 |
| `luma_render` | 绘制队列、光照、离屏目标、滤镜、输出编码 | scene、gfx | 窗口创建、Qt 控件、AI SDK、交换缓冲 |
| `luma_presets` | 场景配方：放什么对象、灯光、材质 | scene、render 配置 | 窗口循环、GL 资源上传细节 |
| `luma_studio` | ApiContract、AIService、HTTP/演示执行器、StudioDocument | CPU、Qt Core/Gui/Network/Multimedia | QWidget、GL 调用、供应商 SDK |
| `luma_qt` | 窗口、输入适配、页面、工程 JSON、导出记录 | presets、Qt | 把 QWidget 类型传入 CPU 模型 |
| `luma_core` / `luma_app` | GLFW 生命周期、输入、组合与运行循环 | 下层模块 | 反向要求 CPU 链接 GLFW |

构建依赖由 CMake 显式声明。CPU 光源头文件的禁止依赖也纳入脚本。当前脚本不是完整的 C++ 静态依赖分析，模块审查仍需检查 include 与链接关系。

## 编辑、渲染与所有权

用户编辑走 `View → EditorController → EditorModel`。View 通过 `QtViewport::model()` 取得 const 数据；相机输入由宿主统一更新，并发送变化通知使工作流下游失效。组合点创建预设时可持有可写 SceneContext，它不等同于给控件开放直接写入口。

LightRig、LightingSettings、PostProcessSettings、MaterialDesc 都是可复制 CPU 数据。Controller 验证候选值后再提交；不在控件回调里创建纹理、编译 shader 或绑定 FBO。

GPU 对象由 AssetStore / Renderer 独占持有。上下文先创建，GPU 资源后创建；GPU 资源先销毁，上下文后销毁。Qt 在 `cleanup()` 中 makeCurrent 后清理 Session；GLFW 的声明与析构顺序实现同样约束。所有资源操作都在渲染主线程执行。

DisplayTarget 借用宿主 FBO，不负责其释放。Qt 传 `defaultFramebufferObject()`，禁止假定它是 0。后处理目标按实际像素大小分配；None 走原先路径，启用滤镜才使用离屏管线。

## 后续扩展的落点

| 后续学习 / 业务 | 首先增加的位置 | 保留的接口 |
| --- | --- | --- |
| Assimp 模型 | 新 `model` 模块，输出 MeshData 与材质描述 | AssetStore、Scene |
| 天空盒、阴影、PBR | 新 render pass 和材质类型 | Renderer 的宿主输出边界 |
| Qt 自带资源包装 | 新 `QtResourceFactory` 适配器 | IResourceFactory / IShader / IMesh / ITexture |
| 后端工程库 | 新 repository / service 模块，版本化 DTO | CPU 数据与 Controller |
| AI 模型与新增业务 | Go provider 和受限业务命令；客户端替换 AIService | OpenAPI、人工确认、Controller |

工厂替换的范围是 shader、mesh、texture 的资源包装。RenderState、FBO 和具体算法仍由自己的渲染模块实现；更换图形 API 需要额外抽象，不能只替换工厂。

当前只持久化内置预设的场景配方。`.luma` v2 用内置资源的相对路径与完整 TextureOptions 描述纹理，空槽保存 null，不保存运行期 TextureId。ProjectTexture 负责磁盘描述，AssetStore 保存资源来源并缓存实际纹理；因此把一个带贴图的材质共享给其他对象后，跨会话仍能恢复相同资源。读取时限制为受支持的内置图片，并验证槽位组合。QtViewport 在候选 Scene 上完成预设构建、资源解析和文档应用，全部成功后才发布；加载或解码失败时保留旧场景。未来外部资产导入应增加稳定资产 URI、文件打包与缺失资源处理。

当前已为对话与镜头编排实现 AIService、取消与 revision 冲突处理；只执行受限镜头建议，确认后由 Qt 组装时间线。尚无 Go 服务、真实模型 SDK、通用任务调度器或分布式存储。后端扩展遵守 [Go 约束](backend-go-contract.md)，先更新 OpenAPI 和验收，再改实现。

## Qt 媒体与工程动作边界

MediaGallery 管理纯 Qt 的媒体记录、筛选、选择与动作状态；MediaPreviewDialog 只做图像预览。二者不依赖 OpenGL。WorkbenchMediaActions 连接截图、机位与导出，WorkbenchProjectActions 负责文件对话框、未保存保护与项目菜单。图库通过 onRestore/onRename/onRemove 回调复用，不把相机或工程规则写进通用组件。

StudioWorkspace 持有项目文档，组合常驻 ChatPanel 与内部项目视图栈；WorkbenchWindow 只组合首页、项目根容器和宿主工程动作。HomeComposer 只产生创作意图，复用 ChatPanel 提交，不另建 AIService。StudioAssetsPanel 只读取同一文档，不复制资产库。借用文档的面板在文档析构前显式销毁。TimelinePlayer 管媒体解码，TimelineModel 管源区间与顺序。图片/视频导入只属于编排资产，不绕过现有 3D 场景纹理的内置资源约束。HTTP 只能提交受规范限制的消息和附图，不能提交本地视频路径。OpenAPI 3.1.1 与外部 schema 作为资源嵌入；未知字段拒绝，修改规范必须同时通过标准验证器、Qt DTO 校验和 HTTP 回归。`.luma` v2 的 studio 是可选扩展，令牌永不入盘。

## 界面入口与删除规则

首页从品牌按钮进入，项目内容只通过顶栏视图切换；打开、保存、模型服务统一使用顶栏；镜头快照在镜头构图阶段创建；PNG 导出使用导出阶段底部的单一按钮。首页提供创作输入、模板选择和继续当前工程；只有首页显示“继续当前工程”，只有项目显示内容导航与助手开关，项目菜单只补充新建当前场景工程、另存为。模型设置独立于项目文档，空闲时新建/打开工程保留现有连接，token 仍不入盘。

增加按钮前先检查是否已有同样的可见入口。相同行为、没有新增上下文价值的按钮直接删除，不换名字继续保留。快捷键与列表右键菜单用于键盘和上下文操作，可以复用现有业务动作；颜色选择器与数值输入分别服务直观选色和精确输入，不按同名业务一律删除。

删除控件时同步删除构建、信号连接、专用样式和无调用成员，修改原有测试为保留入口，不加兼容空壳。每次验证实际打开、保存、快照和导出路径，避免只验证按钮数量。

# 公共接口契约：让场景代码保持简单

状态：C++17 第一、二阶段、MVC 复用边界与 Qt6 中文宿主已实现。以下是便于阅读的接口摘录，完整签名以 [include/luma](../native/include/luma) 的头文件为准；用法见 [独立场景示例](../native/examples/reusable_scene.cpp)、[无窗口控制器示例](../native/examples/controller_example.cpp) 与 [Qt 视口](../native/include/luma/qt/QtViewport.hpp)。QtResourceFactory 尚未实现；AIService、HTTP 适配与编排时间线已接入，业务 HTTP 规范见 [OpenAPI](../api/openapi.json)。

## 场景入口

```cpp
namespace luma {
struct FrameInfo {
    float deltaSeconds;                  // 本帧用于更新的秒数
    double elapsedSeconds;               // 累积的模拟时间
    const InputSnapshot& input;          // 已完成 UI 输入路由
};

struct SceneContext {
    AssetStore& assets;
    Scene& scene;
    Camera& camera;
    CameraControls& controls;
    LightRig& lights;
    LightingSettings& lighting;
    PostProcessSettings& postProcess;
};

class SceneLayer {
public:
    virtual ~SceneLayer() = default;
    virtual void onCreate(SceneContext&) = 0;
    virtual void onUpdate(SceneContext&, const FrameInfo&) {}
    virtual void onDestroy(SceneContext&) {}
};

class Application {
public:
    Application(std::string title, int logicalWidth, int logicalHeight);
    int run(std::unique_ptr<SceneLayer> layer, RunOptions options = {});
};
}
```

`RunOptions` 提供有限帧、隐藏窗口、固定时间步长、预设和 PPM 截图等运行配置，详见头文件及 `--help`。

`Application` 是 app 层门面：隐藏启动流程，不承担网格生成、材质计算或文件解析。构造只保存和验证 CPU 配置，GPU 初始化放在 `run()`。`run()` 接收非空 layer，同一实例只运行一次；正常退出返回 0，初始化或运行异常返回非零，并输出阶段、文件路径及底层错误。逻辑尺寸必须为正数。

`SceneContext` 不拥有资源。只在回调期间使用它的引用，场景类可以持有实体 ID、数值参数和自己的 CPU 数据。`FrameInfo::input` 是当前帧快照，不保存到下一帧。

## 实体、变换与材质

```cpp
namespace luma {
struct EntityId   { std::uint32_t value = 0; };
struct MeshId     { std::uint32_t value = 0; };
struct TextureId  { std::uint32_t value = 0; };
struct MaterialId { std::uint32_t value = 0; };

enum class Primitive { Cube, Plane, Triangle };
enum class AlphaMode { Opaque, Mask, Blend };
enum class CameraMode { Orbit, Fly };

struct MaterialDesc {
    glm::vec4 baseColor{1.f};             // 线性 RGBA
    std::optional<TextureId> baseColorTexture;
    std::optional<TextureId> secondaryColorTexture;
    float textureMix = 0.5f;
    glm::vec3 specularColor{0.5f};
    float shininess = 32;
    std::optional<TextureId> specularTexture;
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;

    static MaterialDesc unlit(glm::vec4 linearRgba);
};

class Transform {
public:
    void setPosition(glm::vec3 position);
    void setRotation(glm::quat rotation);
    void setScale(glm::vec3 positiveScale);
    glm::vec3 position() const;
    glm::quat rotation() const;
    glm::vec3 scale() const;
    glm::mat4 matrix() const;             // T * R * S
};

struct RenderItem {
    EntityId entity;
    MeshId mesh;
    MaterialId material;
};

class Scene {
public:
    EntityId create(std::string name, MeshId mesh, MaterialId material);
    bool contains(EntityId entity) const;
    Transform& transform(EntityId entity);
    const Transform& transform(EntityId entity) const;
    std::string_view name(EntityId entity) const;
    const std::vector<RenderItem>& renderItems() const;
    void setMaterial(EntityId entity, MaterialId material);
    void erase(EntityId entity);
    void clear();
};

class AssetStore {
public:
    explicit AssetStore(IResourceFactory& factory);
    MaterialLibrary& materials();
    const MaterialLibrary& materials() const;
    MeshId primitive(Primitive primitive); // 首次生成与上传，之后复用
    TextureId loadTexture(const std::filesystem::path& assetPath,
                          const TextureOptions& options);
    MaterialId createMaterial(MaterialDesc material);
    MaterialId cloneMaterial(MaterialId source);
    MaterialDesc& material(MaterialId material);
    const MaterialDesc& material(MaterialId material) const;
    const IMesh& mesh(MeshId mesh) const;
    const ITexture& texture(TextureId texture) const;
    const TextureSource& textureSource(TextureId texture) const;
};
}
```

行为约定：

| 输入 / 操作 | 规则 |
| --- | --- |
| 无效 ID | 0 无效。查询缺失 ID 报带类型和值的错误，不静默返回别的对象 |
| ID 生命周期 | 同一应用内单调分配且不复用；场景清空后旧 EntityId 失效 |
| 资源归属 | ID 只属于创建它的 AssetStore / Scene，不跨应用传递 |
| 共享材质 | 相同 MaterialId 的修改共同可见；独立编辑先 clone，再 setMaterial |
| 共享网格 | 多个实体可以引用相同 MeshId，各自拥有 Transform |
| Transform 引用 | 立即使用；不跨容器结构变更保存引用。长期保存 EntityId |
| 旋转 | 拒绝零长度或非有限四元数；有效四元数在设置时归一化 |
| 缩放 | 首版只支持有限正缩放；拒绝 0 和负数，避免奇异矩阵与镜像绕序问题 |
| 材质参数 | RGB / alpha 非有限值拒绝；首版 unlit 通道要求在 [0, 1]，alphaCutoff 在 [0, 1] |
| CPU 编辑 | 业务场景可修改 CPU 数据；View 只读 Model，编辑交给 Controller |

`Scene` 在自身容器内验证 EntityId。MeshId / MaterialId 是否存在由创建场景的应用入口检查，Renderer 在构建队列时再次校验，给出实体名和资源 ID。Scene 不为验证而持有整个渲染器。

`renderItems()` 返回只读实例列表，Renderer 通过 const 查询访问变换和资源。`name()` 的视图及实例列表引用仅在场景未发生结构修改时有效，UI 不跨帧保存这些引用；持久 UI 选择状态保存 EntityId。

`MaterialLibrary` 是纯 CPU 材质仓库，提供 createMaterial / cloneMaterial / material；AssetStore 的同名便利函数转发到它，`materials()` 返回同一份仓库。CPU 控制器示例可直接创建 MaterialLibrary，无需构造 GPU 缓存。

`createMaterial()` 校验初始参数；`material()` 返回的可编辑数据在构建绘制队列前再次校验，错误指明材质 ID 与字段。View 根据字段约束限制输入，Controller 在提交前统一校验，不能依赖 GL 对非法值的偶然表现。CPU 类型和强类型资源 ID 放在 scene 的独立头文件，gfx 无需包含 Scene 或 AssetStore。

## MVC 编辑入口

[EditorModel.hpp](../native/include/luma/editor/EditorModel.hpp) 提供只读查询；[EditorController.hpp](../native/include/luma/editor/EditorController.hpp) 提供同步 CPU 操作。两者位于 `luma_cpu`，不依赖窗口、Qt、ImGui 或 OpenGL。

```cpp
class EditorModel {
public:
    EditorModel(Scene&, MaterialLibrary&, Camera&, CameraControls&);
    const Scene& scene() const;
    const MaterialLibrary& materials() const;
    const Camera& camera() const;
    const CameraControls& controls() const;
    EntityId selected() const;
    MaterialId selectedMaterial() const;
    const RenderOptions& options() const;
    StyleKind style() const;
};

class EditorController {
public:
    explicit EditorController(EditorModel& model);
    void select(EntityId entity);
    void clearSelection();
    void reconcileSelection();
    void setTransform(glm::vec3 position, glm::quat rotation, glm::vec3 scale);
    void setMaterial(MaterialDesc material);
    MaterialId cloneSelectedMaterial();
    void setCameraMode(CameraMode mode);
    void setCameraPose(glm::vec3 eye, glm::vec3 target,
                       glm::vec3 up = {0, 1, 0});
    void setStyle(StyleKind style);
    void setRenderOptions(RenderOptions options);
};
```

控制器的变换与材质编辑作用于当前选中对象，缺失或已删除的选择会报错。变换先验证完整副本，再提交，因此非法缩放不会留下已经改过的位置。材质编辑同样先验证，普通编辑保持共享 ID；`cloneSelectedMaterial()` 给当前实体独立材质引用，继续共享纹理。相机姿态也先在副本上验证，失败保留旧观察参数。

场景切换或删除后调用 `reconcileSelection()` 清理已失效的选择。`setRenderOptions()` 保留当前选择，忽略传入副本的 selected 字段；选择必须通过 select / clearSelection / reconcileSelection 修改。长期保存 EntityId，不保存 Scene 内部的对象引用。RenderOptions / StyleKind 是纯 CPU 设置；Application 在绘制前把 Model 的设置交给 Renderer，Controller 不调用 Renderer 或 GL。

模型借用的 Scene、MaterialLibrary、Camera、CameraControls 必须比模型活得更久；Controller 借用的模型同理。渲染时应使用同一 Scene 和 AssetStore 中的 MaterialLibrary，不能把另一仓库中数值相同的 MaterialId 当成同一资源。Model 与 Controller 当前属于编辑线程，不提供并发安全。

## Camera 与输入

```cpp
namespace luma {
class Camera {
public:
    void lookAt(glm::vec3 eye, glm::vec3 target,
                glm::vec3 up = {0.f, 1.f, 0.f});
    void setPerspective(float verticalFovDegrees,
                        float nearPlane, float farPlane);
    void setAspect(float framebufferAspect);
    glm::mat4 view() const;
    glm::mat4 projection() const;
};

class CameraControls {
public:
    void setMode(CameraMode mode);
    void update(Camera& camera, const InputSnapshot& input,
                float deltaSeconds);
};
}
```

Camera 默认透视参数为 45°、aspect 1、near 0.1、far 100；Application 在第一帧绘制前提供真实 aspect。要求 `0 < FOV < 180`、`0 < near < far`、aspect 为正且有限。eye 与 target 不能重合，up 不能为零或与观察方向共线。越界参数报错，不产生 NaN 矩阵。

`InputSnapshot` 区分按住、按下、释放、鼠标位移、滚轮与视口焦点；鼠标位移统一使用窗口逻辑坐标单位，相机灵敏度不随 framebuffer 缩放重复乘倍数。WASD 速度使用 delta，鼠标位移不再重复乘 delta。失焦后状态清空；输入文本和拖动 UI 时屏蔽相应相机输入。

首版 Orbit 与 Fly 切换保留相机 eye 和观察方向，Orbit target 根据已有有效距离重建；模式状态由 CameraControls 维护，不塞进 Camera 的矩阵函数。

## GPU 包装对象

```cpp
namespace luma {
class ShaderProgram {
public:
    static ShaderProgram fromFiles(const std::filesystem::path& vertex,
                                   const std::filesystem::path& fragment);
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    ShaderProgram& operator=(ShaderProgram&&) noexcept;
    ~ShaderProgram();

    void use() const;
    void set(std::string_view name, int value);
    void set(std::string_view name, float value);
    void set(std::string_view name, const glm::vec3& value);
    void set(std::string_view name, const glm::vec4& value);
    void set(std::string_view name, const glm::mat3& value);
    void set(std::string_view name, const glm::mat4& value);
};

class Mesh {
public:
    static Mesh upload(const MeshData& data);
    // 与 ShaderProgram 相同：禁止复制，支持移动，析构释放。
    void draw() const;
};

class Texture2D {
public:
    static Texture2D fromFile(const std::filesystem::path& path,
                              const TextureOptions& options);
    // 与 ShaderProgram 相同：禁止复制，支持移动，析构释放。
    void bind(unsigned unit) const;
};
}
```

- 工厂、uniform 设置、上传、绑定、draw 和析构均要求有效 current context；应用层保证这一前提。`ShaderProgram::set()` 内部确保该 program 已绑定，首版优先可靠性。
- 必需 uniform 不存在时输出错误；某个风格可选的 uniform 使用单独的内部可选路径，不能把所有拼写错误都当成“可能被优化”吞掉。
- shader 错误包含源文件路径、shader 阶段和编译 / 链接日志。片元编译失败时也释放已成功编译的顶点 shader。
- MeshData 顶点布局使用 `sizeof(Vertex)` 与 `offsetof`，约定位置 / 法线 / UV / 顶点色的 location 为 0 / 1 / 2 / 3；索引统一为 uint32。拒绝空网格、越界索引及超过 GLsizei 能表示的 draw 数量。
- stb_image 解码和 GPU 上传分开。首版像素统一成 RGBA8，解码后释放 CPU 图像；失败包含资产路径和解码原因。纹理单元要求小于实现报告的可用单元数量。
- TextureOptions 记录色彩空间、Y 翻转、wrap、min/mag filter、是否生成 mipmap；min filter 需要 mipmap 时必须实际生成，mag filter 只允许 nearest / linear。

## 可替换资源契约

[IResourceFactory.hpp](../native/include/luma/gfx/IResourceFactory.hpp) 是资源创建与使用契约。IShader 提供 use 和 int / float / vec3 / vec4 / mat3 / mat4 的 set；IMesh 提供 draw；ITexture 提供 bind(unit)。接口不暴露具体 GPU handle、GLFW 或 Qt 类型。

```cpp
class IResourceFactory {
public:
    virtual ~IResourceFactory() = default;
    virtual std::unique_ptr<IShader> createShader(
        const std::filesystem::path& vertex,
        const std::filesystem::path& fragment) = 0;
    virtual std::unique_ptr<IMesh> createMesh(const MeshData&) = 0;
    virtual std::unique_ptr<ITexture> createTexture(
        const std::filesystem::path&, const TextureOptions&) = 0;
};

// 当前 Application 在有效上下文中组合这些对象。
NativeResourceFactory factory;
AssetStore assets{factory};
Renderer renderer{factory};
```

`NativeResourceFactory` 返回适配对象，内部持有并委托现有 ShaderProgram、Mesh、Texture2D。AssetStore 缓存唯一拥有的 IMesh / ITexture；Renderer 和 DisplayTransfer 持有 IShader。显式注入的工厂比使用者活得更久，原生资源仍必须在其有效 current context 中释放。

创建失败可以抛异常；若工厂返回空资源，使用者在发布到缓存前拒绝它。失败的请求不能留下可命中的空缓存；后续可以重试。纹理缓存键保留完整加载选项，材质仓库与资源缓存使用不同所有权规则。无窗口替身测试覆盖这些契约，原生 GL 测试继续验证实际驱动行为。

`gfx` 不调用 glfwGetCurrentContext，不链接 GLFW。宿主负责 makeCurrent 和加载 GLAD；原生实现检查函数已加载且存在可查询的当前 OpenGL 上下文，不把某一种窗口库当成 GPU 前提。

Qt UI / 宿主已经复用 NativeResourceFactory；根据需要实现 Qt 工厂仍是后续工作；矩阵转换、纹理采样与资源生命周期都由适配器处理。RenderState 和 DisplayTransfer 的状态 / FBO 算法仍使用 OpenGL，这个资源契约不表示可自动切换 Vulkan 或 Metal。

## Renderer 的输出边界

```cpp
namespace luma {
enum class StyleKind { Basic, Wireframe };

struct DisplayTarget {
    std::uint32_t framebuffer;            // 借用宿主 GL framebuffer 名称
    int pixelWidth;
    int pixelHeight;
    bool srgbAttachment;                 // 由宿主查询真实附件编码
};

class Renderer {
public:
    explicit Renderer(IResourceFactory& factory);
    void initialize(const std::filesystem::path& shaderDirectory);
    void setStyle(StyleKind style);
    void render(const Scene& scene, const AssetStore& assets,
                const Camera& camera, const DisplayTarget& output);
};
}
```

DisplayTarget 是宿主适配边界，不拥有 FBO，不负责删除；Renderer 不交换缓冲，场景代码不接触该结构。尺寸非正时不绘制。输出附件含深度和模板，是第一阶段状态示例的宿主要求，创建时检查实际能力并报告。手动输出编码路径对用户 FBO 支持单采样、同一非分层 packed D24/S8 或 D32F/S8 图像；内部附件匹配其格式，格式变化时即使尺寸相同也会重建。不支持的布局在绘制前明确拒绝。系统默认目标不提供 sized internal format，归一化深度使用已在本机读回验证的 D24/S8 传输路径。

GLFW 的窗口默认目标由宿主传入；当前 Qt 宿主在 paintGL 中传入当前 `defaultFramebufferObject()`，不能在 Renderer 内写死 0。Qt 官方也明确指出 widget 渲染使用其 FBO。[Qt framebuffer 约定](https://doc.qt.io/qt-6/qopenglwidget.html)

`srgbAttachment` 为 true 时输出线性颜色并启用硬件 sRGB 编码；为 false 时先在内部线性附件混合，最终显示 shader 编码，保持 `GL_FRAMEBUFFER_SRGB` 关闭。未来离屏线性 RenderTarget 是内部 pass 资源，不冒充 DisplayTarget。

## 第二阶段：光照、后期与工程加载

[完整可编译示例](../native/examples/lighting_example.cpp) 和[第五篇中文文章](../articles/05-stage-two-lighting/index.zh.md) 展示端到端用法。

| 接口 | 契约 |
| --- | --- |
| `LightRig::addDirectional/addPoint/addSpot` | 返回稳定 LightId；上限为 1 / 8 / 4；非法数据或超限抛异常 |
| `LightRig::entry/update/remove/entries` | 通过 ID 查询、更新、删除；update 验证后提交，删除后的 ID 不复用 |
| `EditorController::setLightingSettings/updateLight/clearLights` | 同步 CPU 编辑；控件只提交本次修改的字段，保留其余精度 |
| `Renderer::setLighting(const LightingSettings&, const LightRig&)` | 保存验证后的 CPU 快照；不借用 Qt 控件；默认 13 项预算覆盖全部光源 |
| `LightingPipeline::draw(...)` | 上传世界空间坐标、逆转置法线矩阵、材质与固定长度光源数组 |
| `Renderer::setPostProcess(const PostProcessSettings&)` | None 保留原路径；启用效果后才经过离屏与滤镜 |
| `PostProcessTarget::resize(w,h,format)` | 独占 move-only FBO/纹理/深度模板；支持 D24/S8 与 D32F/S8，拒绝非正尺寸 |
| `PostProcessPipeline::apply(source,scratch,destination,settings)` | 反相、灰度、锐化为一遍；模糊水平/垂直两遍；最终编码一次 |
| `ProjectTexture::capture/resolve` | 内置资产路径 + 完整 TextureOptions；不把运行期 TextureId 写进磁盘 |
| `QtViewport::captureProject/applyProject` | 在宿主 current context 中解析资源；先完整构建候选场景，成功后才替换旧场景 |

`.luma` v2 保存光照、镜面材质、贴图描述与后期设置；v1 保持 Unlit/None，并沿用预设纹理。损坏数值、非法纹理组合和资源读取失败都保留旧工作区。磁盘目前只支持内置资源，外部导入和打包尚未实现。

模块依赖、所有权与文件硬上限见[开发边界](development-boundaries.md)。

## 模式使用的位置

| 手法 | 应用位置 | 实际解决的问题 |
| --- | --- | --- |
| RAII + move-only | GPU 资源和窗口 | 所有权、退出与失败清理 |
| Facade | Application / AssetStore | 缩短场景启动和资源获取链路 |
| Template Method | Application 调度 SceneLayer | 固定运行顺序，留下场景扩展点 |
| MVC | EditorModel / EditorController / EditorUI | UI 与无窗口调用共用编辑规则 |
| 工厂注入 + 适配 | IResourceFactory / NativeResourceFactory | 调用端保持契约，具体资源实现可替换 |
| 风格选择 | Renderer 选择 Basic / Wireframe | 在同一场景上替换绘制状态；未来复杂风格再增加管线策略 |
| 组合 | Entity = Transform + MeshId + MaterialId | 复用资源而分别控制实例 |

这些选择只服务于已有工作流。首版不加入单例资源管理器、通用消息总线、反射或插件注册。工程存档通过稳定的资产路径 / UUID 保存引用，不把运行时 ID 当作持久标识。

当前编排通过 AIService 接口选择本地演示或 HTTP 服务，使用 PipelineState 做人工确认、TimelineModel 做剪辑；StudioWorkspace 将结果组装为静帧片段。Go 服务与真实模型推理未实现，遵守 [后端约束](backend-go-contract.md)。未来自动修改场景应先映射为有校验的 Controller 操作，由宿主安排编辑线程与 GPU 上下文。

关联：[架构设计](architecture.md) · [实现里程碑](milestones.md) · [文章安排](../articles/README.md)

## 编排公共接口

`TimelineModel` 和 `PipelineState` 属于纯 CPU 模块。Qt 的 `AIService` 统一 capabilities/start/cancel；`DemoAIService` 不调用真实模型，`HttpAIService` 使用规范中的 operationId 与 DTO。`StudioDocument` 处理版本化数据，`StudioWorkspace` 拥有项目会话、常驻对话和右侧视图栈，连接服务与播放器。`viewStack()` 用于装入宿主场景/图库，`beginPrompt(text, capability, image)` 复用对话提交，`setAssistantVisible(bool)` 只改变可见性；`onReviewReady/onAssembled` 通知宿主导航。`ChatPanel::openSettings()` 提供唯一服务设置窗口，`TimelinePanel::locateAsset(id)` 定位已存在的片段。完整声明见 [studio](../native/include/luma/studio) 与 [Qt](../native/include/luma/qt)，调用链和真实测试见 [第六篇](../articles/06-studio-pipeline/index.zh.md)。

HTTP 唯一来源是 api/openapi.json；本页不复制另一套 HTTP DTO。新增模型能力、资产上传或远端工程同步时，先补规范和契约测试，再添加 Go handler 与客户端适配。已有 GPU 资源接口继续使用自己的实现。

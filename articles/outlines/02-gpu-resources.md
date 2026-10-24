> 历史提纲：三篇完整中文版已交付，阅读 [正文目录](../README.md)。以下保留实现前的安排。

# 文章 02 提纲：封装 Shader、Mesh、Texture

状态：详细提纲。依赖 M2；设计中的接口尚未实现，负例与运行记录需来自真实原生程序。

## 读者任务与判断

读者用同一套 GPU 资源类完成彩色三角形和纹理矩形，再把资源交给多个物体复用。核心判断是：资源封装首先管理所有权、上下文前提和错误信息，然后才是缩短 draw 之前的代码。

适用范围是固定顶点布局、uint32 索引、二维 RGBA8 图片和 OpenGL 3.3 program。动态蒙皮顶点、纹理数组、流式上传在相应版本单独扩展。

## 正文推进

### 从调用端看三种资源

先展示实际 API 组成的完整绘制例子，再分别追踪 `fromFiles → compile → link`、`MeshData → VAO/VBO/EBO` 和 `decode → upload → bind(unit)`。三个模块都能独立解释输入、所有者和销毁条件。

### ShaderProgram：让错误能回到源文件

解释 program 与两个 shader 的不同生命周期。展示片元编译失败时，已创建顶点 shader 的清理；链接失败时，program 与 shader 的清理。uniform location 按 program 缓存，必需参数缺失不能静默忽略。

具体负例：拼错一个必需 uniform；制造一处 GLSL 语法错误。引用真实日志，并说明“只出现黑屏”为什么不足以定位失败阶段。

### Mesh：顶点布局和网格实例分别处理

展示 `sizeof / offsetof`、location、步长、EBO 与 VAO 的关系。CPU PrimitiveFactory 不分配 GL 资源，Mesh 负责上传和绘制。用绕序 / 法线检查说明“数据能上传”并不等于“几何正确”。

画资源所有权图：AssetStore 拥有一个 Mesh，多个 Entity 持有 MeshId，各自拥有 Transform。澄清复制 GPU 名称和真正复制资源的差异。

### Texture2D：像素、纹理单元和采样选项

跟踪图片路径、解码、局部 Y 翻转、色彩空间、上传、mipmap、采样和 sampler uniform。给出相同图片按不同色彩空间加载时的缓存 key 规则；讲清颜色贴图与数据贴图的不同用途。

具体负例：图片不存在；min filter 依赖 mipmap 却没有生成。截图对比并解释失败原因，不能用“试试翻转图片”覆盖所有黑色 / 颠倒问题。

## 必须收集的三组证据

1. 三角形、索引矩形与纹理矩形实际画面，引用公共模块而非各写一套 GL 调用。
2. move-only 编译约束、移动后所有权和异常路径资源计数记录。
3. shader / 纹理负例日志、几何测试输出和缓存命中行为。

## 需要拆解的误解

- RAII 能自动弥补错误的上下文退出顺序；实际上 RAII 仍需要有效上下文前提。
- GLuint 是整数，按值复制后各自析构也安全；实际上两个所有者可能重复删除同一个名称。

## 读者带走的产物

三个有清楚所有权的资源类、两个最小绘制例子、能定位失败阶段的日志方式，以及可在后续模型导入中继续使用的 MeshData。

## FAQ 方向

- Mesh 为什么不直接保存 model / view / projection？
- Texture2D::bind(0) 与 sampler uniform 设置成 0 是同一步吗？
- 为什么相同图片路径还可能需要两个纹理缓存项？

依据：[接口契约](../../docs/api-contract.md) · [里程碑 M2](../../docs/milestones.md) · [已有几何检查](../../tests/geometry.test.cjs) · [GLFW context guide](https://www.glfw.org/docs/latest/context_guide.html)

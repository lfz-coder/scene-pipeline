# 早期 WebGL 产品原型

这份原型用于记录最初的产品视觉目标。桌面软件源码、模块入口和构建方式见 [仓库首页](../README.md) 与 [原生工程](../native/README.md)。以下路径和命令均相对于仓库根目录。

## 直接打开

双击 **index.html**，用 Chrome、Safari 或 Edge 打开。渲染代码、着色器和程序生成的场景都包含在这个 HTML 文件里，不需要安装依赖或联网。也可以只复制这一个文件给其他人体验。

## 可以体验的内容

| 场景 | 可编辑对象 |
| --- | --- |
| 星际信使 | 机器人角色、轨道伙伴、信号灯塔 |
| 材质展台 | 香槟金属、樱粉陶瓷、月光铬银、薄荷磨砂、紫晶涂层 |
| 天空庭院 | 庭院小屋、春色树冠、庭院陈设 |

同一场景可以切换写实、二次元、霓虹和线框四种风格。左侧对象列表和视口点击都可以选择对象；右侧能调节主题色、金属度、粗糙度、旋转、缩放、光源方向、曝光、阴影、辉光和网格。

金属度和粗糙度主要在写实模式中比较；二次元模式使用分段光照和描边，霓虹模式使用轮廓光及 FBO 多 Pass 辉光。

| 操作 | 效果 |
| --- | --- |
| 1 / 2 / 3 / 4 | 写实 / 二次元 / 霓虹 / 线框 |
| 左键拖动 | 围绕场景旋转相机 |
| 右键或 Shift + 左键拖动 | 平移相机 |
| 滚轮 | 缩放视图 |
| WASD / Q、E | 水平平移 / 上下移动 |
| Space | 开关自动转台 |
| R | 重置当前场景、参数和视角，恢复二次元风格 |
| F1 | 隐藏或显示视口标题和操作提示 |
| 导出画面 | 保存当前 3D 画面的 PNG 图片 |

## 开发文件

```text
index.html                  完整离线入口；CSS、界面与生成后的脚本
workbench.js                便于继续开发的源代码
tools/build-demo.cjs        把源代码嵌入 index.html
tests/geometry.test.cjs     检查三角形绕序与法线是否一致
tests/browser-smoke.md      浏览器验收记录
previews/                   浏览器实测截图
```

修改 `workbench.js` 后运行：

```bash
node tools/build-demo.cjs
node --check workbench.js
node tests/geometry.test.cjs
```

## 可继续扩展的模块

`Mat4` 管理矩阵；`MeshFactory` 生成网格；`Mesh` 封装 VAO/VBO/EBO；`Camera` 管理视图与输入；`ShaderProgram` 封装 GLSL 编译、链接和 uniform；`Entity` 和 `Scene` 管理对象、材质与变换；`StylePipeline` 选择渲染风格；`RenderTarget` 管理 FBO；`Renderer` 执行阴影、场景、描边、辉光和合成，并通过颜色 ID Pass 完成拾取。

当前版本是用于确认软件方向和交互的 WebGL2 原型。写实模式使用简化的金属/粗糙度着色和程序化环境反射，不包含完整 IBL、外部模型导入、角色骨骼动画或工程存档。Bloom 使用 RGBA8 颜色缓冲；完整 HDR 管线可以后续接入。之后的 C++/OpenGL 软件可沿用这些模块的职责、材质参数和风格设计。

# 光场工坊开发手记（中文版）

六篇正文已完成，围绕同一份可运行 C++17 / OpenGL / Qt 工程讲解完整链路。**[打开 HTML 阅读版](index.html)**，或直接阅读下面的 Markdown 原稿。

| 篇次 | 完整正文 | 本篇完成的任务 |
| --- | --- | --- |
| 01 | [从练习代码到可复用工作台](01-workbench-architecture/index.zh.md) | MVC、Application / SceneLayer、上下文生命周期、构建与失败清理 |
| 02 | [封装 Shader、Mesh 和 Texture](02-gpu-resources/index.zh.md) | GLSL 编译链接、顶点上传、图片纹理、所有权、缓存与工厂适配 |
| 03 | [场景类只写业务：物体、材质、变换与相机](03-scene-and-camera/index.zh.md) | 独立场景与无窗口控制器示例、TRS / MVP、Orbit / Fly、共享与状态实验 |
| 04 | [用 Qt 做一条人工创作流水线](04-qt-manual-workflow/index.zh.md) | 五步创作、状态失效、工程存档、镜头快照与真实 PNG 导出 |
| 05 | [把光照与 FBO 接进可复用工作台](05-stage-two-lighting/index.zh.md) | 三类光源、材质、FBO、工程兼容与像素验证 |
| 06 | [OpenAPI、对话与视频时间线](06-studio-pipeline/index.zh.md) | 整体 AI 创作布局、常驻助手、AIService、人工 Pipeline、视频剪辑、Go 约束与真实窗口测试 |

每篇包含调用链图、实际源码链接、运行命令、排错案例、FAQ 和发布元数据。没有英文正文版本。当前采用首页创作输入和项目常驻助手布局，保留人工编辑与确认流程，本地演示不调用模型，真实模型通过 HTTP 适配器接入。Go 本轮只交规范和约束。Qt 继续复用自有 OpenGL 封装，QtResourceFactory 留待后续。阅读版内嵌文章与 SVG / PNG 图片，打开后不请求 CDN；源码链接使用本地相对路径，需要保留项目目录结构。HTML 中的代码有复制按钮，可切换深色 / 浅色主题。

更新正文后在仓库根目录执行（Python 3.11+，仅标准库）：

```sh
python3 tools/build-articles.py
python3 articles/tests/test_reader_build.py
```

[原生工程运行说明](../native/README.md) · [原生实际验证记录](../native/tests/verification.md) · [HTML 产品原型](../index.html)。浏览器原型的截图不作为原生运行证明；`assets/native-robot.png` 来自本次真实 OpenGL 程序。

## 第二阶段已交付

[第五篇：把光照与 FBO 接进可复用工作台](05-stage-two-lighting/index.zh.md) 包括 CPU 光源集合、材质高光、三类光源、Qt 编辑、后处理、v1/v2 工程兼容、完整示例和真实测试。截图来自原生 Qt/OpenGL 程序。

[模块开发边界](../docs/development-boundaries.md) · [第二阶段实际验证](../docs/implementation/2026-10-24-stage-two-verification.md)。

后续学习继续扩展同一工程：Assimp、天空盒、阴影、实例化、PBR、HDR/Bloom、粒子与骨骼动画。当前没有将 HTML 产品原型里的未来效果当作原生完成项。

## 独立仓库中的命令路径

项目现托管于 [scene-pipeline](https://github.com/lfz-coder/scene-pipeline)。早期正文与历史验收记录中的 `opengl_visual_workbench/…` 指原学习工作区下的项目路径；从独立仓库根目录执行时，去掉此前缀即可。正文源码链接仍使用项目内相对路径，不受克隆目录名影响。

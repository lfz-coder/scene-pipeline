# 项目协作约束

本目录是独立项目仓库，对应 https://github.com/lfz-coder/scene-pipeline 。本地目录可以继续叫 `opengl_visual_workbench`；从 GitHub 克隆时默认叫 `scene-pipeline`，不影响构建。

- **所有新提交的标题和正文使用中文**，代码标识符、库名和 API 名可以保留英文。例如：`完善项目首页与常驻 AI 助手布局`、`修复时间线暂停时的首帧预览`。不改写远程已有提交的语言或历史。
- 开发前阅读 [开发边界](docs/development-boundaries.md)，遵守模块依赖、GPU 生命周期、界面入口去重和文件行数上限。
- Qt 负责界面、输入与上下文；OpenGL 继续使用项目自己的封装。后端扩展先对照 [OpenAPI](api/openapi.json) 和 [Go 约束](docs/backend-go-contract.md)。
- 文档与博客只维护中文版。实现与文档描述保持一致，本地演示和未来功能必须明确标注。
- 不提交编译产物、本机 IDE 配置、缓存、服务令牌或私钥；第三方源码与许可证、项目所需资源、中文文章和离线 HTML 阅读版保留在仓库。
- 提交前检查实际差异，运行与改动相关的验证；推送后核对远程提交。不强制推送，不覆盖远程已有历史。

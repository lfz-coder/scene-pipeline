# MVC 与渲染模块复用实施计划

> 使用 superpowers:subagent-driven-development 执行同一原生任务并进行独立审查；宿主负责中文文档和真实图形验证。用户已确认设计方向，连续完成本轮交付。任务级与最终总体审查均已通过，详见 mvc-reuse-progress.md。

**目标：** 不加入 AI Pipeline，将现有工作台调整为有实际独立调用证据的 MVC 与可替换资源封装。

**架构：** CPU Model / Controller 与宿主、GPU 所有权分离；资源通过显式工厂注入；应用组合点连接现有 GLFW / ImGui 界面。Qt 和未来服务复用这些边界。

**技术：** C++17、OpenGL 3.3 Core、现有 GLAD / GLFW / GLM / Dear ImGui，不新增框架依赖。

**设计：** [mvc-reuse-design.md](mvc-reuse-design.md)。

## 全局约束

- 不加入 AI Pipeline、模型推理、远程服务或空壳占位模块。
- 控制器不引用 GLFW、Qt、Dear ImGui、OpenGL，也不上传资源。
- gfx 不检查 glfwGetCurrentContext、不链接 GLFW。
- QtResourceFactory 和 Qt UI 在后续实际接入时实现，本次不宣称完成。
- 保持现有场景、学习入口、独立 SceneLayer 示例和真实 GL 状态行为。
- 没有 Git，不进行 worktree / commit；使用已保存基线生成审查差异。

## 任务 1：可运行的原生复用边界

文件范围：`native/include/luma/{scene,editor,gfx,render}/`、对应 `src/`、`src/app/Application.cpp`、`examples/`、`tests/`、`CMakeLists.txt` 和原生 README。

- [x] 从 CPU 材质参数提取 MaterialLibrary；AssetStore 保留便利函数并转发，避免 SceneLayer 调用端重写。
- [x] 新增 EditorModel / EditorController；先写真实行为失败用例，再实现选择、变换、材质和视图操作；校验成功后提交修改。
- [x] 引入资源工厂与三个资源接口，NativeResourceFactory 委托原有包装类；显式注入 AssetStore、Renderer 与 DisplayTransfer。保留原始 GPU 包装的移动语义测试。
- [x] 移除 gfx 的 GLFW current-context 检查与链接，让宿主负责加载函数；当前上下文缺失仍给出明确错误。
- [x] 将 EditorUI 改为读取 Model，通过 Controller 提交编辑；主循环统一应用渲染设置。
- [x] 新增 `luma_controller_example` 无窗口示例，并保留 `luma_reuse_example`。
- [x] CPU 测试验证控制器原子性与共享规则；资源替身测试验证注入和缓存；构建并运行 CPU 测试。
- [x] 原生实现者记录真实 RED / GREEN、命令、接口与变动清单；宿主执行 GL / 生命周期 / 截图。
- [x] 根据第一方基线生成差异文件，独立审查并解决问题。

## 任务 2：中文博客与交付入口

文件范围：`articles/01…03/index.zh.md`、`articles/assets/architecture.svg`、`articles/assets/frame-chain.svg`、`docs/architecture.md`、`docs/api-contract.md`、根 README、`tools/build-articles.py`。

- [x] 同步真实依赖方向、控制器调用链和注入方式，链接实际头文件及两个独立示例。
- [x] 保留第一阶段知识讲解，明确“已有实现”和“未来 Qt / 业务 / AI 接入”范围。
- [x] 重新生成离线阅读版，运行构建器回归及脚本语法检查，浏览器查看章节与配图。
- [x] 更新实际验证与进度记录，提供最直接的中文阅读和运行入口。

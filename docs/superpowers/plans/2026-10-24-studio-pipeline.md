# 编排工作区实施计划

> 在当前会话使用 executing-plans 顺序执行；用户已要求开始，不另开审批回合。当前目录无 Git 仓库，不创建提交或工作树。

**Goal:** 可运行的 Qt AI 对话、pipeline 与时间线，严格对应 OpenAPI；Go 只写实现约束。

**Architecture:** CPU 编辑模型 + QtCore 服务适配 + Qt 页面；现有窗口仅组合。规范通过资源嵌入，路径按 operationId 查询，DTO 按实际 schema 验证。

**Tech Stack:** C++17、Qt6 Widgets/Network/Multimedia、OpenAPI 3.1.1、Python JSON Schema 校验。

**Spec:** ../specs/2026-10-24-studio-pipeline.md

## 约束

Qt cpp ≤320、CPU cpp ≤360、hpp ≤260、测试 cpp ≤420 行。只中文。不要实现 Go 服务、模型 SDK、视频编码和自动执行脚本。未知契约关键字必须使校验失败，不声称实现完整通用 JSON Schema 引擎。

### 1. 契约与 CPU 模型

- [x] 新建 api/openapi.json、schemas/{common,ai,pipeline,timeline,studio}.json；tests/test_api_contract.py 先检查不存在的契约，再用 openapi-spec-validator 与 Draft202012Validator 验证规范、引用和正反例。
- [x] 新建 TimelineModel.hpp/.cpp，接口 add/trim/split/move/remove/duration/locate；tests/studio_model_tests.cpp 验证拆分后总时长保持、非法裁剪不修改数据、定位最终边界。
- [x] 新建 PipelineState.hpp/.cpp，接口 start/analyzed/approve/assembled/fail/cancel/reset；验证缺前置确认不能组装、取消不能被迟到响应完成。

### 2. Qt 服务与资产

- [x] ApiContract.hpp/.cpp 加载嵌入规范，operation(id) 返回 method/path；validate(schema,value) 支持契约实际使用的受控关键词集合。
- [x] AIService.hpp 定义 capabilities/start/cancel 和回调；DemoAIService 与 HttpAIService 分文件。HTTP 设超时、验证响应、单活动请求、取消 revision 冲突处理；重复回调不得覆盖新会话。
- [x] StudioDocument 与资产索引负责 JSON、图片压缩、路径/缺失文件反馈，不持有 GPU；Qt 对照测试实际导出 DTO 并经规范验证。

### 3. 工作区与时间线

- [x] ChatPanel 只做对话/附件/服务配置；PipelinePanel 展示四节点、建议与确认；TimelinePanel 编辑单轨和播放头；TimelinePlayer 处理图像/视频与时间映射。
- [x] StudioWorkspace 组合上述模块，提供 captureScene 回调、onChanged、save/restore。
- [x] WorkbenchWindow 新增一个“编排”入口；保存、加载、新建接入 studio 扩展，token 不持久化。顶部保存与未保存保护继续复用。
- [x] 用真实 Qt 测试发送/取消/错误、VLM 图片、确认组装、视频播放、定位、裁剪拆分重排及工程读写；截图检查 1440×900、1100×720。

### 4. 交付

- [x] 写 docs/backend-go-contract.md：模块边界、provider LLM/VLM 能力、幂等/取消/校验/错误约束，以及 Go 接口示意，不生成服务实现。
- [x] 更新开发边界、README、中文博客及离线 HTML。
- [x] 全量构建、规范校验、CPU/HTTP/GUI 回归、原 Qt 与 CTest；写实际结果并运行新版。

验收完成，具体命令、实际失败与修复、最终结果见 [实施记录](../../implementation/2026-10-24-studio-pipeline.md)。独立只读审查最后确认没有剩余阻断问题。

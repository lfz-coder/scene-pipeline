# AI 对话、pipeline 与视频时间线

用户已授权开始，Go 后端本轮只交付约束与 OpenAPI，不实现服务。当前 Qt/OpenGL 主程序继续作为产品入口，只中文，延续已有模块和文件预算。

## 可用结果

新增唯一侧栏入口“编排”，包含 AI 对话、可观察的 pipeline、媒体预览和单轨视频时间线。既有七步 3D 编辑保留；本轮不添加重复的首页/保存/返回按钮。

对话支持文本、当前场景图片或本地图片、LLM/VLM 能力选择、模型列表、发送、取消、错误和重试。默认本地演示明确标注，不称为真实模型；HTTP 适配器调用同一 OpenAPI，可填写服务地址和会话级 bearer token。没有 Go 服务时依然可以完整走通交互。

pipeline 为素材准备 → 模型分析 → 人工确认 → 时间线组装。四个节点有依赖、状态和错误；生成镜头建议后等待确认，用户可调整标题和时长再组装。取消使延迟回调失效；模型输出不能直接执行 C++、GL、脚本或任意命令。

时间线支持本地视频/图片导入、当前 3D 场景静帧、真实视频播放、暂停、全局定位、片段选择、裁剪、拆分、调整顺序、移除，以及总时长。采用无空隙单轨，单位为整数毫秒。视频编码导出本轮不做，现有 PNG 导出保留原位置。

编排数据随现有 .luma 工程保存，作为 v2 的可选 studio 扩展；旧文档仍可读取。媒体路径与内嵌快照属于本地资产索引，HTTP 只发送用户本次选择的缩放图片与对话。token 不写入工程。

## 契约

`api/openapi.json` 使用 OpenAPI 3.1.1，分拆公共、AI、pipeline 与时间线 schema。它是 HTTP 路径、方法、请求/响应 DTO、状态和错误的唯一来源。

接口：GET /v1/capabilities；POST /v1/runs；GET /v1/runs/{run_id}；POST /v1/runs/{run_id}/cancel。创建使用 Idempotency-Key；取消使用 expected_revision；运行快照 revision 单调递增，轮询取得进度、完整文本和镜头建议。当前不用 SSE，避免声明客户端没有实现的流协议。

运行状态 queued/running/succeeded/failed/cancelled。失败统一 application/problem+json。LLM 允许无图片；VLM 必须带图片。模型 ID 来自能力列表，前端不绑定模型厂商。客户端验证请求、响应及本地 DTO；规范验证、示例验证和 HTTP 适配集成测试纳入验收。

## 模块

- CPU：TimelineModel、PipelineState，无 QWidget、网络或 GPU。
- studio/QtCore：契约读取与校验、AIService、本地演示和 HTTP 适配、资产与序列化。
- Qt View：StudioWorkspace、ChatPanel、PipelinePanel、TimelinePanel、TimelinePlayer；工作台只负责组合、文件和场景快照回调。
- Go 文档：transport → application → domain → provider/repository；后端负责 LLM/VLM 供应商差异，GPU/播放仍属于客户端。

## 验收

CPU 校验裁剪、拆分、重排、边界定位与 pipeline 取消/确认；契约校验有效与无效 DTO；本地 HTTP 测试实际方法、路径、header、轮询、错误和取消；GUI 用真实视频与窗口验证播放、编辑、保存恢复及 VLM 附图。原 CTest、Qt 回归继续通过；更新中文博客与离线 HTML，并启动新版。

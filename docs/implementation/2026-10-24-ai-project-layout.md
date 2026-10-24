# AI 项目整体布局实现与验收

日期：2026-10-24。范围仅为 `opengl_visual_workbench`。参考用户登录态 Pixo 首页、故事版、媒体与时间线的实际布局；未在参考站提交任务或改动项目内容。

## 实现后的页面关系

- 首页：居中创作输入，文本规划/画面分析、参考图、起始场景；继续当前工程和七个手动模板。
- 项目：左侧同一个创作助手常驻，顶栏切换方案、3D 场景、镜头、媒体和时间线。取消与任务状态继续保留；助手开关只控制可见性。
- 建议就绪后显示方案；人工确认成功后显示时间线。确认前不修改时间线，确认后更新助手状态。
- 3D 场景保留完整七步编辑，对象与属性共用右侧标签面板；长阶段栏滚动，1100×720 保留有效视口。
- 项目媒体读取 StudioDocument 同一份资产；按图片/视频和名称筛选，选择素材可定位到现有时间线片段。导出记录独立成次级标签。
- 模型服务配置收进顶栏单一入口。设置窗口与助手同时显示连接/错误状态；请求执行中禁用服务修改，token 不入盘。

删除独立编排导航、原全局侧栏和相关专用样式，没有添加空白兼容页或付费/社区占位入口。首页不显示项目内容导航和助手开关；项目不显示多余的首页输入框。

## 模块和调用链

```text
WorkbenchWindow
  ├─ WorkbenchShell：顶栏、首页/项目外层栈、工程动作
  ├─ HomeComposer：只发出输入意图
  │    → confirmLeave → createProject → ChatPanel::beginPrompt
  └─ StudioWorkspace：拥有 StudioDocument
       ├─ ChatPanel：唯一 AIService、历史、附图、输入与取消
       └─ 项目内容栈：方案 / 3D 场景 / 镜头 / 媒体 / 时间线
            ├─ PipelinePanel → 候选文档校验 → 人工组装
            └─ StudioAssetsPanel ↔ TimelinePanel → TimelinePlayer
```

StudioWorkspace 显式先销毁借用文档的面板，再销毁值成员文档；Qt 父子树继续管理其余控件。QtViewport 和自有 Renderer、资源工厂、Controller 的边界保持原有设计。OpenAPI 和 Go 约束没有因界面调整而产生第二套接口。

## 本次实际发现并修正

1. 回归首先确认旧首页没有创作输入、3D 页看不到助手，随后以同一会话容器替换页面分离结构。
2. 独立审查发现新建工程清空 HTTP 连接：首页紧接提交就失败。新增本地 TCP 回归先失败，再保留空闲服务和模型列表；未连接时也保留输入与附图。
3. 真实视频测试发现播放头前进但没有帧。最小独立播放器能解码；Qt 6.12 macOS AVFoundation 使用原生显示回调，改用 QEventLoop 驱动测试等待后，真实像素变化通过。没有移除解码断言或替换成时间计数。
4. 导入视频暂停时曾显示上一张静帧；增加首帧断言先失败，LoadedMedia 时激活暂停输出后通过。预览本身的尺寸事件负责重新按比例缩放，切页和拖动分隔条均可更新。
5. 确认方案后助手曾继续写“等待人工确认”，新增断言并让组装成功通知同步更新状态。

## 验证命令

从工作区根目录运行。GUI 检查逐个执行，避免抢占焦点。

```sh
cmake --build opengl_visual_workbench/native/build -j6
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
opengl_visual_workbench/native/build/bin/luma_qt_tests
opengl_visual_workbench/native/build/bin/luma_qt_interaction_tests
opengl_visual_workbench/native/build/bin/luma_studio_ui_tests /private/tmp/luma-studio-fixture.mov
python3 opengl_visual_workbench/tools/check-native-boundaries.py
python3 opengl_visual_workbench/tools/build-articles.py
python3 opengl_visual_workbench/articles/tests/test_reader_build.py
```

下列截图均来自最终回归。截图来自真实 Qt 窗口：[首页](../../articles/assets/qt-project-home.png)、[方案审阅](../../articles/assets/qt-project-review.png)、[时间线](../../articles/assets/qt-studio-wide.png)、[1100×720 时间线](../../articles/assets/qt-studio-narrow.png)、[1100×720 场景](../../articles/assets/qt-project-scene-narrow.png)。

## 当前业务边界

默认本地演示，不执行真实 LLM/VLM 推理；HTTP 适配器以本机 TCP fixture 验证，Go 服务与供应商接入仍按原约束后续实现。时间线是单轨预览与非破坏编辑，尚无视频生成、多轨合成、转场或视频编码导出。图片方案用同一引用画面组装，不能当作新生成视频。镜头快照和导出记录仍是会话数据，视频资产保存原本地路径。


## 最终结果

| 验证 | 实际结果 |
| --- | --- |
| 全量 Debug 编译 | 成功，无本次代码编译警告 |
| CTest | 11/11 通过 |
| 原 Qt 回归 | MVC、FBO 像素、相机输入、工程兼容和 PNG 正反例通过 |
| 原按钮交互回归 | 133 checks，0 interaction failures |
| 项目窗口与真实视频回归 | 47 项通过，包括首页文本/附图/HTTP 提交、人工确认、首帧与连续解码、共享媒体和工程恢复 |
| 文件边界 | 166 个手写文件全部在硬上限内 |
| 独立代码审查 | 首页 HTTP 连接问题修复后复审通过 |
| 中文博客离线构建 | 六篇生成成功，阅读器五项校验通过，布局图 XML 可解析 |

最终参考图自动选择使用有界重试等待 QFileSystemModel 准备，测试增加四秒弹窗退出和四十五秒总超时；消除一次偶发自动选择停留。此处修正的是 GUI 测试同步，不改变用户文件选择器的行为。平台 visual-effect/IMK 日志未使检查失败，Qt 的故意缺图/错误预设日志属于原有负例。

已通过实际程序默认入口截图确认首页，随后保持最新「光场工坊 · AI 创作工作台」运行，启动命令为 `native/build/bin/luma_qt_workbench --preset lighting`。

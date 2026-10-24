# AI 项目整体布局 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** 将现有 Qt 工坊组织成首页创作入口、常驻 AI 助手和共享项目视图的软件布局。

**Architecture:** StudioWorkspace 持有项目会话和内部视图栈，WorkbenchWindow 持有首页/项目的外层视图栈。首页只发出创作意图，复用 ChatPanel 和 AIService；所有 AI 结果继续通过人工确认进入 TimelineModel。

**Tech Stack:** C++17、Qt Widgets/Multimedia、自有 OpenGL 渲染器。

**Spec:** `docs/superpowers/specs/2026-10-24-ai-project-layout.md`

## Global Constraints

- Qt 实现文件最多 320 行，头文件最多 260 行，原生测试最多 420 行；不压缩排版规避。
- 最低窗口 1100×720，常用按钮和主内容可访问，长内容滚动。
- 所有文档和博客只交付中文版。
- 沿用 OpenAPI 与本地演示执行器；不实现 Go 和真实供应商。本目录没有 Git 元数据，直接修改工作区，不创建提交。

### Task 1：项目容器和导航

**Files:** 修改 `native/include/luma/qt/{WorkbenchWindow,StudioWorkspace}.hpp`、`native/src/qt/{WorkbenchWindow,WorkbenchWorkflow,WorkbenchStudio,StudioWorkspace}.cpp`；新增 `WorkbenchShell.cpp`；测试 `native/tests/studio_ui_tests.cpp`。

**Interfaces:** `StudioWorkspace::viewStack()` 返回右侧 QStackedWidget；`timelineView()`、`reviewView()` 返回已构建视图；`onReviewReady`、`onAssembled` 控制导航。外部 `showPage(0..5)` 保持首页、场景、镜头、媒体、时间线、方案的语义，内部项目索引减一。

- [x] 先加入真实 GUI 断言并运行得到旧布局失败：
  ```cpp
  window.showPage(1);
  check(window.findChild<QWidget*>("chatPanel")->isVisible(), "assistant remains visible in scene view");
  check(window.findChild<QPlainTextEdit*>("homePrompt"), "home has a creation prompt");
  ```
- [x] 用外层 `shellPages` 和内层 `pages` 替换并列页面，助手只创建一次；顶栏只保留有实际行为的按钮，移除旧侧栏和 navStudio。
- [x] 加入方案就绪/确认后的自动导航；显式先销毁借用 StudioDocument 的控件，再销毁文档。
- [x] 更新现有导航断言为外层首页/内层项目索引，运行 GUI 回归。

### Task 2：首页与可用的窄窗口

**Files:** 新增 `HomeComposer.hpp/.cpp`、`WorkbenchHome.cpp`、`ChatSettings.cpp`；修改 `ChatPanel.hpp/.cpp`、`WorkbenchProjectActions.cpp`、`WorkbenchEditorPage.cpp`、`WorkbenchTheme.cpp`、CMake 源文件清单。

**Interfaces:** `HomeComposer::onSubmit(QString, QString, QImage)` 发出输入；`ChatPanel::beginPrompt(...)` 返回是否开始；`openSettings()` 显示现有服务配置；`createProject(...)` 返回成功或取消。

- [x] 增加首页文本/附图入口、设置单一入口和助手隐藏恢复的断言。
- [x] 构建首页居中输入框和模板；提交前按原有 confirmLeave 保护工程，复用 beginPrompt。
- [x] 将服务地址/令牌/连接移动到设置对话框；左侧只保留历史、附图和底部输入；请求运行期间禁止修改服务。
- [x] 场景页面把对象和属性放进右侧标签面板，保留已有控件名和行为；步骤栏可滚动。
- [x] 1440×900 和 1100×720 实际运行、截图，确认窗口宽度和按钮可访问。

### Task 3：共享媒体与交付

**Files:** 新增 `StudioAssetsPanel.hpp/.cpp`；修改 `WorkbenchPages.cpp`、`StudioWorkspace.cpp`、`TimelinePanel.hpp/.cpp`、`studio_ui_tests.cpp`；更新 README、架构/边界文档和第六篇中文博客、示意图与截图。

**Interfaces:** 资产面板只读取 StudioDocument，`onLocate(QString assetId)` 请求选择已有时间线片段；`TimelinePanel::locateAsset(...)` 设置选择和游标，返回是否找到。

- [x] 断言导入视频出现在项目素材、媒体过滤生效、定位保持片段身份。
- [x] 实现图像/视频共享资产视图以及项目素材/导出记录分栏，不重复维护资产副本。
- [x] 构建全部目标，运行 CTest、Qt、交互和真实视频 GUI 测试，修复失败。
- [x] 根据实际界面更新第六篇文章和截图，生成离线 HTML；运行边界和文章校验。
- [x] 请求代码审查并处理有效问题，启动最新 Qt 程序供用户使用。

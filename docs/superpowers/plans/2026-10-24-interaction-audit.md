# Qt 交互修正实施计划

> **For agentic workers:** 使用 superpowers:executing-plans 在本会话逐项执行。用户已授权检查并完善现有交互。

**Goal:** 修正现有交互缺陷，并把 Pixo 的媒体浏览方式适配为可复用的本地图库。

**Architecture:** MediaGallery 管理 Qt 媒体记录和筛选，MediaPreviewDialog 管理图像预览。WorkbenchWindow 负责项目/镜头业务；编辑字段通过 Controller 单独提交。

**Tech Stack:** C++17、Qt6 Widgets/Test、现有 OpenGL 3.3 渲染模块。

**Spec:** ../specs/2026-10-24-interaction-audit.md

## 全局约束

Qt cpp ≤320 行；hpp ≤260 行；测试 cpp ≤420 行。只中文。快照和记录仅本次会话。当前目录无 Git 仓库，不执行提交。

### 1. 字段与相机修正

修改 WorkbenchInspector.cpp、WorkbenchStages.cpp、WorkbenchPostPage.cpp、WorkbenchEditorPage.cpp。新增 tests/qt_interaction_tests.cpp，独立目标 luma_qt_interaction_tests。

- [x] 先写测试：设置材质颜色 .075、坐标 .1234、后处理强度 .1234，分别修改光泽度、位置 X、后处理开关，验证其他值保持；对 lighting 预设移动后复位，验证恢复原始 eye/target。
- [x] 运行真实 Qt 测试并记录失败。
- [x] 以 field/component 参数只修改对应值；从预设组合点统一提供默认机位。
- [x] 复跑至通过。

### 2. 可复用媒体交互

新增 include/luma/qt/MediaGallery.hpp、MediaPreviewDialog.hpp；src/qt/MediaGallery.cpp、MediaGalleryActions.cpp、MediaPreviewDialog.cpp；镜头动作拆到 WorkbenchMediaActions.cpp。

接口：MediaGallery::addEntry(MediaEntry)、clear()、list()；回调 onRestore(id)、onRename(id,name)、onRemove(id)。MediaEntry 包含 id/title/group/detail/image/path/success。

- [x] 测试无选择禁用、搜索无结果、分组、状态筛选、预览前后/详情/Esc、重命名/删除取消/确认与机位恢复。
- [x] 建立共享图库，替换原镜头/作品的薄列表，保留 shots/exports 对象名兼容原回归测试。
- [x] 失败导出不能打开文件；已删除文件明确报错；删除记录不删除文件。
- [x] 真实运行并截图检查 1440×900 / 1100×720。

### 3. 工程入口与操作状态

新增 WorkbenchProjectActions.cpp 拆分工程对话框和脏状态保护，修改 WorkbenchWindow/LightingPage/Theme/Workflow。

- [x] 测试已保存路径直接保存、另存为取消、打开前取消/保存/放弃、关闭取消；工程菜单和任务抽屉开关。
- [x] 添加中文项目菜单、明确顶栏“导出设置”语义；使用平台快捷键。
- [x] 灯光类型达到容量禁用添加，无选择禁用删除；搜索显示可见/总数。
- [x] 补齐焦点、选择、菜单、禁用样式。

### 4. 验收与文档

- [x] 构建、CTest、原 Qt 回归、新交互回归、GL 光照回归。
- [x] 更新文章中的交互说明与离线 HTML。
- [x] 写实际控件清单、Pixo 对照、修正项、验证命令、限制；启动新程序供使用。

# SDD ledger — plan: docs/implementation/qt-stage-one-plan.md

基线：/private/tmp/luma-qt-before-20261020。无 Git 仓库，沿用用户已授权的当前工程；不创建工作树或提交。安装 qtbase 已通过系统审批。

| 范围 | 共享文件/接口 | 核对 |
|---|---|---|
| Task 1 自洽 | 双纹理字段 → CPU/GL 测试与两宿主 | 默认单纹理不变，资源必须验证 |
| Task 2 自洽 | Qt View → Controller → Model | Qt 不替换自有 GPU 包装；输出用 Qt 当前 FBO |
| Task 3 自洽 | 实际源码/运行证据 → 中文文章 | 仅已实现功能标完成 |
| Task 1 → 2 | MaterialDesc、Presets、CMake | 顺序实施，Qt 界面读取新增混合参数 |
| Task 1/2 → 3 | 运行目标、API、截图 | 实现完成后才更新现状说明 |

Task 1: complete — stage-one-textures-report.md；双纹理 CPU/GL 证据与截图。
Task 2: complete — qt-stage-one-qt-report.md；Qt 真实窗口测试通过，CTest 无屏幕环境限制已记录。
Task 3: complete — stage-one-checklist.md、native README、architecture/api-contract、中文 HTML 已更新。

验证：`cmake --build` 成功；默认 CTest 4/4 通过；真实窗口 `luma_qt_tests` 通过；`luma_gl_tests` 与 `luma_lifecycle_tests` 通过；文章构建测试 2/2 通过。

# 编排工作区实际验收记录

日期：2026-10-24。环境：macOS / Apple M2 Pro，Qt 6.12，OpenGL 4.1 Metal。只扩展 opengl_visual_workbench，未修改 opengl_image_lab。Go 服务未实现，未连接真实模型供应商。

## 本轮完成

- 一个真实 Qt“编排”页面：对话、本地演示与 HTTP 模式、LLM/VLM 能力筛选、当前画面/文件附图、发送、取消、失败重试。
- 准备、分析、人工确认、组装四节点；建议可以编辑，确认前不修改时间线。
- 单轨图片与本地视频，真实解码播放、拖动定位、源入出点裁剪、拆分、移动、移除；总时长一小时、最多 100 段。
- StudioDocument 接入已有工程读写、场景草稿、新建与未保存保护。旧 v1/v2 工程兼容；studio 可选，token 不入盘。
- OpenAPI 3.1.1、五份外部 schema、请求/响应正反例、Qt 受控运行期验证器、Go 接入约束。
- 第六篇中文博客、完整源码链接和同步代码实录、架构图、两种窗口尺寸截图及离线 HTML。

## 实际运行

下列命令从工作区根目录执行。图形程序需要正常 macOS 图形会话，HTTP 测试需要本地监听权限。

```sh
cmake --build opengl_visual_workbench/native/build -j4
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
opengl_visual_workbench/native/build/bin/luma_studio_contract_tests /private/tmp/luma-studio-client-dtos.json
LUMA_CLIENT_FIXTURES=/private/tmp/luma-studio-client-dtos.json /private/tmp/luma-contract-venv/bin/python opengl_visual_workbench/tests/test_api_contract.py
opengl_visual_workbench/native/build/bin/luma_qt_tests
opengl_visual_workbench/native/build/bin/luma_qt_interaction_tests
opengl_visual_workbench/native/build/bin/luma_studio_ui_tests /private/tmp/luma-studio-fixture.mov
python3 opengl_visual_workbench/tools/check-native-boundaries.py
python3 opengl_visual_workbench/tools/build-articles.py
python3 opengl_visual_workbench/articles/tests/test_reader_build.py
```

已观察的结果：

| 验证 | 结果 |
| --- | --- |
| 全量 Debug 编译 | 成功 |
| CTest | 11/11 通过，不创建 GUI |
| CPU studio model | 片段区间、总时长、拆分、非法裁剪原子性、边界定位、人工确认与取消检查通过 |
| Qt 契约 | 正例、未知字段、VLM 空附件、UUID、操作路径、取消状态恢复、不可达节点组合、Unicode 上下文通过 |
| 本地真实 HTTP | 13 项检查通过：创建/轮询/鉴权/幂等键、取消、同版本异快照拒绝、在途 GET 超时取消、GET 503 后仍取消、截止时间成功竞态、409 重试、取消与成功竞态、Problem 状态码 |
| 标准 OpenAPI 与 JSON Schema | 主文档、外部引用、示例及实际 Qt 导出数据通过；反例被拒绝 |
| 原 Qt 回归 | 导航、MVC、FBO 像素、resize、WASD、工程兼容、PNG 成功失败、关闭通过 |
| 原交互回归 | 133 checks, 0 interaction failures |
| 新编排真实窗口 | 18 项检查通过，含真实视频帧变化、100 ms 源区间跳切、移动后删除正确片段、工程完整恢复 |

真实视频测试文件由 [Swift 工具](../../tools/make-video-fixture.swift) 在本机生成：3 秒 320×180 H264 MOV，非外部下载素材。沙箱中编码器不可用，已在获准的本机图形环境生成；不能拿纯播放头计时充当解码验收。

截图为 [1440×900](../../articles/assets/qt-studio-wide.png) 和 [1100×720](../../articles/assets/qt-studio-narrow.png)，来自真实 Qt 窗口，已目视检查操作入口与布局。

## 测试与审查实际发现的错误

1. Qt 的 QJsonValue 与 QVariant 比较方式不同，模型筛选意外变空，真实窗口测试首先失败；改为明确 QString 比较后通过。
2. 人工审核后取消的状态恢复错误，以及不可能的节点组合被接受：新增回归先失败，改为可达状态恢复后通过。
3. 同 revision 不同快照原先被接受：HTTP 回归先失败，增加快照比较后通过。
4. 独立只读审查发现：总超时提前清 active 会漏发取消、同素材小于 250 ms 的跳切未 seek、重排按行号恢复选中导致后续删错片段、Unicode 提示词按 UTF-16 截断。均已修复，并增加对应检查。最后复核还补到截止时间后 GET 返回 503 的路径，回归先失败再修复：读取失败也会继续尝试有界取消；迟到成功终态保持成功。

Qt 日志中的 visual-effect layer / IMK 提示属于平台输出，没有使检查失败。原 Qt 回归会主动制造缺图、未知预设和失败导出，相关错误是负例预期。

## 本轮范围与后续验收

本地演示返回固定示例，不代表真实 LLM/VLM 推理。HTTP 适配器以本地 TCP fixture 验证，没有运行 Go 服务，也没有做真实供应商端到端验收。Go 实现后必须依据 [backend-go-contract.md](../backend-go-contract.md) 再验账号归属、幂等重放、provider 取消、限额和真实附图。

时间线当前为单轨预览与非破坏编辑，不含视频生成、视频编码导出、多轨、转场、精确帧同步或独立音频编辑。外部视频保存本地路径；工程文件不是便携媒体包。恢复时运行中节点标为已中断，不自动恢复远端任务。

依据：[OpenAPI 3.1.1](https://spec.openapis.org/oas/v3.1.1.html)、[QMediaPlayer](https://doc.qt.io/qt-6/qmediaplayer.html)、[QNetworkAccessManager](https://doc.qt.io/qt-6/qnetworkaccessmanager.html)。

# MVC 复用改造：宿主实际验证

日期：2026-10-20。所有命令均对本轮 native/build/bin 中的可执行文件实际执行。测试失败注入的诊断按预期被捕获，不当成程序失败。

## 无窗口行为

```text
ctest --test-dir opengl_visual_workbench/native/build --output-on-failure
controller_behaviors: Passed
resource_factory_behaviors: Passed
cpu_behaviors: Passed
camera_control_edges: Passed
100% tests passed, 0 tests failed out of 4
```

```text
opengl_visual_workbench/native/build/bin/luma_controller_example
CPU controller: selection, transform, shared color and independent clone passed
```

`otool -L .../luma_controller_example` 只列出 libc++.1.dylib 与 libSystem.B.dylib，没有 OpenGL、GLFW、Qt 或 ImGui 动态依赖。它展示纯 CPU 操作；示例中的 MeshId 是数据引用示意，没有创建可绘制的 GPU 网格。

## 真实 OpenGL

在本机图形会话执行：

```sh
opengl_visual_workbench/native/build/bin/luma_gl_tests --hidden
opengl_visual_workbench/native/build/bin/luma_lifecycle_tests
```

两者退出 0。最终 gl_tests 已包含 GLAD 加载后解除 current context、确认 requireContext 拒绝、再恢复 current 的回归，追加断言后重新执行通过。

```text
OpenGL 4.1 Metal - 89.4; depth=32 stencil=8; output=manual sRGB
PASS GL: resource moves, negative shader/image, cache, material clone, poisoned-state reset, real depth/blend/stencil pixels; no GL errors
PASS lifecycle: create/update/destroy failures, cleanup, run-once
```

生命周期测试出现 deliberate create/update/destroy failure、run-once 拒绝信息是注入的预期诊断，测试已检查回调次数与返回码。

## 实际画面与入口

每个预设 `--preset robot / texture / state --hidden --frames 3 --screenshot ...` 均退出 0，报告 Completed 3 frames; no OpenGL errors。PPM 位于 native/build/captures/mvc-{robot,texture,state}.ppm；转为 2560×1600 PNG 后逐张目测，更新 articles/assets/native-{robot,texture,state}.png。三幅图均包含当前 Model / Controller 面板接入，实体、纹理、透明与模板选择轮廓正常显示。

五个入口分别以 `--example window / triangle / texture / camera / state --no-ui --hidden --frames 3` 运行，全部退出 0，报告无 GL 错误。

截图与自动断言不等于手动验证所有 UI 操作。本轮未把窗口缩放、最小化、失焦以及实际鼠标逐项编辑记为全部通过；编辑规则由无窗口控制器用例覆盖，实际输入路径继续保留此前记录的验证边界。

## 中文阅读版

三个章节均已在浏览器打开，新的 MVC 图、资源适配说明与控制器源码全文正常显示；图片全部加载，无页面水平溢出或未解析的 Markdown 加粗标记。构建器回归 2/2 通过，文章元数据与 SVG XML 有效。原生截图更新后重新生成离线 HTML。

Qt UI 已实现并由独立 Qt 报告验收；QtResourceFactory、后端与 AI Pipeline 未实现，亦没有将它们计为本阶段能力。

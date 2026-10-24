# 中文博客阅读版验收

时间：2026-10-20。本机 Chrome，HTTP 本地预览。

- 三篇正文元数据解析成功；各有三条 FAQ、完整代码围栏与超过 4000 个中文字符。
- 构建器验证相对源码链接存在，三篇正文和九张配图内嵌，不依赖 CDN。
- 阅读版生成后全部脚本通过 Node 语法检查；构建器两条回归通过（内联 JSON 转义、嵌图与源码路径）。
- 浏览器实际切换全部三篇；目录、完整示例代码正常；各篇三张图片完成加载。
- 正文的跨篇链接切换 HTML 内章节，无需转到 Markdown；浅色 / 深色开关实际生效。
- 桌面宽度没有水平溢出；截图 `previews/chinese-blogs.jpg`。
- 原生实际截图与原型截图分别标记，未把原型高级特效写成第一阶段已实现。

原稿位于 `articles/01…03/index.zh.md`，入口 `articles/index.html`。示例源码在生成阅读版时从实际参与编译的 `native/examples/reusable_scene.cpp` 同步。


## MVC 与资源复用更新

2026-10-20：三篇正文同步 CPU Model / Controller 与显式资源工厂，更新 architecture / frame-chain / ownership / upload-chain 四张 SVG；PNG 更新为本轮实际原生画面。阅读版额外内嵌 controller_example.cpp 全文，两个示例源码录均与实际参与编译的文件逐字一致。

重新构建成功，阅读器回归仍为 2/2，内联脚本通过 node --check；元数据各有 3 条 FAQ，中文字符数均超过 4000，SVG XML 有效，新增本地链接存在。浏览器实际切换三章，新 MVC 说明、资源工厂和无窗口源码均存在，所有图片正常加载，无水平溢出或未解析的加粗标记。最终浏览器截图：previews/mvc-blogs.png。

本轮只交付可复用边界，正文明确 Qt / 后端 / AI 尚未实现，没有把接口预留当作已完成的集成。

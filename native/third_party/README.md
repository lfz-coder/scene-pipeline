# 第三方源码来源

依赖固定到下面的版本，保留各自许可证；第一方封装位于 `include/` 和 `src/`，请勿把第三方实现当作项目自行编写的算法。

| 目录 | 版本 / 来源 | 本工程用途 |
| --- | --- | --- |
| glad | GLAD 2.0.8，Core 3.3，工作区已有生成代码；头文件中保留生成版本与配置 | 加载 OpenGL 函数 |
| glm | [1.0.1](https://github.com/g-truc/glm/releases/tag/1.0.1) | 向量、矩阵、四元数 |
| imgui | [v1.91.9b](https://github.com/ocornut/imgui/tree/v1.91.9b) | 编辑面板与 GLFW / OpenGL3 后端 |
| stb | [stb_image 固定提交](https://github.com/nothings/stb/blob/f0569113c93ad095470c54bf34a17b36646bbbb5/stb_image.h) | PNG 解码 |

GLFW 优先使用本机 CMake 包，缺少时由 CMake 从官方仓库获取固定 3.4；没有网络的机器应预先安装 GLFW。纹理图片为项目程序生成，未引入外部模型或图片素材。

下载校验（SHA-256）：

```text
glm-1.0.1.tar.gz   9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c
imgui-1.91.9b.tgz  8e1bbc76c71d74fef2fb85db7e7ca8eba13d6a86623c54992b60162db554ffdb
stb_image.h       594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3
```

# 第一阶段双纹理与正方形验收记录

日期：2026-10-20。

`MaterialDesc` 增加可选 `secondaryColorTexture` 和默认 0.5 的 `textureMix`。CPU 校验拒绝非有限或越界系数、ID 0 与没有主纹理的第二纹理。控制器保留先验证再提交，克隆材质复制两张纹理引用且独立编辑系数。

Renderer 校验两张纹理资源，绑定 unit 0/1；片元着色器先对已由 sRGB 纹理格式解码的线性 RGBA 执行 `mix`，再乘基础颜色与顶点颜色，之后继续原有 alpha 模式。单纹理与纯色路径保持有效。未使用的第二 sampler 指向主单元，避免 macOS 驱动对未绑定 unit 1 的诊断。

`--example quad` 创建朝向相机的纹理正方形（复用 Plane 网格并旋转），`texture` 预设中间方块使用 checker 与 cutout 的真实双纹理混合。ImGui 可切换正方形并通过控制器编辑混合系数。

## 测试先行证据

- CPU 接口未实现时，新增测试首先因缺少字段编译失败；加入字段后，运行得到 `FAIL CPU: invalid value must throw`，证明缺失校验。
- GPU 像素测试在 renderer/shader 未实现双纹理时得到 `FAIL GL: dual textures must mix decoded linear samples, including endpoints`。
- 预设测试依次得到 `FAIL GL: unknown preset: quad` 和 `FAIL GL: texture studio must contain an actual mixed two-texture material`，随后实现两项行为。

## 验证结果

- `cmake --build native/build -j2`：成功。
- `ctest --test-dir native/build --output-on-failure`：4/4 通过。
- `native/build/bin/luma_gl_tests --hidden`：通过，无 GL 错误。
- `native/build/bin/luma_lifecycle_tests`：通过；预期的回调故障均正确处理。
- GPU 用两张 1×1 PPM sRGB 纹理（128 红与 128 蓝）验证 mix=0、0.5、1：期望编码像素分别为 (128,0,0)、(92,0,92)、(0,0,128)，容差 2。真实 GL_SRGB8_ALPHA8 借用 FBO 与手动显示编码两条路径都覆盖。
- 另覆盖单纹理、第二纹理不存在时的实体错误上下文、克隆独立编辑、控制器无效系数不提交。
- 隐藏运行 `quad`、`texture` 各一帧并保存截图；实际查看图片，正方形居中、纹理方向与方块混合显示正常。

临时证据：`/private/tmp/luma-stage1-textures.diff`；截图 `/private/tmp/luma-stage1-quad.png`、`/private/tmp/luma-stage1-texture.png`。独立审查由总任务统一记录。

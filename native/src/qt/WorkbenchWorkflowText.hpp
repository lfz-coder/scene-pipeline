#pragma once
#include <QStringList>
namespace luma {
inline const QStringList titles{"场景搭建", "材质与色彩", "光照实验", "镜头构图", "渲染检查", "后处理", "导出作品"};
inline const QStringList descriptions{
    "选择对象，调整位置、旋转与比例。", "调整配色与纹理，赋予作品自己的气质。",
    "用环境光、方向光和材质高光塑造空间。", "找到想留下的角度，保存镜头快照。",
    "检查画面、透明叠加与轮廓，然后确认。", "在最终输出前叠加反相、灰度、锐化或模糊。",
    "使用下方按钮导出 PNG；可编辑工程由顶栏保存，导出记录见左侧作品页。"};
} // namespace luma

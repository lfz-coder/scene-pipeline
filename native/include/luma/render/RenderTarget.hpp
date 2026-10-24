#pragma once
#include <cstdint>

namespace luma {
// Borrowed host output. The renderer never owns or swaps this framebuffer.
struct DisplayTarget {
    std::uint32_t framebuffer = 0;
    int pixelWidth = 0;
    int pixelHeight = 0;
    bool srgbAttachment = false;
};
// Validates the packed depth/stencil contract shared by display and postprocess.
unsigned destinationDepthStencilFormat(unsigned framebuffer);
} // namespace luma

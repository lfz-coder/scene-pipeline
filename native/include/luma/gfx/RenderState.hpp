#pragma once
#include <array>
#include <glad/gl.h>
namespace luma {
// Complete pass state: no dependency on whichever state ImGui or a prior pass left.
struct RenderState {
    bool depthTest = true, depthWrite = true, stencilTest = false, blend = false, cull = true,
         scissor = false;
    GLenum depthFunc = GL_LESS, stencilFunc = GL_ALWAYS;
    GLint stencilRef = 0;
    GLuint stencilReadMask = 0xff, stencilWriteMask = 0xff;
    GLenum stencilFail = GL_KEEP, stencilDepthFail = GL_KEEP, stencilPass = GL_KEEP;
    GLenum blendEquationRGB = GL_FUNC_ADD, blendEquationAlpha = GL_FUNC_ADD;
    GLenum blendSrcRGB = GL_SRC_ALPHA, blendDstRGB = GL_ONE_MINUS_SRC_ALPHA, blendSrcAlpha = GL_ONE,
           blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    GLenum cullFace = GL_BACK, frontFace = GL_CCW, polygonMode = GL_FILL;
    std::array<bool, 4> colorMask{true, true, true, true};
    int x = 0, y = 0, width = 1, height = 1;
    void apply() const;
    static void prepareClear();
};
void requireContext();
void checkGl(const char* stage);
} // namespace luma

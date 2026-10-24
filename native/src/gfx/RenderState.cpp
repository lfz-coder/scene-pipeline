#include <luma/gfx/RenderState.hpp>
#include <sstream>
#include <stdexcept>
namespace luma {
void requireContext() {
    if (!glGetString || !glGetString(GL_VERSION))
        throw std::logic_error("GPU operation requires initialized current OpenGL context");
}
void checkGl(const char* stage) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::ostringstream s;
        s << stage << ": OpenGL error 0x" << std::hex << error;
        while (glGetError() != GL_NO_ERROR) {
        }
        throw std::runtime_error(s.str());
    }
}
namespace {
void enable(GLenum flag, bool value) {
    if (value)
        glEnable(flag);
    else
        glDisable(flag);
}
} // namespace
void RenderState::apply() const {
    requireContext();
    enable(GL_DEPTH_TEST, depthTest);
    glDepthFunc(depthFunc);
    glDepthMask(depthWrite);
    enable(GL_STENCIL_TEST, stencilTest);
    glStencilFunc(stencilFunc, stencilRef, stencilReadMask);
    glStencilMask(stencilWriteMask);
    glStencilOp(stencilFail, stencilDepthFail, stencilPass);
    enable(GL_BLEND, blend);
    glBlendEquationSeparate(blendEquationRGB, blendEquationAlpha);
    glBlendFuncSeparate(blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha);
    enable(GL_CULL_FACE, cull);
    glCullFace(cullFace);
    glFrontFace(frontFace);
    glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    enable(GL_SCISSOR_TEST, scissor);
    glScissor(x, y, width, height);
    glViewport(x, y, width, height);
    glDepthRange(0, 1);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_RASTERIZER_DISCARD);
}
void RenderState::prepareClear() {
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
}
} // namespace luma

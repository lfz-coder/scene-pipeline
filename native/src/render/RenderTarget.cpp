#include <luma/render/RenderTarget.hpp>
#include <luma/gfx/RenderState.hpp>
#include <stdexcept>
#include <string>
namespace luma {
GLenum destinationDepthStencilFormat(unsigned output) {
    glBindFramebuffer(GL_FRAMEBUFFER, output);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::invalid_argument("DisplayTarget framebuffer is incomplete");
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (samples != 0)
        throw std::invalid_argument(
            "DisplayTarget manual encoding requires a single-sample target");
    const GLenum depthAttachment = output == 0 ? GL_DEPTH : GL_DEPTH_ATTACHMENT;
    const GLenum stencilAttachment = output == 0 ? GL_STENCIL : GL_STENCIL_ATTACHMENT;
    // Only OBJECT_TYPE/OBJECT_NAME may be queried for an absent attachment;
    // reject it before querying precision so validation itself creates no GL error.
    GLint depthObjectType = 0, stencilObjectType = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &depthObjectType);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, stencilAttachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                          &stencilObjectType);
    if (depthObjectType == GL_NONE || stencilObjectType == GL_NONE)
        throw std::invalid_argument(
            "DisplayTarget manual encoding requires packed depth/stencil (D24/S8 or D32F/S8)");
    GLint depthBits = 0, stencilBits = 0, depthType = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, stencilAttachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilBits);
    if (depthBits == 0 || stencilBits != 8)
        throw std::invalid_argument(
            "DisplayTarget manual encoding requires packed depth/stencil (D24/S8 or D32F/S8)");
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment,
                                          GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &depthType);
    if (output != 0) {
        GLint depthName = 0, stencilName = 0;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                              &depthObjectType);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, stencilAttachment,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                              &stencilObjectType);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthName);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, stencilAttachment,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &stencilName);
        if (depthObjectType != stencilObjectType || depthName != stencilName || depthName == 0)
            throw std::invalid_argument(
                "DisplayTarget manual encoding requires a shared packed depth/stencil image");
        if (depthObjectType == GL_TEXTURE) {
            for (GLenum property :
                 {GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER,
                  GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
                  GL_FRAMEBUFFER_ATTACHMENT_LAYERED}) {
                GLint depthValue = 0, stencilValue = 0;
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, depthAttachment, property,
                                                      &depthValue);
                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, stencilAttachment, property,
                                                      &stencilValue);
                if (depthValue != stencilValue ||
                    (property == GL_FRAMEBUFFER_ATTACHMENT_LAYERED && depthValue))
                    throw std::invalid_argument("DisplayTarget manual encoding requires one "
                                                "nonlayered packed depth/stencil image");
            }
        }
    }
    checkGl("DisplayTarget depth/stencil query");
    // Default framebuffer metadata has no sized internal-format query. On
    // macOS Metal it reports 32 normalized bits although both packed transfers
    // are valid (see --probe-display). Keep the established D24/S8 path there;
    // user-created FBOs below retain exact format matching.
    if (output == 0 && depthType == GL_UNSIGNED_NORMALIZED && depthBits >= 24)
        return GL_DEPTH24_STENCIL8;
    if (depthBits == 24 && depthType == GL_UNSIGNED_NORMALIZED)
        return GL_DEPTH24_STENCIL8;
    if (depthBits == 32 && depthType == GL_FLOAT)
        return GL_DEPTH32F_STENCIL8;
    throw std::invalid_argument(
        "DisplayTarget unsupported depth/stencil format: depth bits=" + std::to_string(depthBits) +
        ", stencil bits=" + std::to_string(stencilBits) +
        "; manual encoding supports D24/S8 or D32F/S8");
}
} // namespace luma

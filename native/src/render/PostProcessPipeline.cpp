#include <luma/gfx/RenderState.hpp>
#include <luma/render/PostProcessPipeline.hpp>
#include <stdexcept>

namespace luma {
void PostProcessPipeline::initialize(IResourceFactory& factory,
                                     const std::filesystem::path& shaderDirectory) {
    auto shader = factory.createShader(shaderDirectory / "postprocess.vert",
                                       shaderDirectory / "postprocess.frag");
    if (!shader)
        throw std::runtime_error("resource factory returned null post-process shader");
    requireContext();
    if (!vao_) glGenVertexArrays(1, &vao_);
    factory_ = &factory;
    shader_ = std::move(shader);
}

PostProcessPipeline::~PostProcessPipeline() {
    if (vao_ && glGetString && glGetString(GL_VERSION))
        glDeleteVertexArrays(1, &vao_);
}

void PostProcessPipeline::pass(GLuint inputTexture, GLuint outputFramebuffer, int width, int height,
                               PostProcessEffect effect, float intensity, int radius,
                               bool horizontal, bool manualEncoding) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
    RenderState state;
    state.width = width;
    state.height = height;
    state.depthTest = false;
    state.depthWrite = false;
    state.cull = false;
    state.blend = false;
    state.stencilTest = false;
    state.apply();
    shader_->use();
    shader_->set("uSource", 0);
    shader_->set("uEffect", static_cast<int>(effect));
    shader_->set("uIntensity", intensity);
    shader_->set("uRadius", radius);
    shader_->set("uHorizontal", horizontal ? 1 : 0);
    shader_->set("uManualEncoding", manualEncoding ? 1 : 0);
    shader_->set("uTexelSize", glm::vec3{1.0f / width, 1.0f / height, 0});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void PostProcessPipeline::apply(PostProcessTarget& source, PostProcessTarget& scratch,
                                DisplayTarget destination,
                                const PostProcessSettings& settings) {
    requireContext();
    settings.validate();
    if (!shader_ || !factory_ || !vao_)
        throw std::logic_error("PostProcessPipeline not initialized");
    if (source.width() != destination.pixelWidth || source.height() != destination.pixelHeight)
        throw std::invalid_argument("post-process source and destination dimensions differ");
    if (source.depthStencilFormat() != destinationDepthStencilFormat(destination.framebuffer))
        throw std::invalid_argument("post-process source and destination depth formats differ");
    scratch.resize(source.width(), source.height());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.framebuffer());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.framebuffer);
    glBlitFramebuffer(0, 0, source.width(), source.height(), 0, 0, destination.pixelWidth,
                      destination.pixelHeight, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, destination.framebuffer);
    if (destination.srgbAttachment)
        glEnable(GL_FRAMEBUFFER_SRGB);
    else
        glDisable(GL_FRAMEBUFFER_SRGB);
    if (settings.effect == PostProcessEffect::Blur && settings.enabled) {
        pass(source.texture(), scratch.framebuffer(), source.width(), source.height(),
             PostProcessEffect::Blur, settings.intensity, settings.radius, true, false);
        pass(scratch.texture(), destination.framebuffer, destination.pixelWidth,
             destination.pixelHeight, PostProcessEffect::Blur, settings.intensity,
             settings.radius, false, !destination.srgbAttachment);
    } else {
        const auto effect = settings.enabled ? settings.effect : PostProcessEffect::None;
        pass(source.texture(), destination.framebuffer, destination.pixelWidth,
             destination.pixelHeight, effect, settings.intensity, settings.radius, false,
             !destination.srgbAttachment);
    }
    RenderState baseline;
    baseline.width = destination.pixelWidth;
    baseline.height = destination.pixelHeight;
    baseline.apply();
}
} // namespace luma

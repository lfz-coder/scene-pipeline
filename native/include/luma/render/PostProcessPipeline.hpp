#pragma once
#include <filesystem>
#include <luma/gfx/IResourceFactory.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <luma/render/PostProcessTarget.hpp>
#include <luma/render/RenderTarget.hpp>
#include <memory>

namespace luma {
class PostProcessPipeline {
  public:
    ~PostProcessPipeline();
    void initialize(IResourceFactory& factory, const std::filesystem::path& shaderDirectory);
    void apply(PostProcessTarget& source, PostProcessTarget& scratch, DisplayTarget destination,
               const PostProcessSettings& settings);

  private:
    IResourceFactory* factory_ = nullptr;
    std::unique_ptr<IShader> shader_;
    GLuint vao_ = 0;
    void pass(GLuint inputTexture, GLuint outputFramebuffer, int width, int height,
              PostProcessEffect effect, float intensity, int radius, bool horizontal,
              bool manualEncoding);
};
} // namespace luma

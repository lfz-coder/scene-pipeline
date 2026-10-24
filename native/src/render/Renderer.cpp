#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/render/Renderer.hpp>
#include <stdexcept>
namespace luma {
void Renderer::initialize(const std::filesystem::path& dir) {
    auto shader = factory_.createShader(dir / "unlit.vert", dir / "unlit.frag");
    if (!shader)
        throw std::runtime_error("resource factory returned null shader");
    display_.initialize(dir);
    lighting_.initialize(factory_, dir);
    postProcessPipeline_.initialize(factory_, dir);
    shader_ = std::move(shader);
}
void Renderer::draw(const RenderItem& item, const Scene& scene, const AssetStore& assets,
                    const Camera& camera, bool manual, bool outline) {
    shader_->use();
    const auto& material = assets.material(item.material);
    auto model = scene.transform(item.entity).matrix();
    if (outline)
        model = model * glm::scale(glm::mat4(1), glm::vec3(1.07f));
    shader_->set("uModel", model);
    shader_->set("uViewProjection", camera.projection() * camera.view());
    shader_->set("uBaseColor", outline ? glm::vec4(0.1f, 0.85f, 1, 1) : material.baseColor);
    shader_->set("uManualEncoding", manual ? 1 : 0);
    shader_->set("uAlphaMode", outline ? 0 : static_cast<int>(material.alphaMode));
    shader_->set("uAlphaCutoff", material.alphaCutoff);
    shader_->set("uTexture", 0);
    // Keep an unused sampler on the primary unit too. Some drivers validate
    // both samplers even when the material's branch does not sample unit 1.
    shader_->set("uTexture2", material.secondaryColorTexture ? 1 : 0);
    shader_->set("uUseTexture", !outline && material.baseColorTexture ? 1 : 0);
    shader_->set("uUseTexture2", !outline && material.secondaryColorTexture ? 1 : 0);
    shader_->set("uTextureMix", material.textureMix);
    if (!outline && material.baseColorTexture)
        assets.texture(*material.baseColorTexture).bind(0);
    if (!outline && material.secondaryColorTexture)
        assets.texture(*material.secondaryColorTexture).bind(1);
    assets.mesh(item.mesh).draw();
}
void Renderer::render(const Scene& scene, const AssetStore& assets, const Camera& camera,
                      const DisplayTarget& output) {
    if (output.pixelWidth <= 0 || output.pixelHeight <= 0)
        return;
    requireContext();
    if (!shader_)
        throw std::logic_error("Renderer not initialized");
    const bool usePostProcess = postProcess_.enabled && postProcess_.effect != PostProcessEffect::None;
    if (usePostProcess) {
        postProcessSource_.resize(output.pixelWidth, output.pixelHeight,
                                  destinationDepthStencilFormat(output.framebuffer));
        postProcessSource_.bindForScene();
        glDisable(GL_FRAMEBUFFER_SRGB);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, output.srgbAttachment
                                              ? output.framebuffer
                                              : display_.prepare(output.pixelWidth,
                                                                 output.pixelHeight,
                                                                 output.framebuffer));
        if (output.srgbAttachment)
            glEnable(GL_FRAMEBUFFER_SRGB);
        else
            glDisable(GL_FRAMEBUFFER_SRGB);
    }
    RenderState state;
    state.width = output.pixelWidth;
    state.height = output.pixelHeight;
    state.depthTest = options_.depth;
    state.cull = options_.cull;
    state.polygonMode = style_ == StyleKind::Wireframe ? GL_LINE : GL_FILL;
    state.apply();
    RenderState::prepareClear();
    auto bg = options_.background;
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    glClearDepth(1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    std::vector<RenderItem> opaque, transparent;
    const RenderItem* selected = nullptr;
    for (const auto& item : scene.renderItems()) {
        try {
            const auto& m = assets.material(item.material);
            m.validate();
            assets.mesh(item.mesh);
            if (m.baseColorTexture)
                assets.texture(*m.baseColorTexture);
            if (m.secondaryColorTexture)
                assets.texture(*m.secondaryColorTexture);
            (m.alphaMode == AlphaMode::Blend ? transparent : opaque).push_back(item);
        } catch (const std::exception& e) {
            throw std::runtime_error("entity '" + std::string(scene.name(item.entity)) +
                                     "' material " + std::to_string(item.material.value) + ": " +
                                     e.what());
        }
        if (item.entity.value == options_.selected.value)
            selected = &item;
    }
    // Center sorting is intentionally simple; intersecting transparent surfaces remain ambiguous.
    std::stable_sort(transparent.begin(), transparent.end(), [&](const auto& a, const auto& b) {
        auto da = scene.transform(a.entity).position() - camera.eye(),
             db = scene.transform(b.entity).position() - camera.eye();
        return glm::dot(da, da) > glm::dot(db, db);
    });
    auto drawItem = [&](const RenderItem& item, bool outlinePass) {
        if (!outlinePass && lightingSettings_.model == ShadingModel::BlinnPhong)
            lighting_.draw(item, scene, assets, camera, lights_, lightingSettings_, false);
        else
            draw(item, scene, assets, camera, false, outlinePass);
    };
    auto pass = [&](const std::vector<RenderItem>& items, bool alpha) {
        for (const auto& item : items) {
            const auto& m = assets.material(item.material);
            state.depthWrite = !alpha;
            state.blend = alpha && options_.blend;
            state.cull = options_.cull && !m.doubleSided;
            state.stencilTest = options_.outline;
            state.stencilRef = 1;
            state.stencilWriteMask = item.entity.value == options_.selected.value ? 0xff : 0;
            state.stencilPass = GL_REPLACE;
            state.apply();
            drawItem(item, false);
        }
    };
    pass(opaque, false);
    pass(transparent, true);
    if (options_.outline && selected) {
        state.depthTest = options_.depth;
        state.depthFunc = GL_LEQUAL;
        state.depthWrite = false;
        state.blend = false;
        state.cull = false;
        state.polygonMode = style_ == StyleKind::Wireframe ? GL_LINE : GL_FILL;
        state.stencilTest = true;
        state.stencilFunc = GL_NOTEQUAL;
        state.stencilRef = 1;
        state.stencilWriteMask = 0;
        state.stencilPass = GL_KEEP;
        state.apply();
        drawItem(*selected, true);
    }
    // Establish a full known baseline for the host's next pass.
    RenderState baseline;
    baseline.width = output.pixelWidth;
    baseline.height = output.pixelHeight;
    baseline.apply();
    if (usePostProcess)
        postProcessPipeline_.apply(postProcessSource_, postProcessScratch_, output, postProcess_);
    else if (!output.srgbAttachment)
        display_.present(output.framebuffer, output.pixelWidth, output.pixelHeight);
    checkGl("successful scene frame");
}
} // namespace luma

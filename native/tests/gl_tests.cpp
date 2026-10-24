#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <luma/app/Application.hpp>
#include <luma/core/GlfwWindow.hpp>
#include <luma/gfx/Mesh.hpp>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/gfx/ShaderProgram.hpp>
#include <luma/gfx/Texture2D.hpp>
#include <luma/render/Renderer.hpp>
#include <stdexcept>
#include <utility>
using namespace luma;
void presetExamples(Renderer&, AssetStore&, DisplayTarget);
namespace {
void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F function, const std::string& fragment) {
    bool rejected = false;
    try {
        function();
    } catch (const std::exception& e) {
        rejected = true;
        check(std::string(e.what()).find(fragment) != std::string::npos,
              "failure must retain useful context");
    }
    check(rejected, "invalid operation must fail");
}
std::array<unsigned char, 4> center() {
    std::array<unsigned char, 4> pixel{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    return pixel;
}
// Solid sRGB fixtures make the expected mixture independent of UV/filtering:
// 128 sRGB decodes to about 0.216 linear; a 50% mix encodes to 92, not 64.
struct TextureFixtures {
    std::filesystem::path directory = std::filesystem::temp_directory_path() /
        ("luma-dual-texture-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    TextureFixtures() {
        std::filesystem::create_directory(directory);
        for (auto name : {"red", "blue"}) {
            std::ofstream file(directory / (std::string(name) + ".ppm"), std::ios::binary);
            file << "P6\n1 1\n255\n";
            const unsigned char rgb[3] = {static_cast<unsigned char>(name[0] == 'r' ? 128 : 0),
                                          0,
                                          static_cast<unsigned char>(name[0] == 'b' ? 128 : 0)};
            file.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
            check(bool(file), "solid texture fixture write failed");
        }
    }
    ~TextureFixtures() {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }
};
void dualTexturePixels(Renderer& renderer, AssetStore& assets, const Camera& camera,
                       DisplayTarget target) {
    TextureFixtures fixtures;
    auto material = MaterialDesc{};
    material.baseColorTexture = assets.loadTexture(fixtures.directory / "red.ppm");
    material.secondaryColorTexture = assets.loadTexture(fixtures.directory / "blue.ppm");
    auto id = assets.createMaterial(material);
    auto clone = assets.cloneMaterial(id);
    assets.material(clone).textureMix = 0.75f;
    check(assets.material(id).textureMix == 0.5f &&
              assets.material(clone).secondaryColorTexture->value ==
                  material.secondaryColorTexture->value,
          "dual-texture material clone must preserve textures and edit independently");
    Scene scene;
    scene.create("dual texture cube", assets.primitive(Primitive::Cube), id);
    renderer.options().outline = false;
    for (bool srgb : {target.srgbAttachment, false}) {
        target.srgbAttachment = srgb;
        for (const auto& sample : {std::array<int, 3>{0, 128, 0}, {50, 92, 92}, {100, 0, 128}}) {
            assets.material(id).textureMix = sample[0] / 100.0f;
            renderer.render(scene, assets, camera, target);
            const auto pixel = center();
            check(std::abs(int(pixel[0]) - sample[1]) <= 2 && pixel[1] < 3 &&
                      std::abs(int(pixel[2]) - sample[2]) <= 2,
                  "dual textures must mix decoded linear samples, including endpoints");
        }
        assets.material(id).secondaryColorTexture.reset();
        renderer.render(scene, assets, camera, target);
        const auto pixel = center();
        check(std::abs(int(pixel[0]) - 128) <= 2 && pixel[1] < 3 && pixel[2] < 3,
              "single-texture material must retain its original sample");
        assets.material(id).secondaryColorTexture = material.secondaryColorTexture;
    }
    assets.material(id).secondaryColorTexture = TextureId{999};
    rejects([&] { renderer.render(scene, assets, camera, target); }, "dual texture cube");
    assets.material(id).secondaryColorTexture = material.secondaryColorTexture;
}
// A real borrowed target fixture. Packed D32F/S8 is valid in OpenGL 3.3 and
// deliberately differs from the original fallback's hard-coded D24/S8 buffer.
struct BorrowedTarget {
    GLuint framebuffer = 0, color = 0, depthStencil = 0;
    bool textureDepth = false;
    explicit BorrowedTarget(GLenum format, bool useTexture = false, bool srgbColor = false)
        : textureDepth(useTexture) {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, srgbColor ? GL_SRGB8_ALPHA8 : GL_RGBA8, 64, 64, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
        if (textureDepth) {
            glGenTextures(1, &depthStencil);
            glBindTexture(GL_TEXTURE_2D, depthStencil);
            glTexImage2D(GL_TEXTURE_2D, 0, format, 64, 64, 0, GL_DEPTH_STENCIL,
                         format == GL_DEPTH32F_STENCIL8 ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV
                                                        : GL_UNSIGNED_INT_24_8,
                         nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                   depthStencil, 0);
        } else {
            glGenRenderbuffers(1, &depthStencil);
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
            glRenderbufferStorage(GL_RENDERBUFFER, format, 64, 64);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      depthStencil);
        }
        check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
              "borrowed test target must be a complete real framebuffer");
        checkGl("borrowed target fixture");
    }
    ~BorrowedTarget() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (textureDepth)
            glDeleteTextures(1, &depthStencil);
        else
            glDeleteRenderbuffers(1, &depthStencil);
        glDeleteTextures(1, &color);
        glDeleteFramebuffers(1, &framebuffer);
    }
};
void probeDefaultOutput() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLint bits = 0, type = 0, stencil = 0, samples = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                          GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL,
                                          GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil);
    glGetIntegerv(GL_SAMPLES, &samples);
    checkGl("default output diagnostic query");
    std::cout << "DEFAULT depthBits=" << bits << " depthType=0x" << std::hex << type << std::dec
              << " stencilBits=" << stencil << " samples=" << samples << '\n';
    for (GLenum format : {GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8}) {
        BorrowedTarget candidate(format);
        RenderState::prepareClear();
        glClearDepth(0.25);
        glClearStencil(7);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, candidate.framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
                          GL_NEAREST);
        const GLenum error = glGetError();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        float depth = 1;
        unsigned char stencilValue = 0;
        glReadPixels(32, 32, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        glReadPixels(32, 32, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencilValue);
        std::cout << "PROBE sourceFormat=0x" << std::hex << format << " blitError=0x" << error
                  << std::dec << " outputDepth=" << depth << " outputStencil=" << int(stencilValue)
                  << '\n';
        checkGl("default output diagnostic readback");
    }
}
void borrowedDepthFormats(Renderer& renderer, const Scene& scene, const AssetStore& assets,
                          const Camera& camera) {
    BorrowedTarget fixedDepth(GL_DEPTH24_STENCIL8);
    BorrowedTarget floatingDepth(GL_DEPTH32F_STENCIL8);
    BorrowedTarget textureDepth(GL_DEPTH32F_STENCIL8, true);
    // Same dimensions force the cache to notice format changes, not only size.
    for (auto framebuffer : {fixedDepth.framebuffer, floatingDepth.framebuffer,
                             textureDepth.framebuffer, fixedDepth.framebuffer}) {
        renderer.render(scene, assets, camera, {framebuffer, 64, 64, false});
        const auto pixel = center();
        check(pixel[1] > 180 && pixel[1] < 195 && pixel[2] > 180 && pixel[2] < 195,
              "borrowed depth formats must preserve encoded linear alpha blend");
        float depth = 1;
        unsigned char stencil = 0;
        glReadPixels(32, 32, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        glReadPixels(32, 32, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
        check(depth < 1 && stencil == 1, "borrowed target must receive rendered depth and stencil");
        checkGl("borrowed D24/S8 and D32F/S8 render");
    }
    // No matching depth/stencil attachment is an explicit unsupported target,
    // rather than a late driver error from an incompatible blit.
    glBindFramebuffer(GL_FRAMEBUFFER, fixedDepth.framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
    rejects(
        [&] { renderer.render(scene, assets, camera, {fixedDepth.framebuffer, 64, 64, false}); },
        "DisplayTarget");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              fixedDepth.depthStencil);
    checkGl("unsupported target validation");
}
} // namespace
int main(int argc, char** argv) {
    try {
        bool hidden = false, probe = false;
        for (int i = 1; i < argc; ++i) {
            hidden = hidden || std::string(argv[i]) == "--hidden";
            probe = probe || std::string(argv[i]) == "--probe-display";
        }
        GlfwRuntime runtime;
        GlfwWindow window("Luma integration checks", 64, 64, hidden);
        // GLAD stays loaded when the host detaches its context. The GPU layer
        // must reject that state without consulting this GLFW host directly.
        glfwMakeContextCurrent(nullptr);
        try {
            rejects([] { requireContext(); }, "current OpenGL context");
        } catch (...) {
            glfwMakeContextCurrent(window.native());
            throw;
        }
        glfwMakeContextCurrent(window.native());
        requireContext();
        if (probe) {
            probeDefaultOutput();
            return 0;
        }
        {
            auto shader = ShaderProgram::fromFiles(assetRoot() / "shaders/unlit.vert",
                                                   assetRoot() / "shaders/unlit.frag");
            auto program = shader.handle();
            auto moved = std::move(shader);
            check(!shader.handle() && moved.handle() == program && glIsProgram(program),
                  "shader move must transfer ownership");
            auto other = ShaderProgram::fromFiles(assetRoot() / "shaders/unlit.vert",
                                                  assetRoot() / "shaders/unlit.frag");
            auto old = other.handle();
            glUseProgram(0);
            other = std::move(moved);
            check(!moved.handle() && !glIsProgram(old) && other.handle() == program,
                  "shader move assignment must delete previous program");
            rejects([&] { other.set("misspelledRequiredUniform", 1); },
                    "misspelledRequiredUniform");
            glUseProgram(0);
            auto invalid = std::filesystem::temp_directory_path() / "luma-invalid-test.frag";
            {
                std::ofstream out(invalid);
                out << "#version 330 core\n deliberate invalid GLSL\n";
            }
            rejects([&] { ShaderProgram::fromFiles(assetRoot() / "shaders/unlit.vert", invalid); },
                    invalid.string());
            std::filesystem::remove(invalid);
            auto mesh = Mesh::upload(PrimitiveFactory::cube());
            auto vao = mesh.handle();
            auto transferred = std::move(mesh);
            check(!mesh.handle() && transferred.handle() == vao && glIsVertexArray(vao),
                  "Mesh move must preserve uploaded VAO");
            auto second = Mesh::upload(PrimitiveFactory::triangle());
            auto oldVao = second.handle();
            second = std::move(transferred);
            check(!transferred.handle() && !glIsVertexArray(oldVao),
                  "Mesh assignment must delete previous VAO");
            rejects([&] { Texture2D::fromFile(assetRoot() / "textures/missing.png", {}); },
                    "missing.png");
            TextureOptions bad;
            bad.mipmaps = false;
            rejects([&] { Texture2D::fromFile(assetRoot() / "textures/checker.png", bad); },
                    "mipmap");
            auto texture = Texture2D::fromFile(assetRoot() / "textures/checker.png", {});
            auto name = texture.handle();
            auto texMoved = std::move(texture);
            check(!texture.handle() && texMoved.handle() == name && glIsTexture(name),
                  "texture move must transfer ownership");
            auto oldTex = Texture2D::fromFile(assetRoot() / "textures/cutout.png", {});
            auto discarded = oldTex.handle();
            oldTex = std::move(texMoved);
            check(!texMoved.handle() && !glIsTexture(discarded),
                  "texture assignment must delete prior texture");
            rejects([&] { oldTex.bind(100000); }, "texture unit");
            auto normal = ImageData::decode(assetRoot() / "textures/checker.png", false),
                 flipped = ImageData::decode(assetRoot() / "textures/checker.png", true);
            check(normal.rgba[0] != flipped.rgba[0] &&
                      normal.rgba[0] == flipped.rgba[(normal.height - 1) * normal.width * 4],
                  "image flip must be local row reversal");
            NativeResourceFactory factory;
            AssetStore assets(factory);
            auto cube = assets.primitive(Primitive::Cube);
            check(assets.primitive(Primitive::Cube).value == cube.value,
                  "primitive cache must reuse GPU mesh");
            auto a = assets.loadTexture(assetRoot() / "textures/checker.png");
            auto b = assets.loadTexture(assetRoot() / "textures/../textures/checker.png");
            check(a.value == b.value, "canonical texture path must reuse cache");
            TextureOptions linear;
            linear.colorSpace = ColorSpace::Linear;
            auto data = assets.loadTexture(assetRoot() / "textures/checker.png", linear);
            check(data.value != a.value, "color-space change must create distinct texture");
            TextureOptions flip;
            flip.flipY = false;
            check(assets.loadTexture(assetRoot() / "textures/checker.png", flip).value != a.value,
                  "flip option must be in texture cache key");
            auto shared = assets.createMaterial(MaterialDesc::unlit({1, 0, 0, 1}));
            auto clone = assets.cloneMaterial(shared);
            assets.material(clone).baseColor = {0, 0, 1, 1};
            check(assets.material(shared).baseColor.r == 1,
                  "cloned material must edit independently");
            Scene scene;
            auto near = scene.create("near red", cube, shared);
            auto green = assets.createMaterial(MaterialDesc::unlit({0, 1, 0, 1}));
            auto far = scene.create("far green", cube, green);
            scene.transform(far).setPosition({0, 0, -1});
            Camera camera;
            camera.lookAt({0, 0, 5}, {0, 0, 0});
            Renderer renderer(factory);
            renderer.initialize(assetRoot() / "shaders");
            renderer.options().outline = false;
            DisplayTarget target{0, 64, 64, window.srgbAttachment()};
            dualTexturePixels(renderer, assets, camera, target);
            {
                BorrowedTarget srgb(GL_DEPTH24_STENCIL8, false, true);
                dualTexturePixels(renderer, assets, camera, {srgb.framebuffer, 64, 64, true});
            }
            presetExamples(renderer, assets, target);
            // Poison inherited state: render must restore writes, scissor, polygon, depth.
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glStencilMask(0);
            glEnable(GL_SCISSOR_TEST);
            glScissor(0, 0, 0, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            renderer.render(scene, assets, camera, target);
            auto pixel = center();
            check(pixel[0] > 240 && pixel[1] < 15, "depth test must preserve nearer red surface");
            float depth;
            glReadPixels(32, 32, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
            check(depth < 1, "frame must actually write depth");
            renderer.options().depth = false;
            renderer.render(scene, assets, camera, target);
            pixel = center();
            check(pixel[1] > 240 && pixel[0] < 15, "depth disabled must expose submission order");
            renderer.options().depth = true;
            auto glass = assets.createMaterial(MaterialDesc::unlit({0, 0, 1, 0.5f}));
            assets.material(glass).alphaMode = AlphaMode::Blend;
            scene.setMaterial(near, glass);
            renderer.render(scene, assets, camera, target);
            pixel = center();
            check(pixel[1] > 110 && pixel[2] > 110 && std::abs(int(pixel[1]) - int(pixel[2])) < 6,
                  "alpha blend must mix near blue with far green");
            auto fallback = target;
            fallback.srgbAttachment = false;
            renderer.render(scene, assets, camera, fallback);
            pixel = center();
            check(pixel[1] > 180 && pixel[1] < 195 && pixel[2] > 180 && pixel[2] < 195,
                  "manual display encoding must follow linear alpha blending exactly once");
            renderer.options().blend = false;
            renderer.render(scene, assets, camera, target);
            pixel = center();
            check(pixel[2] > 240 && pixel[1] < 15,
                  "blend experiment must change actual framebuffer");
            renderer.options().blend = true;
            renderer.options().outline = true;
            renderer.options().selected = near;
            renderer.render(scene, assets, camera, target);
            unsigned char stencil = 0;
            glReadPixels(32, 32, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
            check(stencil == 1, "selection pass must mark visible selected fragments in stencil");
            // A fully hidden selection must not become a solid cyan shape through its occluder.
            scene.setMaterial(near, green);
            renderer.options().selected = far;
            renderer.render(scene, assets, camera, target);
            pixel = center();
            check(pixel[1] > 240 && pixel[0] < 15 && pixel[2] < 15,
                  "occluded selection outline must obey scene depth");
            scene.setMaterial(near, glass);
            renderer.options().selected = near;
            borrowedDepthFormats(renderer, scene, assets, camera);
            assets.material(glass).baseColor.r = 2;
            rejects([&] { renderer.render(scene, assets, camera, target); }, "near red");
            assets.material(glass).baseColor.r = 0;
            renderer.render(scene, assets, camera, {0, 0, 0, false});
            checkGl("integration test completion");
        }
        checkGl("resource cleanup");
        std::cout << "PASS GL: resource moves, negative shader/image, cache, material clone, "
                     "poisoned-state reset, real depth/blend/stencil pixels; no GL errors\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL GL: " << e.what() << '\n';
        return 1;
    }
}

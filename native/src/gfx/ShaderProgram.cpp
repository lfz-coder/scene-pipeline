#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/gfx/ShaderProgram.hpp>
#include <sstream>
#include <stdexcept>
#include <utility>
namespace luma {
namespace {
std::string read(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in)
        throw std::runtime_error("cannot open shader " + p.string());
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}
GLuint compile(GLenum type, const std::filesystem::path& p) {
    auto text = read(p);
    const char* source = text.c_str();
    GLuint id = glCreateShader(type);
    if (!id)
        throw std::runtime_error("glCreateShader failed: " + p.string());
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint n;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &n);
        std::string log(n > 0 ? n : 1, '\0');
        glGetShaderInfoLog(id, n, nullptr, log.data());
        glDeleteShader(id);
        throw std::runtime_error(std::string(type == GL_VERTEX_SHADER ? "vertex" : "fragment") +
                                 " shader " + p.string() + "\n" + log);
    }
    return id;
}
} // namespace
ShaderProgram ShaderProgram::fromFiles(const std::filesystem::path& v,
                                       const std::filesystem::path& f) {
    requireContext();
    ShaderProgram result;
    GLuint vs = 0, fs = 0;
    try {
        vs = compile(GL_VERTEX_SHADER, v);
        fs = compile(GL_FRAGMENT_SHADER, f);
        result.handle_ = glCreateProgram();
        if (!result.handle_)
            throw std::runtime_error("glCreateProgram failed");
        glAttachShader(result.handle_, vs);
        glAttachShader(result.handle_, fs);
        glLinkProgram(result.handle_);
        GLint ok;
        glGetProgramiv(result.handle_, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint n;
            glGetProgramiv(result.handle_, GL_INFO_LOG_LENGTH, &n);
            std::string log(n > 0 ? n : 1, '\0');
            glGetProgramInfoLog(result.handle_, n, nullptr, log.data());
            throw std::runtime_error("link " + v.string() + " + " + f.string() + "\n" + log);
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        vs = fs = 0;
        checkGl("shader link");
        return result;
    } catch (...) {
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        throw;
    }
}
ShaderProgram::~ShaderProgram() {
    if (handle_)
        glDeleteProgram(handle_);
}
ShaderProgram::ShaderProgram(ShaderProgram&& o) noexcept
    : handle_(std::exchange(o.handle_, 0)), uniforms_(std::move(o.uniforms_)) {}
ShaderProgram& ShaderProgram::operator=(ShaderProgram&& o) noexcept {
    if (this != &o) {
        if (handle_)
            glDeleteProgram(handle_);
        handle_ = std::exchange(o.handle_, 0);
        uniforms_ = std::move(o.uniforms_);
    }
    return *this;
}
void ShaderProgram::use() const {
    requireContext();
    if (!handle_)
        throw std::logic_error("empty ShaderProgram");
    glUseProgram(handle_);
}
int ShaderProgram::location(std::string_view n) {
    use();
    std::string key(n);
    auto i = uniforms_.find(key);
    if (i != uniforms_.end())
        return i->second;
    int loc = glGetUniformLocation(handle_, key.c_str());
    if (loc < 0)
        throw std::runtime_error("required uniform missing: " + key);
    uniforms_.emplace(key, loc);
    return loc;
}
void ShaderProgram::set(std::string_view n, int v) {
    glUniform1i(location(n), v);
}
void ShaderProgram::set(std::string_view n, float v) {
    glUniform1f(location(n), v);
}
void ShaderProgram::set(std::string_view n, const glm::vec3& v) {
    glUniform3fv(location(n), 1, glm::value_ptr(v));
}
void ShaderProgram::set(std::string_view n, const glm::vec4& v) {
    glUniform4fv(location(n), 1, glm::value_ptr(v));
}
void ShaderProgram::set(std::string_view n, const glm::mat3& v) {
    glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(v));
}
void ShaderProgram::set(std::string_view n, const glm::mat4& v) {
    glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(v));
}
} // namespace luma

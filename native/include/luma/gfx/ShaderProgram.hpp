#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
namespace luma {
// All nonempty GPU resources require the creating context current until destruction.
class ShaderProgram {
  public:
    ShaderProgram() = default;
    ~ShaderProgram();
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) noexcept;
    ShaderProgram& operator=(ShaderProgram&&) noexcept;
    static ShaderProgram fromFiles(const std::filesystem::path& vertex,
                                   const std::filesystem::path& fragment);
    void use() const;
    void set(std::string_view name, int value);
    void set(std::string_view name, float value);
    void set(std::string_view name, const glm::vec3& value);
    void set(std::string_view name, const glm::vec4& value);
    void set(std::string_view name, const glm::mat3& value);
    void set(std::string_view name, const glm::mat4& value);
    unsigned handle() const {
        return handle_;
    }

  private:
    unsigned handle_ = 0;
    std::unordered_map<std::string, int> uniforms_;
    int location(std::string_view name);
};
} // namespace luma

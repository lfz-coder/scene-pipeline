#include <cmath>
#include <luma/lighting/LightTypes.hpp>
#include <luma/lighting/LightingSettings.hpp>
#include <string>
#include <stdexcept>

namespace luma {
namespace {
void finiteVector(glm::vec3 value, const char* name, bool normalizedRange) {
    for (int i = 0; i < 3; ++i) {
        const auto component = value[i];
        if (!std::isfinite(component) || (normalizedRange && (component < 0 || component > 1)))
            throw std::invalid_argument(std::string(name) + " must be finite in [0,1]");
    }
}
void validateDirection(glm::vec3 value, const char* name) {
    finiteVector(value, name, false);
    if (glm::dot(value, value) < 1e-12f)
        throw std::invalid_argument(std::string(name) + " must not be zero");
}
void validateIntensity(float value, const char* name) {
    if (!std::isfinite(value) || value < 0)
        throw std::invalid_argument(std::string(name) + " must be finite and non-negative");
}
void validateAttenuation(float constant, float linear, float quadratic) {
    if (!std::isfinite(constant) || constant <= 0 || !std::isfinite(linear) || linear < 0 ||
        !std::isfinite(quadratic) || quadratic < 0)
        throw std::invalid_argument("light attenuation is invalid");
}
} // namespace

void DirectionalLight::validate() const {
    validateDirection(direction, "direction");
    finiteVector(color, "color", true);
    validateIntensity(this->intensity, "intensity");
}

void PointLight::validate() const {
    finiteVector(position, "position", false);
    finiteVector(color, "color", true);
    validateIntensity(this->intensity, "intensity");
    validateAttenuation(constant, linear, quadratic);
}

void SpotLight::validate() const {
    finiteVector(position, "position", false);
    validateDirection(direction, "direction");
    finiteVector(color, "color", true);
    validateIntensity(this->intensity, "intensity");
    validateAttenuation(constant, linear, quadratic);
    if (!std::isfinite(innerCutoffDegrees) || !std::isfinite(outerCutoffDegrees) ||
        innerCutoffDegrees <= 0 || outerCutoffDegrees <= innerCutoffDegrees ||
        outerCutoffDegrees > 90)
        throw std::invalid_argument("spotlight cone angles are invalid");
}

void LightingSettings::validate() const {
    finiteVector(ambientColor, "ambientColor", true);
    if (!std::isfinite(ambientStrength) || ambientStrength < 0 || ambientStrength > 1)
        throw std::invalid_argument("ambientStrength must be finite in [0,1]");
    if (model != ShadingModel::Unlit && model != ShadingModel::BlinnPhong)
        throw std::invalid_argument("invalid shading model");
    if (maxLights < 1 || maxLights > 13)
        throw std::invalid_argument("maxLights must be in [1,13]");
}
} // namespace luma

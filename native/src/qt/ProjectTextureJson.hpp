#pragma once
#include <QJsonValue>
#include <luma/qt/ProjectTexture.hpp>

namespace luma {
QJsonValue writeTexture(const std::optional<ProjectTexture>&);
std::optional<ProjectTexture> readTexture(const QJsonValue&);
} // namespace luma

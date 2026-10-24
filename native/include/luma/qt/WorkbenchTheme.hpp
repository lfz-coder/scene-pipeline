#pragma once
#include <QIcon>
#include <QString>
namespace luma {
QString workbenchStyleSheet();
QIcon workbenchIcon(const QString& name, const QString& color = "#b1b3bf");
} // namespace luma

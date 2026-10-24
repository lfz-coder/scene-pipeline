#pragma once
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QScrollArea>
namespace luma::qtui {
inline QLabel* label(const QString& text, QWidget* parent, const char* name = nullptr) {
    auto* w = new QLabel(text, parent);
    if (name) {
        w->setObjectName(name);
        w->setProperty("variant", name);
    }
    w->setWordWrap(true);
    return w;
}
inline QPushButton* button(const QString& text, QWidget* parent, const char* name = nullptr) {
    auto* w = new QPushButton(text, parent);
    w->setCursor(Qt::PointingHandCursor);
    w->setFocusPolicy(Qt::StrongFocus);
    w->setAutoDefault(false);
    w->setAccessibleName(text);
    if (name) {
        w->setObjectName(name);
        w->setProperty("variant", name);
    }
    return w;
}
inline QWidget* card(QWidget* parent) {
    auto* w = new QWidget(parent);
    w->setObjectName("card");
    return w;
}
inline QDoubleSpinBox* spin(QWidget* parent, double lo, double hi, double step) {
    auto* w = new QDoubleSpinBox(parent);
    w->setRange(lo, hi);
    w->setDecimals(2);
    w->setSingleStep(step);
    w->setKeyboardTracking(false);
    w->setMinimumWidth(58);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return w;
}
inline QScrollArea* makeScroll(QWidget* content, QWidget* parent) {
    auto* w = new QScrollArea(parent);
    w->setWidgetResizable(true);
    w->setFrameShape(QFrame::NoFrame);
    content->setObjectName("scrollContent");
    w->viewport()->setAutoFillBackground(false);
    w->setWidget(content);
    return w;
}
}

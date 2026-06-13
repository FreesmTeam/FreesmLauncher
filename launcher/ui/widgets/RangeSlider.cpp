// RangeSlider.cpp
// Copyright (C) 2026 fractal <fractal@nebula-nook.ru>
#include "RangeSlider.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

RangeSlider::RangeSlider(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(80, 24);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

int RangeSlider::minimum() const
{
    return m_minimum;
}

int RangeSlider::maximum() const
{
    return m_maximum;
}

int RangeSlider::lowerValue() const
{
    return m_lowerValue;
}

int RangeSlider::upperValue() const
{
    return m_upperValue;
}

QString RangeSlider::lowerLabel() const
{
    return m_lowerLabel;
}

QString RangeSlider::upperLabel() const
{
    return m_upperLabel;
}

void RangeSlider::setMinimum(int value)
{
    if (m_minimum == value) {
        return;
    }

    m_minimum = value;

    const bool valuesChanged = clampValues();
    update();
    emit minimumChanged(m_minimum);

    if (valuesChanged) {
        emit lowerValueChanged(m_lowerValue);
        emit upperValueChanged(m_upperValue);
        emit valueChanged(m_lowerValue, m_upperValue);
    }
}

void RangeSlider::setMaximum(int value)
{
    if (m_maximum == value) {
        return;
    }

    m_maximum = value;

    const bool valuesChanged = clampValues();
    update();
    emit maximumChanged(m_maximum);

    if (valuesChanged) {
        emit lowerValueChanged(m_lowerValue);
        emit upperValueChanged(m_upperValue);
        emit valueChanged(m_lowerValue, m_upperValue);
    }
}

void RangeSlider::setLowerValue(int value)
{
    const int clampedValue = std::clamp(value, m_minimum, m_upperValue);
    if (m_lowerValue == clampedValue) {
        return;
    }

    m_lowerValue = clampedValue;
    update();
    emit lowerValueChanged(m_lowerValue);
    emit valueChanged(m_lowerValue, m_upperValue);
}

void RangeSlider::setUpperValue(int value)
{
    const int clampedValue = std::clamp(value, m_lowerValue, m_maximum);
    if (m_upperValue == clampedValue) {
        return;
    }

    m_upperValue = clampedValue;
    update();
    emit upperValueChanged(m_upperValue);
    emit valueChanged(m_lowerValue, m_upperValue);
}

void RangeSlider::setLowerLabel(const QString& label)
{
    if (m_lowerLabel == label) {
        return;
    }

    m_lowerLabel = label;
    update();
}

void RangeSlider::setUpperLabel(const QString& label)
{
    if (m_upperLabel == label) {
        return;
    }

    m_upperLabel = label;
    update();
}

bool RangeSlider::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
            const bool isBacktab = keyEvent->key() == Qt::Key_Backtab;

            if (isBacktab && m_selectedHandle == ActiveHandle::Upper) {
                m_selectedHandle = ActiveHandle::Lower;
                update();

                const int value = m_lowerValue;
                const int xPos = posFromValue(value);
                showTooltip(value, xPos);

                keyEvent->accept();
                return true;
            }

            if (!isBacktab && m_selectedHandle == ActiveHandle::Lower) {
                m_selectedHandle = ActiveHandle::Upper;
                update();

                const int value = m_upperValue;
                const int xPos = posFromValue(value);
                showTooltip(value, xPos);

                keyEvent->accept();
                return true;
            }

            return QWidget::event(event);
        }
    }
    return QWidget::event(event);
}

void RangeSlider::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    const auto palette = this->palette();

    const QPalette::ColorGroup colorGroup = !isEnabled() ? QPalette::Disabled : (hasFocus() ? QPalette::Active : QPalette::Inactive);

    const QColor trackBgColor = palette.color(colorGroup, QPalette::Midlight);
    const QColor rangeColor = palette.color(colorGroup, QPalette::Highlight);
    const QColor handleColor = palette.color(colorGroup, QPalette::Base);
    const QColor handleBorderColor = palette.color(colorGroup, QPalette::Shadow);
    const QColor focusBorderColor = palette.color(colorGroup, QPalette::Highlight);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int h = height();
    const int y = h / 2;
    const int handleRadius = 6;
    const int padding = handleRadius;

    painter.setPen(Qt::NoPen);

    painter.setBrush(trackBgColor);
    painter.drawRect(padding, y - 2, std::max(0, width() - (2 * padding)), 4);

    const int x1 = posFromValue(m_lowerValue);
    const int x2 = posFromValue(m_upperValue);

    painter.setBrush(rangeColor);
    painter.drawRect(x1, y - 3, std::max(0, x2 - x1), 6);

    auto drawHandle = [&](int x, ActiveHandle handle) {
        const bool isSelected = hasFocus() && m_selectedHandle == handle;

        QPen pen(isSelected ? focusBorderColor : handleBorderColor);
        pen.setWidth(isSelected ? 2 : 1);
        painter.setPen(pen);
        painter.setBrush(handleColor);
        painter.drawEllipse(QPointF(x, y), handleRadius, handleRadius);
    };

    drawHandle(x1, ActiveHandle::Lower);
    drawHandle(x2, ActiveHandle::Upper);
}

RangeSlider::ActiveHandle RangeSlider::handleFromPos(int x) const
{
    const int x1 = posFromValue(m_lowerValue);
    const int x2 = posFromValue(m_upperValue);

    return (std::abs(x - x1) <= std::abs(x - x2)) ? ActiveHandle::Lower : ActiveHandle::Upper;
}

void RangeSlider::mousePressEvent(QMouseEvent* event)
{
    if (!isEnabled()) {
        return;
    }

    setFocus(Qt::MouseFocusReason);

    const int x = static_cast<int>(event->position().x());

    m_selectedHandle = handleFromPos(x);
    m_activeHandle = m_selectedHandle;

    const int value = (m_activeHandle == ActiveHandle::Lower) ? m_lowerValue : m_upperValue;
    const int xPos = (m_activeHandle == ActiveHandle::Lower) ? posFromValue(m_lowerValue) : posFromValue(m_upperValue);
    showTooltip(value, xPos);

    update();
}

void RangeSlider::mouseMoveEvent(QMouseEvent* event)
{
    if (!isEnabled()) {
        return;
    }

    const int x = static_cast<int>(event->position().x());
    const int value = valueFromPos(x);

    if (m_activeHandle == ActiveHandle::Lower) {
        setLowerValue(std::min(value, m_upperValue));
        showTooltip(m_lowerValue, posFromValue(m_lowerValue));
    } else if (m_activeHandle == ActiveHandle::Upper) {
        setUpperValue(std::max(value, m_lowerValue));
        showTooltip(m_upperValue, posFromValue(m_upperValue));
    } else if (m_hovered) {
        m_selectedHandle = handleFromPos(x);
        const int shownValue = (m_selectedHandle == ActiveHandle::Lower) ? m_lowerValue : m_upperValue;
        const int shownX = (m_selectedHandle == ActiveHandle::Lower) ? posFromValue(m_lowerValue) : posFromValue(m_upperValue);
        showTooltip(shownValue, shownX);
    }
}

void RangeSlider::mouseReleaseEvent([[maybe_unused]] QMouseEvent* event)
{
    m_activeHandle = ActiveHandle::None;

    if (!m_hovered) {
        hideTooltip();
    }
}

void RangeSlider::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    m_selectedHandle = handleFromPos(static_cast<int>(event->position().x()));
    QWidget::enterEvent(event);
}

void RangeSlider::leaveEvent(QEvent* event)
{
    m_hovered = false;
    m_activeHandle = ActiveHandle::None;
    hideTooltip();
    QWidget::leaveEvent(event);
}

void RangeSlider::keyPressEvent(QKeyEvent* event)
{
    if (!isEnabled()) {
        event->ignore();
        return;
    }

    if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) {
        m_selectedHandle = (m_selectedHandle == ActiveHandle::Lower) ? ActiveHandle::Upper : ActiveHandle::Lower;
        update();

        const int value = (m_selectedHandle == ActiveHandle::Lower) ? m_lowerValue : m_upperValue;
        const int xPos = posFromValue(value);
        showTooltip(value, xPos);

        event->accept();
        return;
    }

    const int step = 1;
    const bool isIncrease = (event->key() == Qt::Key_Right) || (event->key() == Qt::Key_Up);
    const bool isDecrease = (event->key() == Qt::Key_Left) || (event->key() == Qt::Key_Down);

    if (!isIncrease && !isDecrease) {
        QWidget::keyPressEvent(event);
        return;
    }

    const int delta = isIncrease ? step : -step;

    if (m_selectedHandle == ActiveHandle::Upper) {
        setUpperValue(m_upperValue + delta);
        showTooltip(m_upperValue, posFromValue(m_upperValue));
    } else {
        setLowerValue(m_lowerValue + delta);
        showTooltip(m_lowerValue, posFromValue(m_lowerValue));
    }

    event->accept();
}

void RangeSlider::wheelEvent(QWheelEvent* event)
{
    if (!isEnabled()) {
        event->ignore();
        return;
    }

    const QPoint delta = event->angleDelta();
    const int direction = (std::abs(delta.y()) >= std::abs(delta.x())) ? delta.y() : delta.x();

    if (direction == 0) {
        event->ignore();
        return;
    }

    const int step = (direction > 0) ? 1 : -1;

    ActiveHandle handleToChange = m_selectedHandle;

    if (m_lowerValue == m_upperValue) {
        const int mouseX = static_cast<int>(event->position().x());
        const int handleX = posFromValue(m_lowerValue);
        handleToChange = (mouseX >= handleX) ? ActiveHandle::Upper : ActiveHandle::Lower;
        m_selectedHandle = handleToChange;
    }

    if (handleToChange == ActiveHandle::Upper) {
        setUpperValue(m_upperValue + step);
        showTooltip(m_upperValue, posFromValue(m_upperValue));
    } else {
        setLowerValue(m_lowerValue + step);
        showTooltip(m_lowerValue, posFromValue(m_lowerValue));
    }

    event->accept();
}

void RangeSlider::focusInEvent(QFocusEvent* event)
{
    m_selectedHandle = ActiveHandle::Lower;
    QWidget::focusInEvent(event);
    update();
}

void RangeSlider::focusOutEvent(QFocusEvent* event)
{
    m_activeHandle = ActiveHandle::None;
    hideTooltip();
    QWidget::focusOutEvent(event);
    update();
}

int RangeSlider::valueFromPos(int x) const
{
    const int handleRadius = 6;
    const int padding = handleRadius;
    const int usableWidth = std::max(1, width() - (2 * padding));
    const double t = std::clamp(double(x - padding) / usableWidth, 0.0, 1.0);
    return m_minimum + static_cast<int>(t * (m_maximum - m_minimum));
}

int RangeSlider::posFromValue(int value) const
{
    const int handleRadius = 6;
    const int padding = handleRadius;
    const int usableWidth = std::max(1, width() - (2 * padding));
    const double range = std::max(1, m_maximum - m_minimum);
    const double t = std::clamp(double(value - m_minimum) / range, 0.0, 1.0);
    return padding + static_cast<int>(t * usableWidth);
}

bool RangeSlider::clampValues()
{
    const int oldLowerValue = m_lowerValue;
    const int oldUpperValue = m_upperValue;

    m_lowerValue = std::clamp(m_lowerValue, m_minimum, m_maximum);
    m_upperValue = std::clamp(m_upperValue, m_minimum, m_maximum);

    if (m_lowerValue > m_upperValue) {
        std::swap(m_lowerValue, m_upperValue);
    }

    return oldLowerValue != m_lowerValue || oldUpperValue != m_upperValue;
}

void RangeSlider::showTooltip(int value, int xPos)
{
    QString text;

    if (!m_lowerLabel.isEmpty() && value == m_lowerValue) {
        text = m_lowerLabel;
    } else if (!m_upperLabel.isEmpty() && value == m_upperValue) {
        text = m_upperLabel;
    } else {
        text = QString::number(value);
    }

    const auto size = fontMetrics().size(Qt::TextSingleLine, text);
    const QPoint globalPos = mapToGlobal(QPoint(xPos - (size.width() / 2) - 10, -size.height() - 22));

    QToolTip::showText(globalPos, text);
}

void RangeSlider::hideTooltip()
{
    QToolTip::hideText();
}

// RangeSlider.h
// Copyright (C) 2026 fractal <fractal@nebula-nook.ru>
#pragma once

#include <QWidget>
#include <cstdint>

class RangeSlider final : public QWidget {
    Q_OBJECT

    Q_PROPERTY(int minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue NOTIFY lowerValueChanged)
    Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue NOTIFY upperValueChanged)
    Q_PROPERTY(QString lowerLabel READ lowerLabel WRITE setLowerLabel)
    Q_PROPERTY(QString upperLabel READ upperLabel WRITE setUpperLabel)

   public:
    explicit RangeSlider(QWidget* parent = nullptr);

    [[nodiscard]] int minimum() const;
    [[nodiscard]] int maximum() const;

    [[nodiscard]] int lowerValue() const;
    [[nodiscard]] int upperValue() const;

    [[nodiscard]] QString lowerLabel() const;
    [[nodiscard]] QString upperLabel() const;

   public slots:
    void setMinimum(int value);
    void setMaximum(int value);
    void setLowerValue(int value);
    void setUpperValue(int value);
    void setLowerLabel(const QString& label);
    void setUpperLabel(const QString& label);

   signals:
    void minimumChanged(int);
    void maximumChanged(int);
    void lowerValueChanged(int);
    void upperValueChanged(int);
    void valueChanged(int, int);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool event(QEvent* event) override;

   private:
    enum class ActiveHandle : std::uint8_t { None, Lower, Upper };

    int m_minimum = 0;
    int m_maximum = 100;
    int m_lowerValue = 25;
    int m_upperValue = 75;

    QString m_lowerLabel;
    QString m_upperLabel;

    ActiveHandle m_activeHandle = ActiveHandle::None;
    ActiveHandle m_selectedHandle = ActiveHandle::Lower;
    bool m_hovered = false;

   private:
    int valueFromPos(int x) const;
    int posFromValue(int value) const;
    bool clampValues();

    ActiveHandle handleFromPos(int x) const;

    void showTooltip(int value, int xPos);
    static void hideTooltip();
};

// SnowflakePack.h
// Copyright (C) 2026 fractal <fractal@nebula-nook.ru>
#pragma once

#include <QFileInfo>
#include <QMovie>
#include <QObject>
#include <QPainter>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QString>
#include <QTimer>
#include <QtMath>
#include <utility>

class SnowflakePack {
   public:
    virtual ~SnowflakePack() = default;

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QString resourcePath() const { return QString(":/backgrounds/%1").arg(m_id); }

    virtual void draw(QPainter& painter, const QPolygonF& polygon) = 0;

   protected:
    SnowflakePack(QString id, QString name) : m_id(std::move(id)), m_name(std::move(name)) {}

   private:
    QString m_id;
    QString m_name;
};

class BuiltinSnowflakePack : public SnowflakePack {
   public:
    enum class Shape : std::uint8_t { Snowflake, Circle, Star };

    explicit BuiltinSnowflakePack(Shape shape) : SnowflakePack(getId(shape), getName(shape)), m_shape(shape) {}

    void draw(QPainter& painter, const QPolygonF& polygon) override
    {
        QRectF bounds = polygon.boundingRect();
        qreal size = qMin(bounds.width(), bounds.height());
        QPointF center = bounds.center();

        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);

        switch (m_shape) {
            case Shape::Circle:
                painter.drawEllipse(center, size * 0.4, size * 0.4);
                break;
            case Shape::Star:
                painter.drawPolygon(createStarPolygon(center, size * 0.45, size * 0.2));
                break;
            case Shape::Snowflake:
            default:
                painter.drawPolygon(createSnowflakePolygon(center, size * 0.45));
                break;
        }
        painter.restore();
    }

   private:
    static QString getId(Shape shape)
    {
        switch (shape) {
            case Shape::Circle:
                return "builtin-circle";
            case Shape::Star:
                return "builtin-star";
            default:
                return "builtin-snowflake";
        }
    }

    static QString getName(Shape shape)
    {
        switch (shape) {
            case Shape::Circle:
                return QObject::tr("Simple Circle");
            case Shape::Star:
                return QObject::tr("Star");
            default:
                return QObject::tr("Classic Snowflake");
        }
    }

    static QPolygonF createStarPolygon(const QPointF& center, qreal outerRadius, qreal innerRadius)
    {
        QPolygonF polygon;
        polygon.reserve(10);
        for (int i = 0; i < 10; ++i) {
            qreal angle = (M_PI / 5.0 * i) - (M_PI / 2.0);
            qreal radius = (i % 2 == 0) ? outerRadius : innerRadius;
            polygon << QPointF(center.x() + (std::cos(angle) * radius), center.y() + (std::sin(angle) * radius));
        }
        return polygon;
    }

    static QPointF rotatePoint(const QPointF& point, qreal angle)
    {
        const qreal s = std::sin(angle);
        const qreal c = std::cos(angle);

        return { (point.x() * c) - (point.y() * s), (point.x() * s) + (point.y() * c) };
    }

    static qreal randRange(qreal min, qreal max) { return min + (QRandomGenerator::global()->generateDouble() * (max - min)); }

    static int randInt(int min, int max) { return QRandomGenerator::global()->bounded(min, max + 1); }

    static QPolygonF createSnowflakePolygon(const QPointF& center, qreal radius)
    {
        const int armCount = randInt(5, 9);

        const qreal pi = qDegreesToRadians(180.0);
        const qreal armStep = (2.0 * pi) / armCount;

        const qreal coreRadius = radius * randRange(0.12, 0.18);
        const qreal armWidth = radius * randRange(0.06, 0.09);

        QPolygonF polygon;
        polygon.reserve((armCount * 40) + 1);

        auto rotate = [](const QPointF& p, qreal a) {
            const qreal s = std::sin(a);
            const qreal c = std::cos(a);
            return QPointF((p.x() * c) - (p.y() * s), (p.x() * s) + (p.y() * c));
        };

        auto addPoint = [&](qreal x, qreal y, qreal angle) {
            const QPointF r = rotate({ x, y }, angle);
            polygon << QPointF(center.x() + r.x(), center.y() + r.y());
        };

        for (int i = 0; i < armCount; ++i) {
            const qreal angle = i * armStep;

            const qreal armLength = radius * randRange(0.7, 1.0);
            const int branchCount = randInt(0, 3);

            addPoint(0.0, coreRadius, angle);
            addPoint(armWidth, coreRadius * 0.6, angle);

            const qreal mid = armLength * 0.55;

            for (int b = 0; b < branchCount; ++b) {
                const qreal t = (b + 1) / (branchCount + 1.0);
                const qreal base = mid * t;

                const qreal side = radius * (0.15 + 0.1 * b);
                const qreal jitter = randRange(-0.1, 0.1) * radius;

                addPoint(armWidth, base + jitter, angle);
                addPoint(side, base + (radius * 0.06), angle);
                addPoint(armWidth * 0.4, base + (radius * 0.12), angle);
                addPoint(armWidth, base + (radius * 0.18), angle);
            }

            addPoint(armWidth, armLength * 0.9, angle);
            addPoint(0.0, armLength, angle);
            addPoint(-armWidth, armLength * 0.9, angle);
            addPoint(-armWidth, coreRadius * 0.6, angle);
        }

        if (!polygon.isEmpty()) {
            polygon << polygon.first();
        }

        return polygon;
    }

    Shape m_shape;
};

class ImageSnowflakePack : public SnowflakePack {
   public:
    explicit ImageSnowflakePack(const QFileInfo& fileInfo)
        : SnowflakePack(fileInfo.baseName(), fileInfo.baseName()), m_pixmap(fileInfo.absoluteFilePath())
    {}

    void draw(QPainter& painter, const QPolygonF& polygon) override
    {
        if (m_pixmap.isNull()) {
            return;
        }

        QRectF boundingRect = polygon.boundingRect();
        painter.drawPixmap(boundingRect.toRect(), m_pixmap);
    }

   private:
    QPixmap m_pixmap;
};

class GifSnowflakePack : public QObject, public SnowflakePack {
    Q_OBJECT

   public:
    explicit GifSnowflakePack(const QFileInfo& fileInfo, QObject* parent = nullptr)
        : QObject(parent), SnowflakePack(fileInfo.baseName(), fileInfo.baseName()), m_path(fileInfo.absoluteFilePath()), m_movie(m_path)
    {
        m_movie.start();

        m_frameTimer.setParent(this);
        connect(&m_frameTimer, &QTimer::timeout, this, [this]() {
            if (m_movie.state() == QMovie::Running) {
                m_movie.jumpToNextFrame();
            }
        });
        m_frameTimer.start(50);
    }

    void draw(QPainter& painter, const QPolygonF& polygon) override
    {
        if (!m_movie.isValid() || m_movie.currentFrameNumber() < 0) {
            return;
        }

        QPixmap pixmap = m_movie.currentPixmap();
        if (pixmap.isNull()) {
            return;
        }

        QRectF boundingRect = polygon.boundingRect();
        painter.drawPixmap(boundingRect.toRect(), pixmap);
    }

   private:
    QString m_path;
    QMovie m_movie;
    QTimer m_frameTimer;
};

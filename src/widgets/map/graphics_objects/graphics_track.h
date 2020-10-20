#ifndef GRAPHICS_TRACK_H
#define GRAPHICS_TRACK_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"

class GraphicsTrack : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit GraphicsTrack(QColor color_idle, QColor color_unfocused, QObject *parent = nullptr);
    explicit GraphicsTrack(QColor color_idle, QColor color_unfocused, QPolygonF points, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void addPoint(QPointF);
    void setPoints(QPolygonF pts);

protected:
    virtual void changeFocus();

signals:

public slots:

private:
    QPolygonF points;

    QColor color_idle;
    QColor color_unfocused;
    QColor *current_color;
};

#endif // GRAPHICS_TRACK_H

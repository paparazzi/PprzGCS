#ifndef GRAPHICSLINE_H
#define GRAPHICSLINE_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"
#include <QPen>

class GraphicsLine : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:

    explicit GraphicsLine(QPointF a, QPointF b, QColor color, int stroke, QObject *parent = nullptr);
    void setColors(QColor color_unfocused);
    virtual void changeFocus() override;
    void setIgnoreEvent(bool ignore) {ignore_events = ignore;}
    //void setText(QString t) {text = t;}
    void setLine(QPointF a, QPointF b);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;


signals:

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QPointF A;
    QPointF B;

    QColor* current_color;

    QColor color_idle;
    QColor color_unfocused;

    int base_stroke;
    int stroke;

    QRectF last_bounding_rect;

    bool ignore_events;
    //QString text;
};

#endif // GRAPHICSLINE_H

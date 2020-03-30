#ifndef GRAPHICSLINE_H
#define GRAPHICSLINE_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"
#include <QPen>

class GraphicsLine : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
public:

    explicit GraphicsLine(QPointF a, QPointF b, QColor color, int stroke, QObject *parent = nullptr);
    void setColors(QColor color_unfocused);
    virtual void changeFocus();
    void setIgnoreEvent(bool ignore) {ignore_events = ignore;}
    //void setText(QString t) {text = t;}
    void setLine(QPointF a, QPointF b);
    void setStyle(Style s);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;


signals:

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

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

    Style style;
};

#endif // GRAPHICSLINE_H

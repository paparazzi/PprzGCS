#ifndef GRAPHICSLINE_H
#define GRAPHICSLINE_H

#include <QObject>
#include <QGraphicsLineItem>
#include "graphicsobject.h"
#include <QPen>

class GraphicsLine : public GraphicsObject, public QGraphicsLineItem
{
    Q_OBJECT
public:
    explicit GraphicsLine(QLineF linef, QPen pen_idle, QObject *parent = nullptr);
    void setColors(QColor color_unfocused);
    virtual void changeFocus();
    void setIgnoreEvent(bool ignore) {ignore_events = ignore;}

signals:

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPen pen_idle;
    QPen pen_unfocused;

    bool ignore_events;

};

#endif // GRAPHICSLINE_H

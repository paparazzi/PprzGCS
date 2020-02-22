#ifndef GRAPHICSPOINT_H
#define GRAPHICSPOINT_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include "graphicsobject.h"
#include <QBrush>

enum PointMoveState {
    PMS_IDLE,
    PMS_PRESSED,
    PMS_MOVED,
};

class GraphicsPoint : public GraphicsObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    explicit GraphicsPoint(qreal size, QColor color, QObject *parent = nullptr);
    void setColors(QColor colPressed, QColor colMoving, QColor colUnfocused);
    void setIgnoreEvent(bool ignore) {ignore_events = ignore;}

signals:
    void pointMoved(QPointF scenePos);
    void pointMoveFinished();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void changeFocus();



public slots:

private:
    //QPointF scene_pos;
    QPointF pressPos;
    PointMoveState move_state;

    QBrush brush_idle;
    QBrush brush_pressed;
    QBrush brush_moved;
    QBrush brush_unfocused;

    bool ignore_events;
};

#endif // GRAPHICSPOINT_H

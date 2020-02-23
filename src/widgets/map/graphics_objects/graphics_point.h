#ifndef GRAPHICSPOINT_H
#define GRAPHICSPOINT_H

#include <QObject>
#include <QGraphicsItem>
#include "graphics_object.h"
#include <QBrush>

enum PointMoveState {
    PMS_IDLE,
    PMS_PRESSED,
    PMS_MOVED,
};

class GraphicsPoint : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit GraphicsPoint(int size, QColor color, QObject *parent = nullptr);
    void setColors(QColor colPressed, QColor colMoving, QColor colUnfocused);
    void setIgnoreEvent(bool ignore) {ignore_events = ignore;}

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

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
    int halfSize;
    QPointF pressPos;
    PointMoveState move_state;

    QColor* current_color;
    QColor color_idle;
    QColor color_pressed;
    QColor color_moved;
    QColor color_unfocused;

    bool ignore_events;
};

#endif // GRAPHICSPOINT_H

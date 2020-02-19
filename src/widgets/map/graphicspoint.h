#ifndef GRAPHICSPOINT_H
#define GRAPHICSPOINT_H

#include <QObject>
#include <QGraphicsEllipseItem>

enum PointMoveState {
    PMS_IDLE,
    PMS_PRESSED,
    PMS_MOVED,
};

class GraphicsPoint : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    explicit GraphicsPoint(qreal size, QObject *parent = nullptr);

signals:
    void pointMoved(QPointF scenePos);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);


public slots:

private:
    //QPointF scene_pos;
    QPointF pressPos;
    PointMoveState move_state;
    bool movable;
};

#endif // GRAPHICSPOINT_H

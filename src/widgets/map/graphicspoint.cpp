#include "graphicspoint.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>

GraphicsPoint::GraphicsPoint(qreal size, QObject *parent) :
    QObject(parent),
    QGraphicsEllipseItem (-size/2, -size/2, size, size),
    move_state(PMS_IDLE), movable(true)
{

}


void GraphicsPoint::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    move_state = PMS_PRESSED;
}

void GraphicsPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(move_state == PMS_PRESSED) {
        QPointF dp = event->pos() - pressPos;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
            move_state = PMS_MOVED;
        }
    } else if(move_state == PMS_MOVED) {
        if(movable) {
            setPos(event->scenePos() - pressPos);
            emit(pointMoved(event->scenePos() - pressPos));
        }
    }
}

void GraphicsPoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(move_state == PMS_PRESSED) {
        //emit(itemClicked());
    }
    else if(move_state == PMS_MOVED) {
        emit(pointMoved(event->scenePos() - pressPos));
    }
    move_state = PMS_IDLE;
}

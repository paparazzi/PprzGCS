#include "graphicspoint.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

GraphicsPoint::GraphicsPoint(qreal size, QColor color, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsEllipseItem (-size/2, -size/2, size, size),
    move_state(PMS_IDLE), movable(true), ignore_events(false)
{
    brush_idle = QBrush(color);
    setBrush(brush_idle);
}

void GraphicsPoint::setColors(QColor colPressed, QColor colMoving, QColor colUnfocused) {
    brush_pressed = QBrush(colPressed);
    brush_moved = QBrush(colMoving);
    brush_unfocused = QBrush(colUnfocused);
}


void GraphicsPoint::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    GraphicsObject::mousePressEvent(event);
    pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    move_state = PMS_PRESSED;
    setBrush(brush_pressed);
}

void GraphicsPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_PRESSED) {
        QPointF dp = event->pos() - pressPos;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
            move_state = PMS_MOVED;
            setBrush(brush_moved);
        }
    } else if(move_state == PMS_MOVED) {
        if(movable) {
            setPos(event->scenePos() - pressPos);
            emit(pointMoved(event->scenePos() - pressPos));
        }
    }
}

void GraphicsPoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_PRESSED) {
        emit(objectClicked(event->scenePos()));
    }
    else if(move_state == PMS_MOVED) {
        emit(pointMoveFinished());
    }
    move_state = PMS_IDLE;
    setBrush(brush_idle);
}

void GraphicsPoint::changeFocus() {
    if(!isHighlighted()) {
        setBrush(brush_unfocused);
    } else {
        switch (move_state) {
        case PMS_IDLE:
            setBrush(brush_idle);
            break;
        case PMS_PRESSED:
            setBrush(brush_pressed);
            break;
        case PMS_MOVED:
            setBrush(brush_moved);
            break;
        }
    }
}

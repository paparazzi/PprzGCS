#include "graphics_point.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>

GraphicsPoint::GraphicsPoint(int size, QColor color, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsItem (),
    halfSize(size), move_state(PMS_IDLE), current_color(nullptr), ignore_events(false)
{
    color_idle = color;
    current_color = &color_idle;
}

void GraphicsPoint::setColors(QColor colPressed, QColor colMoving, QColor colUnfocused) {
    color_pressed = colPressed;
    color_moved = colMoving;
    color_unfocused = colUnfocused;
}


QRectF GraphicsPoint::boundingRect() const {
    return QRectF(-halfSize-1, -halfSize-1, 2*halfSize+2, 2*halfSize+2);
}


void GraphicsPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    QPainterPath path;
    double fx = 0.8;
    double fy = 1.0;
    if(!isHighlighted()) {
        current_color = &color_unfocused;
        fx /= qApp->property("SIZE_HIGHLIGHT_FACTOR").toDouble();
        fy /= qApp->property("SIZE_HIGHLIGHT_FACTOR").toDouble();
    }

    QPolygonF poly;
    poly.append(QPointF(0, halfSize*fy));
    poly.append(QPointF(halfSize*fx, 0));
    poly.append(QPointF(0, -halfSize*fy));
    poly.append(QPointF(-halfSize*fx, 0));
    path.addPolygon(poly);

    painter->setBrush(QBrush(*current_color));
    painter->setPen(Qt::NoPen);
    painter->drawPath(path);
}


void GraphicsPoint::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    GraphicsObject::mousePressEvent(event);
    pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    move_state = PMS_PRESSED;
    changeFocus();
}

void GraphicsPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_PRESSED) {
        QPointF dp = event->pos() - pressPos;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > qApp->property("MAP_MOVE_HYSTERESIS").toInt() && editable) {
            move_state = PMS_MOVED;
            changeFocus();
        }
    } else if(move_state == PMS_MOVED) {
        setPos(event->scenePos() - pressPos);
        emit(pointMoved(event->scenePos() - pressPos));
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
    changeFocus();
}

void GraphicsPoint::changeFocus() {
    if(!isHighlighted()) {
        current_color = &color_unfocused;
    } else {
        switch (move_state) {
        case PMS_IDLE:
            current_color = &color_idle;
            break;
        case PMS_PRESSED:
            current_color = &color_pressed;
            break;
        case PMS_MOVED:
            current_color = &color_moved;
            break;
        }
    }
    update();
}

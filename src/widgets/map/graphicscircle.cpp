#include "graphicscircle.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPen>

GraphicsCircle::GraphicsCircle(double radius, QObject *parent) :
    QObject(parent),
    QGraphicsEllipseItem (-radius, -radius, radius*2, radius*2),
    radius(radius), scalable(true), scale_state(CSS_IDLE)

{

}

void GraphicsCircle::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    dr = sqrt(pressPos.x()*pressPos.x() + pressPos.y()*pressPos.y()) - radius;
    if(qAbs(dr) < pen().width()*2) {
        scale_state = CSS_PRESSED;
        qDebug() << "Circle Pressed ! " << dr;
    } else {
        event->ignore();
    }
}

void GraphicsCircle::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF p = event->pos();
    double r = sqrt(p.x()*p.x() + p.y()*p.y()) - dr;

    if(scale_state == CSS_PRESSED) {
        if(qAbs(radius - r) > qApp->property("MAP_MOVE_HYSTERESIS").toInt()) {
            scale_state = CSS_SCALED;
        }
    } else if(scale_state == CSS_SCALED) {
        if(scalable) {
            radius = static_cast<int>(r);
            setRect(-radius, -radius, radius*2, radius*2);
            emit(circleScaled(radius));
        }
    }
}

void GraphicsCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
    if(scale_state == CSS_PRESSED) {
        //emit(itemClicked());
    }
    else if(scale_state == CSS_SCALED) {
        emit(circleScaled(radius));
    }
    scale_state = CSS_IDLE;
}

void GraphicsCircle::setRadius(double r) {
    radius = r;
    setRect(-radius, -radius, radius*2, radius*2);
}

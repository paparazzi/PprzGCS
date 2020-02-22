#include "graphicscircle.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPen>
#include <QColor>

GraphicsCircle::GraphicsCircle(double radius, QPen pen_idle, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsEllipseItem (-radius, -radius, radius*2, radius*2),
    radius(radius), scale_state(CSS_IDLE),
    pen_idle(pen_idle), pen_pressed(pen_idle), pen_scaling(pen_idle), pen_unfocused(pen_idle)

{
    setPen(pen_idle);
}

void GraphicsCircle::setColors(QColor colPressed, QColor colScaling, QColor colUnfocused) {
    pen_pressed = pen();
    pen_pressed.setColor(colPressed);

    pen_scaling = pen();
    pen_scaling.setColor(colScaling);

    pen_unfocused = pen();
    pen_unfocused.setColor(colUnfocused);
}



void GraphicsCircle::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    GraphicsObject::mousePressEvent(event);
    QPointF pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    dr = sqrt(pressPos.x()*pressPos.x() + pressPos.y()*pressPos.y()) - radius;
    if(qAbs(dr) < pen().width()*2) {
        scale_state = CSS_PRESSED;
        setPen(pen_pressed);
        qDebug() << "Circle Pressed ! " << dr;
    } else {
        event->ignore();
    }
}

void GraphicsCircle::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF p = event->pos();
    double r = sqrt(p.x()*p.x() + p.y()*p.y()) - dr;

    if(scale_state == CSS_PRESSED) {
        if(qAbs(radius - r) > qApp->property("MAP_MOVE_HYSTERESIS").toInt() && editable) {
            scale_state = CSS_SCALED;
            setPen(pen_scaling);
        }
    } else if(scale_state == CSS_SCALED) {
        radius = static_cast<int>(r);
        setRect(-radius, -radius, radius*2, radius*2);
        emit(circleScaled(radius));
    }
}

void GraphicsCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
    if(scale_state == CSS_PRESSED) {
        emit(objectClicked(event->scenePos()));
    }
    else if(scale_state == CSS_SCALED) {
        emit(circleScaleFinished());
    }
    scale_state = CSS_IDLE;
    changeFocus();
}

void GraphicsCircle::setRadius(double r) {
    radius = r;
    setRect(-radius, -radius, radius*2, radius*2);
}

void GraphicsCircle::changeFocus() {
    if(!isHighlighted()) {
        setPen(pen_unfocused);
        qDebug() << "Change Focus ! Unfocused !";
    } else {
        switch (scale_state) {
        case CSS_IDLE:
            setPen(pen_idle);
            qDebug() << "Change Focus ! Idle !";
            break;
        case CSS_PRESSED:
            setPen(pen_pressed);
            qDebug() << "Change Focus ! Pressed !";
            break;
        case CSS_SCALED:
            setPen(pen_scaling);
            qDebug() << "Change Focus ! Scaling !";
            break;
        }
    }
}

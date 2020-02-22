#include "graphicscircle.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPen>
#include <QColor>
#include <QPainter>
#include <QPainterPath>

GraphicsCircle::GraphicsCircle(double radius, QColor color, int stroke, QObject *parent) :
        GraphicsObject(parent),
        QGraphicsItem (),
        radius(radius), scale_state(CSS_IDLE),
        current_color(nullptr), color_idle(color), color_pressed(color), color_scaling(color), color_unfocused(color),
        base_stroke(stroke), stroke(stroke)
{
    current_color = &color_idle;
}

void GraphicsCircle::setColors(QColor colPressed, QColor colScaling, QColor colUnfocused) {
    color_pressed = colPressed;
    color_scaling = colScaling;
    color_unfocused = colUnfocused;
}


QRectF GraphicsCircle::boundingRect() const {
    double recthalf = radius + (stroke + 10)*scale_factor;
    return QRectF(-recthalf, -recthalf, 2*recthalf, 2*recthalf);
}


void GraphicsCircle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    path_draw.clear();
    double out_draw = radius + stroke*scale_factor;
    double in_draw = radius - stroke*scale_factor;
    QRectF outter_draw = QRectF(-out_draw, -out_draw, 2*out_draw, 2*out_draw);
    QRectF inner_draw = QRectF(-in_draw, -in_draw, 2*in_draw, 2*in_draw);
    path_draw.addEllipse(outter_draw);
    path_draw.addEllipse(inner_draw);

    path_shape.clear();
    double out_path = radius + (stroke + 5)*scale_factor;
    double in_path = radius - (stroke - 1)*scale_factor;
    QRectF outter_shape = QRectF(-out_path, -out_path, 2*out_path, 2*out_path);
    QRectF inner_shape = QRectF(-in_path, -in_path, 2*in_path, 2*in_path);
    path_shape.addEllipse(outter_shape);
    path_shape.addEllipse(inner_shape);


    painter->setBrush(QBrush(*current_color));
    painter->setPen(Qt::NoPen);
    painter->drawPath(path_draw);
}

QPainterPath GraphicsCircle::shape() const
{
    return path_shape;
}



void GraphicsCircle::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    dr = sqrt(pressPos.x()*pressPos.x() + pressPos.y()*pressPos.y()) - radius;
    GraphicsObject::mousePressEvent(event);
    scale_state = CSS_PRESSED;
    changeFocus();
}

void GraphicsCircle::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF p = event->pos();
    double r = sqrt(p.x()*p.x() + p.y()*p.y()) - dr;

    if(scale_state == CSS_PRESSED) {
        if(qAbs(radius - r) > qApp->property("MAP_MOVE_HYSTERESIS").toInt() && editable) {
            scale_state = CSS_SCALED;
            changeFocus();
        }
    } else if(scale_state == CSS_SCALED) {
        prepareGeometryChange();
        radius = static_cast<int>(r);
        update();
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
    update();
}

void GraphicsCircle::setRadius(double r) {
    prepareGeometryChange();
    radius = r;
}

void GraphicsCircle::changeFocus() {
    if(!isHighlighted()) {
        current_color = &color_unfocused;
        stroke = static_cast<int>(base_stroke / qApp->property("SIZE_HIGHLIGHT_FACTOR").toDouble());
    } else {
        stroke = base_stroke;
        switch (scale_state) {
        case CSS_IDLE:
            current_color = &color_idle;
            break;
        case CSS_PRESSED:
            current_color = &color_pressed;
            break;
        case CSS_SCALED:
            current_color = &color_scaling;
            break;
        }
    }
    update();
}

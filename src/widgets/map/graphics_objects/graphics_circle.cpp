#include "graphics_circle.h"
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
        base_stroke(stroke), stroke(stroke), text(), display_radius(true), style(DEFAULT)
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

    QRectF brect;
    if(display_radius) {
        double maxWidth = qMax(2*recthalf, 2*textPos.x() + 100);
        brect = QRectF(-recthalf, -recthalf, maxWidth, 2*recthalf);
    } else {
        brect = QRectF(-recthalf, -recthalf, 2*recthalf, 2*recthalf);
    }

    if(brect.width() > last_bounding_rect.width()) {
        return brect;
    } else {
        return last_bounding_rect;
    }
}


void GraphicsCircle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    path_draw = QPainterPath();

    if(style == DEFAULT) {
        double out_draw = radius + stroke*scale_factor;
        double in_draw = radius - stroke*scale_factor;
        QRectF outter_draw = QRectF(-out_draw, -out_draw, 2*out_draw, 2*out_draw);
        QRectF inner_draw = QRectF(-in_draw, -in_draw, 2*in_draw, 2*in_draw);
        path_draw.addEllipse(outter_draw);
        path_draw.addEllipse(inner_draw);
    } else if (style == CURRENT_NAV) {
        path_draw.addEllipse(QRectF(-radius, -radius, 2*radius, 2*radius));
    }



    path_shape = QPainterPath();
    double out_path = radius + (stroke + 5)*scale_factor;
    double in_path = radius - (stroke - 1)*scale_factor;
    QRectF outter_shape = QRectF(-out_path, -out_path, 2*out_path, 2*out_path);
    QRectF inner_shape = QRectF(-in_path, -in_path, 2*in_path, 2*in_path);
    path_shape.addEllipse(outter_shape);
    path_shape.addEllipse(inner_shape);


    if(style == DEFAULT) {
        painter->setBrush(QBrush(*current_color));
        painter->setPen(Qt::NoPen);
        painter->drawPath(path_draw);
    } else if (style == CURRENT_NAV) {
        painter->setPen(QPen(Qt::green));
        painter->drawPath(path_draw);
    }



    double recthalf = radius + (stroke + 10)*scale_factor;
    if(display_radius) {
        path_draw = QPainterPath();
        QFont font;
        font.setPointSize (qApp->property("MAPITEMS_FONT").toInt()*scale_factor);
        font.setWeight(QFont::DemiBold);
        path_draw.addText(textPos.x()+10, textPos.y(), font, text);
        painter->setBrush(QBrush(color_idle));
        painter->setPen(Qt::NoPen);
        painter->drawPath(path_draw);

        QFontMetrics fm(font);

        double maxWidth = qMax(2*recthalf, 2*textPos.x() + fm.horizontalAdvance(text) + 100);
        last_bounding_rect = QRectF(-recthalf, -recthalf, maxWidth, 2*recthalf);
    } else {
        last_bounding_rect = QRectF(-recthalf, -recthalf, 2*recthalf, 2*recthalf);
    }
}

QPainterPath GraphicsCircle::shape() const
{
    return path_shape;
}



void GraphicsCircle::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    textPos = event->pos();
    QPointF pressPos = QPointF(event->pos().x() * scale(), event->pos().y() * scale());
    dr = sqrt(pressPos.x()*pressPos.x() + pressPos.y()*pressPos.y()) - radius;
    GraphicsObject::mousePressEvent(event);
    scale_state = CSS_PRESSED;
    display_radius = true;
    changeFocus();
}

void GraphicsCircle::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF mousePos = event->pos();
    double r = sqrt(mousePos.x()*mousePos.x() + mousePos.y()*mousePos.y()) - dr;

    if(scale_state == CSS_PRESSED) {
        if(qAbs(radius - r) > qApp->property("MAP_MOVE_HYSTERESIS").toInt() && editable) {
            scale_state = CSS_SCALED;
            changeFocus();
        }
    } else if(scale_state == CSS_SCALED) {
        prepareGeometryChange();
        radius = static_cast<int>(r);
        textPos = mousePos;
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
    display_radius = false;
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

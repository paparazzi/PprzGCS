#include "graphics_point.h"
#include "math.h"
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include "gcs_utils.h"

#define COLOR_IDLE 0
#define COLOR_PRESSED 1
#define COLOR_MOVED 2
#define COLOR_UNFOCUSED 3

GraphicsPoint::GraphicsPoint(int size, PprzPalette palette, QObject *parent) :
    GraphicsObject(palette, parent),
    QGraphicsItem (),
    halfSize(size), move_state(PMS_IDLE), animation_couter(0)
{
    current_color = COLOR_IDLE;
    setAcceptHoverEvents(true);     // its name marked while hovering

    animation_timer->setInterval(500);
    connect(animation_timer, &QTimer::timeout, this, [=,this]()
    {
        animation_couter += 1;
        prepareGeometryChange();
    });
}

QRectF GraphicsPoint::boundingRect() const {
    // with the white border and the shadow
    return QRectF(-halfSize-4, -halfSize-4, 2*halfSize+8, 2*halfSize+9);
}


void GraphicsPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;
    auto settings = getAppSettings();

    if(hover_mark && style == DEFAULT) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(0x4da3ff), 2.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(QPointF(0, 0), halfSize + 2, halfSize + 2);
        painter->restore();
    }
    if(animation == WP_MOVING) {
        painter->rotate(animation_couter * 45 + 1);
    }

    switch (style) {
    case DEFAULT: {
        QPainterPath path;
        // a square diamond
        double fx = 0.9;
        double fy = 0.9;
        if(!isHighlighted()) {
            current_color = COLOR_UNFOCUSED;
            fx /= settings.value("map/size_highlight_factor").toDouble();
            fy /= settings.value("map/size_highlight_factor").toDouble();
        }

        QPolygonF poly;
        poly.append(QPointF(0, halfSize*fy));
        poly.append(QPointF(halfSize*fx, 0));
        poly.append(QPointF(0, -halfSize*fy));
        poly.append(QPointF(-halfSize*fx, 0));
        poly.append(QPointF(0, halfSize*fy));


        path.addPolygon(poly);

        // a shadow, then the waypoint with a white border: visible on any background
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 90));
        painter->drawPath(path.translated(0, 1.5));
        painter->setPen(QPen(QColor(255, 255, 255, isHighlighted() ? 255 : 170), 2));
        painter->setBrush(QBrush(palette.getVariant(current_color)));
        painter->drawPath(path);
        }
        break;
    case CARROT: {
        QPainterPath path;
        QPolygonF poly;
        poly.append(QPointF(halfSize*cos(M_PI/6), -halfSize*sin(M_PI/6)));
        poly.append(QPointF(halfSize*cos(5*M_PI/6), -halfSize*sin(5*M_PI/6)));
        poly.append(QPointF(halfSize*cos(3*M_PI/2), -halfSize*sin(3*M_PI/2)));

        path.addPolygon(poly);
        // the navigation target: orange, with a shadow and a white border like the waypoints
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 90));
        painter->drawPath(path.translated(0, 1.5));
        painter->setPen(QPen(Qt::white, 1.5));
        painter->setBrush(QBrush(QColor(255, 170, 0)));
        painter->drawPath(path);
        }
        break;
    case CURRENT_NAV: {
        //draw nothing
        }
        break;
    case GCS: {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(palette.getVariant(current_color), 3));
        painter->drawEllipse(QPoint(0,0), 2*halfSize/3, 2*halfSize/3);
        painter->drawLine(0, -halfSize, 0, halfSize);
        painter->drawLine(-halfSize, 0, halfSize, 0);
        }
        break;
    case DCSHOT: {
        painter->setBrush(QBrush(palette.getVariant(current_color)));
        painter->drawEllipse(QPoint(0, 0), halfSize/2, halfSize/2);
        }
        break;
    case CRASH: {
        // Draw a red cross with thickness 3
        painter->setPen(QPen(QColor(255, 0, 0), 3));
        painter->drawLine(QPoint(-halfSize/2, -halfSize/2), QPoint(halfSize/2, halfSize/2));
        painter->drawLine(QPoint(-halfSize/2, halfSize/2), QPoint(halfSize/2, -halfSize/2));
        }
        break;
    }

}

QPainterPath GraphicsPoint::shape() const {
    if(style == CURRENT_NAV || style == CARROT) {
        QPainterPath path;
        return path;
    } else {
        return QGraphicsItem::shape();
    }
}

void GraphicsPoint::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    GraphicsObject::mousePressEvent(event);
    // in the scene: also right when pressed on its name, away from the point
    pressPos = event->scenePos() - scenePos();
    pressScreen = event->screenPos();
    move_state = PMS_PRESSED;
    changeFocus();
    emit pointPressed(event->scenePos(), event->modifiers() & Qt::ControlModifier);
}

void GraphicsPoint::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverEnterEvent(event);
    emit hoverChanged(true);
}

void GraphicsPoint::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    QGraphicsItem::hoverLeaveEvent(event);
    emit hoverChanged(false);
}

bool GraphicsPoint::sceneEventFilter(QGraphicsItem* watched, QEvent* event) {
    (void)watched;
    if(event->type() == QEvent::GraphicsSceneHoverEnter || event->type() == QEvent::GraphicsSceneHoverLeave) {
        emit nameHoverChanged(event->type() == QEvent::GraphicsSceneHoverEnter);
        return false;
    }
    if(ignore_events) {
        return false;
    }
    auto me = static_cast<QGraphicsSceneMouseEvent*>(event);
    switch(event->type()) {
    case QEvent::GraphicsSceneMousePress:
        if(me->button() != Qt::LeftButton) {
            return false;
        }
        mousePressEvent(me);
        return true;
    case QEvent::GraphicsSceneMouseMove:
        mouseMoveEvent(me);
        return true;
    case QEvent::GraphicsSceneMouseRelease:
        mouseReleaseEvent(me);
        return true;
    case QEvent::GraphicsSceneMouseDoubleClick:
        mouseDoubleClickEvent(me);
        return true;
    default:
        return false;
    }
}

void GraphicsPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    auto settings = getAppSettings();
    if(move_state == PMS_PRESSED) {
        QPointF dp = event->screenPos() - pressScreen;
        double d = sqrt(dp.x()*dp.x() + dp.y()*dp.y());
        if(d > settings.value("map/move_hyteresis").toInt() && editable) {
            move_state = PMS_MOVED;
            changeFocus();
        }
    } else if(move_state == PMS_MOVED) {
        setPos(event->scenePos() - pressPos);
        emit pointMoved(event->scenePos() - pressPos);
    }
}

void GraphicsPoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    if(move_state == PMS_PRESSED) {
        if(double_pressed) {
            emit objectDoubleClicked(event->scenePos());
        } else {
            emit objectClicked(event->scenePos());
        }
    }
    else if(move_state == PMS_MOVED) {
        emit pointMoveFinished(event->scenePos() - pressPos);
    }
    move_state = PMS_IDLE;
    double_pressed = false;
    changeFocus();
}

/// The second press of a double click: the card once released without moving (a quick second
/// press that drags is a drag, the card opens when it is released)
void GraphicsPoint::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    mousePressEvent(event);
    double_pressed = true;
}

void GraphicsPoint::changeFocus() {
    if(!isHighlighted()) {
        current_color = COLOR_UNFOCUSED;
    } else {
        switch (move_state) {
        case PMS_IDLE:
            current_color = COLOR_IDLE;
            break;
        case PMS_PRESSED:
            current_color = COLOR_PRESSED;
            break;
        case PMS_MOVED:
            current_color = COLOR_MOVED;
            break;
        }
    }
    update();
}

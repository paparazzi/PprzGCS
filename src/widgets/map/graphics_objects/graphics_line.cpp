#include "graphics_line.h"
#include <QDebug>
#include <QPainter>
#include <QApplication>

GraphicsLine::GraphicsLine(QPointF a, QPointF b, QColor color, int stroke, QObject *parent) :
    GraphicsObject(parent),
    QGraphicsItem (),
    A(a), B(b),
    color_idle(color), color_unfocused(color),
    base_stroke(stroke), stroke(stroke),
    style(DEFAULT)
{
    current_color = &color_idle;
}


QRectF GraphicsLine::boundingRect() const {
    QPointF topLeft = QPointF(qMin(A.x(), B.x()), qMin(A.y(), B.y()));
    QPointF bottomRight = QPointF(qMax(A.x(), B.x()), qMax(A.y(), B.y()));
    QRectF rect = QRectF(topLeft, bottomRight);
    rect = rect.united(last_bounding_rect);
    return rect;
}


void GraphicsLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    (void)option;
    (void)widget;

    if(style == Style::DEFAULT) {
        painter->setPen(QPen(*current_color, stroke));
    } else if(style == Style::CURRENT_NAV) {
        painter->setPen(QPen(Qt::green));
    }

    painter->drawLine(A, B);

    QPointF topLeft = QPointF(qMin(A.x(), B.x()), qMin(A.y(), B.y()));
    QPointF bottomRight = QPointF(qMax(A.x(), B.x()), qMin(A.y(), B.y()));
    last_bounding_rect = QRectF(topLeft, bottomRight);
    //qDebug() << "paint!";
}

QPainterPath GraphicsLine::shape() const {
    QPainterPath path;
    path.moveTo(A);
    path.lineTo(B);
    QPainterPathStroker pps;
    pps.setWidth(stroke);
    QPainterPath p = pps.createStroke(path);
    p.addPath(path);
    return p;
}

void GraphicsLine::changeFocus() {
    if(!isHighlighted()) {
        current_color = &color_unfocused;
        stroke = static_cast<int>(base_stroke / qApp->property("SIZE_HIGHLIGHT_FACTOR").toDouble());
    } else {
        stroke = base_stroke;
        current_color = &color_idle;
    }
}


void GraphicsLine::setColors(QColor colUnfocused) {
    color_unfocused = colUnfocused;
}

void GraphicsLine::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    GraphicsObject::mousePressEvent(event);
    qDebug() << "Line PRESSED !";
}

void GraphicsLine::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    qDebug() << "Line MOVED !";
}

void GraphicsLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if(ignore_events) {
        event->ignore();
        return;
    }
    (void)event;
    qDebug() << "Line RELEASE !";
}

void GraphicsLine::setLine(QPointF a, QPointF b) {
    A=a;
    B=b;
    prepareGeometryChange();
}

void GraphicsLine::setStyle(Style s) {
    qDebug() << "NAV style bro!";
    style = s;
}

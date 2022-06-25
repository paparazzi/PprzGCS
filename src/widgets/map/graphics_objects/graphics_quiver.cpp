#include "graphics_quiver.h"

GraphicsQuiver::GraphicsQuiver(float size, PprzPalette palette, QObject *parent) : 
    GraphicsObject(palette, parent),
    QGraphicsItem (),
    size(size)
{
    setFlag(ItemIsMovable);
}

QRectF GraphicsQuiver::boundingRect() const
{
    return QRectF(-size, -size, 2*size, 2*size);
}

void GraphicsQuiver::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)option;
    (void)widget;
    painter->setPen(QPen(Qt::black, 3));

    painter->drawLine(0, 0, 0, -size);

    painter->drawLine(0, -size, -size/10, -9*size/10);
    painter->drawLine(0, -size, size/10, -9*size/10);
}

void GraphicsQuiver::changeFocus() {

}
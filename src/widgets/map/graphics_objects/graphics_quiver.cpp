#include "graphics_quiver.h"

GraphicsQuiver::GraphicsQuiver(double size, PprzPalette palette, QPen pen, QObject *parent) : 
    GraphicsObject(palette, parent),
    QGraphicsItem (),
    size(size),
    pen(pen)
{

}

QRectF GraphicsQuiver::boundingRect() const
{
    return QRectF(-size, -size, 2*size, 2*size);
}

void GraphicsQuiver::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)option;
    (void)widget;
    painter->setPen(pen);

    painter->drawLine(0, 0, 0, -size);

    painter->drawLine(0, -size, -size/10, -9*size/10);
    painter->drawLine(0, -size, size/10, -9*size/10);
    
    painter->setRenderHint(QPainter::Antialiasing);
}

void GraphicsQuiver::changeFocus() {

}
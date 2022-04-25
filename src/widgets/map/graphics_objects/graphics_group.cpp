#include "graphics_group.h"

GraphicsGroup::GraphicsGroup(PprzPalette palette, QObject *parent) :
    GraphicsObject(palette, parent),
    QGraphicsItemGroup()
{

}

void GraphicsGroup::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    emit mousePressed(event);
}

void GraphicsGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseMoved(event);
}

void GraphicsGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseReleased(event);
}

void GraphicsGroup::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseDoubleClicked(event);
}

void GraphicsGroup::changeFocus() {

}

#include "graphicsobject.h"
#include <QDebug>

GraphicsObject::GraphicsObject(QObject *parent) : QObject(parent), highlighted(true)
{

}


void GraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
    if(!highlighted) {
        highlighted = true;
        emit(objectGainedHighlight());
    }
}

void GraphicsObject::setHighlighted(bool h) {
    highlighted = h;
    changeFocus();
}

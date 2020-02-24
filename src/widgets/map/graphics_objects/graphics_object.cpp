#include "graphics_object.h"
#include <QDebug>

GraphicsObject::GraphicsObject(QObject *parent) : QObject(parent), editable(true), scale_factor(1.0), highlighted(true)
{

}


void GraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    (void)event;
    if(!highlighted && !forbid_highlight) {
        highlighted = true;
        emit(objectGainedHighlight());
    }
}

void GraphicsObject::setHighlighted(bool h) {
    highlighted = h;
    changeFocus();
}

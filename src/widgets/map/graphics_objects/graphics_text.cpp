#include "graphics_text.h"
#include <QDebug>

GraphicsText::GraphicsText(const QString &text, QObject *parent):
    GraphicsObject(parent),
    QGraphicsTextItem (text),
    style(DEFAULT)

{

}

void GraphicsText::changeFocus() {
    switch (style) {
    case DEFAULT:
        if(isHighlighted()) {
            setVisible(true);
        } else {
            setVisible(false);
        }

        break;
    case CARROT:
        setVisible(false);
        break;
    case CURRENT_NAV:
        setVisible(false);
        break;
    }

    update();
}

void GraphicsText::setStyle(Style s) {
    style = s;
    changeFocus();
}


//void GraphicsText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
//    qDebug() << "highlited: " << isHighlighted();



//    QGraphicsTextItem::paint(painter, option, widget);
//}

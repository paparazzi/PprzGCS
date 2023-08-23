#include "graphics_text.h"
#include <QDebug>

GraphicsText::GraphicsText(const QString &text, PprzPalette palette,QObject *parent):
    GraphicsObject(palette, parent),
    QGraphicsTextItem (text)

{
    setDefaultTextColor(palette.getColor());
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
    case GCS:
        setVisible(false);
        break;
    case DCSHOT:
        setVisible(false);
        break;
    }

    update();
}

void GraphicsText::setStyle(Style s) {
    GraphicsObject::setStyle(s);
    changeFocus();
}


//void GraphicsText::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
//    qDebug() << "highlited: " << isHighlighted();



//    QGraphicsTextItem::paint(painter, option, widget);
//}

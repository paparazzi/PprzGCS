#ifndef GRAPHICS_TEXT_H
#define GRAPHICS_TEXT_H

#include <QGraphicsTextItem>
#include "graphics_object.h"

class GraphicsText : public GraphicsObject, public QGraphicsTextItem
{
public:
    GraphicsText(const QString &text, PprzPalette palette,QObject *parent = nullptr);
    void setStyle(Style s);
    //void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    virtual void changeFocus() override;

};

#endif // GRAPHICS_TEXT_H

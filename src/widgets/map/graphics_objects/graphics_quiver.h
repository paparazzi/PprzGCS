#ifndef GRAPHICSQUIVER_H
#define GRAPHICSQUIVER_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPen>
#include "graphics_object.h"

class GraphicsQuiver : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    explicit GraphicsQuiver(float size, PprzPalette palette, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    float size;

protected:
    virtual void changeFocus() override;
};

#endif // GRAPHICSQUIVER_H
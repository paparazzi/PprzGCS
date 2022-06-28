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
    explicit GraphicsQuiver(double size, PprzPalette palette, QPen pen, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    double size;

protected:
    virtual void changeFocus() override;

private:
    QPen pen;
};

#endif // GRAPHICSQUIVER_H
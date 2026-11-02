#ifndef GRAPHICS_AIRCRAFT_H
#define GRAPHICS_AIRCRAFT_H

#include <QObject>
#include <QGraphicsItem>
#include <QDomElement>
#include "graphics_object.h"

class GraphicsAircraft : public GraphicsObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:
    GraphicsAircraft(PprzPalette palette, QString icon_path, int size, QObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    virtual void changeFocus() override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;     // select the aircraft

private:
    void loadSvg(QString path);
    void changeColor(QColor color);

    int size;
    QPixmap pixmap;
    QDomDocument svgdoc;
};

#endif // GRAPHICS_AIRCRAFT_H

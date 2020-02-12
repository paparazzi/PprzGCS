#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "map2d.h"
#include <QGraphicsItem>
#include <QList>
#include "mapitem.h"

class MapWidget : public Map2D
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addCircle(Point2DLatLon latlon, int size);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    QList<MapItem*> _items;

signals:

};

#endif // MAPWIDGET_H

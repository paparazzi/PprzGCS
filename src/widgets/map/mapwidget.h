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

    void addItem(QGraphicsItem* graphicItem, Point2DLatLon latlon, int zValue = 10, double zoomFactor = 1);

    void addCircle(Point2DLatLon latlon, int size, QBrush brush = QBrush(Qt::red));

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    QList<MapItem*> _items;

signals:

};

#endif // MAPWIDGET_H

#include "mapwidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "mapitem.h"
#include <iostream>

MapWidget::MapWidget(QWidget *parent) : Map2D(QString("://tile_sources.xml"), parent)
{
}

void MapWidget::addCircle(Point2DLatLon latlon, int size) {
    QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(- size/2,- size/2, size, size);
     circle->setBrush(QBrush(Qt::red));

     MapItem* item = new MapItem(circle, latlon);
     QPointF point = scenePoint(latlon, zoomLevel());
     item->setPos(point);
     item->setScale(1/scaleFactor());
     scene()->addItem(item);
     item->setZValue(30);
     _items.append(item);
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    Map2D::mousePressEvent(event);
    if(event->buttons() & Qt::RightButton) {
        Point2DLatLon latlon = latlonFromView(event->pos(), zoomLevel());
        addCircle(latlon, 25);
    }
}


void MapWidget::wheelEvent(QWheelEvent* event) {
    Map2D::wheelEvent(event);
    for(auto item: _items) {
        QPointF point = scenePoint(item->position(), zoomLevel());
        item->setPos(point);
        item->setScale(1/scaleFactor());
    }
}

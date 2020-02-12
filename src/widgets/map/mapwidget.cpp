#include "mapwidget.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include "mapitem.h"
#include <iostream>

MapWidget::MapWidget(QWidget *parent) : Map2D(QString("://tile_sources.xml"), parent)
{
}

void MapWidget::addItem(QGraphicsItem* graphicItem, Point2DLatLon latlon, int zValue, double zoomFactor) {
    MapItem* map_item = new MapItem(graphicItem, latlon);
    QPointF point = scenePoint(latlon, zoomLevel());
    map_item->setPos(point);
    map_item->setScale(1/scaleFactor());
    scene()->addItem(map_item);
    map_item->setZValue(zValue);
    map_item->setZoomFactor(zoomFactor);
    map_item->scaleToZoom(zoom(), scaleFactor());
    _items.append(map_item);
}

void MapWidget::addCircle(Point2DLatLon latlon, int size, QBrush brush) {
    QGraphicsEllipseItem *circle = new QGraphicsEllipseItem(- size/2,- size/2, size, size);
    circle->setBrush(brush);
     addItem(circle, latlon, 10, 1.15);
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
        item->scaleToZoom(zoom(), scaleFactor());
    }
}

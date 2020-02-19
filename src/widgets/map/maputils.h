#ifndef MAPUTILS_H
#define MAPUTILS_H

#include <QPointF>
#include "point2dlatlon.h"
#include "point2dtile.h"
#include "math.h"

Point2DTile tilePoint(QPointF scenePos, int zoom, int tileSize);
QPointF scenePoint(Point2DTile tilePoint, int tileSize);
QPointF scenePoint(Point2DLatLon latlon, int zoomLvl, int tileSize);
Point2DLatLon latlonPoint(QPointF scenePos, int zoom, int tileSize);
int zoomLevel(double zoom);
//Point2DLatLon latlonFromView(QPoint viewPos, int zoom);



#endif // MAPUTILS_H

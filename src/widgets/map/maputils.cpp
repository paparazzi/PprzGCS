#include "maputils.h"

Point2DTile tilePoint(QPointF scenePos, int zoom, int tileSize) {
    return Point2DTile(scenePos.x()/tileSize, scenePos.y()/tileSize, zoom);
}

QPointF scenePoint(Point2DTile tilePoint, int tileSize) {
    return QPointF(tilePoint.x()*tileSize, tilePoint.y()*tileSize);
}

QPointF scenePoint(Point2DLatLon latlon, int zoomLvl, int tileSize) {
    Point2DTile tile_pos = Point2DTile(latlon, zoomLvl);
    return scenePoint(tile_pos, tileSize);
}

Point2DLatLon latlonPoint(QPointF scenePos, int zoom, int tileSize) {
    return Point2DLatLon(tilePoint(scenePos, zoom, tileSize));
}

int zoomLevel(double zoom) {
    return static_cast<int>(ceil(zoom));
}
//Point2DLatLon latlonFromView(QPoint viewPos, int zoom) {
//    return Point2DLatLon(tilePoint(mapToScene(viewPos), zoom));
//}

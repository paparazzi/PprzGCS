#include "circleitem.h"
#include <QPen>
#include "maputils.h"
#include <QDebug>

CircleItem::CircleItem(Point2DLatLon pt, double radius, QColor color, double zoom, int tile_size, double neutral_scale_zoom, QGraphicsItem *parent) :
    MapItem(tile_size, neutral_scale_zoom, parent), _radius(radius)
{
    center = new WaypointItem(pt, 20, color,  tile_size);
    addToGroup(center);
    int circle_radius = tile_size * (radius * pow(2, zoomLevel(zoom))) / (2*M_PI*6356752.3*cos(pt.lat()*M_PI/180.0));
    //int circle_radius = radius*10;    // TODO take radius in meters and convert it to pixels.
    circle = new QGraphicsEllipseItem(-circle_radius, -circle_radius, 2*circle_radius, 2*circle_radius);
    circle->setPen(QPen(QBrush(color), 5));
    addToGroup(circle);
    _zoom = zoom;
}


void CircleItem::scaleToZoom(double zoom, double viewScale) {
    _zoom = zoom;
    QPointF point = scenePoint(center->position(), zoomLevel(zoom), tile_size);
    setPos(point);
    int r = tile_size * (_radius * pow(2, zoomLevel(_zoom))) / (2*M_PI*6535*1000*cos(center->position().lat()*M_PI/180.0));
    //int r = pow(2, zoomLevel(zoom)-zoomLevel(15)) * _radius;
    qDebug() << "r=" << r;
    circle->setRect(-r, -r, 2*r, 2*r);
    double s = pow(zoom_factor, zoom - neutral_scale_zoom)/viewScale;
    center->setScale(s);

    //center->scaleToZoom(zoom, viewScale);
}

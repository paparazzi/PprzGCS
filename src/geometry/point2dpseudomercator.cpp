#include "point2dpseudomercator.h"
#include "point2dtile.h"

constexpr double EXTENT = 20037508.342789244;

const QString Point2DPseudoMercator::EPSG = "EPSG:3857";
//EPSG:900913

Point2DPseudoMercator::Point2DPseudoMercator(): GeographicCoordinate(Point2DPseudoMercator::EPSG),
    _x(0), _y(0)
{
}

Point2DPseudoMercator::Point2DPseudoMercator(double x, double y):  GeographicCoordinate(Point2DPseudoMercator::EPSG),
    _x(x), _y(y)
{
}

Point2DPseudoMercator::Point2DPseudoMercator(Point2DTile pt) :  GeographicCoordinate(Point2DPseudoMercator::EPSG)
{
    _x = (2*pt.x()/(1<<pt.zoom()) - 1) * EXTENT;
    _y = -(2*pt.y()/(1<<pt.zoom()) - 1) * EXTENT;
}

Point2DPseudoMercator::Point2DPseudoMercator(PJ_COORD coord) : GeographicCoordinate(Point2DPseudoMercator::EPSG),
    _x(coord.xy.x), _y(coord.xy.y)
{
}

//compatibility
Point2DPseudoMercator::Point2DPseudoMercator(CartesianCoor car): GeographicCoordinate(Point2DPseudoMercator::EPSG),
    _x(car.x()), _y(car.y())
{
    assert(car.getCRS() == "EPSG:3857");
}

Point2DTile Point2DPseudoMercator::toTile(int zoom) {
    double xTile = ((1<<zoom)*(_x/EXTENT+ 1))/2.;
    double yTile = ((1<<zoom)*(-_y/EXTENT+ 1))/2.;
    return Point2DTile(xTile, yTile, zoom);
}


QRectF Point2DPseudoMercator::getBounds() {
    return QRectF(-EXTENT, -EXTENT, 2*EXTENT, 2*EXTENT);
}

Point2DPseudoMercator Point2DPseudoMercator::operator-(const Point2DPseudoMercator& other) const {
    return Point2DPseudoMercator(x() - other.x(), y() - other.y());
}

Point2DPseudoMercator Point2DPseudoMercator::operator+(const Point2DPseudoMercator& other) const {
    return Point2DPseudoMercator(x() + other.x(), y() + other.y());
}

Point2DPseudoMercator Point2DPseudoMercator::operator/(double d) const {
    return Point2DPseudoMercator(x()/d, y()/d);
}

Point2DPseudoMercator Point2DPseudoMercator::operator*(double d) const {
    return Point2DPseudoMercator(x()*d, y()*d);
}

PJ_COORD Point2DPseudoMercator::toProj() {
    return proj_coord (_x, _y, 0, 0);
}

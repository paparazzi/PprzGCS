#include "point2dpseudomercator.h"
#include "point2dtile.h"

constexpr double EXTENT_3857 = 20037508.342789244;

const QString Point2DPseudoMercator::EPSG = "EPSG:3857";
//EPSG:900913

Point2DPseudoMercator::Point2DPseudoMercator(): Coordinate(Point2DPseudoMercator::EPSG),
    _x(0), _y(0)
{
}

Point2DPseudoMercator::Point2DPseudoMercator(double x, double y):  Coordinate(Point2DPseudoMercator::EPSG),
    _x(x), _y(y)
{
}

Point2DPseudoMercator::Point2DPseudoMercator(Point2DTile pt) :  Coordinate(Point2DPseudoMercator::EPSG)
{
    _x = (2*pt.x()/(1<<pt.zoom()) - 1) * EXTENT_3857;
    _y = -(2*pt.y()/(1<<pt.zoom()) - 1) * EXTENT_3857;
}

Point2DPseudoMercator::Point2DPseudoMercator(PJ_COORD coord) : Coordinate(Point2DPseudoMercator::EPSG),
    _x(coord.xy.x), _y(coord.xy.y)
{
}

//compatibility
Point2DPseudoMercator::Point2DPseudoMercator(CartesianCoor car): Coordinate(Point2DPseudoMercator::EPSG),
    _x(car.x()), _y(car.y())
{
    assert(car.getCRS() == "EPSG:3857");
}

Point2DTile Point2DPseudoMercator::toTile(int zoom) {
    double xTile = ((1<<zoom)*(_x/EXTENT_3857+ 1))/2.;
    double yTile = ((1<<zoom)*(-_y/EXTENT_3857+ 1))/2.;
    return Point2DTile(xTile, yTile, zoom);
}


QRectF Point2DPseudoMercator::getBounds() {
    return QRectF(-EXTENT_3857, -EXTENT_3857, 2*EXTENT_3857, 2*EXTENT_3857);
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

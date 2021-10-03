#include "cartesian_coor.h"

#include "point2dpseudomercator.h"

CartesianCoor::CartesianCoor(): Coordinate("")
{

}


CartesianCoor::CartesianCoor(double x, double y, QString crs): Coordinate(crs),
    _x(x), _y(y)
{

}

CartesianCoor::CartesianCoor(Point2DPseudoMercator pm) : Coordinate("EPSG:3857"),
    _x(pm.x()), _y(pm.y())
{

}


QString CartesianCoor::utm_epsg(double lat, double lon) {
    QString epsg("EPSG:32");
    if(lat > 0) {
        epsg += "6";
    } else {
        epsg += "7";
    }
    int zone_nb = static_cast<int>((lon + 180.0)/6.0 + 1.0);
    epsg += QString("%1").arg(zone_nb, 2, 10, QChar('0'));
    return epsg;
}


QString CartesianCoor::utm_epsg(int zone, bool isNorth) {
    char ns = isNorth ? '6': '7';
    return QString("EPSG:32%1%2").arg(ns).arg(zone, 2, 10, QChar('0'));
}

PJ_COORD CartesianCoor::toProj() {
    return proj_coord (_x, _y, 0, 0);
}

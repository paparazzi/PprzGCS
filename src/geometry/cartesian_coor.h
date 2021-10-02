#ifndef CARTESIANCOOR_H
#define CARTESIANCOOR_H

#include "geographic_coordinate.h"

// compatibility
class Point2DPseudoMercator;

class CartesianCoor : public GeographicCoordinate
{
public:
    CartesianCoor();
    CartesianCoor(double x, double y, QString crs);

    CartesianCoor(Point2DPseudoMercator pm);

    PJ_COORD toProj();

    double x() {return _x;}
    double y() {return _y;}

    static QString utm_epsg(double lat, double lon);
    static QString utm_epsg(int zone, bool isNorth);

private:
    double _x;
    double _y;
};

#endif // CARTESIANCOOR_H

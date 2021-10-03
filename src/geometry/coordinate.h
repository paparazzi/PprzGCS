#ifndef GEOGRAPHICCOORDINATE_H
#define GEOGRAPHICCOORDINATE_H

#include <QString>
#include <proj.h>

/**
 * @brief The GeographicCoordinate class describes a position on earth.
 * Not to be confused with a ProjectedCoordinate, which is a position on a map.
 */
class Coordinate
{
public:

    Coordinate(QString crs);

    QString getCRS() {return crs;}
    virtual PJ_COORD toProj() = 0;

private:
    // EPSG reference or Proj string
    QString crs;
};

#endif // GEOGRAPHICCOORDINATE_H

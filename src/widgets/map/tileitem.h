#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsPixmapItem>
#include "point2dtile.h"

//typedef std::tuple<int, int, int> TileCoorI;

class TileItem : public QGraphicsPixmapItem
{

public:
    TileItem(const QPixmap &pixmap, Point2DTile coordinates, QGraphicsItem *parent = nullptr);

    bool isInScene() { return inScene;}
    void setInScene(bool in_s) {inScene = in_s;}
    Point2DTile coordinates() {return _coordinates;}

private:
    bool inScene;
    Point2DTile _coordinates;
};

#endif // TILEITEM_H

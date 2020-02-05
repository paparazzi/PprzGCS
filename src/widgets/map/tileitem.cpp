#include "tileitem.h"

TileItem::TileItem(const QPixmap &pixmap, Point2DTile coordinates, QGraphicsItem *parent) :
    QGraphicsPixmapItem (pixmap, parent), inScene(false), _coordinates(coordinates)
{

}

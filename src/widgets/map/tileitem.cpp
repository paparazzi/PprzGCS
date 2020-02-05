#include "tileitem.h"

TileItem::TileItem(const QPixmap &pixmap, TileCoorI coordinates, QGraphicsItem *parent) :
    QGraphicsPixmapItem (pixmap, parent), inScene(false), _coordinates(coordinates)
{

}

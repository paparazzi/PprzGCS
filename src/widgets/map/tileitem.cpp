#include "tileitem.h"

TileItem::TileItem(const QPixmap &pixmap, QGraphicsItem *parent) :
    QGraphicsPixmapItem (pixmap, parent), displayed(false)
{

}

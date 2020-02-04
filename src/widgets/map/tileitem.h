#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsPixmapItem>

class TileItem : QGraphicsPixmapItem
{
public:
    TileItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr);

private:
    bool isDisplayed;
};

#endif // TILEITEM_H

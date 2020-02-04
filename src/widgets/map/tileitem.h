#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsPixmapItem>

class TileItem : public QGraphicsPixmapItem
{

public:
    TileItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr);

    bool isDisplayed() { return displayed;}
    void setDisplayed(bool d) {displayed = d;}

private:
    bool displayed;
};

#endif // TILEITEM_H

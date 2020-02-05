#ifndef TILEITEM_H
#define TILEITEM_H

#include <QGraphicsPixmapItem>
typedef std::tuple<int, int, int> TileCoorI;

class TileItem : public QGraphicsPixmapItem
{

public:
    TileItem(const QPixmap &pixmap, TileCoorI coordinates, QGraphicsItem *parent = nullptr);

    bool isInScene() { return inScene;}
    void setInScene(bool in_s) {inScene = in_s;}
    TileCoorI coordinates() {return _coordinates;}

private:
    bool inScene;
    TileCoorI _coordinates;
};

#endif // TILEITEM_H

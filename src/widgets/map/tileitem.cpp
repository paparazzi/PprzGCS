#include "tileitem.h"

TileItem::TileItem(const QPixmap &pixmap, Point2DTile coordinates, TileItem* mother, QGraphicsItem *parent) :
    QGraphicsPixmapItem (pixmap, parent), inScene(false), _hasData(true), _coordinates(coordinates), _mother(mother)
{
    for(int i=0; i<2; i++) {
        for(int j=0; j<2; j++) {
            _childs[i][j] = nullptr;
        }
    }
}


TileItem::TileItem(Point2DTile coordinates, TileItem* mother, QGraphicsItem *parent) :
    QGraphicsPixmapItem (parent), inScene(false), _hasData(false), _coordinates(coordinates), _mother(mother)
{
    for(int i=0; i<2; i++) {
        for(int j=0; j<2; j++) {
            _childs[i][j] = nullptr;
        }
    }
}

TileItem::TileItem(TileItem* mother, QGraphicsItem *parent) :
    QGraphicsPixmapItem (parent), inScene(false), _hasData(false), _coordinates(Point2DTile(0, 0, 0)), _mother(mother)
{
    for(int i=0; i<2; i++) {
        for(int j=0; j<2; j++) {
            _childs[i][j] = nullptr;
        }
    }
}

void TileItem::setPixmap(const QPixmap &pixmap) {
    QGraphicsPixmapItem::setPixmap(pixmap);
    _hasData = true;
    _dataGood = true;
}

void TileItem::setAltPixmap(const QPixmap &pixmap) {
    QGraphicsPixmapItem::setPixmap(pixmap);
    _hasData = true;
    _dataGood = false;
}



TileIterator::TileIterator(TileItem* base, int maxDepth) : current(base), maxDepth(maxDepth)
{
    explore.push_back(0);
}

TileItem* TileIterator::next() {
    if(current==nullptr) {  // the iterator has been initialized with a null base!) {
        return nullptr;
    }

    if(explore.last() < 4 && explore.length() <= maxDepth) {   // explore the children (unless maxDEPTH is reached)
        int x,y;
        switch (explore.last()) {
        case 0:
            x=0; y=0;
            break;
        case 1:
            x=1; y=0;
            break;
        case 2:
            x=0; y=1;
            break;
        case 3:
            x=1; y=1;
            break;
        }

        TileItem* n = current->child(x, y);

        //    //prepare for next time

        if(n != nullptr) {
            // got a child ! explore this child from the beginning
            current = n;
            explore.push_back(0);
            return next();
        } else {
            // no child :-( ! Then will try his brother...
            explore.last() += 1;
            return next();
        }

    } else {    // No more children. return the current then go to the parent
        TileItem* ret = current;
        explore.pop_back();
        if(explore.isEmpty()) {
            return nullptr;
        } else {
            explore.last() += 1;
        }
        current = (TileItem*) current->mother();
        return ret;
    }
}

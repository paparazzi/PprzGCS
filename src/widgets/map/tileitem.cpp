#include "tileitem.h"
#include <iostream>

TileItem::TileItem(TileItem* mother, Point2DTile coordinates, QGraphicsItem *parent) :
    QGraphicsPixmapItem (parent),
    inScene(false), _hasData(false), _dataGood(false),
    _coordinates(coordinates), _mother(mother)
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



bool TileItem::setInheritedData() {
    if(dataGood()) {
        std::cout << "No need to go to ancestors, I already have good data!!!" << std::endl;
        return true;
    }

    TileItem* current = this;

    int posX = 0;
    int posY = 0;

    int dz = 1;

    while(current->mother() != nullptr) {   // && (dz >< _coordinates.zoom())
        posX |= (current->coordinates().xi() & 1) << (dz-1);
        posY |= (current->coordinates().yi() & 1) << (dz-1);

        if(current->mother()->hasData()) {
            int TILE_SIZE = current->mother()->pixmap().size().width();
            int PART_SIZE = TILE_SIZE >> dz;
            QRect rect((posX*TILE_SIZE)>>dz, (posY*TILE_SIZE)>>dz, PART_SIZE, PART_SIZE);
            QPixmap cropped = current->mother()->pixmap().copy(rect);
            QPixmap scaled = cropped.scaled(TILE_SIZE, TILE_SIZE);
            setAltPixmap(scaled);
            return true;
        }

        current = current->mother();
        dz += 1;
    }
    return false;   // no data in any parent tile
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
        int x=0, y=0;
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
        if(!explore.isEmpty()) {
            explore.last() += 1;
        }

        current = static_cast<TileItem*>(current->mother());
        return ret;
    }
}

#include "mapitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>

MapItem::MapItem(int tile_size, double neutral_scale_zoom, QGraphicsItem *parent) :
    QGraphicsItemGroup(parent),
    zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom), tile_size(tile_size)
{
}

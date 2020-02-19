#include "mapitem.h"
#include "math.h"
#include <QApplication>
#include <QDebug>

MapItem::MapItem(double zoom, int tile_size, double neutral_scale_zoom, QObject *parent) :
    QObject(parent),
    zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom), tile_size(tile_size), _zoom(zoom), _view_scale(1)
{
}

#include "map_item.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include "mapwidget.h"

MapItem::MapItem(int ac_id, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    QObject(parent),
    ac_id(ac_id),
    zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom),
    z_value(z_value),
    map(map)
{
}


QList<QColor> MapItem::makeColorVariants(QColor color) {
    QList<QColor> list;

    int h, s, v, a;
    color.getHsv(&h, &s, &v, &a);

    int v1 = qMin(static_cast<int>(v/1.2), 255);
    QColor c1 = QColor(color);
    c1.setHsv(h, s, v1, a);
    list.append(c1);

    int a2 = qMin(static_cast<int>(a/2), 255);
    QColor c2 = QColor(color);
    c2.setHsv(h, s, v, a2);
    list.append(c2);


    int s3 = static_cast<int>(s/2);
    QColor c3 = QColor(color);
    //c3.setHsv(h, s3, v, a);
    c3.setHsv(h, s3, v, a2);
    list.append(c3);

    return list;
}

double MapItem::getScale() {
    return pow(zoom_factor, map->zoom() - neutral_scale_zoom)/map->scaleFactor();
}

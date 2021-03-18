#include "map_item.h"
#include "math.h"
#include <QApplication>
#include <QDebug>
#include <utility>
#include "mapwidget.h"

MapItem::MapItem(QString ac_id, double neutral_scale_zoom, QObject *parent) :
    QObject(parent),
    ac_id(std::move(ac_id)),
    zoom_factor(1), neutral_scale_zoom(neutral_scale_zoom),
    z_value(0)
{
}

QColor MapItem::unfocusedColor(const QColor &color) {
    int h, s, v, a;
    color.getHsv(&h, &s, &v, &a);

    int a2 = qMin(static_cast<int>(a/2), 255);
    int s3 = static_cast<int>(s/2);
    QColor c = QColor(color);
    c.setHsv(h, s3, v, a2);

    return c;
}

QColor MapItem::trackUnfocusedColor(const QColor &color) {
    int h, s, v, a;
    color.getHsv(&h, &s, &v, &a);

    int s3 = static_cast<int>(s/2);
    QColor c = QColor(color);
    c.setHsv(h, s3, v, a);

    return c;
}

QColor MapItem::labelUnfocusedColor(const QColor &color) {
    int h, s, v, a;
    color.getHsv(&h, &s, &v, &a);

    int v2 = static_cast<int>(v/2);
    QColor c = QColor(color);
    c.setHsv(h, s, v2, a);

    return c;
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

double MapItem::getScale(double zoom, double scale_factor) {
    return pow(zoom_factor, zoom - neutral_scale_zoom)/scale_factor;
}

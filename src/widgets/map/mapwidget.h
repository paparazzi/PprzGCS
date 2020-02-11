#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "map2d.h"

class MapWidget : public Map2D
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

signals:

};

#endif // MAPWIDGET_H

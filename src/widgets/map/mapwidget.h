#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QGraphicsItem>
#include <QList>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include "map2d.h"
#include "mapitem.h"
#include "tileprovider.h"
#include "maplayercontrol.h"
#include "layertab.h"

class MapWidget : public Map2D
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addItem(MapItem* map_item, Point2DLatLon latlon, int zValue = 10);
    void addLayerControl(QString name, bool initialState, int z);

signals:
    void rightClick(QMouseEvent *event);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    void setupUi();

    QList<MapItem*> _items;

    QHBoxLayout* horizontalLayout;
    LayerTab* layer_tab;
    QVBoxLayout* columnLeft;
    QVBoxLayout* columnRight;
    QSpacerItem* spacer;
    QScrollArea* leftScrollArea;

};

#endif // MAPWIDGET_H

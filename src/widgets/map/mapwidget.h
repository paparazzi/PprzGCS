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

class MapWidget : public Map2D
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void addItem(QGraphicsItem* graphicItem, Point2DLatLon latlon, int zValue = 10, double zoomFactor = 1);

    void addCircle(Point2DLatLon latlon, int size, QBrush brush = QBrush(Qt::red));

    void addLayerControl(QString name, bool initialState = false);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent* event);

private:
    void setupUi();

    QList<MapItem*> _items;
    QHBoxLayout* horizontalLayout;
    QWidget* widgetTabLeft;
    QVBoxLayout* layoutTabLeft;
    QVBoxLayout* columnLeft;
    QVBoxLayout* columnRight;
    QSpacerItem* spacer;
    QScrollArea* leftScrollArea;

    std::map<QString, MapLayerControl*> map_layer_controls;

signals:

};

#endif // MAPWIDGET_H

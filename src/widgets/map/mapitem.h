#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include "point2dlatlon.h"
#include <QGraphicsSceneMouseEvent>
#include <QVariant>

enum MoveState {
    MIS_IDLE,
    MIS_PRESSED,
    MIS_MOVED,
};

class MapItem : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT
public:
    MapItem(int tile_size, double neutral_scale_zoom = 15, QGraphicsItem *parent = nullptr);
    virtual void scaleToZoom(double zoom, double viewScale) = 0;

    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}

protected:
    double zoom_factor;
    double neutral_scale_zoom;
    int tile_size;
    double _zoom;

signals:
    //void itemMoved(QPointF scenePos);
    void itemClicked();
};

#endif // MAPITEM_H

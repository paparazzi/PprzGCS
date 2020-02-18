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
    MapItem(Point2DLatLon pt, QGraphicsItem *parent = nullptr);
    MapItem(QGraphicsItem* child, Point2DLatLon pt, double neutral_scale_zoom = 15, QGraphicsItem *parent = nullptr);
    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}
    void scaleToZoom(double zoom, double viewScale);
    Point2DLatLon position() {return latlon;}
    void setPosition(Point2DLatLon ll) {latlon = ll;}

signals:
    void itemMoved(QPointF scenePos);
    void itemClicked();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPointF pressPos;
    MoveState move_state;
    bool movable;

    Point2DLatLon latlon;
    double zoom_factor;
    double neutral_scale_zoom;
};

#endif // MAPITEM_H

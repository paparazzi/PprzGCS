#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include "point2dlatlon.h"
#include <QGraphicsSceneMouseEvent>
#include <QVariant>

class MapWidget;

enum MoveState {
    MIS_IDLE,
    MIS_PRESSED,
    MIS_MOVED,
};

class MapItem : public QObject
{
    Q_OBJECT
public:
    MapItem(qreal z_value, MapWidget* map, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    QList<QColor> makeColorVariants(QColor);
    virtual void updateGraphics() = 0;
    virtual void setHighlighted(bool h) = 0;
    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}
    virtual void setZValue(qreal z) = 0;
    qreal zValue() {return z_value;}

signals:
    void itemClicked(QPointF scene_pos);
    void itemGainedHighlight();

protected:
    double getScale();

    double zoom_factor;
    double neutral_scale_zoom;
    qreal z_value;

    MapWidget* map;



};

#endif // MAPITEM_H

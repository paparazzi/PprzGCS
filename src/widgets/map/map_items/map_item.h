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

enum ItemType {
    ITEM_WAYPOINT,
    ITEM_CIRCLE,
    ITEM_PATH,
    ITEM_AIRCRAFT,
    //ITEM_OVAL,
};

class MapItem : public QObject
{
    Q_OBJECT
public:
    MapItem(QString ac_id, qreal z_value, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    QList<QColor> makeColorVariants(QColor);
    QColor unfocusedColor(const QColor&);
    QColor trackUnfocusedColor(const QColor&);
    QColor labelUnfocusedColor(const QColor&);
    virtual void addToMap(MapWidget* map) = 0;
    virtual void updateGraphics(double zoom, double scale_factor, int tile_size) = 0;
    virtual void removeFromScene(MapWidget* map) = 0;
    virtual void setHighlighted(bool h) = 0;
    virtual void setForbidHighlight(bool fh) = 0;
    virtual void setEditable(bool ed) = 0;
    virtual ItemType getType() = 0;
    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}
    virtual void setZValue(qreal z) = 0;
    qreal zValue() {return z_value;}
    QString acId() {return ac_id;}
    void requestUpdate() {emit(itemChanged());}

signals:
    void itemClicked(QPointF scene_pos);
    void itemDoubleClicked(QPointF scene_pos);
    void itemGainedHighlight();
    void itemChanged();

protected:
    double getScale(double zoom, double scale_factor);

    const QString ac_id;

    double zoom_factor;
    double neutral_scale_zoom;
    qreal z_value;

};

#endif // MAPITEM_H

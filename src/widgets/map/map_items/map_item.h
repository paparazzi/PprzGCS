#ifndef MAPITEM_H
#define MAPITEM_H

#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include "point2dlatlon.h"
#include <QGraphicsSceneMouseEvent>
#include <QVariant>
#include "pprzpalette.h"

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
    MapItem(QString ac_id, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    MapItem(QString ac_id, PprzPalette palette, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    QList<QColor> makeColorVariants(QColor);
    QColor unfocusedColor(const QColor&);
    QColor trackUnfocusedColor(const QColor&);
    QColor labelUnfocusedColor(const QColor&);
    virtual void addToMap(MapWidget* map) = 0;
    virtual void updateGraphics(MapWidget* map) = 0;
    virtual void removeFromScene(MapWidget* map) = 0;
    virtual void setHighlighted(bool h) {
        highlighted = h;
        z_value = h ? z_value_highlighted : z_value_unhighlighted;
    }
    virtual void setForbidHighlight(bool fh) = 0;
    virtual void setEditable(bool ed) = 0;
    virtual ItemType getType() = 0;
    double zoomFactor() {return zoom_factor;}
    double neutralScaleZoom() {return neutral_scale_zoom;}
    void setZoomFactor(double zf) {zoom_factor = zf;}
    virtual void updateZValue() = 0;
    qreal zValue() {return z_value;}
    QString acId() {return ac_id;}
    void requestUpdate() {emit itemChanged();}
    void setZValues(qreal zh, qreal zuh) {
        z_value_highlighted = zh;
        z_value_unhighlighted = zuh;
        updateZValue();
    }
    void setPalette(PprzPalette p) {
        palette = p;
    }

signals:
    void itemClicked(QPointF scene_pos);
    void itemDoubleClicked(QPointF scene_pos);
    void itemGainedHighlight();
    void itemChanged();

protected:
    double getScale(double zoom, double scale_factor);

    const QString ac_id;
    PprzPalette palette;

    double zoom_factor;
    double neutral_scale_zoom;
    qreal z_value;
    bool highlighted;

    qreal z_value_highlighted;
    qreal z_value_unhighlighted;

};

#endif // MAPITEM_H

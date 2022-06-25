#ifndef QUIVERITEM_H
#define QUIVERITEM_H

#include "map_item.h"
#include "graphics_quiver.h"

class QuiverItem : public MapItem
{
    Q_OBJECT
public:
    QuiverItem(Point2DLatLon pos, Point2DLatLon vpos, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    virtual void addToMap(MapWidget* map) override;
    virtual void updateGraphics(MapWidget* map, uint32_t update_event) override;
    virtual void removeFromScene(MapWidget* map) override;
    virtual void setForbidHighlight(bool fh) override;
    virtual void setEditable(bool ed) override;
    virtual void updateZValue() override;

    void setPosition(Point2DLatLon pos);
    void setVector(Point2DLatLon vpos);

private:
    GraphicsQuiver* graphics_quiver;

    Point2DLatLon latlon;
    Point2DLatLon vlatlon;
    double distance;
    double azimut;
};

#endif // QUIVERITEM_H
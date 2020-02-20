#ifndef SEGMENT_H
#define SEGMENT_H

#include "mapitem.h"
#include <QBrush>
#include "waypointitem.h"
#include "graphicsline.h"

class Path : public MapItem
{
    Q_OBJECT
public:
    explicit Path(Point2DLatLon start, QColor color, int tile_size, double zoom, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    void addPoint(Point2DLatLon pos);
    void add_to_scene(QGraphicsScene* scene);
    void scaleToZoom(double zoom, double viewScale);
    void updateGraphics();
    virtual void setHighlighted(bool h);
signals:

public slots:

private:
    QList<WaypointItem*> waypoints;
    QList<GraphicsLine*> lines;
    int line_widht;
    QColor line_color;
    bool highlighted;
    //QGraphicsLineItem* line;
};

#endif // SEGMENT_H

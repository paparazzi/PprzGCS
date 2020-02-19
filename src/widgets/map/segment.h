#ifndef SEGMENT_H
#define SEGMENT_H

#include "mapitem.h"
#include <QBrush>
#include "waypointitem.h"
#include "graphicsline.h"

class Segment : public MapItem
{
    Q_OBJECT
public:
    explicit Segment(Point2DLatLon start, Point2DLatLon end, QColor color, int tile_size, double zoom, double neutral_scale_zoom = 15, QObject *parent = nullptr);
    void add_to_scene(QGraphicsScene* scene);
    void scaleToZoom(double zoom, double viewScale);
    void updateGraphics();
signals:

public slots:

private:
    WaypointItem* wpStart;
    WaypointItem* wpEnd;

    GraphicsLine* line;
    int line_widht;
    //QGraphicsLineItem* line;
};

#endif // SEGMENT_H

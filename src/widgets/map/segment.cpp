#include "segment.h"
#include "maputils.h"
#include <QGraphicsScene>

Segment::Segment(Point2DLatLon start, Point2DLatLon end, QColor color, int tile_size, double zoom, double neutral_scale_zoom, QObject *parent) :
    MapItem(zoom, tile_size, neutral_scale_zoom, parent)
{
    wpStart = new WaypointItem(start, 20, Qt::green, tile_size, zoom, neutral_scale_zoom, parent);
    wpEnd = new WaypointItem(end, 20, Qt::green, tile_size, zoom, neutral_scale_zoom, parent);

    QPointF start_pos = scenePoint(start, zoomLevel(zoom), tile_size);
    QPointF end_pos = scenePoint(end, zoomLevel(zoom), tile_size);

    line_widht = 5;

    line = new GraphicsLine(QLineF(start_pos, end_pos));
    line->setZValue(100);
    line->setPen(QPen(QBrush(color), line_widht));

    connect(
        wpStart, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );

    connect(
        wpEnd, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );


}

void Segment::add_to_scene(QGraphicsScene* scene) {
    wpStart->add_to_scene(scene);
    wpEnd->add_to_scene(scene);
    scene->addItem(line);
}

void Segment::scaleToZoom(double zoom, double viewScale) {
    _zoom = zoom;
    _view_scale = viewScale;
    updateGraphics();
}

void Segment::updateGraphics() {
    wpStart->scaleToZoom(_zoom, _view_scale);
    wpEnd->scaleToZoom(_zoom, _view_scale);


    QPointF start_scene_pos = scenePoint(wpStart->position(), zoomLevel(_zoom), tile_size);
    QPointF end_scene_pos = scenePoint(wpEnd->position(), zoomLevel(_zoom), tile_size);
    line->setLine(QLineF(start_scene_pos, end_scene_pos));

    double s = pow(zoom_factor, _zoom - neutral_scale_zoom)/_view_scale;
    QPen p = line->pen();
    p.setWidth(static_cast<int>(line_widht * s));
    line->setPen(p);
}

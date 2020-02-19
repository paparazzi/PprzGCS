#include "path.h"
#include "maputils.h"
#include <QGraphicsScene>

Path::Path(Point2DLatLon start, QColor color, int tile_size, double zoom, double neutral_scale_zoom, QObject *parent) :
    MapItem(zoom, tile_size, neutral_scale_zoom, parent),
    line_widht(5), line_color(color)
{
    WaypointItem* wpStart = new WaypointItem(start, 20, Qt::green, tile_size, zoom, neutral_scale_zoom, parent);

    waypoints.append(wpStart);

    connect(
        wpStart, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );
}

void Path::addPoint(Point2DLatLon pos) {
    WaypointItem* last_wp = waypoints.last();
    WaypointItem* wp = new WaypointItem(pos, 20, Qt::green, tile_size, _zoom, neutral_scale_zoom, parent());
    waypoints.append(wp);

    QPointF start_pos = scenePoint(last_wp->position(), zoomLevel(_zoom), tile_size);
    QPointF end_pos = scenePoint(wp->position(), zoomLevel(_zoom), tile_size);

    GraphicsLine* line = new GraphicsLine(QLineF(start_pos, end_pos));
    lines.append(line);
    line->setZValue(100);
    line->setPen(QPen(QBrush(line_color), line_widht));

    connect(
        wp, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );
}

void Path::add_to_scene(QGraphicsScene* scene) {
    for(auto w:waypoints) {
        w->add_to_scene(scene);
    }
    for(auto l:lines) {
        scene->addItem(l);
    }
}

void Path::scaleToZoom(double zoom, double viewScale) {
    _zoom = zoom;
    _view_scale = viewScale;
    updateGraphics();
}

void Path::updateGraphics() {
    assert(waypoints.length() == lines.length() + 1);

    for(auto w:waypoints) {
        w->scaleToZoom(_zoom, _view_scale);
    }

    double s = pow(zoom_factor, _zoom - neutral_scale_zoom)/_view_scale;

    for(int i=0; i<lines.length(); i++) {
        QPointF start_scene_pos = scenePoint(waypoints[i]->position(), zoomLevel(_zoom), tile_size);
        QPointF end_scene_pos = scenePoint(waypoints[i+1]->position(), zoomLevel(_zoom), tile_size);
        lines[i]->setLine(QLineF(start_scene_pos, end_scene_pos));

        QPen p = lines[i]->pen();
        p.setWidth(static_cast<int>(line_widht * s));
        lines[i]->setPen(p);
    }

}

#include "path.h"
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "mapitem.h"

Path::Path(Point2DLatLon start, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(z_value, map, neutral_scale_zoom, parent),
    line_widht(5), line_color(color)
{
    WaypointItem* wpStart = new WaypointItem(start, 20, Qt::green, z_value, map, neutral_scale_zoom, parent);
    init(wpStart);
}

Path::Path(WaypointItem* wpStart, QColor color, qreal z_value, MapWidget* map, double neutral_scale_zoom, QObject *parent) :
    MapItem(z_value, map, neutral_scale_zoom, parent),
    line_widht(5), line_color(color)
{
    assert(wpStart != nullptr);
    init(wpStart);
}

void Path::init(WaypointItem* wpStart) {
    waypoints.append(wpStart);

    connect(
        wpStart, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );

    connect(
        wpStart, &MapItem::itemGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );

    map->addItem(this);
}

void Path::addPoint(Point2DLatLon pos) {
    WaypointItem* wp = new WaypointItem(pos, 20, Qt::green, z_value, map, neutral_scale_zoom, parent());
    addPoint(wp);
}

void Path::addPoint(WaypointItem* wp) {
    assert(wp != nullptr);
    WaypointItem* last_wp = waypoints.last();

    waypoints.append(wp);

    QPointF start_pos = scenePoint(last_wp->position(), zoomLevel(map->zoom()), map->tileSize());
    QPointF end_pos = scenePoint(wp->position(), zoomLevel(map->zoom()), map->tileSize());

    GraphicsLine* line = new GraphicsLine(QLineF(start_pos, end_pos), QPen(QBrush(line_color), line_widht), this);

    QList<QColor> color_variants = makeColorVariants(line_color);
    line->setColors(color_variants[2]);

    lines.append(line);
    line->setZValue(z_value - 0.5);

    map->scene()->addItem(line);

    connect(
        wp, &WaypointItem::waypointMoved,
        [=](Point2DLatLon newPos) {
            (void) newPos;
            this->updateGraphics();
        }
    );

    connect(
        wp, &MapItem::itemGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );

    connect(
        line, &GraphicsObject::objectGainedHighlight,
        [=]() {
            setHighlighted(true);
            emit(itemGainedHighlight());
        }
    );
}

void Path::setHighlighted(bool h) {
    highlighted = h;
    for(auto wp: waypoints) {
        wp->setHighlighted(h);
    }
    for(auto line: lines) {
        line->setHighlighted(h);
    }
}

void Path::setZValue(qreal z) {
    z_value = z;
    //waypoints above lines
    for(auto w:waypoints) {
        w->setZValue(z);
    }
    for(auto l:lines) {
        l->setZValue(z-0.5);
    }
}

void Path::updateGraphics() {
    assert(waypoints.length() == lines.length() + 1);

    double s = getScale();

    for(int i=0; i<lines.length(); i++) {
        QPointF start_scene_pos = scenePoint(waypoints[i]->position(), zoomLevel(map->zoom()), map->tileSize());
        QPointF end_scene_pos = scenePoint(waypoints[i+1]->position(), zoomLevel(map->zoom()), map->tileSize());
        lines[i]->setLine(QLineF(start_scene_pos, end_scene_pos));

        QPen p = lines[i]->pen();
        p.setWidth(static_cast<int>(line_widht * s));
        lines[i]->setPen(p);
    }
}

void Path::removeFromScene() {
    for(auto l:lines) {
        map->scene()->removeItem(l);
        delete l;
    }
    lines.clear();
}

void Path::setLastLineIgnoreEvents(bool ignore) {
    if(lines.length() > 0) {
        lines.last()->setIgnoreEvent(ignore);
    }
}

void Path::setLinesIgnoreEvents(bool ignore) {
    for(auto line: lines) {
        line->setIgnoreEvent(ignore);
    }
}

void Path::removeLastWaypoint() {
    auto lastLine = lines.takeLast();
    waypoints.removeLast();
    map->scene()->removeItem(lastLine);
    delete lastLine;
}

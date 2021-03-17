#include "path_item.h"
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "map_item.h"
#include "AircraftManager.h"

PathItem::PathItem(QString ac_id, qreal z_value, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, neutral_scale_zoom, parent),
    line_width(5)
{

}

void PathItem::addPoint(WaypointItem* wp, bool own) {
    assert(wp != nullptr);
    waypoints.append(wp);
    owned[wp] = own;

    if(waypoints.size() > 1){
        Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);
        GraphicsLine* line = new GraphicsLine(QPointF(0, 0), QPointF(0, 0), aircraft.getColor(), line_width, this);

        QList<QColor> color_variants = makeColorVariants(aircraft.getColor());
        line->setColors(color_variants[2]);

        lines.append(line);
        to_be_added.append(line);
        line->setZValue(z_value - 0.5);

        connect(
            wp, &WaypointItem::itemChanged, this,
            [=]() {
                emit itemChanged();
            }
        );

        auto lastWp = waypoints[waypoints.size()-2];
        connect(
            lastWp, &WaypointItem::itemChanged, this,
            [=]() {
                emit itemChanged();
            }
        );

        connect(
            wp, &MapItem::itemGainedHighlight, this,
            [=]() {
                setHighlighted(true);
                emit itemGainedHighlight();
            }
        );

        connect(
            line, &GraphicsObject::objectGainedHighlight, this,
            [=]() {
                setHighlighted(true);
                emit itemGainedHighlight();
            }
        );
    }
}

void PathItem::setHighlighted(bool h) {
    highlighted = h;
    for(auto wp: waypoints) {
        wp->setHighlighted(h);
    }
    for(auto line: lines) {
        line->setHighlighted(h);
    }
}

void PathItem::setForbidHighlight(bool sh) {
    for(auto wp: waypoints) {
        wp->setForbidHighlight(sh);
    }
    for(auto line: lines) {
        line->setForbidHighlight(sh);
    }
}

void PathItem::setEditable(bool ed) {
    for(auto wp: waypoints) {
        wp->setEditable(ed);
    }
    for(auto line: lines) {
        line->setEditable(ed);
    }
}

void PathItem::setZValue(qreal z) {
    z_value = z;
    //waypoints above lines
    for(auto w:waypoints) {
        w->setZValue(z);
    }
    for(auto l:lines) {
        l->setZValue(z-0.5);
    }
}

void PathItem::updateGraphics(MapWidget* map) {
    // add new graphics objects to map
    while(to_be_added.size() > 0) {
        auto l = to_be_added.takeLast();
        map->scene()->addItem(l);
    }

    while(to_be_removed.size() > 0) {
        auto l = to_be_removed.takeLast();
        map->scene()->removeItem(l);
    }

    while(waypoints_to_remove.size() > 0) {
        auto wp = waypoints_to_remove.takeLast();
        map->removeItem(wp);
    }

    double s = getScale(map->zoom(), map->scaleFactor());
(void)s;

    for(int i=0; i<lines.length(); i++) {
        QPointF start_scene_pos = scenePoint(waypoints[i]->position(), zoomLevel(map->zoom()), map->tileSize());
        QPointF end_scene_pos = scenePoint(waypoints[i+1]->position(), zoomLevel(map->zoom()), map->tileSize());
        lines[i]->setLine(start_scene_pos, end_scene_pos);
    }
}

void PathItem::addToMap(MapWidget* map) {
    (void)map;
}

void PathItem::removeFromScene(MapWidget* map) {
    for(auto l:lines) {
        map->scene()->removeItem(l);
        delete l;
    }
    lines.clear();

    for(auto wp: waypoints) {
        if(owned[wp]) {
            map->removeItem(wp);
        }
    }
    waypoints.clear();
    owned.clear();

}

void PathItem::setLastLineIgnoreEvents(bool ignore) {
    if(lines.length() > 0) {
        lines.last()->setIgnoreEvent(ignore);
    }
}

void PathItem::setLinesIgnoreEvents(bool ignore) {
    for(auto line: lines) {
        line->setIgnoreEvent(ignore);
    }
}

void PathItem::removeLastWaypoint() {
    auto lastLine = lines.takeLast();
    to_be_removed.append(lastLine);
    auto wp = waypoints.takeLast();
    if(owned[wp]) {
        waypoints_to_remove.append(wp);
        owned.remove(wp);
    }
}

void PathItem::setStyle(GraphicsLine::Style s) {
    for(auto line: lines) {
        line->setStyle(s);
    }
    if(s == GraphicsLine::Style::CURRENT_NAV) {
        for(auto w:waypoints) {
            w->setStyle(GraphicsObject::Style::CURRENT_NAV);
        }
    }
}

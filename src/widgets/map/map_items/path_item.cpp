#include "path_item.h"
#include "maputils.h"
#include <QGraphicsScene>
#include "mapwidget.h"
#include "map_item.h"
#include "AircraftManager.h"

PathItem::PathItem(QString ac_id, qreal z_value, double neutral_scale_zoom, QObject *parent) :
    MapItem(ac_id, z_value, neutral_scale_zoom, parent),
    closing_line(nullptr), line_width(5)
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
    }
}

void PathItem::setClosedPath(bool closed) {
    if(closed) {
        if(!closing_line) {
            // add closing line
            qDebug() << "add closing line";
            Aircraft aircraft = AircraftManager::get()->getAircraft(ac_id);
            QList<QColor> color_variants = makeColorVariants(aircraft.getColor());
            closing_line = new GraphicsLine(QPointF(0, 0), QPointF(0, 0), aircraft.getColor(), line_width, this);
            closing_line->setColors(color_variants[2]);
            to_be_added.append(closing_line);
            closing_line->setZValue(z_value - 0.5);
        }
    } else {
        if(closing_line) {
            // remove closing line
            to_be_removed.append(closing_line);
            closing_line = nullptr;
        }
    }
}

void PathItem::setHighlighted(bool h) {
    highlighted = h;
    for(auto wp: waypoints) {
        wp->setHighlighted(h);
    }

    if(closing_line) {
        closing_line->setHighlighted(h);
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
    if(closing_line) {
        closing_line->setForbidHighlight(sh);
    }
}

void PathItem::setEditable(bool ed) {
    for(auto wp: waypoints) {
        wp->setEditable(ed);
    }
    for(auto line: lines) {
        line->setEditable(ed);
    }
    if(closing_line) {
        closing_line->setEditable(ed);
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
    if(closing_line) {
        closing_line->setZValue(z-0.5);
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

    if(closing_line) {
        QPointF start_scene_pos = scenePoint(waypoints.first()->position(), zoomLevel(map->zoom()), map->tileSize());
        QPointF end_scene_pos = scenePoint(waypoints.last()->position(), zoomLevel(map->zoom()), map->tileSize());
        closing_line->setLine(start_scene_pos, end_scene_pos);
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

    if(closing_line) {
        map->scene()->removeItem(closing_line);
        delete closing_line;
        closing_line = nullptr;
    }

    for(auto wp: waypoints) {
        if(owned[wp]) {
            map->removeItem(wp);
        }
    }
    waypoints.clear();
    owned.clear();

}

//void PathItem::setLastLineIgnoreEvents(bool ignore) {
//    if(lines.length() > 0) {
//        lines.last()->setIgnoreEvent(ignore);
//    }
//}

//void PathItem::setLinesIgnoreEvents(bool ignore) {
//    for(auto line: lines) {
//        line->setIgnoreEvent(ignore);
//    }
//}

void PathItem::removeLastWaypoint() {
    auto lastLine = lines.takeLast();
    to_be_removed.append(lastLine);
    auto wp = waypoints.takeLast();
    if(owned[wp]) {
        waypoints_to_remove.append(wp);
        owned.remove(wp);
    }

    if(closing_line && waypoints.size() < 3) {
        to_be_removed.append(closing_line);
        closing_line = nullptr;
    }
}

void PathItem::setStyle(GraphicsLine::Style s) {
    for(auto line: lines) {
        line->setStyle(s);
    }
    if(closing_line) {
        closing_line->setStyle(s);
    }
    if(s == GraphicsLine::Style::CURRENT_NAV) {
        for(auto w:waypoints) {
            w->setStyle(GraphicsObject::Style::CURRENT_NAV);
        }
    }
}

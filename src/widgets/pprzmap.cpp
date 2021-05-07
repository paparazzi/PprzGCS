#include "pprzmap.h"
#include "ui_pprzmap.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QMenu>
#include <QAction>
#include "mapscene.h"
#include <QGraphicsSceneMouseEvent>
#include "waypoint_item.h"
#include "circle_item.h"
#include "maputils.h"
#include "path_item.h"
#include <QCursor>
#include "AircraftManager.h"
#include "pprzmain.h"
#include "waypointeditor.h"
#include "srtm_manager.h"
#include "point2dpseudomercator.h"
#include <QSet>
#include "gcs_utils.h"

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap),
    drawState(false),
    interaction_state(PMIS_OTHER), fp_edit_sm(nullptr), current_ac("0")
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());

    connect(ui->map, &MapWidget::itemRemoved,
        [](MapItem* item) {
            //remove this item from everywhere, including dependencies
            //if nobody know it recursively, delete it !
            delete item;
        });

    connect(ui->map, &MapWidget::itemAdded, this,
        [=](MapItem* map_item) {
            map_item->setHighlighted(map_item->acId() == current_ac);

            if(map_item->getType() == ITEM_WAYPOINT) {
                registerWaypoint(dynamic_cast<WaypointItem*>(map_item));
            }
        });

    connect(scene, &MapScene::eventScene, this,
        [=](SmEditEvent eventType, QGraphicsSceneMouseEvent *mouseEvent) {
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(eventType, mouseEvent, nullptr, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });

    connect(ui->srtm_button, &QPushButton::clicked, [=]() {

        QSet<QString> tiles;

        // Download SRTM tile(s) for each flightplan footprint
        for(auto &ac: AircraftManager::get()->getAircrafts()) {
            auto [nw, se] = ac.getFlightPlan().boundingBox();
            auto tile_names = SRTMManager::get()->get_tile_names(se.lat(), nw.lat(), nw.lon(), se.lon());
            tiles.unite(tile_names.toSet());
        }

        // Download SRTM tile(s) for view footprint
        Point2DLatLon nw(0, 0), se(0, 0);
        ui->map->getViewPoints(nw, se);
        auto tile_names = SRTMManager::get()->get_tile_names(se.lat(), nw.lat(), nw.lon(), se.lon());
        tiles.unite(tile_names.toSet());

        SRTMManager::get()->load_tiles(tiles.toList());

    });

    connect(DispatcherUi::get(), &DispatcherUi::showHiddenWaypoints, this, [=](bool state) {
        for(auto &itemManager: ac_items_managers) {
            for(auto &wpi: itemManager->getWaypointsItems()) {
                if(state) {
                    wpi->setStyle(GraphicsObject::Style::DEFAULT);
                } else {
                    if(wpi->getOriginalWaypoint()->getName()[0] == '_') {
                        wpi->setStyle(GraphicsPoint::Style::CURRENT_NAV);
                    }
                }
            }
        }
    });

    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &PprzMap::updateAircraftItem);
    connect(PprzDispatcher::get(), &PprzDispatcher::waypoint_moved, this, &PprzMap::moveWaypoint);
    connect(PprzDispatcher::get(), &PprzDispatcher::nav_status, this, &PprzMap::updateTarget);
    connect(PprzDispatcher::get(), &PprzDispatcher::circle_status, this, &PprzMap::updateNavShape);
    connect(PprzDispatcher::get(), &PprzDispatcher::segment_status, this, &PprzMap::updateNavShape);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &PprzMap::handleNewAC);
    connect(DispatcherUi::get(), &DispatcherUi::ac_deleted, this, &PprzMap::removeAC);

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &PprzMap::changeCurrentAC);
    connect(ui->map, &MapWidget::mouseMoved, this, &PprzMap::handleMouseMove);

}

void PprzMap::changeCurrentAC(QString id) {
    if(AircraftManager::get()->aircraftExists(id)) {
        current_ac = id;
        ui->map->updateHighlights(id);
        auto waypoints = AircraftManager::get()->getAircraft(id).getFlightPlan().getWaypoints();

        // remove previous waypoints (the fist 2 items are WGS84 reference)
        while(ui->reference_combobox->count() > 2) {
            ui->reference_combobox->removeItem(2);
        }

        for(auto &wp: waypoints) {
            if(wp->getName()[0] != '_') {
                ui->reference_combobox->addItem(wp->getName());
            }
        }
    }


}

void PprzMap::registerWaypoint(WaypointItem* waypoint) {
    connect(waypoint, &WaypointItem::itemClicked, this,
        [=](QPointF pos) {
            (void)pos;
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(FPEE_WP_CLICKED, nullptr, waypoint, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });
}

void PprzMap::setMapLayout(QLayout* layout) {
    ui->map->setLayout(layout);
}

PprzMap::~PprzMap()
{
    delete ui;
}

void PprzMap::configure(QDomElement e) {
    ui->map->configure(e);
}

void PprzMap::keyPressEvent(QKeyEvent *event) {
    (void)event;
}

void PprzMap::keyReleaseEvent(QKeyEvent *event) {
    (void)event;
    if(event->key() == Qt::Key_Space) {
        interaction_state = PMIS_FLIGHT_PLAN_EDIT;
        setEditorMode();
        switch (drawState) {
        default:
            break;
        }
        drawState = (drawState + 1) % 3;
    }
    else if(event->key() == Qt::Key_Escape) {
        if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
            MapItem* item = fp_edit_sm->update(FPEE_CANCEL, nullptr, nullptr, current_ac);
            (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
        }
        ui->map->setMouseTracking(false);
        ui->map->scene()->setShortcutItems(false);
        interaction_state = PMIS_OTHER;
        setEditorMode();
        drawState = 0;
        ui->map->setCursor(Qt::ArrowCursor);
    } else if(event->key() == Qt::Key_F) {
        interaction_state = PMIS_FROZEN;
        setEditorMode();
    }
    else if (event->key() == Qt::Key_H) {
        ui->map->itemsForbidHighlight(false);
    }
    else if (event->key() == Qt::Key_C) {
        if(AircraftManager::get()->aircraftExists(current_ac)) {



            Point2DLatLon pos = AircraftManager::get()->getAircraft(current_ac).getPosition();
            auto [nw, se] = AircraftManager::get()->getAircraft(current_ac).getFlightPlan().boundingBoxWith(pos);
            double zoo = ui->map->zoomBox(nw, se);
            ui->map->setZoom(zoo);
            Point2DLatLon center((nw.lat()+se.lat()) / 2.0, (nw.lon()+se.lon()) / 2.0);
            ui->map->centerLatLon(center);
        }
        ui->map->itemsForbidHighlight(false);
    }
}

void PprzMap::setEditorMode() {
    switch(interaction_state) {
        case PMIS_FLIGHT_PLAN_EDIT:
            ui->map->itemsForbidHighlight(true);
            ui->map->itemsEditable(false);
            break;
        case PMIS_FROZEN:
            ui->map->itemsForbidHighlight(false);
            ui->map->itemsEditable(false);
            break;
        default:
            ui->map->itemsForbidHighlight(false);
            ui->map->itemsEditable(true);
            break;
    }
}

void PprzMap::updateAircraftItem(pprzlink::Message msg) {
    QString ac_id;
    float lat, lon, heading;
    msg.getField("ac_id", ac_id);
    msg.getField("lat", lat);
    msg.getField("long", lon);
    msg.getField("heading", heading);

    if(AircraftManager::get()->aircraftExists(ac_id)) {
        auto ai = ac_items_managers[ac_id]->getAircraftItem();
        Point2DLatLon pos(static_cast<double>(lat), static_cast<double>(lon));
        ai->setPosition(pos);
        ai->setHeading(static_cast<double>(heading));
        AircraftManager::get()->getAircraft(ac_id).setPosition(pos);
    }

}

void PprzMap::handleNewAC(QString ac_id) {
    auto settings = getAppSettings();
    // create aircraft item at dummy position
    auto aircraft_item = new AircraftItem(Point2DLatLon(0, 0), ac_id, 16);
    ui->map->addItem(aircraft_item);

    //create carrot at dummy position
    WaypointItem* target = new WaypointItem(Point2DLatLon(0, 0), ac_id);
    ui->map->addItem(target);
    target->setStyle(GraphicsObject::Style::CARROT);
    target->setEditable(false);
    double z_carrot = settings.value("map/z_values/carrot").toDouble();
    target->setZValues(z_carrot, z_carrot);

    //create the ACItemManager for this aircraft
    auto item_manager = make_shared<ACItemManager>(ac_id, target, aircraft_item);
    ac_items_managers[ac_id] = item_manager;

    for(auto &wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        WaypointItem* wpi = new WaypointItem(wp, ac_id);
        ui->map->addItem(wpi);
        item_manager->addWaypointItem(wpi);

        auto dialog_move_waypoint = [=]() {
            wpi->setMoving(true);
            auto we = new WaypointEditor(wpi, ac_id);
            auto view_pos = ui->map->mapFromScene(wpi->scenePos());
            auto global_pos = ui->map->mapToGlobal(view_pos);
            we->show(); //show just to get the width and height right.
            we->move(global_pos - QPoint(we->width()/2, we->height()/2));
            connect(we, &QDialog::finished, wpi, [=](int result) {
                (void)result;
                wpi->setMoving(false);

            });
            we->open();
        };

        connect(wpi, &WaypointItem::waypointMoveFinished, this, dialog_move_waypoint);

        connect(wpi, &WaypointItem::itemDoubleClicked, this, dialog_move_waypoint);

        connect(wpi, &WaypointItem::waypointMoved, this, [=](){
            wpi->setAnimate(true);
        });

        if(wp->getName()[0] == '_') {
            wpi->setStyle(GraphicsPoint::Style::CURRENT_NAV);
        }
    }

    for(auto sector: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getSectors()) {
        // static sector are not supported
        if(sector->getType() == Sector::DYNAMIC) {
            PathItem* pi = new PathItem(ac_id);
            for(auto &wp: sector->getCorners()) {
                for(auto wpi: item_manager->getWaypointsItems()) {
                    if(wpi->getOriginalWaypoint() == wp) {
                        pi->addPoint(wpi);
                    }
                }
            }

            pi->setClosedPath(true);
            ui->map->addItem(pi);
            item_manager->addPathItem(pi);

        }
    }
}

void PprzMap::removeAC(QString ac_id) {
    ac_items_managers[ac_id]->removeItems(ui->map);
    ac_items_managers.remove(ac_id);
}

void PprzMap::handleMouseMove(QPointF scenePos) {
    auto tp = tilePoint(scenePos, zoomLevel(ui->map->zoom()), ui->map->tileSize());
    Point2DPseudoMercator ppm(tp);
    auto pt = CoordinatesTransform::get()->pseudoMercator_to_WGS84(ppm);

    if(ui->reference_combobox->currentIndex() == 0) {
        // Why 6 digits ? Its smaller than a person, and bigger than a waldo: https://xkcd.com/2170/
        QString txt = QString("%1").arg(pt.lat(), 10, 'f', 6, QChar(' ')) + "," + QString("%1").arg(pt.lon(), 11, 'f', 6, QChar(' '));
        ui->pos_label->setText(txt);
    } else if (ui->reference_combobox->currentIndex() == 1) {
        QString txt = sexagesimalFormat(pt.lat(), pt.lon());
        ui->pos_label->setText(txt);
    } else {
        size_t wp_index = static_cast<size_t>(ui->reference_combobox->currentIndex() - 2);
        auto ref_wp = AircraftManager::get()->getAircraft(current_ac).getFlightPlan().getWaypoints()[wp_index];
        Point2DLatLon pt_wp(ref_wp);

        double distance, azimut;
        CoordinatesTransform::get()->distance_azimut(pt_wp, pt, distance, azimut);
        //ct_wgs84_utm.distance_azimut(pt_wp, pt, distance, azimut);

        ui->pos_label->setText(QString("%1").arg(static_cast<int>(azimut), 3, 10, QChar(' ')) + "° " +
                               QString("%1").arg(static_cast<int>(distance), 4, 10, QChar(' ')) + "m");
    }

    auto ele = SRTMManager::get()->get_elevation(pt.lat(), pt.lon());
    if(ele) {
        QString txt = QString("%1").arg(ele.value(), 4, 10, QChar(' ')) + "m";
        ui->srtm_label->setText(txt);
    } else {
        ui->srtm_label->setText("No SRTM");
    }
}

QString PprzMap::sexagesimalFormat(double lat, double lon) {
    auto sexformat = [](double nb) {
        int deg = static_cast<int>(nb);
        int min = static_cast<int>((nb - deg) * 60);
        int sec = static_cast<int>((((nb - deg) * 60) - min)*60);
        QString txt = QString("%1").arg(deg, 3, 10, QChar(' ')) + "° " + QString("%1").arg(min, 2, 10, QChar('0')) + "' " + QString("%1").arg(sec, 2, 10, QChar('0')) + "\"";
        return txt;
    };

    QString txtLat = sexformat(abs(lat));
    QString txtLon = sexformat(abs(lon));

    QString latGeo = lat > 0 ? "N" : "S";
    QString lonGeo = lon > 0 ? "E" : "W";

    QString txt = txtLat + latGeo + " " + txtLon + lonGeo;
    return txt;
}

void PprzMap::moveWaypoint(pprzlink::Message msg) {
    QString ac_id;
    uint8_t wp_id = 0;
    float lat, lon, alt, ground_alt;
    msg.getField("ac_id", ac_id);
    msg.getField("wp_id", wp_id);
    msg.getField("lat", lat);
    msg.getField("long", lon);
    msg.getField("alt", alt);
    msg.getField("ground_alt", ground_alt);

    if(AircraftManager::get()->aircraftExists(ac_id) && wp_id != 0) {
        shared_ptr<Waypoint> wp = AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoint(wp_id);
        wp->setLat(static_cast<double>(lat));
        wp->setLon(static_cast<double>(lon));
        wp->setAlt(static_cast<double>(alt));

        for(auto wpi: ac_items_managers[ac_id]->getWaypointsItems()) {
            if(wpi->getOriginalWaypoint() == wp && !wpi->isMoving()) {
                wpi->updatePosition();
                wpi->setAnimate(false);
            }
        }
    }
}

void PprzMap::updateTarget(pprzlink::Message msg) {
    QString ac_id;
    float target_lat, target_lon;
    msg.getField("ac_id", ac_id);
    msg.getField("target_lat", target_lat);
    msg.getField("target_long", target_lon);

    if(AircraftManager::get()->aircraftExists(ac_id)) {
        ac_items_managers[ac_id]->getTarget()->setPosition(Point2DLatLon(static_cast<double>(target_lat), static_cast<double>(target_lon)));
    }
}

void PprzMap::updateNavShape(pprzlink::Message msg) {
    auto settings = getAppSettings();

    QString ac_id;
    msg.getField("ac_id", ac_id);

    MapItem* prev_item = ac_items_managers[ac_id]->getCurrentNavShape();

    double z = settings.value("map/z_values/nav_shape").toDouble();

    if(msg.getDefinition().getName() == "CIRCLE_STATUS") {
        if(prev_item!= nullptr && prev_item->getType() != ITEM_CIRCLE) {
            ac_items_managers[ac_id]->setCurrentNavShape(nullptr);
            ui->map->removeItem(prev_item);
            prev_item = nullptr;
        }

        float circle_lat, circle_long;
        int16_t radius;
        msg.getField("circle_lat", circle_lat);
        msg.getField("circle_long", circle_long);
        msg.getField("radius", radius);


        Point2DLatLon pos(static_cast<double>(circle_lat), static_cast<double>(circle_long));
        if(prev_item == nullptr) {
            auto wcenter = new WaypointItem(pos, ac_id);
            wcenter->setZValues(z, z);
            ui->map->addItem(wcenter);
            CircleItem* ci = new CircleItem(wcenter, radius, ac_id);
            ci->setZValues(z, z);
            ci->setScalable(false);
            ci->setEditable(false);
            ci->setOwnCenter(true);
            ui->map->addItem(ci);
            ci->setStyle(GraphicsObject::Style::CURRENT_NAV);
            ac_items_managers[ac_id]->setCurrentNavShape(ci);
        } else {
            CircleItem* ci = static_cast<CircleItem*>(prev_item);
            ci->getCenter()->getOriginalWaypoint()->setLat(pos.lat());
            ci->getCenter()->getOriginalWaypoint()->setLon(pos.lon());
            ci->getCenter()->updatePosition();
            ci->setRadius(radius);
        }

    } else if (msg.getDefinition().getName() == "SEGMENT_STATUS") {
        if(prev_item!= nullptr && prev_item->getType() != ITEM_PATH) {
            ac_items_managers[ac_id]->setCurrentNavShape(nullptr);
            ui->map->removeItem(prev_item);
            prev_item = nullptr;
        }

        float segment1_lat, segment1_long, segment2_lat, segment2_long;
        msg.getField("segment1_lat", segment1_lat);
        msg.getField("segment1_long", segment1_long);
        msg.getField("segment2_lat", segment2_lat);
        msg.getField("segment2_long", segment2_long);

        Point2DLatLon p1(static_cast<double>(segment1_lat), static_cast<double>(segment1_long));
        Point2DLatLon p2(static_cast<double>(segment2_lat), static_cast<double>(segment2_long));
        if(prev_item == nullptr) {
            PathItem* pi = new PathItem(ac_id);
            pi->setZValues(z, z);

            auto w1 = new WaypointItem(p1, ac_id);
            w1->setZValues(z, z);
            ui->map->addItem(w1);
            auto w2 = new WaypointItem(p2, ac_id);
            w2->setZValues(z, z);
            ui->map->addItem(w2);
            pi->addPoint(w1);
            pi->addPoint(w2);
            pi->setStyle(GraphicsObject::Style::CURRENT_NAV);
            ui->map->addItem(pi);
            ac_items_managers[ac_id]->setCurrentNavShape(pi);
            //qDebug() << "segment created!";
        } else {
            PathItem* pi = static_cast<PathItem*>(prev_item);
            auto wps = pi->getWaypoints();
            assert(wps.size() == 2);
            wps[0]->getOriginalWaypoint()->setLat(p1.lat());
            wps[0]->getOriginalWaypoint()->setLon(p1.lon());
            wps[0]->updatePosition();

            wps[1]->getOriginalWaypoint()->setLat(p2.lat());
            wps[1]->getOriginalWaypoint()->setLon(p2.lon());
            wps[1]->updatePosition();
        }

    }


}

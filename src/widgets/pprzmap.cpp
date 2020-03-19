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
#include "waypointitem_sm.h"
#include "circleitem_sm.h"
#include "pathitem_sm.h"
#include <QCursor>
#include "AircraftManager.h"

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap),
    drawState(false),
    interaction_state(PMIS_OTHER), fp_edit_sm(nullptr), current_ac("0")
{
    ui->setupUi(this);
    MapScene* scene = static_cast<MapScene*>(ui->map->scene());

    connect(ui->map, &MapWidget::itemRemoved,
        [=](MapItem* item) {
            //remove this item from everywhere, including dependencies
            //if nobody know it recursively, delete it !
            delete item;
        });

    connect(ui->map, &MapWidget::itemAdded,
        [=](MapItem* map_item) {
            saveItem(map_item);
            map_item->setHighlighted(map_item->acId() == current_ac);

            if(map_item->getType() == ITEM_WAYPOINT) {
                registerWaypoint(dynamic_cast<WaypointItem*>(map_item));
            }
        });

    connect(scene, &MapScene::eventScene,
        [=](SmEditEvent eventType, QGraphicsSceneMouseEvent *mouseEvent) {
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(eventType, mouseEvent, nullptr, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });

    connect(PprzDispatcher::get(), &PprzDispatcher::flight_param, this, &PprzMap::updateAircraftItem);
    connect(PprzDispatcher::get(), &PprzDispatcher::waypoint_moved, this, &PprzMap::moveWaypoint);

    connect(DispatcherUi::get(), &DispatcherUi::new_ac_config, this, &PprzMap::handleNewAC);
    connect(ui->map, &MapWidget::mouseMoved, this, &PprzMap::handleMouseMove);

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected,
        [=](QString id) {
            current_ac = id;
            ui->map->updateHighlights(id);
        }
    );
}

void PprzMap::registerWaypoint(WaypointItem* waypoint) {
    connect(waypoint, &WaypointItem::itemClicked,
        [=](QPointF pos) {
            (void)pos;
            if(interaction_state == PMIS_FLIGHT_PLAN_EDIT && fp_edit_sm != nullptr) {
                MapItem* item = fp_edit_sm->update(FPEE_WP_CLICKED, nullptr, waypoint, current_ac);
                (void)item; //put item in a list relative to the drone (in a drone FP, in a block)
            }
        });
}

PprzMap::~PprzMap()
{
    delete ui;
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
        case 0:
            fp_edit_sm = new SmWaypointItem(ui->map);
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_waypoint_black.svg"),0, 0));
            break;
        case 1:
            fp_edit_sm = new SmCircleItem(ui->map);
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_circle_black.svg"),0, 0));
            break;
        case 2:
            fp_edit_sm = new SmPathItem(ui->map);
            ui->map->setCursor(QCursor(QPixmap(":/pictures/cursor_path_black.svg"),0, 0));
            break;
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
}


void PprzMap::saveItem(MapItem* item) {
    item->setForbidHighlight(true);
    item->setEditable(false);

    connect(item, &MapItem::itemGainedHighlight,
        [=]() {
            QString ac_id = item->acId();
            emit(DispatcherUi::get()->ac_selected(ac_id));
        });
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
    std::string ac_id;
    float lat, lon, course;
    msg.getField("ac_id", ac_id);
    msg.getField("lat", lat);
    msg.getField("long", lon);
    msg.getField("course", course);

    QString id = QString(ac_id.c_str());
    AircraftItem* ai;

    if(aircraft_items.find(id) != aircraft_items.end()) {
        ai = aircraft_items[id];
    } else {
        ai = new AircraftItem(Point2DLatLon(static_cast<double>(lat), static_cast<double>(lon)), id, ui->map, 16);
        ai->setZValue(300);
        aircraft_items[id] = ai;
    }
    ai->setPosition(Point2DLatLon(static_cast<double>(lat), static_cast<double>(lon)));
    ai->setHeading(static_cast<double>(course));
}

void PprzMap::handleNewAC(QString ac_id) {
    qDebug() << "new AC: " << ac_id;
    qDebug() << AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getDefaultAltitude();
    for(auto wp: AircraftManager::get()->getAircraft(ac_id).getFlightPlan().getWaypoints()) {
        if(wp->getName()[0] != '_') {
            int z = (current_ac == ac_id) ? qApp->property("ITEM_Z_VALUE_HIGHLIGHTED").toInt():
                                           qApp->property("ITEM_Z_VALUE_UNHIGHLIGHTED").toInt();
            (void)z;
            WaypointItem* wpi = new WaypointItem(wp, ac_id, z, ui->map);
            wpi->setEditable(true);
            wpi->setForbidHighlight(false);
            waypointItems.append(wpi);
            //waypointMoved
            connect(wpi, &WaypointItem::waypointMoveFinished,
                [=](Point2DLatLon latlon_pos) mutable {
                    qDebug() << "waypoint " << wp->getName().c_str() << " moved to " << latlon_pos.lat() << ", " << latlon_pos.lon();
                    emit(DispatcherUi::get()->move_waypoint(wp, ac_id));
                }
            );
        }
    }
}

void PprzMap::handleMouseMove(QPointF scenePos) {
    Point2DLatLon pt = latlonPoint(scenePos, zoomLevel(ui->map->zoom()), ui->map->tileSize());

    if(ui->reference_combobox->currentIndex() == 0) {
        QString txt = QString::number(pt.lat(), 'f', 7) + ", " + QString::number(pt.lon(), 'f', 7);
        ui->pos_label->setText(txt);
    } else if (ui->reference_combobox->currentIndex() == 1) {
        QString txt = sexagesimalFormat(pt.lat(), pt.lon());
        ui->pos_label->setText(txt);
    }
}

QString PprzMap::sexagesimalFormat(double lat, double lon) {
    auto sexformat = [=](double nb) {
        int deg = static_cast<int>(nb);
        int min = static_cast<int>((nb - deg) * 60);
        int sec = static_cast<int>((((nb - deg) * 60) - min)*60);
        QString txt = QString("%1").arg(deg, 3, 10, QChar('0')) + "° " + QString("%1").arg(min, 2, 10, QChar('0')) + "' " + QString("%1").arg(sec, 2, 10, QChar('0')) + "\"";
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
    std::string id;
    uint8_t wp_id = 0;
    float lat, lon, alt, ground_alt;
    msg.getField("ac_id", id);
    msg.getField("wp_id", wp_id);
    msg.getField("lat", lat);
    msg.getField("long", lon);
    msg.getField("alt", alt);
    msg.getField("ground_alt", ground_alt);
    if(AircraftManager::get()->aircraftExists(id.c_str()) && wp_id != 0) {
        shared_ptr<Waypoint> wp = AircraftManager::get()->getAircraft(id.c_str()).getFlightPlan().getWaypoint(wp_id);
        wp->setLat(static_cast<double>(lat));
        wp->setLon(static_cast<double>(lon));
        wp->setAlt(static_cast<double>(alt));

        for(auto wpi: waypointItems) {
            if(wpi->getWaypoint() == wp && !wpi->isMoving()) {
                wpi->updateGraphics();
            }
        }
    }
}

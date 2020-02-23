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
    interaction_state(PMIS_OTHER), fp_edit_sm(nullptr), current_ac(0)
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

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected,
        [=](int id) {
            current_ac = id;
            ui->map->updateHighlights(id);
        }
    );

    AircraftManager::get()->addAircraft(0, Qt::green);
    AircraftManager::get()->addAircraft(1, Qt::red);
    AircraftManager::get()->addAircraft(2, Qt::yellow);

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
            int ac_id = item->acId();
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


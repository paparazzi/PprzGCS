#include "pprzmap.h"
#include "ui_pprzmap.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include "maputils.h"
#include "AircraftManager.h"
#include "pprzmain.h"
#include "srtm_manager.h"
#include "point2dpseudomercator.h"
#include <QSet>
#include "gcs_utils.h"

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap),
    current_ac("0")
{
    ui->setupUi(this);

    connect(ui->srtm_button, &QPushButton::clicked, [=]() {

        QSet<QString> tiles;

        // Download SRTM tile(s) for each flightplan footprint
        for(auto ac: AircraftManager::get()->getAircrafts()) {
            auto [nw, se] = ac->getFlightPlan()->boundingBox();
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

    connect(DispatcherUi::get(), &DispatcherUi::ac_selected, this, &PprzMap::changeCurrentAC);
    connect(ui->map, &MapWidget::mouseMoved, this, &PprzMap::handleMouseMove);

}

void PprzMap::changeCurrentAC(QString id) {
    if(AircraftManager::get()->aircraftExists(id)) {
        current_ac = id;
        auto waypoints = AircraftManager::get()->getAircraft(id)->getFlightPlan()->getWaypoints();

        // remove previous waypoints (the fist 2 items are WGS84 reference)
        while(ui->reference_combobox->count() > 2) {
            ui->reference_combobox->removeItem(2);
        }

        for(auto &wp: waypoints) {
            auto wp_name = wp->getName();
            if(wp_name[0] != '_') {
                ui->reference_combobox->addItem(wp_name);
            }
        }
    }
}

void PprzMap::configure(QDomElement e) {
    ui->map->configure(e);
}

void PprzMap::handleMouseMove(QPointF scenePos) {
    auto tp = tilePoint(scenePos, zoomLevel(ui->map->zoom()), ui->map->tileSize());
    Point2DPseudoMercator ppm(tp);
    auto pt = CoordinatesTransform::get()->pseudoMercator_to_WGS84(ppm);

    if(ui->reference_combobox->currentIndex() == 0) {
        auto txt = pt.toString();
        ui->pos_label->setText(txt);
    } else if (ui->reference_combobox->currentIndex() == 1) {
        auto txt = pt.toString(true);
        ui->pos_label->setText(txt);
    } else {
        auto wps = AircraftManager::get()->getAircraft(current_ac)->getFlightPlan()->getWaypoints();
        for(auto ref_wp: qAsConst(wps)) {
            if(ref_wp->getName() == ui->reference_combobox->currentText()) {
                Point2DLatLon pt_wp(ref_wp);
                double distance, azimut;
                CoordinatesTransform::get()->distance_azimut(pt_wp, pt, distance, azimut);
                ui->pos_label->setText(QString("%1").arg(static_cast<int>(azimut), 3, 10, QChar(' ')) + "° " +
                                       QString("%1").arg(static_cast<int>(distance), 4, 10, QChar(' ')) + "m");
                break;
            }
        }
    }

    auto ele = SRTMManager::get()->get_elevation(pt.lat(), pt.lon());
    if(ele) {
        QString txt = QString("%1").arg(ele.value(), 4, 10, QChar(' ')) + "m";
        ui->srtm_label->setText(txt);
    } else {
        ui->srtm_label->setText("No SRTM");
    }
}

MapWidget* PprzMap::map() {
    return ui->map;
}

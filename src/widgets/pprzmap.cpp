#include "pprzmap.h"
#include "ui_pprzmap.h"
#include "pprz_dispatcher.h"
#include "dispatcher_ui.h"
#include <iostream>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QMenu>
#include <QAction>

PprzMap::PprzMap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PprzMap)
{
    ui->setupUi(this);

    ui->map->setZoom(17);
    ui->map->centerLatLon(Point2DLatLon(43.462344,1.273044));
    ui->map->setTilesPath(QString("/home/fabien/DEV/test_qt/PprzGCS/data/map"));
    auto tileProviders = ui->map->tileProvidersNames();
    if(tileProviders.length() > 0) {

        QString default_provider = tileProviders[0];
        if(tileProviders.contains(QString("Google"))) {
            default_provider = QString("Google");
        }
        ui->map->toggleTileProvider(default_provider, true, 0, 1);


        ui->tileSourcesButton->setMenu(new QMenu);
        for(auto name: tileProviders) {
            QAction* a = new QAction(name);
            a->setCheckable(true);
            if(name == default_provider) {
                a->setChecked(true);
            }
            ui->tileSourcesButton->menu()->addAction(a);
            connect(a,SIGNAL(toggled(bool)),this,SLOT(toggleTileProvider(bool)));
        }
    }
}

void PprzMap::toggleTileProvider(bool trig) {
    QAction* a = qobject_cast<QAction*>(QObject::sender());
    if(a != nullptr) {
        ui->map->toggleTileProvider(a->text(), trig, 10, 0.3);
        ui->map->updateTiles();
    }
}

PprzMap::~PprzMap()
{
    delete ui;
}


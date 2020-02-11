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

    auto tileProviders = ui->map->tileProviders();

    ui->map->setZoom(17);
    ui->map->centerLatLon(Point2DLatLon(43.462344,1.273044));

    QString default_provider = tileProviders->begin()->first;
    if(tileProviders->find(QString("Google")) != tileProviders->end()) {
        default_provider = QString("Google");
    }
    ui->map->toggleTileProvider(default_provider, true, 0, 1);


    ui->tileSourcesButton->setMenu(new QMenu);

    for(auto conf: *tileProviders) {
        QAction* a = new QAction(conf.first);
        a->setCheckable(true);
        if(conf.first == default_provider) {
            a->setChecked(true);
        }
        ui->tileSourcesButton->menu()->addAction(a);
        connect(a,SIGNAL(toggled(bool)),this,SLOT(toggleTileProvider(bool)));
    }
}

void PprzMap::toggleTileProvider(bool trig) {
    QAction* a = qobject_cast<QAction*>(QObject::sender());
    if(a != nullptr) {
        std::cout << a->text().toStdString() << " : " << trig << std::endl;
        ui->map->toggleTileProvider(a->text(), trig, 10, 0.3);
        ui->map->updateTiles();
    }
}

PprzMap::~PprzMap()
{
    delete ui;
}

